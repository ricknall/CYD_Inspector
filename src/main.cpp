#include <Arduino.h>

#include "BoardConfig.h"
#include "BoardProfile.h"
#include "CydDisplay.h"
#include "LightProbe.h"
#include "PeripheralReport.h"
#include "ReportPages.h"
#include "RgbProbe.h"
#include "SystemReport.h"
#include "TouchProbe.h"

namespace {

CydDisplay display;

SystemReport systemReport;
DisplayProbe displayProbe;
SdStatus sdStatus;
BoardProfile boardProfile = BoardProfile::Unknown;
bool boardProfilePersisted = false;
String commandBuffer;
bool swallowNextLineFeed = false;
bool touchMonitorActive = false;
bool touchWasPressed = false;
uint32_t nextTouchSampleMillis = 0;
bool calibrationActive = false;
uint8_t calibrationTargetIndex = 0;
bool touchCalibrationPersisted = false;

struct CalibrationCapture {
  uint32_t sumRawX = 0;
  uint32_t sumRawY = 0;
  uint16_t sampleCount = 0;
  uint16_t averageRawX = 0;
  uint16_t averageRawY = 0;
};

CalibrationCapture calibrationCaptures[5];

constexpr int32_t CAL_TARGET_LEFT = 20;
constexpr int32_t CAL_TARGET_RIGHT = 299;
constexpr int32_t CAL_TARGET_TOP = 20;
constexpr int32_t CAL_TARGET_BOTTOM = 219;

int32_t extrapolateRaw(const int32_t rawAtStart,
                       const int32_t rawAtEnd,
                       const int32_t screenStart,
                       const int32_t screenEnd,
                       const int32_t screenWanted) {
  return rawAtStart +
      static_cast<int32_t>(
          (static_cast<int64_t>(rawAtEnd - rawAtStart) *
           (screenWanted - screenStart)) /
          (screenEnd - screenStart));
}

void resetCalibrationCaptures() {
  for (CalibrationCapture& capture : calibrationCaptures) {
    capture = CalibrationCapture{};
  }
}

void finishCalibration() {
  const int32_t rawTopInset =
      (calibrationCaptures[0].averageRawX +
       calibrationCaptures[1].averageRawX) / 2;
  const int32_t rawBottomInset =
      (calibrationCaptures[3].averageRawX +
       calibrationCaptures[4].averageRawX) / 2;
  const int32_t rawLeftInset =
      (calibrationCaptures[0].averageRawY +
       calibrationCaptures[3].averageRawY) / 2;
  const int32_t rawRightInset =
      (calibrationCaptures[1].averageRawY +
       calibrationCaptures[4].averageRawY) / 2;

  TouchCalibration calibration;
  calibration.rawTop = extrapolateRaw(rawTopInset,
                                       rawBottomInset,
                                       CAL_TARGET_TOP,
                                       CAL_TARGET_BOTTOM,
                                       0);
  calibration.rawBottom = extrapolateRaw(rawTopInset,
                                          rawBottomInset,
                                          CAL_TARGET_TOP,
                                          CAL_TARGET_BOTTOM,
                                          239);
  calibration.rawLeft = extrapolateRaw(rawLeftInset,
                                        rawRightInset,
                                        CAL_TARGET_LEFT,
                                        CAL_TARGET_RIGHT,
                                        0);
  calibration.rawRight = extrapolateRaw(rawLeftInset,
                                         rawRightInset,
                                         CAL_TARGET_LEFT,
                                         CAL_TARGET_RIGHT,
                                         319);
  setTouchCalibration(calibration);
  touchCalibrationPersisted = saveTouchCalibration(calibration);

  uint16_t centerX = 0;
  uint16_t centerY = 0;
  mapTouchCoordinates(calibrationCaptures[2].averageRawX,
                      calibrationCaptures[2].averageRawY,
                      centerX,
                      centerY);

  Serial.println("--- CALIBRATION COMPLETE ---");
  Serial.printf("Raw edges: top=%ld bottom=%ld left=%ld right=%ld\n",
                static_cast<long>(calibration.rawTop),
                static_cast<long>(calibration.rawBottom),
                static_cast<long>(calibration.rawLeft),
                static_cast<long>(calibration.rawRight));
  Serial.printf("Center check: expected=(160,120) measured=(%u,%u)\n",
                centerX,
                centerY);
  Serial.printf("Saved across resets: %s\n",
                touchCalibrationPersisted ? "YES" : "NO - SAVE FAILED");

  calibrationActive = false;
  touchMonitorActive = false;
  touchWasPressed = false;
  showCalibrationResult(display,
                        calibration.rawTop,
                        calibration.rawBottom,
                        calibration.rawLeft,
                        calibration.rawRight,
                        touchCalibrationPersisted);
}

void printTouchReport() {
  Serial.println("--- TOUCH ---");

  if (boardProfile != BoardProfile::ClassicSingleUsb) {
    Serial.println("Controller and pins: not assigned for this profile.");
    return;
  }

  const TouchCalibration calibration = getTouchCalibration();
  Serial.println("Controller: XPT2046-compatible (confirmed)");
  Serial.printf("Pins: MOSI=%d MISO=%d SCLK=%d CS=%d IRQ=%d\n",
                cyd::TOUCH_MOSI,
                cyd::TOUCH_MISO,
                cyd::TOUCH_SCLK,
                cyd::TOUCH_CS,
                cyd::TOUCH_IRQ);
  Serial.printf("Raw edges: top=%ld bottom=%ld left=%ld right=%ld\n",
                static_cast<long>(calibration.rawTop),
                static_cast<long>(calibration.rawBottom),
                static_cast<long>(calibration.rawLeft),
                static_cast<long>(calibration.rawRight));
  Serial.printf("Calibration saved: %s\n",
                touchCalibrationPersisted ? "YES" : "NO");
}

void completeCalibrationTarget() {
  CalibrationCapture& capture =
      calibrationCaptures[calibrationTargetIndex];

  if (capture.sampleCount == 0) {
    Serial.println("No valid samples captured; tap the target again.");
    return;
  }

  capture.averageRawX = static_cast<uint16_t>(
      capture.sumRawX / capture.sampleCount);
  capture.averageRawY = static_cast<uint16_t>(
      capture.sumRawY / capture.sampleCount);
  Serial.printf("CAL target %u/5 samples=%u RAW_X=%u RAW_Y=%u\n",
                calibrationTargetIndex + 1,
                capture.sampleCount,
                capture.averageRawX,
                capture.averageRawY);

  ++calibrationTargetIndex;
  if (calibrationTargetIndex >= 5) {
    finishCalibration();
  } else {
    showCalibrationTarget(display, calibrationTargetIndex);
  }
}

void printHexBytes(const char* label,
                   const uint8_t* values,
                   const size_t length) {
  Serial.print(label);
  Serial.print(':');

  for (size_t index = 0; index < length; ++index) {
    Serial.printf(" %02X", values[index]);
  }

  Serial.println();
}

void printJsonByteArray(const uint8_t* values, const size_t length) {
  Serial.print('[');

  for (size_t index = 0; index < length; ++index) {
    if (index != 0) {
      Serial.print(',');
    }

    Serial.printf("\"%02X\"", values[index]);
  }

  Serial.print(']');
}

void printDisplayReport(const DisplayProbe& probe) {
  Serial.println("--- DISPLAY ---");
  Serial.println("Profile: compatible manual CYD profile");
  Serial.printf("Controller result: %s\n",
                displayControllerName(probe.controller));
  Serial.printf("Resolution: %u x %u, orientation=landscape\n",
                cyd::DISPLAY_WIDTH,
                cyd::DISPLAY_HEIGHT);
  Serial.printf("SPI: MOSI=%d MISO=%d SCLK=%d CS=%d DC=%d at %lu MHz\n",
                cyd::LCD_MOSI,
                cyd::LCD_MISO,
                cyd::LCD_SCLK,
                cyd::LCD_CS,
                cyd::LCD_DC,
                static_cast<unsigned long>(cyd::SPI_FREQUENCY / 1000000UL));
  Serial.printf("Backlight: pin=%d, active=HIGH\n", cyd::LCD_BL);
  printHexBytes("  RDDID  04", probe.id04, sizeof(probe.id04));
  printHexBytes("  RDID4  D3", probe.idD3, sizeof(probe.idD3));
  printHexBytes("  RDID1  DA", probe.idDA, sizeof(probe.idDA));
  printHexBytes("  RDID2  DB", probe.idDB, sizeof(probe.idDB));
  printHexBytes("  RDID3  DC", probe.idDC, sizeof(probe.idDC));
  printHexBytes("  MADCTL 0B", probe.madctl, sizeof(probe.madctl));
  printHexBytes("  PIXFMT 0C", probe.pixelFormat,
                sizeof(probe.pixelFormat));
}

void printInventory() {
  Serial.println();
  Serial.println("=== CYD BOARD INSPECTOR REPORT ===");
  printBoardProfileReport(boardProfile);
  Serial.printf("Saved across resets: %s\n",
                boardProfilePersisted ? "YES" : "NO");
  printSystemReport(systemReport);
  printDisplayReport(displayProbe);
  printTouchReport();
  printPeripheralReport(sdStatus);
}

void printJsonReport() {
  Serial.println('{');
  Serial.println("  \"board\": {");
  Serial.printf("    \"profile\": \"%s\",\n",
                boardProfileJsonName(boardProfile));
  Serial.printf("    \"usb_connector_count\": %u,\n",
                boardProfile == BoardProfile::ClassicSingleUsb ? 1U :
                boardProfile == BoardProfile::Cyd2Usb ? 2U : 0U);
  Serial.println("    \"selection_method\": \"human_assisted\",");
  Serial.printf("    \"persisted\": %s\n",
                boardProfilePersisted ? "true" : "false");
  Serial.println("  },");
  Serial.println("  \"system\": {");
  Serial.printf("    \"mcu\": \"%s\",\n", systemReport.chipModel.c_str());
  Serial.printf("    \"revision\": %u,\n",
                static_cast<unsigned>(systemReport.chipRevision));
  Serial.printf("    \"cores\": %u,\n",
                static_cast<unsigned>(systemReport.coreCount));
  Serial.printf("    \"cpu_mhz\": %lu,\n",
                static_cast<unsigned long>(systemReport.cpuFrequencyMhz));
  Serial.printf("    \"flash_bytes\": %lu,\n",
                static_cast<unsigned long>(systemReport.flashSizeBytes));
  Serial.printf("    \"flash_hz\": %lu,\n",
                static_cast<unsigned long>(systemReport.flashSpeedHz));
  Serial.printf("    \"flash_mode\": \"%s\",\n",
                systemReport.flashMode.c_str());
  Serial.printf("    \"psram_found\": %s,\n",
                systemReport.psramFound ? "true" : "false");
  Serial.printf("    \"psram_bytes\": %lu,\n",
                static_cast<unsigned long>(systemReport.psramSizeBytes));
  Serial.printf("    \"heap_total_bytes\": %lu,\n",
                static_cast<unsigned long>(systemReport.heapSizeBytes));
  Serial.printf("    \"heap_free_bytes\": %lu,\n",
                static_cast<unsigned long>(systemReport.freeHeapBytes));
  Serial.printf("    \"heap_minimum_free_bytes\": %lu,\n",
                static_cast<unsigned long>(
                    systemReport.minimumFreeHeapBytes));
  Serial.printf("    \"reset_reason\": \"%s\",\n",
                systemReport.resetReason.c_str());
  Serial.printf("    \"arduino_esp32\": \"%s\",\n",
                systemReport.arduinoCoreVersion.c_str());
  Serial.printf("    \"esp_idf\": \"%s\"\n",
                systemReport.espIdfVersion.c_str());
  Serial.println("  },");

  Serial.println("  \"display\": {");
  Serial.printf("    \"controller\": \"%s\",\n",
                displayControllerName(displayProbe.controller));
  Serial.printf("    \"width\": %u,\n", cyd::DISPLAY_WIDTH);
  Serial.printf("    \"height\": %u,\n", cyd::DISPLAY_HEIGHT);
  Serial.printf("    \"mosi\": %d, \"miso\": %d, \"sclk\": %d,\n",
                cyd::LCD_MOSI,
                cyd::LCD_MISO,
                cyd::LCD_SCLK);
  Serial.printf("    \"cs\": %d, \"dc\": %d, \"backlight\": %d,\n",
                cyd::LCD_CS,
                cyd::LCD_DC,
                cyd::LCD_BL);
  Serial.print("    \"rddid_04\": ");
  printJsonByteArray(displayProbe.id04, sizeof(displayProbe.id04));
  Serial.println(',');
  Serial.print("    \"rdid4_d3\": ");
  printJsonByteArray(displayProbe.idD3, sizeof(displayProbe.idD3));
  Serial.println(',');
  Serial.print("    \"madctl_0b\": ");
  printJsonByteArray(displayProbe.madctl, sizeof(displayProbe.madctl));
  Serial.println(',');
  Serial.print("    \"pixfmt_0c\": ");
  printJsonByteArray(displayProbe.pixelFormat,
                     sizeof(displayProbe.pixelFormat));
  Serial.println();
  Serial.println("  },");

  const TouchCalibration calibration = getTouchCalibration();
  Serial.println("  \"touch\": {");
  Serial.printf("    \"supported\": %s,\n",
                boardProfile == BoardProfile::ClassicSingleUsb
                    ? "true" : "false");
  Serial.printf("    \"calibration_saved\": %s,\n",
                touchCalibrationPersisted ? "true" : "false");
  Serial.printf("    \"raw_top\": %ld, \"raw_bottom\": %ld,\n",
                static_cast<long>(calibration.rawTop),
                static_cast<long>(calibration.rawBottom));
  Serial.printf("    \"raw_left\": %ld, \"raw_right\": %ld\n",
                static_cast<long>(calibration.rawLeft),
                static_cast<long>(calibration.rawRight));
  Serial.println("  },");

  Serial.println("  \"sd\": {");
  Serial.printf("    \"mounted\": %s,\n",
                sdStatus.mounted ? "true" : "false");
  Serial.printf("    \"type\": \"%s\",\n",
                sdCardTypeName(sdStatus.cardType));
  Serial.printf("    \"card_bytes\": %llu,\n",
                static_cast<unsigned long long>(sdStatus.cardBytes));
  Serial.printf("    \"volume_bytes\": %llu,\n",
                static_cast<unsigned long long>(sdStatus.volumeBytes));
  Serial.printf("    \"used_bytes\": %llu\n",
                static_cast<unsigned long long>(sdStatus.usedBytes));
  Serial.println("  }");
  Serial.println('}');
}

void printHelp() {
  Serial.println();
  Serial.println("CYD Inspector serial commands:");
  Serial.println("  help         Show this command list");
  Serial.println("  report       Print the complete human-readable report");
  Serial.println("  profile      Show board-profile selection instructions");
  Serial.println("  profile 1    Select classic CYD with one USB connector");
  Serial.println("  profile 2    Select CYD2USB with two USB connectors");
  Serial.println("  profile clear  Clear the human-assisted selection");
  Serial.println("  system       Print MCU, memory, reset, and software details");
  Serial.println("  display      Print display profile and probe details");
  Serial.println("  peripherals  Print SD and peripheral-assumption details");
  Serial.println("  touch        Start or stop the mapped touch monitor");
  Serial.println("  calibrate    Run five-point touch calibration");
  Serial.println("  calibrate clear  Delete saved touch calibration");
  Serial.println("  rgb          Show confirmed RGB mapping and current state");
  Serial.println("  rgb 4        Turn RED channel on (active LOW)");
  Serial.println("  rgb 16       Turn GREEN channel on (active LOW)");
  Serial.println("  rgb 17       Turn BLUE channel on (active LOW)");
  Serial.println("  rgb off      Turn all RGB channels off");
  Serial.println("  light        Read the confirmed GPIO 34 light sensor");
  Serial.println("  json         Print the complete report as JSON");
}

void runCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    return;
  }

  if (command == "help") {
    printHelp();
  } else if (command == "report") {
    showOverviewPage(
        display, systemReport, displayProbe, sdStatus, boardProfile);
    printInventory();
  } else if (command == "profile") {
    showProfilePage(display, boardProfile);
    printBoardProfileReport(boardProfile);
    Serial.printf("Saved across resets: %s\n",
                  boardProfilePersisted ? "YES" : "NO");
  } else if (command.startsWith("profile ")) {
    String argument = command.substring(8);
    argument.trim();

    if (argument == "clear" || argument == "unknown" || argument == "0") {
      boardProfile = BoardProfile::Unknown;
      boardProfilePersisted = false;

      if (clearBoardProfile()) {
        Serial.println("Board profile selection and saved value cleared.");
      } else {
        Serial.println(
            "WARNING: profile cleared for this session, but saved-value "
            "removal failed.");
      }
    } else {
      const BoardProfile selected = boardProfileFromArgument(argument);

      if (!boardProfileIsKnown(selected)) {
        Serial.printf("Unknown profile choice: %s\n", argument.c_str());
        Serial.println("Use 'profile 1', 'profile 2', or 'profile clear'.");
        showProfilePage(display, boardProfile);
        return;
      }

      boardProfile = selected;
      boardProfilePersisted = saveBoardProfile(boardProfile);
      if (boardProfile == BoardProfile::ClassicSingleUsb) {
        touchCalibrationPersisted = loadTouchCalibration();
      } else {
        rgbProbeOff();
        setTouchCalibration(TouchCalibration{});
        touchCalibrationPersisted = false;
      }
      Serial.printf("Board profile selected: %s\n",
                    boardProfileName(boardProfile));
      Serial.printf("Saved across resets: %s\n",
                    boardProfilePersisted ? "YES" : "NO - SAVE FAILED");
    }

    showOverviewPage(
        display, systemReport, displayProbe, sdStatus, boardProfile);
  } else if (command == "system") {
    showSystemPage(display, systemReport);
    printSystemReport(systemReport);
  } else if (command == "display") {
    showDisplayPage(display, displayProbe);
    printDisplayReport(displayProbe);
  } else if (command == "peripherals") {
    showPeripheralPage(display, sdStatus);
    printPeripheralReport(sdStatus);
  } else if (command == "touch") {
    if (calibrationActive) {
      calibrationActive = false;
      touchMonitorActive = false;
      touchWasPressed = false;
      Serial.println("Touch calibration cancelled.");
      showOverviewPage(
          display, systemReport, displayProbe, sdStatus, boardProfile);
      return;
    }

    if (touchMonitorActive) {
      touchMonitorActive = false;
      touchWasPressed = false;
      Serial.println("Mapped touch monitor stopped.");
      showOverviewPage(
          display, systemReport, displayProbe, sdStatus, boardProfile);
    } else if (beginTouchProbe(boardProfile)) {
      touchMonitorActive = true;
      touchWasPressed = false;
      nextTouchSampleMillis = 0;
      showTouchPage(display, boardProfile);
      printTouchConfiguration(boardProfile);
    } else {
      showTouchPage(display, boardProfile);
      printTouchConfiguration(boardProfile);
    }
  } else if (command == "calibrate clear") {
    calibrationActive = false;
    touchMonitorActive = false;
    touchWasPressed = false;
    touchCalibrationPersisted = false;
    Serial.printf("Saved touch calibration cleared: %s\n",
                  clearTouchCalibration() ? "YES" : "NO - CLEAR FAILED");
    showOverviewPage(
        display, systemReport, displayProbe, sdStatus, boardProfile);
  } else if (command == "calibrate") {
    if (calibrationActive) {
      calibrationActive = false;
      touchMonitorActive = false;
      touchWasPressed = false;
      Serial.println("Touch calibration cancelled.");
      showOverviewPage(
          display, systemReport, displayProbe, sdStatus, boardProfile);
    } else if (beginTouchProbe(boardProfile)) {
      resetCalibrationCaptures();
      calibrationTargetIndex = 0;
      calibrationActive = true;
      touchMonitorActive = true;
      touchWasPressed = false;
      nextTouchSampleMillis = 0;
      Serial.println("Five-point calibration started.");
      Serial.println("Tap and briefly hold each displayed X, then release.");
      showCalibrationTarget(display, calibrationTargetIndex);
    } else {
      printTouchConfiguration(boardProfile);
    }
  } else if (command == "rgb") {
    if (beginRgbProbe(boardProfile)) {
      showRgbProbePage(display, boardProfile, rgbProbeActivePin());
    } else {
      showRgbProbePage(display, boardProfile, -1);
    }
    printRgbProbeConfiguration(boardProfile);
  } else if (command.startsWith("rgb ")) {
    String argument = command.substring(4);
    argument.trim();

    if (!beginRgbProbe(boardProfile)) {
      showRgbProbePage(display, boardProfile, -1);
      printRgbProbeConfiguration(boardProfile);
    } else if (argument == "off" || argument == "0") {
      rgbProbeOff();
      Serial.println("RGB candidate GPIOs 4, 16, and 17 are OFF.");
      showRgbProbePage(display, boardProfile, rgbProbeActivePin());
    } else {
      const int pin = argument.toInt();
      if (!rgbProbeSetPin(pin)) {
        Serial.printf("Invalid RGB candidate: %s\n", argument.c_str());
        Serial.println("Use rgb 4, rgb 16, rgb 17, or rgb off.");
      } else {
        Serial.printf(
            "GPIO %d driven LOW; GPIOs 4, 16, and 17 otherwise HIGH.\n",
            pin);
        Serial.println("Observe the physical LED and report its color.");
      }
      showRgbProbePage(display, boardProfile, rgbProbeActivePin());
    }
  } else if (command == "light") {
    if (!beginLightProbe(boardProfile)) {
      showLightProbePage(display, boardProfile, 0, 0, 0, 0);
      printLightProbeConfiguration(boardProfile);
    } else {
      const LightSample sample = readLightSample();
      printLightProbeConfiguration(boardProfile);
      printLightSample(sample);
      showLightProbePage(display,
                         boardProfile,
                         sample.rawAverage,
                         sample.rawMinimum,
                         sample.rawMaximum,
                         sample.millivolts);
    }
  } else if (command == "json") {
    printJsonReport();
  } else {
    Serial.printf("Unknown command: %s\n", command.c_str());
    Serial.println("Type help for the command list.");
  }
}

