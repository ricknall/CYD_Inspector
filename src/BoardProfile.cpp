#include "BoardProfile.h"

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
