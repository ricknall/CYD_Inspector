#pragma once

#include <Arduino.h>
#include <SD.h>

struct SdStatus {
  bool mounted = false;
  uint8_t cardType = CARD_NONE;

  uint64_t cardBytes = 0;
  uint64_t volumeBytes = 0;
  uint64_t usedBytes = 0;
};

const char* sdCardTypeName(uint8_t type);
SdStatus inspectSdCard();
String sdDisplayText(const SdStatus& status);
void printPeripheralReport(const SdStatus& status);