void printPrompt() {
  Serial.print("> ");
}

void submitCommandBuffer() {
  Serial.println();
  runCommand(commandBuffer);
  commandBuffer = "";
  printPrompt();
}

void serviceSerialConsole() {
  while (Serial.available() > 0) {
    const char character = static_cast<char>(Serial.read());

    if (character == '\r') {
      submitCommandBuffer();
      swallowNextLineFeed = true;
      continue;
    }

    if (character == '\n') {
      if (swallowNextLineFeed) {
        swallowNextLineFeed = false;
        continue;
      }

      submitCommandBuffer();
      continue;
    }

    swallowNextLineFeed = false;

    if (character == '\b' || character == 0x7F) {
      if (commandBuffer.length() > 0) {
        commandBuffer.remove(commandBuffer.length() - 1);
        Serial.print("\b \b");
      }

      continue;
    }

    if (character < ' ' || character > '~') {
      continue;
    }

    if (commandBuffer.length() < 63) {
      commandBuffer += character;
      Serial.write(character);
    }
  }
}

void serviceTouchMonitor() {
  if (!touchMonitorActive || millis() < nextTouchSampleMillis) {
    return;
  }

  nextTouchSampleMillis = millis() + 100;
  const TouchSample sample = readTouchSample();

  if (sample.irqAsserted) {
    if (calibrationActive) {
      CalibrationCapture& capture =
          calibrationCaptures[calibrationTargetIndex];
      capture.sumRawX += sample.x;
      capture.sumRawY += sample.y;
      ++capture.sampleCount;
      touchWasPressed = true;
      return;
    }

    Serial.printf("TOUCH IRQ=LOW RAW_X=%u RAW_Y=%u SCREEN_X=%u SCREEN_Y=%u Z1=%u Z2=%u\n",
                  sample.x,
                  sample.y,
                  sample.screenX,
                  sample.screenY,
                  sample.z1,
                  sample.z2);
    // Keep the 10x14 marker fully on-screen. Repeated samples intentionally
    // leave a short trail so calibration direction and stability are visible.
    const int markerX = min(static_cast<int>(sample.screenX), 310);
    const int markerY = min(static_cast<int>(sample.screenY), 226);
    display.text(markerX, markerY, "X", 0xFFFF, 2);
    touchWasPressed = true;
  } else if (touchWasPressed) {
    if (calibrationActive) {
      touchWasPressed = false;
      completeCalibrationTarget();
      return;
    }

    Serial.println("TOUCH IRQ=HIGH RELEASED");
    touchWasPressed = false;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  boardProfilePersisted = loadBoardProfile(boardProfile);
  if (boardProfile == BoardProfile::ClassicSingleUsb) {
    touchCalibrationPersisted = loadTouchCalibration();
  }
  systemReport = collectSystemReport();

  display.begin();
  displayProbe = display.probeController();
  sdStatus = inspectSdCard();

  showOverviewPage(
      display, systemReport, displayProbe, sdStatus, boardProfile);
  printInventory();
  printHelp();
  printPrompt();
}

void loop() {
  serviceSerialConsole();
  serviceTouchMonitor();
  delay(5);
}
