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
                      const SdStatus& sd) {
  title(display, "CYD BOARD INSPECTOR");
  display.text(10, 32, "DISPLAY SPI: WORKING", CYAN, 2);
  display.text(
      10,
      54,
      String("DISPLAY IC: ") + displayControllerName(probe.controller),
      GREEN,
      2);
  display.text(10, 76, "BOARD PROFILE: MANUAL", GREEN, 2);
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
  display.text(10, 198, "TOUCH: NOT PROBED", AMBER, 2);
  display.text(10, 224, "NO CONTROLLER OR PIN ASSUMPTIONS", DIM, 1);
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
  display.text(10, 228, "BOARD PROFILE REQUIRED", AMBER, 1);
}

void showTouchPage(CydDisplay& display) {
  title(display, "TOUCH");
  display.text(10, 48, "NOT PROBED", AMBER, 3);
  display.text(10, 88, "CONTROLLER: UNKNOWN", WHITE, 2);
  display.text(10, 112, "PINS: UNKNOWN", WHITE, 2);
  display.text(10, 148, "A BOARD PROFILE MUST", CYAN, 2);
  display.text(10, 172, "PROVE THE PIN MAPPING", CYAN, 2);
  display.text(10, 214, "NO GPIO GUESSING", DIM, 1);
}

