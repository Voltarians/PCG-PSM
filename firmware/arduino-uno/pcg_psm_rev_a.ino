/*
 * PCG-PSM Rev A.1
 * Promethean Core Power Supervisor Module
 *
 * Development target: Arduino Uno / ATmega328P
 *
 * Rev A.1 electrical conventions are frozen for the first bench build.
 * Low-voltage thresholds and ADC calibration remain intentionally TBD
 * until the protected power path is measured on the bench and in the Volt.
 */

#include <Arduino.h>
#include <EEPROM.h>

enum class SupervisorState : uint8_t {
  OFF,
  WAKE,
  PRECHECK,
  BOOT,
  RUN,
  SHUTDOWN_REQUEST,
  SHUTDOWN_WAIT,
  POWER_OFF,
  SERVICE,
  FAULT
};

// Frozen Rev A.1 pin plan.
constexpr uint8_t PIN_ACC_SENSE      = 2;   // Active LOW via U4A
constexpr uint8_t PIN_SHUTDOWN_REQ   = 3;   // HIGH lights U4B; Pi GPIO17 sees LOW
constexpr uint8_t PIN_SHUTDOWN_ACK   = 4;   // Active LOW via U4D / Pi gpio-poweroff GPIO22
constexpr uint8_t PIN_PI_HEARTBEAT   = 5;   // Edge-based via U4C / Pi GPIO27
constexpr uint8_t PIN_MAIN_POWER_EN  = 6;   // HIGH enables BTS50060
constexpr uint8_t PIN_SELF_HOLD      = 7;   // HIGH holds Q2 supervisor latch
constexpr uint8_t PIN_SERVICE_WAKE   = 8;   // Active LOW from SW1 pole B
constexpr uint8_t PIN_VEHICLE_VOLT   = A0;
constexpr uint8_t PIN_5V_VOLT        = A1;
constexpr uint8_t PIN_TEMPERATURE    = A2;
constexpr uint8_t PIN_POWER_SENSE    = A3;  // BTS50060 IS diagnostic channel

// Initial development timings. These remain configurable candidates.
constexpr unsigned long BOOT_TIMEOUT_MS          = 120000UL;
constexpr unsigned long HEARTBEAT_TIMEOUT_MS     = 15000UL;
constexpr unsigned long IGNITION_OFF_DELAY_MS    = 30000UL;
constexpr unsigned long SHUTDOWN_HARD_TIMEOUT_MS = 120000UL;
constexpr unsigned long POWER_OFF_SETTLE_MS      = 3000UL;
constexpr unsigned long SERVICE_TIMEOUT_MS       = 3600000UL;

SupervisorState state = SupervisorState::WAKE;
unsigned long stateEnteredMs = 0;
unsigned long lastHeartbeatMs = 0;
bool lastHeartbeatLevel = true;
bool heartbeatSeen = false;
bool serviceBootLatched = false;

void enterState(SupervisorState next) {
  state = next;
  stateEnteredMs = millis();
}

bool accActive() {
  return digitalRead(PIN_ACC_SENSE) == LOW;
}

bool serviceRequested() {
  return digitalRead(PIN_SERVICE_WAKE) == LOW;
}

bool shutdownAcknowledged() {
  // U4D inverts Pi GPIO22. LOW here means Linux has reached the
  // gpio-poweroff safe-to-remove-power indication.
  return digitalRead(PIN_SHUTDOWN_ACK) == LOW;
}

void setMainPower(bool enabled) {
  digitalWrite(PIN_MAIN_POWER_EN, enabled ? HIGH : LOW);
}

void setShutdownRequest(bool active) {
  // HIGH drives the Uno-side optocoupler LED. The Pi receives an
  // active-LOW shutdown request at GPIO17.
  digitalWrite(PIN_SHUTDOWN_REQ, active ? HIGH : LOW);
}

void resetHeartbeatMonitor() {
  lastHeartbeatLevel = digitalRead(PIN_PI_HEARTBEAT);
  lastHeartbeatMs = millis();
  heartbeatSeen = false;
}

void updateHeartbeat() {
  const bool level = digitalRead(PIN_PI_HEARTBEAT);
  if (level != lastHeartbeatLevel) {
    lastHeartbeatLevel = level;
    lastHeartbeatMs = millis();
    heartbeatSeen = true;
  }
}

bool heartbeatAlive() {
  return heartbeatSeen && ((millis() - lastHeartbeatMs) <= HEARTBEAT_TIMEOUT_MS);
}

