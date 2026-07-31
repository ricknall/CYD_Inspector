#pragma once

#include <Arduino.h>

enum class BoardProfile : uint8_t {
  Unknown,
  ClassicSingleUsb,
  Cyd2Usb
};

const char* boardProfileName(BoardProfile profile);
const char* boardProfileShortName(BoardProfile profile);
const char* boardProfileJsonName(BoardProfile profile);
BoardProfile boardProfileFromArgument(String argument);
bool boardProfileIsKnown(BoardProfile profile);
void printBoardProfileReport(BoardProfile profile);
