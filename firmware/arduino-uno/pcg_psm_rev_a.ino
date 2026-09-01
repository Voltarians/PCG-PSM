/*
 * PCG-PSM Rev A
 * Promethean Core Power Supervisor Module
 *
 * Development target: Arduino Uno / ATmega328P
 *
 * This is the initial firmware skeleton. Thresholds, polarity, timings,
 * and pin assignments remain provisional until the Rev A schematic and
 * bench measurements are frozen.
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

// Provisional Rev A pin plan.
constexpr uint8_t PIN_ACC_SENSE      = 2;
constexpr uint8_t PIN_SHUTDOWN_REQ   = 3;
constexpr uint8_t PIN_SHUTDOWN_ACK   = 4;
constexpr uint8_t PIN_PI_HEARTBEAT   = 5;
constexpr uint8_t PIN_MAIN_POWER_EN  = 6;
constexpr uint8_t PIN_SELF_HOLD      = 7;
constexpr uint8_t PIN_SERVICE_WAKE   = 8;
constexpr uint8_t PIN_VEHICLE_VOLT   = A0;
constexpr uint8_t PIN_5V_VOLT        = A1;
constexpr uint8_t PIN_TEMPERATURE    = A2;

// Initial development timings. These are intentionally easy to change.
constexpr unsigned long BOOT_TIMEOUT_MS          = 120000UL;
constexpr unsigned long HEARTBEAT_TIMEOUT_MS     = 15000UL;
constexpr unsigned long IGNITION_OFF_DELAY_MS    = 30000UL;
constexpr unsigned long SHUTDOWN_HARD_TIMEOUT_MS = 120000UL;
constexpr unsigned long POWER_OFF_SETTLE_MS      = 3000UL;
constexpr unsigned long SERVICE_TIMEOUT_MS       = 3600000UL;

SupervisorState state = SupervisorState::WAKE;
unsigned long stateEnteredMs = 0;
unsigned long lastHeartbeatMs = 0;
bool lastHeartbeatLevel = false;
bool heartbeatSeen = false;

void enterState(SupervisorState next) {
  state = next;
  stateEnteredMs = millis();
}

bool accActive() {
  return digitalRead(PIN_ACC_SENSE) == HIGH;
}

bool serviceRequested() {
  return digitalRead(PIN_SERVICE_WAKE) == HIGH;
}

bool shutdownAcknowledged() {
  return digitalRead(PIN_SHUTDOWN_ACK) == HIGH;
}

void setMainPower(bool enabled) {
  digitalWrite(PIN_MAIN_POWER_EN, enabled ? HIGH : LOW);
}

void setShutdownRequest(bool active) {
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
  pinMode(PIN_ACC_SENSE, INPUT);
  pinMode(PIN_SHUTDOWN_ACK, INPUT);
  pinMode(PIN_PI_HEARTBEAT, INPUT);
  pinMode(PIN_SERVICE_WAKE, INPUT);

  pinMode(PIN_SHUTDOWN_REQ, OUTPUT);
  pinMode(PIN_MAIN_POWER_EN, OUTPUT);
  pinMode(PIN_SELF_HOLD, OUTPUT);

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
      // Normally unreachable while the Uno is powered; final hardware
      // will release the supervisor latch after entering POWER_OFF.
      break;

    case SupervisorState::WAKE:
      if (accActive() || serviceRequested()) {
        enterState(SupervisorState::PRECHECK);
      } else {
        enterState(SupervisorState::POWER_OFF);
      }
      break;

    case SupervisorState::PRECHECK:
      // TODO: convert protected ADC readings to real voltages and apply
      // validated low-voltage and temperature limits.
      setMainPower(true);
      resetHeartbeatMonitor();
      enterState(SupervisorState::BOOT);
      break;

    case SupervisorState::BOOT:
      if (heartbeatAlive()) {
        enterState(serviceRequested() && !accActive()
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
        enterState(SupervisorState::RUN);
      } else if ((millis() - stateEnteredMs) >= IGNITION_OFF_DELAY_MS) {
        setShutdownRequest(true);
        enterState(SupervisorState::SHUTDOWN_WAIT);
      }
      break;

    case SupervisorState::SHUTDOWN_WAIT:
      if (accActive()) {
        setShutdownRequest(false);
        enterState(SupervisorState::RUN);
      } else if (shutdownAcknowledged()) {
        setShutdownRequest(false);
        enterState(SupervisorState::POWER_OFF);
      } else if ((millis() - stateEnteredMs) >= SHUTDOWN_HARD_TIMEOUT_MS) {
        // TODO: persist HARD_TIMEOUT shutdown reason before power removal.
        setShutdownRequest(false);
        enterState(SupervisorState::POWER_OFF);
      }
      break;

    case SupervisorState::POWER_OFF:
      setMainPower(false);
      setShutdownRequest(false);
      if ((millis() - stateEnteredMs) >= POWER_OFF_SETTLE_MS) {
        // Final Rev A latch hardware will make this remove supervisor power.
        digitalWrite(PIN_SELF_HOLD, LOW);
      }
      break;

    case SupervisorState::SERVICE:
      if (accActive()) {
        enterState(SupervisorState::RUN);
      } else if (!heartbeatAlive()) {
        enterState(SupervisorState::FAULT);
      } else if ((millis() - stateEnteredMs) >= SERVICE_TIMEOUT_MS) {
        enterState(SupervisorState::SHUTDOWN_REQUEST);
      }
      break;

    case SupervisorState::FAULT:
      // Initial safe policy: ask for a shutdown first, then fall back to the
      // same hard timeout path. Event persistence and automatic restart are
      // added after the electrical interface is bench-validated.
      setShutdownRequest(true);
      enterState(SupervisorState::SHUTDOWN_WAIT);
      break;
  }

  delay(10);
}
