#pragma once

#include <Arduino.h>

#include "BoardProfile.h"
#include "CydDisplay.h"

enum class DisplayAppProfile : uint8_t {
  Unknown,
  Ili9341_2,
  St7789
};

enum class DisplayProfileEvidence : uint8_t {
  None,
  ElectronicReadback,
  UserConfirmed,
  BoardFamilyReference
};

const char* displayAppProfileName(DisplayAppProfile profile);
const char* displayAppProfileArgument(DisplayAppProfile profile);
const char* displayProfileEvidenceName(DisplayProfileEvidence evidence);

DisplayAppProfile displayAppProfileFromArgument(String argument);
DisplayAppProfile displayAppProfileFromController(
    DisplayController controller);
DisplayAppProfile recommendedDisplayAppProfile(BoardProfile boardProfile);

DisplayAppProfile resolveDisplayAppProfile(
    const DisplayProbe& probe,
    DisplayAppProfile confirmedProfile,
    BoardProfile boardProfile,
    DisplayProfileEvidence& evidence);

bool loadConfirmedDisplayAppProfile(BoardProfile boardProfile,
                                    DisplayAppProfile& profile);
bool saveConfirmedDisplayAppProfile(BoardProfile boardProfile,
                                    DisplayAppProfile profile);
bool clearConfirmedDisplayAppProfile(BoardProfile boardProfile);
