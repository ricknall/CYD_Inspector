#include "BoardProfile.h"

#include <Preferences.h>

namespace {

constexpr char PREFERENCES_NAMESPACE[] = "cyd-inspector";
constexpr char PROFILE_KEY[] = "board";

}  // namespace

const char* boardProfileName(const BoardProfile profile) {
  switch (profile) {
    case BoardProfile::ClassicSingleUsb:
      return "CLASSIC CYD / SINGLE USB";
    case BoardProfile::Cyd2Usb:
      return "CYD2USB / TWO USB";
    default:
      return "NOT SELECTED";
  }
}

const char* boardProfileShortName(const BoardProfile profile) {
  switch (profile) {
    case BoardProfile::ClassicSingleUsb:
      return "SINGLE USB";
    case BoardProfile::Cyd2Usb:
      return "TWO USB";
    default:
      return "UNKNOWN";
  }
}

const char* boardProfileJsonName(const BoardProfile profile) {
  switch (profile) {
    case BoardProfile::ClassicSingleUsb:
      return "classic_single_usb";
    case BoardProfile::Cyd2Usb:
      return "cyd2usb";
    default:
      return "unknown";
  }
}

BoardProfile boardProfileFromArgument(String argument) {
  argument.trim();
  argument.toLowerCase();

  if (argument == "1" || argument == "single" ||
      argument == "single-usb" || argument == "classic") {
    return BoardProfile::ClassicSingleUsb;
  }

  if (argument == "2" || argument == "dual" ||
      argument == "two" || argument == "two-usb" ||
      argument == "cyd2usb") {
    return BoardProfile::Cyd2Usb;
  }

  return BoardProfile::Unknown;
}

bool boardProfileIsKnown(const BoardProfile profile) {
  return profile != BoardProfile::Unknown;
}

bool loadBoardProfile(BoardProfile& profile) {
  profile = BoardProfile::Unknown;

  Preferences preferences;
  // Open read-write so a freshly erased board can create the namespace.
  // Read-only mode logs NVS_NOT_FOUND before the first profile is saved.
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const uint8_t storedValue = preferences.getUChar(PROFILE_KEY, 0);
  preferences.end();

  const BoardProfile storedProfile =
      static_cast<BoardProfile>(storedValue);
  if (!boardProfileIsKnown(storedProfile) ||
      storedProfile > BoardProfile::Cyd2Usb) {
    return false;
  }

  profile = storedProfile;
  return true;
}

bool saveBoardProfile(const BoardProfile profile) {
  if (!boardProfileIsKnown(profile)) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const size_t bytesWritten = preferences.putUChar(
      PROFILE_KEY, static_cast<uint8_t>(profile));
  preferences.end();
  return bytesWritten == sizeof(uint8_t);
}

bool clearBoardProfile() {
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const bool cleared =
      !preferences.isKey(PROFILE_KEY) || preferences.remove(PROFILE_KEY);
  preferences.end();
  return cleared;
}

void printBoardProfileReport(const BoardProfile profile) {
  Serial.println("--- BOARD PROFILE ---");
  Serial.printf("Observed family: %s\n", boardProfileName(profile));

  if (!boardProfileIsKnown(profile)) {
    Serial.println(
        "Use 'profile 1' for one USB connector or 'profile 2' for two.");
  }

  Serial.println(
      "Selection is human-assisted and does not prove peripheral pin "
      "mappings.");
}
