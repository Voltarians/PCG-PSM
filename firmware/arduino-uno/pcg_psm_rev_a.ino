/*
 * PCG-PSM Rev A.2
 * Promethean Core Power Supervisor Module
 * Target: Arduino Uno / ATmega328P
 *
 * Rev A.2 adds:
 * - persistent configuration with CRC
 * - EEPROM ring event log
 * - voltage / temperature telemetry
 * - calibratable BTS50060 current telemetry
 * - bounded watchdog restart policy
 * - low-voltage and thermal protection hooks (disabled by default)
 * - serial JSON telemetry and configuration commands
 *
 * IMPORTANT: Volt low-voltage and thermal shutdown thresholds remain disabled
 * until bench/in-vehicle measurements validate them.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <math.h>
#include <limits.h>
#include "config.h"
#include "event_log.h"

constexpr char FW_VERSION[] = "A.2.0";

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
constexpr uint8_t PIN_POWER_SENSE    = A3;

constexpr uint32_t SENSOR_INTERVAL_MS = 250UL;
constexpr uint16_t NTC_R0_OHMS = 10000;
constexpr float NTC_BETA = 3950.0f;
constexpr float NTC_T0_K = 298.15f;

enum class SupervisorState : uint8_t {
  WAKE, PRECHECK, BOOT, RUN, SHUTDOWN_DELAY, SHUTDOWN_WAIT,
  MAIN_OFF_WAIT, POWER_CYCLE_WAIT, SERVICE, FAULT_HOLD
};

enum class ShutdownCause : uint8_t {
  NONE = 0, ACC_OFF, SERVICE_TIMEOUT, HEARTBEAT_LOST, BOOT_TIMEOUT,
  LOW_VOLTAGE, OVER_TEMPERATURE
};

struct SensorSnapshot {
  uint16_t vehicleRaw = 0;
  uint16_t main5Raw = 0;
  uint16_t temperatureRaw = 0;
  uint16_t currentRaw = 0;
  uint16_t vehicleMv = 0;
  uint16_t main5Mv = 0;
  int16_t temperatureC_x10 = INT16_MAX;
  int32_t currentMa = INT32_MIN;
};

PsmConfig cfg{};
SensorSnapshot sensors{};
SupervisorState state = SupervisorState::WAKE;
ShutdownCause shutdownCause = ShutdownCause::NONE;
uint32_t stateEnteredMs = 0, lastHeartbeatMs = 0, lastSensorMs = 0, lastTelemetryMs = 0;
uint32_t stableRunSinceMs = 0, lowSinceMs = 0, hotSinceMs = 0, faultHoldSinceMs = 0;
bool lastHeartbeatLevel = true, heartbeatSeen = false, serviceBootLatched = false;
bool lowWarnLatched = false, stableRunLogged = false;
uint8_t restartAttempts = 0;
char serialLine[96];
uint8_t serialLineLen = 0;

const __FlashStringHelper* stateName(SupervisorState s) {
  switch (s) {
    case SupervisorState::WAKE: return F("WAKE");
    case SupervisorState::PRECHECK: return F("PRECHECK");
    case SupervisorState::BOOT: return F("BOOT");
    case SupervisorState::RUN: return F("RUN");
    case SupervisorState::SHUTDOWN_DELAY: return F("SHUTDOWN_DELAY");
    case SupervisorState::SHUTDOWN_WAIT: return F("SHUTDOWN_WAIT");
    case SupervisorState::MAIN_OFF_WAIT: return F("MAIN_OFF_WAIT");
    case SupervisorState::POWER_CYCLE_WAIT: return F("POWER_CYCLE_WAIT");
    case SupervisorState::SERVICE: return F("SERVICE");
    case SupervisorState::FAULT_HOLD: return F("FAULT_HOLD");
    default: return F("UNKNOWN");
  }
}

const __FlashStringHelper* causeName(ShutdownCause c) {
  switch (c) {
    case ShutdownCause::NONE: return F("NONE");
    case ShutdownCause::ACC_OFF: return F("ACC_OFF");
    case ShutdownCause::SERVICE_TIMEOUT: return F("SERVICE_TIMEOUT");
    case ShutdownCause::HEARTBEAT_LOST: return F("HEARTBEAT_LOST");
    case ShutdownCause::BOOT_TIMEOUT: return F("BOOT_TIMEOUT");
    case ShutdownCause::LOW_VOLTAGE: return F("LOW_VOLTAGE");
    case ShutdownCause::OVER_TEMPERATURE: return F("OVER_TEMPERATURE");
    default: return F("UNKNOWN");
  }
}

void enterState(SupervisorState next) { state = next; stateEnteredMs = millis(); }
bool accActive() { return digitalRead(PIN_ACC_SENSE) == LOW; }
bool serviceRequested() { return digitalRead(PIN_SERVICE_WAKE) == LOW; }
bool shutdownAcknowledged() { return digitalRead(PIN_SHUTDOWN_ACK) == LOW; }
void setMainPower(bool enabled) { digitalWrite(PIN_MAIN_POWER_EN, enabled ? HIGH : LOW); }
void setShutdownRequest(bool active) { digitalWrite(PIN_SHUTDOWN_REQ, active ? HIGH : LOW); }

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
bool heartbeatAlive() { return heartbeatSeen && (millis() - lastHeartbeatMs <= cfg.heartbeatTimeoutMs); }

uint16_t readAdcAverage(uint8_t pin, uint8_t samples = 8) {
  uint32_t total = 0;
  for (uint8_t i = 0; i < samples; ++i) total += analogRead(pin);
  return static_cast<uint16_t>(total / samples);
}
uint16_t vehicleMvFromRaw(uint16_t raw) {
  const uint64_t numerator = static_cast<uint64_t>(raw) * cfg.adcReferenceMv * 202ULL;
  const uint64_t mv = numerator / (1023ULL * 22ULL);
  return static_cast<uint16_t>(mv > 65535ULL ? 65535ULL : mv);
}
uint16_t main5MvFromRaw(uint16_t raw) {
  const uint32_t mv = (static_cast<uint32_t>(raw) * cfg.adcReferenceMv * 2UL) / 1023UL;
  return static_cast<uint16_t>(mv > 65535UL ? 65535UL : mv);
}
int16_t temperatureC_x10FromRaw(uint16_t raw) {
  if (raw == 0 || raw >= 1023) return INT16_MAX;
  const float rNtc = static_cast<float>(NTC_R0_OHMS) * raw / static_cast<float>(1023U - raw);
  const float invT = (1.0f / NTC_T0_K) + (log(rNtc / NTC_R0_OHMS) / NTC_BETA);
  if (invT <= 0.0f) return INT16_MAX;
  const long x10 = lroundf(((1.0f / invT) - 273.15f) * 10.0f);
  if (x10 > INT16_MAX || x10 < INT16_MIN) return INT16_MAX;
  return static_cast<int16_t>(x10);
}
int32_t currentMaFromRaw(uint16_t raw) {
  if (cfg.currentMaPerCount_x1000 == 0) return INT32_MIN;
  const int32_t delta = static_cast<int32_t>(raw) - cfg.currentZeroAdc;
  return static_cast<int32_t>((static_cast<int64_t>(delta) * cfg.currentMaPerCount_x1000) / 1000LL);
}
void sampleSensors(bool force = false) {
  const uint32_t now = millis();
  if (!force && now - lastSensorMs < SENSOR_INTERVAL_MS) return;
  lastSensorMs = now;
  sensors.vehicleRaw = readAdcAverage(PIN_VEHICLE_VOLT);
  sensors.main5Raw = readAdcAverage(PIN_5V_VOLT);
  sensors.temperatureRaw = readAdcAverage(PIN_TEMPERATURE);
  sensors.currentRaw = readAdcAverage(PIN_POWER_SENSE);
  sensors.vehicleMv = vehicleMvFromRaw(sensors.vehicleRaw);
  sensors.main5Mv = main5MvFromRaw(sensors.main5Raw);
  sensors.temperatureC_x10 = temperatureC_x10FromRaw(sensors.temperatureRaw);
  sensors.currentMa = currentMaFromRaw(sensors.currentRaw);
}

bool lowVoltageProtectionEnabled() { return (cfg.flags & PSM_FLAG_LOW_VOLTAGE_ENABLE) != 0; }
bool thermalProtectionEnabled() { return (cfg.flags & PSM_FLAG_THERMAL_ENABLE) != 0; }
bool lowVoltageRecovered() { return !lowVoltageProtectionEnabled() || (cfg.lowRecoverMv > 0 && sensors.vehicleMv >= cfg.lowRecoverMv); }
bool thermalRecovered() { return !thermalProtectionEnabled() || (cfg.thermalRecoverC_x10 != 0 && sensors.temperatureC_x10 != INT16_MAX && sensors.temperatureC_x10 <= cfg.thermalRecoverC_x10); }

ShutdownCause protectionTrip() {
  const uint32_t now = millis();
  if (lowVoltageProtectionEnabled() && cfg.lowWarnMv > 0) {
    if (sensors.vehicleMv <= cfg.lowWarnMv && !lowWarnLatched) {
      lowWarnLatched = true;
      psmLogEvent(PsmEventCode::LOW_VOLT_WARN, 0, static_cast<int16_t>(sensors.vehicleMv));
    } else if (cfg.lowRecoverMv > 0 && sensors.vehicleMv >= cfg.lowRecoverMv) lowWarnLatched = false;
  }
  if (lowVoltageProtectionEnabled() && cfg.lowShutdownMv > 0) {
    if (sensors.vehicleMv <= cfg.lowShutdownMv) {
      if (lowSinceMs == 0) lowSinceMs = now;
      if (now - lowSinceMs >= cfg.lowHoldMs) return ShutdownCause::LOW_VOLTAGE;
    } else lowSinceMs = 0;
  } else lowSinceMs = 0;
  if (thermalProtectionEnabled() && cfg.thermalShutdownC_x10 != 0 && sensors.temperatureC_x10 != INT16_MAX) {
    if (sensors.temperatureC_x10 >= cfg.thermalShutdownC_x10) {
      if (hotSinceMs == 0) hotSinceMs = now;
      if (now - hotSinceMs >= cfg.thermalHoldMs) return ShutdownCause::OVER_TEMPERATURE;
    } else hotSinceMs = 0;
  } else hotSinceMs = 0;
  return ShutdownCause::NONE;
}

bool protectionRecoveredFor(ShutdownCause cause) {
  if (cause == ShutdownCause::LOW_VOLTAGE) return lowVoltageRecovered();
  if (cause == ShutdownCause::OVER_TEMPERATURE) return thermalRecovered();
  return true;
}
void beginShutdown(ShutdownCause cause) {
  shutdownCause = cause;
  setShutdownRequest(true);
  psmLogEvent(PsmEventCode::SHUTDOWN_REQUEST, static_cast<uint8_t>(cause));
  enterState(SupervisorState::SHUTDOWN_WAIT);
}
void releaseSupervisorPower() {
  setMainPower(false); setShutdownRequest(false); digitalWrite(PIN_SELF_HOLD, LOW);
}
void scheduleRestartOrHold() {
  if (!accActive()) { releaseSupervisorPower(); return; }
  if (shutdownCause == ShutdownCause::LOW_VOLTAGE || shutdownCause == ShutdownCause::OVER_TEMPERATURE) {
    faultHoldSinceMs = millis(); enterState(SupervisorState::FAULT_HOLD); return;
  }
  if (restartAttempts < cfg.maxRestartAttempts) {
    ++restartAttempts;
    psmLogEvent(PsmEventCode::POWER_CYCLE, restartAttempts, static_cast<int16_t>(shutdownCause));
    enterState(SupervisorState::POWER_CYCLE_WAIT);
  } else {
    psmLogEvent(PsmEventCode::RESTART_LIMIT, restartAttempts, static_cast<int16_t>(shutdownCause));
    faultHoldSinceMs = millis(); enterState(SupervisorState::FAULT_HOLD);
  }
}

void printStatus() {
  sampleSensors(true);
  Serial.print(F("{\"type\":\"status\",\"fw\":\"")); Serial.print(FW_VERSION);
  Serial.print(F("\",\"ms\":")); Serial.print(millis());
  Serial.print(F(",\"state\":\"")); Serial.print(stateName(state));
  Serial.print(F("\",\"cause\":\"")); Serial.print(causeName(shutdownCause));
  Serial.print(F("\",\"acc\":")); Serial.print(accActive() ? 1 : 0);
  Serial.print(F(",\"service\":")); Serial.print(serviceBootLatched ? 1 : 0);
  Serial.print(F(",\"heartbeat\":")); Serial.print(heartbeatAlive() ? 1 : 0);
  Serial.print(F(",\"vehicle_mv\":")); Serial.print(sensors.vehicleMv);
  Serial.print(F(",\"main5_mv\":")); Serial.print(sensors.main5Mv);
  Serial.print(F(",\"temp_c_x10\":")); if (sensors.temperatureC_x10 == INT16_MAX) Serial.print(F("null")); else Serial.print(sensors.temperatureC_x10);
  Serial.print(F(",\"current_raw\":")); Serial.print(sensors.currentRaw);
  Serial.print(F(",\"current_ma\":")); if (sensors.currentMa == INT32_MIN) Serial.print(F("null")); else Serial.print(sensors.currentMa);
  Serial.print(F(",\"restart_attempts\":")); Serial.print(restartAttempts);
  Serial.print(F(",\"lv_protect\":")); Serial.print(lowVoltageProtectionEnabled() ? 1 : 0);
  Serial.print(F(",\"thermal_protect\":")); Serial.print(thermalProtectionEnabled() ? 1 : 0);
  Serial.println(F("}"));
}

void printConfig() {
  Serial.print(F("{\"type\":\"config\",\"boot_timeout_ms\":")); Serial.print(cfg.bootTimeoutMs);
  Serial.print(F(",\"heartbeat_timeout_ms\":")); Serial.print(cfg.heartbeatTimeoutMs);
  Serial.print(F(",\"ignition_off_delay_ms\":")); Serial.print(cfg.ignitionOffDelayMs);
  Serial.print(F(",\"shutdown_timeout_ms\":")); Serial.print(cfg.shutdownHardTimeoutMs);
  Serial.print(F(",\"service_timeout_ms\":")); Serial.print(cfg.serviceTimeoutMs);
  Serial.print(F(",\"restart_delay_ms\":")); Serial.print(cfg.restartDelayMs);
  Serial.print(F(",\"restart_cooldown_ms\":")); Serial.print(cfg.restartCooldownMs);
  Serial.print(F(",\"max_restart_attempts\":")); Serial.print(cfg.maxRestartAttempts);
  Serial.print(F(",\"adc_ref_mv\":")); Serial.print(cfg.adcReferenceMv);
  Serial.print(F(",\"low_voltage_enable\":")); Serial.print(lowVoltageProtectionEnabled() ? 1 : 0);
  Serial.print(F(",\"low_warn_mv\":")); Serial.print(cfg.lowWarnMv);
  Serial.print(F(",\"low_shutdown_mv\":")); Serial.print(cfg.lowShutdownMv);
  Serial.print(F(",\"low_recover_mv\":")); Serial.print(cfg.lowRecoverMv);
  Serial.print(F(",\"thermal_enable\":")); Serial.print(thermalProtectionEnabled() ? 1 : 0);
  Serial.print(F(",\"thermal_shutdown_c_x10\":")); Serial.print(cfg.thermalShutdownC_x10);
  Serial.print(F(",\"thermal_recover_c_x10\":")); Serial.print(cfg.thermalRecoverC_x10);
  Serial.print(F(",\"current_zero_adc\":")); Serial.print(cfg.currentZeroAdc);
  Serial.print(F(",\"current_ma_per_count_x1000\":")); Serial.print(cfg.currentMaPerCount_x1000);
  Serial.print(F(",\"telemetry_interval_ms\":")); Serial.print(cfg.telemetryIntervalMs);
  Serial.println(F("}"));
}

bool setConfigValue(const char* key, const char* value) {
  const unsigned long u = strtoul(value, nullptr, 10); const long s = strtol(value, nullptr, 10);
  if (!strcmp(key, "boot_timeout_ms")) cfg.bootTimeoutMs = u;
  else if (!strcmp(key, "heartbeat_timeout_ms")) cfg.heartbeatTimeoutMs = u;
  else if (!strcmp(key, "ignition_off_delay_ms")) cfg.ignitionOffDelayMs = u;
  else if (!strcmp(key, "shutdown_timeout_ms")) cfg.shutdownHardTimeoutMs = u;
  else if (!strcmp(key, "power_off_settle_ms")) cfg.powerOffSettleMs = u;
  else if (!strcmp(key, "service_timeout_ms")) cfg.serviceTimeoutMs = u;
  else if (!strcmp(key, "restart_delay_ms")) cfg.restartDelayMs = u;
  else if (!strcmp(key, "restart_cooldown_ms")) cfg.restartCooldownMs = u;
  else if (!strcmp(key, "stable_run_ms")) cfg.stableRunMs = u;
  else if (!strcmp(key, "max_restart_attempts")) cfg.maxRestartAttempts = static_cast<uint8_t>(u > 20 ? 20 : u);
  else if (!strcmp(key, "telemetry_interval_ms")) cfg.telemetryIntervalMs = u < 250 ? 250 : u;
  else if (!strcmp(key, "adc_ref_mv")) cfg.adcReferenceMv = static_cast<uint16_t>(u);
  else if (!strcmp(key, "low_voltage_enable")) { if (u) cfg.flags |= PSM_FLAG_LOW_VOLTAGE_ENABLE; else cfg.flags &= ~PSM_FLAG_LOW_VOLTAGE_ENABLE; }
  else if (!strcmp(key, "low_warn_mv")) cfg.lowWarnMv = static_cast<uint16_t>(u);
  else if (!strcmp(key, "low_shutdown_mv")) cfg.lowShutdownMv = static_cast<uint16_t>(u);
  else if (!strcmp(key, "low_recover_mv")) cfg.lowRecoverMv = static_cast<uint16_t>(u);
  else if (!strcmp(key, "low_hold_ms")) cfg.lowHoldMs = u;
  else if (!strcmp(key, "thermal_enable")) { if (u) cfg.flags |= PSM_FLAG_THERMAL_ENABLE; else cfg.flags &= ~PSM_FLAG_THERMAL_ENABLE; }
  else if (!strcmp(key, "thermal_shutdown_c_x10")) cfg.thermalShutdownC_x10 = static_cast<int16_t>(s);
  else if (!strcmp(key, "thermal_recover_c_x10")) cfg.thermalRecoverC_x10 = static_cast<int16_t>(s);
  else if (!strcmp(key, "thermal_hold_ms")) cfg.thermalHoldMs = u;
  else if (!strcmp(key, "current_zero_adc")) cfg.currentZeroAdc = static_cast<uint16_t>(u);
  else if (!strcmp(key, "current_ma_per_count_x1000")) cfg.currentMaPerCount_x1000 = static_cast<int32_t>(s);
  else return false;
  return true;
}

void handleCommand(char* line) {
  char* cmd = strtok(line, " \t"); if (!cmd) return;
  if (!strcmp(cmd, "STATUS")) printStatus();
  else if (!strcmp(cmd, "CONFIG")) printConfig();
  else if (!strcmp(cmd, "EVENTS")) psmPrintEvents(Serial);
  else if (!strcmp(cmd, "CLEAR_EVENTS")) { psmClearEvents(); Serial.println(F("OK EVENTS_CLEARED")); }
  else if (!strcmp(cmd, "SAVE")) { psmSaveConfig(cfg); psmLogEvent(PsmEventCode::CONFIG_SAVED); Serial.println(F("OK SAVED")); }
  else if (!strcmp(cmd, "DEFAULTS")) { cfg = psmDefaultConfig(); Serial.println(F("OK DEFAULTS_LOADED_NOT_SAVED")); }
  else if (!strcmp(cmd, "SET")) {
    char* key = strtok(nullptr, " \t"); char* value = strtok(nullptr, " \t");
    Serial.println(key && value && setConfigValue(key, value) ? F("OK SET") : F("ERR SET"));
  } else if (!strcmp(cmd, "HELP")) {
    Serial.println(F("OK COMMANDS: STATUS CONFIG EVENTS CLEAR_EVENTS SAVE DEFAULTS SET <key> <value> HELP"));
  } else Serial.println(F("ERR UNKNOWN_COMMAND"));
}

void serviceSerial() {
  while (Serial.available()) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\r') continue;
    if (ch == '\n') {
      serialLine[serialLineLen] = '\0'; if (serialLineLen) handleCommand(serialLine); serialLineLen = 0;
    } else if (serialLineLen < sizeof(serialLine) - 1) serialLine[serialLineLen++] = ch;
    else { serialLineLen = 0; Serial.println(F("ERR LINE_TOO_LONG")); }
  }
}

void setup() {
  pinMode(PIN_ACC_SENSE, INPUT_PULLUP); pinMode(PIN_SHUTDOWN_ACK, INPUT_PULLUP);
  pinMode(PIN_PI_HEARTBEAT, INPUT_PULLUP); pinMode(PIN_SERVICE_WAKE, INPUT_PULLUP);
  pinMode(PIN_SHUTDOWN_REQ, OUTPUT); pinMode(PIN_MAIN_POWER_EN, OUTPUT); pinMode(PIN_SELF_HOLD, OUTPUT);
  digitalWrite(PIN_SELF_HOLD, HIGH); setShutdownRequest(false); setMainPower(false);
  Serial.begin(115200);
  psmEventLogInit();
  if (!psmLoadConfig(cfg)) psmLogEvent(PsmEventCode::CONFIG_DEFAULTED);
  psmLogEvent(PsmEventCode::POWER_UP);
  resetHeartbeatMonitor(); sampleSensors(true); enterState(SupervisorState::WAKE);
}

void loop() {
  serviceSerial(); updateHeartbeat(); sampleSensors();
  const uint32_t now = millis();
  if (cfg.telemetryIntervalMs && now - lastTelemetryMs >= cfg.telemetryIntervalMs) { lastTelemetryMs = now; printStatus(); }

  switch (state) {
    case SupervisorState::WAKE:
      shutdownCause = ShutdownCause::NONE;
      if (accActive()) { serviceBootLatched = false; enterState(SupervisorState::PRECHECK); }
      else if (serviceRequested()) { serviceBootLatched = true; psmLogEvent(PsmEventCode::SERVICE_START); enterState(SupervisorState::PRECHECK); }
      else releaseSupervisorPower();
      break;

    case SupervisorState::PRECHECK: {
      sampleSensors(true); const ShutdownCause trip = protectionTrip();
      if (trip != ShutdownCause::NONE) { shutdownCause = trip; faultHoldSinceMs = now; enterState(SupervisorState::FAULT_HOLD); break; }
      setMainPower(true); resetHeartbeatMonitor(); psmLogEvent(PsmEventCode::BOOT_START, restartAttempts); enterState(SupervisorState::BOOT); break;
    }

    case SupervisorState::BOOT: {
      const ShutdownCause trip = protectionTrip();
      if (trip != ShutdownCause::NONE) {
        psmLogEvent(trip == ShutdownCause::LOW_VOLTAGE ? PsmEventCode::LOW_VOLT_SHUTDOWN : PsmEventCode::THERMAL_SHUTDOWN, 0,
                    trip == ShutdownCause::LOW_VOLTAGE ? static_cast<int16_t>(sensors.vehicleMv) : sensors.temperatureC_x10);
        beginShutdown(trip);
      } else if (heartbeatAlive()) {
        psmLogEvent(PsmEventCode::BOOT_OK, restartAttempts); stableRunSinceMs = now; stableRunLogged = false;
        enterState(serviceBootLatched && !accActive() ? SupervisorState::SERVICE : SupervisorState::RUN);
      } else if (now - stateEnteredMs >= cfg.bootTimeoutMs) { psmLogEvent(PsmEventCode::BOOT_TIMEOUT, restartAttempts); beginShutdown(ShutdownCause::BOOT_TIMEOUT); }
      break;
    }

    case SupervisorState::RUN: {
      const ShutdownCause trip = protectionTrip();
      if (trip != ShutdownCause::NONE) {
        psmLogEvent(trip == ShutdownCause::LOW_VOLTAGE ? PsmEventCode::LOW_VOLT_SHUTDOWN : PsmEventCode::THERMAL_SHUTDOWN, 0,
                    trip == ShutdownCause::LOW_VOLTAGE ? static_cast<int16_t>(sensors.vehicleMv) : sensors.temperatureC_x10);
        beginShutdown(trip);
      } else if (!heartbeatAlive()) { psmLogEvent(PsmEventCode::HEARTBEAT_LOST, restartAttempts); beginShutdown(ShutdownCause::HEARTBEAT_LOST); }
      else if (!accActive()) { psmLogEvent(PsmEventCode::ACC_OFF); shutdownCause = ShutdownCause::ACC_OFF; enterState(SupervisorState::SHUTDOWN_DELAY); }
      else if (!stableRunLogged && now - stableRunSinceMs >= cfg.stableRunMs) { restartAttempts = 0; stableRunLogged = true; psmLogEvent(PsmEventCode::STABLE_RUN); }
      break;
    }

    case SupervisorState::SHUTDOWN_DELAY:
      if (accActive()) { shutdownCause = ShutdownCause::NONE; enterState(SupervisorState::RUN); }
      else if (now - stateEnteredMs >= cfg.ignitionOffDelayMs) beginShutdown(ShutdownCause::ACC_OFF);
      break;

    case SupervisorState::SHUTDOWN_WAIT:
      if (shutdownAcknowledged()) {
        psmLogEvent(PsmEventCode::SHUTDOWN_ACK, static_cast<uint8_t>(shutdownCause)); setShutdownRequest(false); setMainPower(false);
        psmLogEvent(PsmEventCode::MAIN_POWER_OFF, static_cast<uint8_t>(shutdownCause)); enterState(SupervisorState::MAIN_OFF_WAIT);
      } else if (now - stateEnteredMs >= cfg.shutdownHardTimeoutMs) {
        psmLogEvent(PsmEventCode::SHUTDOWN_HARD_TIMEOUT, static_cast<uint8_t>(shutdownCause)); setShutdownRequest(false); setMainPower(false);
        psmLogEvent(PsmEventCode::MAIN_POWER_OFF, static_cast<uint8_t>(shutdownCause)); enterState(SupervisorState::MAIN_OFF_WAIT);
      }
      break;

    case SupervisorState::MAIN_OFF_WAIT:
      if (now - stateEnteredMs >= cfg.powerOffSettleMs) {
        if (shutdownCause == ShutdownCause::ACC_OFF || shutdownCause == ShutdownCause::SERVICE_TIMEOUT) {
          if (accActive()) { restartAttempts = 0; enterState(SupervisorState::POWER_CYCLE_WAIT); }
          else { if (serviceBootLatched) psmLogEvent(PsmEventCode::SERVICE_END); releaseSupervisorPower(); }
        } else scheduleRestartOrHold();
      }
      break;

    case SupervisorState::POWER_CYCLE_WAIT:
      setMainPower(false); setShutdownRequest(false);
      if (!accActive()) releaseSupervisorPower();
      else if (now - stateEnteredMs >= cfg.restartDelayMs) {
        sampleSensors(true); const ShutdownCause trip = protectionTrip();
        if (trip != ShutdownCause::NONE) { shutdownCause = trip; faultHoldSinceMs = now; enterState(SupervisorState::FAULT_HOLD); }
        else { shutdownCause = ShutdownCause::NONE; setMainPower(true); resetHeartbeatMonitor(); psmLogEvent(PsmEventCode::BOOT_START, restartAttempts); enterState(SupervisorState::BOOT); }
      }
      break;

    case SupervisorState::SERVICE: {
      const ShutdownCause trip = protectionTrip();
      if (trip != ShutdownCause::NONE) {
        psmLogEvent(trip == ShutdownCause::LOW_VOLTAGE ? PsmEventCode::LOW_VOLT_SHUTDOWN : PsmEventCode::THERMAL_SHUTDOWN, 0,
                    trip == ShutdownCause::LOW_VOLTAGE ? static_cast<int16_t>(sensors.vehicleMv) : sensors.temperatureC_x10);
        beginShutdown(trip);
      } else if (accActive()) { serviceBootLatched = false; psmLogEvent(PsmEventCode::SERVICE_END); stableRunSinceMs = now; stableRunLogged = false; enterState(SupervisorState::RUN); }
      else if (!heartbeatAlive()) { psmLogEvent(PsmEventCode::HEARTBEAT_LOST, restartAttempts); beginShutdown(ShutdownCause::HEARTBEAT_LOST); }
      else if (now - stateEnteredMs >= cfg.serviceTimeoutMs) beginShutdown(ShutdownCause::SERVICE_TIMEOUT);
      break;
    }

    case SupervisorState::FAULT_HOLD:
      setMainPower(false); setShutdownRequest(false); sampleSensors();
      if (!accActive()) releaseSupervisorPower();
      else if ((shutdownCause == ShutdownCause::LOW_VOLTAGE || shutdownCause == ShutdownCause::OVER_TEMPERATURE) && protectionRecoveredFor(shutdownCause)) {
        psmLogEvent(PsmEventCode::PROTECTION_RECOVERED, static_cast<uint8_t>(shutdownCause)); restartAttempts = 0; enterState(SupervisorState::POWER_CYCLE_WAIT);
      } else if (shutdownCause != ShutdownCause::LOW_VOLTAGE && shutdownCause != ShutdownCause::OVER_TEMPERATURE && now - faultHoldSinceMs >= cfg.restartCooldownMs) {
        restartAttempts = 0; enterState(SupervisorState::POWER_CYCLE_WAIT);
      }
      break;
  }
  delay(10);
}