void setup() {
  // Optocoupler transistor outputs and the service button are active LOW.
  // Internal pull-ups establish safe inactive states if the remote side is
  // disconnected. Rev B may add external pull-ups where EMC testing calls
  // for a stronger bias.
  pinMode(PIN_ACC_SENSE, INPUT_PULLUP);
  pinMode(PIN_SHUTDOWN_ACK, INPUT_PULLUP);
  pinMode(PIN_PI_HEARTBEAT, INPUT_PULLUP);
  pinMode(PIN_SERVICE_WAKE, INPUT_PULLUP);

  pinMode(PIN_SHUTDOWN_REQ, OUTPUT);
  pinMode(PIN_MAIN_POWER_EN, OUTPUT);
  pinMode(PIN_SELF_HOLD, OUTPUT);

  // Take control of the hardware latch immediately after reset.
  digitalWrite(PIN_SELF_HOLD, HIGH);
  setShutdownRequest(false);
  setMainPower(false);

  Serial.begin(115200);
  resetHeartbeatMonitor();
  enterState(SupervisorState::WAKE);
}

void loop() {
  updateHeartbeat();

  switch (state) {
    case SupervisorState::OFF:
      // Normally unreachable while the Uno is powered; Rev A.1 hardware
      // releases supervisor power after POWER_OFF.
      break;

    case SupervisorState::WAKE:
      if (accActive()) {
        serviceBootLatched = false;
        enterState(SupervisorState::PRECHECK);
      } else if (serviceRequested()) {
        // SW1 is momentary. Capture the reason for waking now so the user
        // does not have to hold the button while the Raspberry Pi boots.
        serviceBootLatched = true;
        enterState(SupervisorState::PRECHECK);
      } else {
        enterState(SupervisorState::POWER_OFF);
      }
      break;

    case SupervisorState::PRECHECK:
      // TODO: convert protected ADC readings to calibrated voltages and
      // temperature, then apply validated low-voltage/thermal limits.
      setMainPower(true);
      resetHeartbeatMonitor();
      enterState(SupervisorState::BOOT);
      break;

    case SupervisorState::BOOT:
      // Never accept boot based only on elapsed time. At least one real
      // heartbeat transition must be seen from the Pi.
      if (heartbeatAlive()) {
        enterState(serviceBootLatched && !accActive()
                       ? SupervisorState::SERVICE
                       : SupervisorState::RUN);
      } else if ((millis() - stateEnteredMs) > BOOT_TIMEOUT_MS) {
        enterState(SupervisorState::FAULT);
      }
      break;

    case SupervisorState::RUN:
      if (!heartbeatAlive()) {
        enterState(SupervisorState::FAULT);
      } else if (!accActive()) {
        enterState(SupervisorState::SHUTDOWN_REQUEST);
      }
      break;

    case SupervisorState::SHUTDOWN_REQUEST:
      if (accActive()) {
        setShutdownRequest(false);
        serviceBootLatched = false;
        enterState(SupervisorState::RUN);
      } else if ((millis() - stateEnteredMs) >= IGNITION_OFF_DELAY_MS) {
        setShutdownRequest(true);
        enterState(SupervisorState::SHUTDOWN_WAIT);
      }
      break;

    case SupervisorState::SHUTDOWN_WAIT:
      if (accActive()) {
        setShutdownRequest(false);
        serviceBootLatched = false;
        enterState(SupervisorState::RUN);
      } else if (shutdownAcknowledged()) {
        // Normal path: GPIO22 gpio-poweroff indication has reached U4D.
        setShutdownRequest(false);
        enterState(SupervisorState::POWER_OFF);
      } else if ((millis() - stateEnteredMs) >= SHUTDOWN_HARD_TIMEOUT_MS) {
        // TODO: persist HARD_TIMEOUT reason before power removal.
        setShutdownRequest(false);
        enterState(SupervisorState::POWER_OFF);
      }
      break;

    case SupervisorState::POWER_OFF:
      setMainPower(false);
      setShutdownRequest(false);
      if ((millis() - stateEnteredMs) >= POWER_OFF_SETTLE_MS) {
        // Releasing D7 allows Q2 to turn off and removes supervisor power.
        digitalWrite(PIN_SELF_HOLD, LOW);
      }
      break;

    case SupervisorState::SERVICE:
      if (accActive()) {
        serviceBootLatched = false;
        enterState(SupervisorState::RUN);
      } else if (!heartbeatAlive()) {
        enterState(SupervisorState::FAULT);
      } else if ((millis() - stateEnteredMs) >= SERVICE_TIMEOUT_MS) {
        enterState(SupervisorState::SHUTDOWN_REQUEST);
      }
      break;

    case SupervisorState::FAULT:
      // Initial safe policy: request an orderly shutdown first; the normal
      // hard timeout path prevents a frozen Pi from keeping the system on.
      setShutdownRequest(true);
      enterState(SupervisorState::SHUTDOWN_WAIT);
      break;
  }

  delay(10);
}
