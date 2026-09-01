#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

enum class PsmEventCode : uint8_t {
  POWER_UP = 1, CONFIG_DEFAULTED, CONFIG_SAVED, BOOT_START, BOOT_OK, BOOT_TIMEOUT,
  ACC_OFF, SHUTDOWN_REQUEST, SHUTDOWN_ACK, SHUTDOWN_HARD_TIMEOUT, HEARTBEAT_LOST,
  MAIN_POWER_OFF, POWER_CYCLE, RESTART_LIMIT, SERVICE_START, SERVICE_END,
  LOW_VOLT_WARN, LOW_VOLT_SHUTDOWN, THERMAL_SHUTDOWN, PROTECTION_RECOVERED,
  STABLE_RUN, EVENTS_CLEARED
};

struct __attribute__((packed)) PsmEventRecord {
  uint32_t seq;
  uint32_t uptimeS;
  uint8_t code;
  uint8_t detail;
  int16_t value;
  uint16_t crc;
};

constexpr uint8_t PSM_EVENT_CAPACITY = 32;
static uint32_t psmNextEventSeq = 1;

inline uint16_t psmEventCrc(const PsmEventRecord& r) {
  return psmCrc16(reinterpret_cast<const uint8_t*>(&r), sizeof(PsmEventRecord) - sizeof(r.crc));
}
inline bool psmEventValid(const PsmEventRecord& r) {
  return r.seq != 0 && r.seq != 0xFFFFFFFFUL && psmEventCrc(r) == r.crc;
}
inline int psmEventAddress(uint8_t slot) {
  return PSM_EVENT_EEPROM_ADDR + static_cast<int>(slot) * sizeof(PsmEventRecord);
}
inline void psmEventLogInit() {
  uint32_t maxSeq = 0;
  for (uint8_t i = 0; i < PSM_EVENT_CAPACITY; ++i) {
    PsmEventRecord r{};
    EEPROM.get(psmEventAddress(i), r);
    if (psmEventValid(r) && r.seq > maxSeq) maxSeq = r.seq;
  }
  psmNextEventSeq = maxSeq + 1;
  if (psmNextEventSeq == 0) psmNextEventSeq = 1;
}
inline void psmLogEvent(PsmEventCode code, uint8_t detail = 0, int16_t value = 0) {
  PsmEventRecord r{};
  r.seq = psmNextEventSeq++;
  r.uptimeS = millis() / 1000UL;
  r.code = static_cast<uint8_t>(code);
  r.detail = detail;
  r.value = value;
  r.crc = 0;
  r.crc = psmEventCrc(r);
  EEPROM.put(psmEventAddress(static_cast<uint8_t>((r.seq - 1) % PSM_EVENT_CAPACITY)), r);
}
inline void psmClearEvents() {
  PsmEventRecord blank{};
  for (uint8_t i = 0; i < PSM_EVENT_CAPACITY; ++i) EEPROM.put(psmEventAddress(i), blank);
  psmNextEventSeq = 1;
  psmLogEvent(PsmEventCode::EVENTS_CLEARED);
}
inline const __FlashStringHelper* psmEventName(PsmEventCode code) {
  switch (code) {
    case PsmEventCode::POWER_UP: return F("POWER_UP");
    case PsmEventCode::CONFIG_DEFAULTED: return F("CONFIG_DEFAULTED");
    case PsmEventCode::CONFIG_SAVED: return F("CONFIG_SAVED");
    case PsmEventCode::BOOT_START: return F("BOOT_START");
    case PsmEventCode::BOOT_OK: return F("BOOT_OK");
    case PsmEventCode::BOOT_TIMEOUT: return F("BOOT_TIMEOUT");
    case PsmEventCode::ACC_OFF: return F("ACC_OFF");
    case PsmEventCode::SHUTDOWN_REQUEST: return F("SHUTDOWN_REQUEST");
    case PsmEventCode::SHUTDOWN_ACK: return F("SHUTDOWN_ACK");
    case PsmEventCode::SHUTDOWN_HARD_TIMEOUT: return F("SHUTDOWN_HARD_TIMEOUT");
    case PsmEventCode::HEARTBEAT_LOST: return F("HEARTBEAT_LOST");
    case PsmEventCode::MAIN_POWER_OFF: return F("MAIN_POWER_OFF");
    case PsmEventCode::POWER_CYCLE: return F("POWER_CYCLE");
    case PsmEventCode::RESTART_LIMIT: return F("RESTART_LIMIT");
    case PsmEventCode::SERVICE_START: return F("SERVICE_START");
    case PsmEventCode::SERVICE_END: return F("SERVICE_END");
    case PsmEventCode::LOW_VOLT_WARN: return F("LOW_VOLT_WARN");
    case PsmEventCode::LOW_VOLT_SHUTDOWN: return F("LOW_VOLT_SHUTDOWN");
    case PsmEventCode::THERMAL_SHUTDOWN: return F("THERMAL_SHUTDOWN");
    case PsmEventCode::PROTECTION_RECOVERED: return F("PROTECTION_RECOVERED");
    case PsmEventCode::STABLE_RUN: return F("STABLE_RUN");
    case PsmEventCode::EVENTS_CLEARED: return F("EVENTS_CLEARED");
    default: return F("UNKNOWN");
  }
}
inline void psmPrintEvents(Print& out) {
  const uint32_t first = psmNextEventSeq > PSM_EVENT_CAPACITY ? psmNextEventSeq - PSM_EVENT_CAPACITY : 1;
  for (uint32_t seq = first; seq < psmNextEventSeq; ++seq) {
    PsmEventRecord r{};
    EEPROM.get(psmEventAddress(static_cast<uint8_t>((seq - 1) % PSM_EVENT_CAPACITY)), r);
    if (!psmEventValid(r) || r.seq != seq) continue;
    out.print(F("{\"type\":\"event\",\"seq\":")); out.print(r.seq);
    out.print(F(",\"uptime_s\":")); out.print(r.uptimeS);
    out.print(F(",\"code\":\"")); out.print(psmEventName(static_cast<PsmEventCode>(r.code)));
    out.print(F("\",\"detail\":")); out.print(r.detail);
    out.print(F(",\"value\":")); out.print(r.value);
    out.println(F("}"));
  }
}
