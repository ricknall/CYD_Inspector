#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "BoardConfig.h"
#include "CydDisplay.h"
#include "SystemReport.h"

namespace {

CydDisplay display;

// The display uses VSPI. Give the SD card the other hardware SPI controller.
SPIClass sdSpi(HSPI);

struct SdStatus {
  bool mounted = false;
  uint8_t cardType = CARD_NONE;

  uint64_t cardBytes = 0;    // Entire physical card
  uint64_t volumeBytes = 0;  // Formatted FAT partition
  uint64_t usedBytes = 0;
};

SystemReport systemReport;
DisplayProbe displayProbe;
SdStatus sdStatus;
String commandBuffer;

const char* cardTypeName(const uint8_t type) {
  switch (type) {
    case CARD_MMC:
      return "MMC";
    case CARD_SD:
      return "SDSC";
    case CARD_SDHC:
      return "SDHC";
    default:
      return "UNKNOWN";
  }
}

SdStatus inspectSdCard() {
  SdStatus result;

  pinMode(cyd::SD_CS, OUTPUT);
  digitalWrite(cyd::SD_CS, HIGH);

  sdSpi.begin(cyd::SD_SCLK, cyd::SD_MISO, cyd::SD_MOSI, cyd::SD_CS);

  if (!SD.begin(cyd::SD_CS, sdSpi, cyd::SD_SPI_FREQUENCY)) {
    return result;
  }

  result.cardType = SD.cardType();

  if (result.cardType == CARD_NONE) {
    return result;
  }

  result.mounted = true;
  result.cardBytes = SD.cardSize();
  result.volumeBytes = SD.totalBytes();
  result.usedBytes = SD.usedBytes();

  return result;
}

String sdDisplayText(const SdStatus& status) {
  if (!status.mounted) {
    return "SD: NO CARD OR MOUNT FAIL";
  }

  const uint32_t volumeMiB = static_cast<uint32_t>(
      status.volumeBytes / (1024ULL * 1024ULL));

  return "SD VOLUME: " + String(volumeMiB) + " MB";
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

void showInventory(const SystemReport& system,
                   const DisplayProbe& probe,
                   const SdStatus& sd) {
  display.fill(0x0000);

  display.text(10, 8, "CYD BOARD INSPECTOR", 0xFFE0, 2);
  display.text(10, 32, "DISPLAY SPI: WORKING", 0x07FF, 2);
  display.text(
      10,
      54,
      String("DISPLAY IC: ") + displayControllerName(probe.controller),
      0x07E0,
      2);
  display.text(10, 76, "BOARD PROFILE: MANUAL", 0x07E0, 2);
  display.text(10, 98, sdDisplayText(sd), 0xFBE0, 2);

  display.text(10, 122, "MCU: " + system.chipModel, 0xFFFF, 2);
  display.text(
      10,
      144,
      "CORES: " + String(system.coreCount) +
          " CPU: " + String(system.cpuFrequencyMhz) + " MHZ",
      0xFFFF,
      2);
  display.text(
      10,
      166,
      "FLASH: " +
          String(system.flashSizeBytes / (1024UL * 1024UL)) +
          " MB " + system.flashMode,
      0xFFFF,
      2);
  display.text(
      10,
      188,
      "PSRAM: " + String(system.psramFound ? "YES" : "NO"),
      0xFFFF,
      2);

  display.text(10, 220, "TYPE HELP IN SERIAL MONITOR", 0x8410, 1);
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

void printPeripheralReport(const SdStatus& sd) {
  Serial.println("--- PERIPHERALS ---");
  Serial.printf("SD SPI: MOSI=%d MISO=%d SCLK=%d CS=%d at %lu MHz\n",
                cyd::SD_MOSI,
                cyd::SD_MISO,
                cyd::SD_SCLK,
                cyd::SD_CS,
                static_cast<unsigned long>(
                    cyd::SD_SPI_FREQUENCY / 1000000UL));

  if (sd.mounted) {
    Serial.printf(
        "SD card: type=%s\n"
        "  Physical card: %llu bytes (%llu MiB)\n"
        "  FAT volume:    %llu bytes (%llu MiB)\n"
        "  Used:          %llu bytes (%llu MiB)\n",
        cardTypeName(sd.cardType),
        static_cast<unsigned long long>(sd.cardBytes),
        static_cast<unsigned long long>(
            sd.cardBytes / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(sd.volumeBytes),
        static_cast<unsigned long long>(
            sd.volumeBytes / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(sd.usedBytes),
        static_cast<unsigned long long>(
            sd.usedBytes / (1024ULL * 1024ULL)));
  } else {
    Serial.println("SD card: no card detected or mount failed.");
  }

  Serial.println(
      "Touch/RGB LED/speaker/light sensor: not assumed until a board "
      "profile is selected.");
}

void printInventory() {
  Serial.println();
  Serial.println("=== CYD BOARD INSPECTOR REPORT ===");
  Serial.println("Board identity and USB connector count require inspection.");
  printSystemReport(systemReport);
  printDisplayReport(displayProbe);
  printPeripheralReport(sdStatus);
}

void printJsonReport() {
  Serial.println('{');
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

  Serial.println("  \"sd\": {");
  Serial.printf("    \"mounted\": %s,\n",
                sdStatus.mounted ? "true" : "false");
  Serial.printf("    \"type\": \"%s\",\n",
                cardTypeName(sdStatus.cardType));
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
  Serial.println("  system       Print MCU, memory, reset, and software details");
  Serial.println("  display      Print display profile and probe details");
  Serial.println("  peripherals  Print SD and peripheral-assumption details");
  Serial.println("  touch        Explain current touch status");
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
    printInventory();
  } else if (command == "system") {
    printSystemReport(systemReport);
  } else if (command == "display") {
    printDisplayReport(displayProbe);
  } else if (command == "peripherals") {
    printPeripheralReport(sdStatus);
  } else if (command == "touch") {
    Serial.println(
        "Touch: not probed yet. No controller or pin mapping will be guessed "
        "without a selected board profile.");
  } else if (command == "json") {
    printJsonReport();
  } else {
    Serial.printf("Unknown command: %s\n", command.c_str());
    Serial.println("Type help for the command list.");
  }
}

void serviceSerialConsole() {
  while (Serial.available() > 0) {
    const char character = static_cast<char>(Serial.read());

    if (character == '\r') {
      continue;
    }

    if (character == '\n') {
      runCommand(commandBuffer);
      commandBuffer = "";
      continue;
    }

    if (commandBuffer.length() < 63) {
      commandBuffer += character;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  systemReport = collectSystemReport();

  display.begin();
  displayProbe = display.probeController();
  sdStatus = inspectSdCard();

  showInventory(systemReport, displayProbe, sdStatus);
  printInventory();
  printHelp();
}

void loop() {
  serviceSerialConsole();
  delay(5);
}
