#include "ReportPages.h"

#include "BoardConfig.h"

namespace {

constexpr uint16_t BLACK = 0x0000;
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t YELLOW = 0xFFE0;
constexpr uint16_t CYAN = 0x07FF;
constexpr uint16_t GREEN = 0x07E0;
constexpr uint16_t AMBER = 0xFBE0;
constexpr uint16_t DIM = 0x8410;

String bytesToHex(const uint8_t* values, const size_t length) {
  String result;
  char buffer[4];

  for (size_t index = 0; index < length; ++index) {
    if (index != 0) {
      result += ' ';
    }

    snprintf(buffer, sizeof(buffer), "%02X", values[index]);
    result += buffer;
  }

  return result;
}

void title(CydDisplay& display, const String& text) {
  display.fill(BLACK);
  display.text(10, 8, text, YELLOW, 2);
}

}  // namespace

void showOverviewPage(CydDisplay& display,
                      const SystemReport& system,
                      const DisplayProbe& probe,
                      const SdStatus& sd,
                      const BoardProfile profile) {
  title(display, "CYD BOARD INSPECTOR");
  display.text(10, 32, "DISPLAY SPI: WORKING", CYAN, 2);
  display.text(
      10,
      54,
      String("DISPLAY IC: ") + displayControllerName(probe.controller),
      GREEN,
      2);
  display.text(
      10,
      76,
      String("BOARD: ") + boardProfileShortName(profile),
      boardProfileIsKnown(profile) ? GREEN : AMBER,
      2);
  display.text(10, 98, sdDisplayText(sd), AMBER, 2);

  display.text(10, 122, "MCU: " + system.chipModel, WHITE, 2);
  display.text(
      10,
      144,
      "CORES: " + String(system.coreCount) +
          " CPU: " + String(system.cpuFrequencyMhz) + " MHZ",
      WHITE,
      2);
  display.text(
      10,
      166,
      "FLASH: " +
          String(system.flashSizeBytes / (1024UL * 1024UL)) +
          " MB " + system.flashMode,
      WHITE,
      2);
  display.text(
      10,
      188,
      "PSRAM: " + String(system.psramFound ? "YES" : "NO"),
      WHITE,
      2);

  display.text(10, 220, "TYPE HELP IN SERIAL MONITOR", DIM, 1);
}

void showProfilePage(CydDisplay& display, const BoardProfile profile) {
  title(display, "BOARD PROFILE");
  display.text(
      10,
      38,
      boardProfileShortName(profile),
      boardProfileIsKnown(profile) ? GREEN : AMBER,
      3);
  display.text(10, 82, "HUMAN-ASSISTED RESULT", CYAN, 2);
  display.text(10, 116, "COUNT USB CONNECTORS", WHITE, 2);
  display.text(10, 142, "PROFILE 1 = ONE USB", WHITE, 2);
  display.text(10, 164, "PROFILE 2 = TWO USB", WHITE, 2);
  display.text(10, 198, "DOES NOT PROVE PINS", AMBER, 2);
  display.text(10, 224, "TYPE PROFILE IN SERIAL", DIM, 1);
}

void showSystemPage(CydDisplay& display, const SystemReport& system) {
  title(display, "SYSTEM");
  display.text(10, 34, "MCU: " + system.chipModel, CYAN, 2);
  display.text(
      10,
      56,
      "CORES: " + String(system.coreCount) +
          " CPU: " + String(system.cpuFrequencyMhz) + " MHZ",
      WHITE,
      2);
  display.text(10, 78, "REVISION: " + String(system.chipRevision), WHITE, 2);
  display.text(
      10,
      100,
      "FLASH: " + String(system.flashSizeBytes / (1024UL * 1024UL)) +
          " MB " + system.flashMode,
      WHITE,
      2);
  display.text(
      10,
      122,
      "PSRAM: " + String(system.psramFound ? "YES" : "NO"),
      WHITE,
      2);
  display.text(
      10,
      144,
      "HEAP FREE: " + String(system.freeHeapBytes),
      GREEN,
      2);
  display.text(
      10,
      166,
      "HEAP MIN: " + String(system.minimumFreeHeapBytes),
      GREEN,
      2);
  display.text(10, 188, "RESET: " + system.resetReason, AMBER, 2);
  display.text(
      10,
      216,
      "ARDUINO " + system.arduinoCoreVersion +
          " / IDF " + system.espIdfVersion,
      DIM,
      1);
}

void showDisplayPage(CydDisplay& display, const DisplayProbe& probe) {
  title(display, "DISPLAY + TOUCH");
  display.text(10, 34, "PROFILE: MANUAL", GREEN, 2);
  display.text(
      10,
      56,
      String("LCD IC: ") + displayControllerName(probe.controller),
      CYAN,
      2);
  display.text(10, 78, "SIZE: 320 X 240 LANDSCAPE", WHITE, 2);
  display.text(10, 100, "SPI M:13 I:12 CLK:14", WHITE, 2);
  display.text(10, 122, "CS:15 DC:2 BL:21", WHITE, 2);
  display.text(10, 148, "04: " + bytesToHex(probe.id04, sizeof(probe.id04)),
               DIM, 1);
  display.text(10, 160, "D3: " + bytesToHex(probe.idD3, sizeof(probe.idD3)),
               DIM, 1);
  display.text(
      10,
      172,
      "0B: " + bytesToHex(probe.madctl, sizeof(probe.madctl)) +
          " 0C: " +
          bytesToHex(probe.pixelFormat, sizeof(probe.pixelFormat)),
      DIM,
      1);
  display.text(10, 198, "TOUCH: SEE TOUCH REPORT", GREEN, 2);
  display.text(10, 224, "PROFILE-SPECIFIC MAPPING", DIM, 1);
}

