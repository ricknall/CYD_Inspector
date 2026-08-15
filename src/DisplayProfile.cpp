#include "DisplayProfile.h"

#include <Preferences.h>

namespace {

constexpr char PREFERENCES_NAMESPACE[] = "cyd-display";

const char* preferenceKey(const BoardProfile boardProfile) {
  switch (boardProfile) {
    case BoardProfile::ClassicSingleUsb:
      return "single";
    case BoardProfile::Cyd2Usb:
      return "dual";
    default:
      return nullptr;
  }
}

bool validAppProfile(const DisplayAppProfile profile) {
  return profile == DisplayAppProfile::Ili9341_2 ||
         profile == DisplayAppProfile::St7789;
}

}  // namespace

const char* displayAppProfileName(const DisplayAppProfile profile) {
  switch (profile) {
    case DisplayAppProfile::Ili9341_2:
      return "ILI9341_2";
    case DisplayAppProfile::St7789:
      return "ST7789";
    default:
      return "UNASSIGNED";
  }
}

const char* displayAppProfileArgument(const DisplayAppProfile profile) {
  switch (profile) {
    case DisplayAppProfile::Ili9341_2:
      return "ili9341";
    case DisplayAppProfile::St7789:
      return "st7789";
    default:
      return "unknown";
  }
}

const char* displayProfileEvidenceName(
    const DisplayProfileEvidence evidence) {
  switch (evidence) {
    case DisplayProfileEvidence::ElectronicReadback:
      return "ELECTRONIC ID";
    case DisplayProfileEvidence::UserConfirmed:
      return "VISUAL TEST CONFIRMED";
    case DisplayProfileEvidence::BoardFamilyReference:
      return "BOARD-FAMILY REFERENCE";
    default:
      return "NONE";
  }
}

DisplayAppProfile displayAppProfileFromArgument(String argument) {
  argument.trim();
  argument.toLowerCase();

  if (argument == "ili9341" || argument == "ili9341_2" ||
      argument == "ili9341-2" || argument == "1") {
    return DisplayAppProfile::Ili9341_2;
  }

  if (argument == "st7789" || argument == "st7789v" ||
      argument == "2") {
    return DisplayAppProfile::St7789;
  }

  return DisplayAppProfile::Unknown;
}

DisplayAppProfile displayAppProfileFromController(
    const DisplayController controller) {
  switch (controller) {
    case DisplayController::Ili9341:
      return DisplayAppProfile::Ili9341_2;
    case DisplayController::St7789V:
      return DisplayAppProfile::St7789;
    default:
      return DisplayAppProfile::Unknown;
  }
}

DisplayAppProfile recommendedDisplayAppProfile(
    const BoardProfile boardProfile) {
  switch (boardProfile) {
    case BoardProfile::ClassicSingleUsb:
      return DisplayAppProfile::Ili9341_2;
    case BoardProfile::Cyd2Usb:
      return DisplayAppProfile::St7789;
    default:
      return DisplayAppProfile::Unknown;
  }
}

DisplayAppProfile resolveDisplayAppProfile(
    const DisplayProbe& probe,
    const DisplayAppProfile confirmedProfile,
    const BoardProfile boardProfile,
    DisplayProfileEvidence& evidence) {
  if (validAppProfile(confirmedProfile)) {
    evidence = DisplayProfileEvidence::UserConfirmed;
    return confirmedProfile;
  }

  const DisplayAppProfile electronicProfile =
      displayAppProfileFromController(probe.controller);
  if (validAppProfile(electronicProfile)) {
    evidence = DisplayProfileEvidence::ElectronicReadback;
    return electronicProfile;
  }

  const DisplayAppProfile recommendedProfile =
      recommendedDisplayAppProfile(boardProfile);
  if (validAppProfile(recommendedProfile)) {
    evidence = DisplayProfileEvidence::BoardFamilyReference;
    return recommendedProfile;
  }

  evidence = DisplayProfileEvidence::None;
  return DisplayAppProfile::Unknown;
}

bool loadConfirmedDisplayAppProfile(const BoardProfile boardProfile,
                                    DisplayAppProfile& profile) {
  profile = DisplayAppProfile::Unknown;
  const char* key = preferenceKey(boardProfile);
  if (key == nullptr) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const uint8_t storedValue = preferences.getUChar(key, 0);
  preferences.end();

  const DisplayAppProfile storedProfile =
      static_cast<DisplayAppProfile>(storedValue);
  if (!validAppProfile(storedProfile)) {
    return false;
  }

  profile = storedProfile;
  return true;
}

bool saveConfirmedDisplayAppProfile(const BoardProfile boardProfile,
                                    const DisplayAppProfile profile) {
  const char* key = preferenceKey(boardProfile);
  if (key == nullptr || !validAppProfile(profile)) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const size_t bytesWritten = preferences.putUChar(
      key, static_cast<uint8_t>(profile));
  preferences.end();
  return bytesWritten == sizeof(uint8_t);
}

bool clearConfirmedDisplayAppProfile(const BoardProfile boardProfile) {
  const char* key = preferenceKey(boardProfile);
  if (key == nullptr) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const bool cleared = !preferences.isKey(key) || preferences.remove(key);
  preferences.end();
  return cleared;
}
