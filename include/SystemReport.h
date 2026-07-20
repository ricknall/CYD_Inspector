#pragma once

#include <Arduino.h>

struct SystemReport {
  String chipModel;
  uint16_t chipRevision = 0;
  uint8_t coreCount = 0;
  uint32_t cpuFrequencyMhz = 0;

  uint32_t flashSizeBytes = 0;
  uint32_t flashSpeedHz = 0;
  String flashMode;

  bool psramFound = false;
  uint32_t psramSizeBytes = 0;
  uint32_t freePsramBytes = 0;

  uint32_t heapSizeBytes = 0;
  uint32_t freeHeapBytes = 0;
  uint32_t minimumFreeHeapBytes = 0;
  uint32_t largestFreeHeapBlockBytes = 0;

  String resetReason;
  String arduinoCoreVersion;
  String espIdfVersion;
};

SystemReport collectSystemReport();
void printSystemReport(const SystemReport& report);

