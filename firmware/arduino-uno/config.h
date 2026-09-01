#pragma once

#include <Arduino.h>
#include <EEPROM.h>

constexpr uint32_t PSM_CONFIG_MAGIC = 0x50534D31UL;
constexpr uint16_t PSM_CONFIG_VERSION = 1;
constexpr int PSM_CONFIG_EEPROM_ADDR = 0;
constexpr int PSM_EVENT_EEPROM_ADDR = 128;
constexpr uint8_t PSM_FLAG_LOW_VOLTAGE_ENABLE = 0x01;
constexpr uint8_t PSM_FLAG_THERMAL_ENABLE = 0x02;

struct __attribute__((packed)) PsmConfig {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint8_t flags;
  uint8_t maxRestartAttempts;
  uint16_t reserved0;
  uint32_t bootTimeoutMs;
  uint32_t heartbeatTimeoutMs;
  uint32_t ignitionOffDelayMs;
  uint32_t shutdownHardTimeoutMs;
  uint32_t powerOffSettleMs;
  uint32_t serviceTimeoutMs;
  uint32_t restartDelayMs;
  uint32_t restartCooldownMs;
  uint32_t stableRunMs;
  uint32_t telemetryIntervalMs;
  uint16_t adcReferenceMv;
  uint16_t lowWarnMv;
  uint16_t lowShutdownMv;
  uint16_t lowRecoverMv;
  uint32_t lowHoldMs;
  int16_t thermalShutdownC_x10;
  int16_t thermalRecoverC_x10;
  uint32_t thermalHoldMs;
  uint16_t currentZeroAdc;
  int32_t currentMaPerCount_x1000;
  uint16_t crc;
};

inline uint16_t psmCrc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  while (len--) {
    crc ^= static_cast<uint16_t>(*data++) << 8;
    for (uint8_t i = 0; i < 8; ++i) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

inline PsmConfig psmDefaultConfig() {
  PsmConfig c{};
  c.magic = PSM_CONFIG_MAGIC;
  c.version = PSM_CONFIG_VERSION;
  c.size = sizeof(PsmConfig);
  c.flags = 0;
  c.maxRestartAttempts = 3;
  c.bootTimeoutMs = 120000UL;
  c.heartbeatTimeoutMs = 15000UL;
  c.ignitionOffDelayMs = 30000UL;
  c.shutdownHardTimeoutMs = 120000UL;
  c.powerOffSettleMs = 3000UL;
  c.serviceTimeoutMs = 3600000UL;
  c.restartDelayMs = 10000UL;
  c.restartCooldownMs = 300000UL;
  c.stableRunMs = 600000UL;
  c.telemetryIntervalMs = 1000UL;
  c.adcReferenceMv = 5000;
  c.lowWarnMv = 0;
  c.lowShutdownMv = 0;
  c.lowRecoverMv = 0;
  c.lowHoldMs = 30000UL;
  c.thermalShutdownC_x10 = 0;
  c.thermalRecoverC_x10 = 0;
  c.thermalHoldMs = 5000UL;
  c.currentZeroAdc = 0;
  c.currentMaPerCount_x1000 = 0;
  c.crc = 0;
  c.crc = psmCrc16(reinterpret_cast<const uint8_t*>(&c), sizeof(PsmConfig) - sizeof(c.crc));
  return c;
}

inline bool psmConfigValid(const PsmConfig& c) {
  if (c.magic != PSM_CONFIG_MAGIC || c.version != PSM_CONFIG_VERSION || c.size != sizeof(PsmConfig)) return false;
  return psmCrc16(reinterpret_cast<const uint8_t*>(&c), sizeof(PsmConfig) - sizeof(c.crc)) == c.crc;
}

inline bool psmLoadConfig(PsmConfig& c) {
  EEPROM.get(PSM_CONFIG_EEPROM_ADDR, c);
  if (!psmConfigValid(c)) { c = psmDefaultConfig(); return false; }
  return true;
}

inline void psmSaveConfig(PsmConfig& c) {
  c.magic = PSM_CONFIG_MAGIC;
  c.version = PSM_CONFIG_VERSION;
  c.size = sizeof(PsmConfig);
  c.crc = 0;
  c.crc = psmCrc16(reinterpret_cast<const uint8_t*>(&c), sizeof(PsmConfig) - sizeof(c.crc));
  EEPROM.put(PSM_CONFIG_EEPROM_ADDR, c);
}