void showPeripheralPage(CydDisplay& display, const SdStatus& sd) {
  title(display, "PERIPHERALS");
  display.text(10, 36, sdDisplayText(sd), sd.mounted ? GREEN : AMBER, 2);
  display.text(10, 62, "SD SPI M:23 I:19 C:18", WHITE, 2);
  display.text(10, 84, "SD CS:5  CLOCK:10MHZ", WHITE, 2);

  if (sd.mounted) {
    display.text(
        10,
        110,
        "TYPE: " + String(sdCardTypeName(sd.cardType)),
        CYAN,
        2);
    display.text(
        10,
        132,
        "USED: " +
            String(static_cast<uint32_t>(
                sd.usedBytes / (1024ULL * 1024ULL))) +
            " MIB",
        CYAN,
        2);
  }

  display.text(10, 160, "RGB LED: UNASSIGNED", DIM, 2);
  display.text(10, 182, "SPEAKER: UNASSIGNED", DIM, 2);
  display.text(10, 204, "LIGHT: UNASSIGNED", DIM, 2);
  display.text(10, 228, "PROFILE DOES NOT PROVE PINS", AMBER, 1);
}

void showTouchPage(CydDisplay& display, const BoardProfile profile) {
  title(display, "TOUCH");

  if (profile != BoardProfile::ClassicSingleUsb) {
    display.text(10, 48, "NOT AVAILABLE", AMBER, 3);
    display.text(10, 94, "NO CANDIDATE MAPPING", WHITE, 2);
    display.text(10, 126, "SELECTED PROFILE", CYAN, 2);
    display.text(10, 150, "IS NOT SUPPORTED YET", CYAN, 2);
    display.text(10, 214, "NO GPIO GUESSING", DIM, 1);
    return;
  }

  display.text(10, 42, "MAPPED MONITOR", GREEN, 3);
  display.text(10, 84, "CONTROLLER: XPT2046", WHITE, 2);
  display.text(10, 110, "PRESS SCREEN", CYAN, 2);
  display.text(10, 136, "AN X MARKS EACH TOUCH", CYAN, 2);
  display.text(10, 176, "TYPE TOUCH AGAIN", WHITE, 2);
  display.text(10, 198, "TO STOP", WHITE, 2);
  display.text(10, 226, "ACTIVE RANGE PRINTED IN SERIAL", DIM, 1);
}

void showCalibrationTarget(CydDisplay& display, const uint8_t targetIndex) {
  constexpr int16_t targetX[] = {20, 299, 160, 20, 299};
  constexpr int16_t targetY[] = {20, 20, 120, 219, 219};

  if (targetIndex >= 5) {
    return;
  }

  display.fill(BLACK);
  display.text(126,
               4,
               "CAL " + String(targetIndex + 1) + "/5",
               WHITE,
               1);
  display.text(targetX[targetIndex] - 7,
               targetY[targetIndex] - 10,
               "X",
               WHITE,
               3);
}

void showCalibrationResult(CydDisplay& display,
                           const int32_t rawTop,
                           const int32_t rawBottom,
                           const int32_t rawLeft,
                           const int32_t rawRight,
                           const bool persisted) {
  title(display, "TOUCH CALIBRATED");
  display.text(10, 48, "TOP: " + String(rawTop), WHITE, 2);
  display.text(10, 76, "BOTTOM: " + String(rawBottom), WHITE, 2);
  display.text(10, 104, "LEFT: " + String(rawLeft), WHITE, 2);
  display.text(10, 132, "RIGHT: " + String(rawRight), WHITE, 2);
  display.text(10,
               166,
               persisted ? "SAVED ACROSS RESETS" : "SAVE FAILED",
               persisted ? GREEN : AMBER,
               2);
  display.text(10, 198, "CENTER CHECK IN SERIAL", CYAN, 2);
  display.text(10, 224, "TYPE TOUCH TO VERIFY", GREEN, 1);
}

void showRgbProbePage(CydDisplay& display,
                      const BoardProfile profile,
                      const int activePin) {
  title(display, "RGB LED PIN PROBE");

  if (profile != BoardProfile::ClassicSingleUsb) {
    display.text(10, 50, "NOT AVAILABLE", AMBER, 3);
    display.text(10, 98, "NO CANDIDATE MAPPING", WHITE, 2);
    display.text(10, 126, "FOR THIS PROFILE", WHITE, 2);
    return;
  }

  display.text(10, 42, "ACTIVE LOW CANDIDATES", CYAN, 2);
  display.text(10, 72, "GPIO 4", WHITE, 2);
  display.text(110, 72, "GPIO 16", WHITE, 2);
  display.text(220, 72, "GPIO 17", WHITE, 2);

  if (activePin < 0) {
    display.text(10, 116, "ALL CHANNELS OFF", GREEN, 3);
  } else {
    display.text(10,
                 116,
                 "GPIO " + String(activePin) + " IS ON",
                 AMBER,
                 3);
  }

  display.text(10, 166, "LOOK AT LED ON BACK", WHITE, 2);
  display.text(10, 194, "REPORT OBSERVED COLOR", WHITE, 2);
  display.text(10, 224, "RGB 4 / 16 / 17 / OFF", DIM, 1);
}
