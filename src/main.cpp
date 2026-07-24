#include <Arduino.h>

#include "BoardConfig.h"
#include "CydDisplay.h"
#include "PeripheralReport.h"
#include "ReportPages.h"
#include "SystemReport.h"

namespace {

CydDisplay display;

SystemReport systemReport;
DisplayProbe displayProbe;
SdStatus sdStatus;
String commandBuffer;
bool swallowNextLineFeed = false;

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
    showOverviewPage(display, systemReport, displayProbe, sdStatus);
    printInventory();
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
    showTouchPage(display);
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

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  systemReport = collectSystemReport();

  display.begin();
  displayProbe = display.probeController();
  sdStatus = inspectSdCard();

  showOverviewPage(display, systemReport, displayProbe, sdStatus);
  printInventory();
  printHelp();
  printPrompt();
}

void loop() {
  serviceSerialConsole();
  delay(5);
}
