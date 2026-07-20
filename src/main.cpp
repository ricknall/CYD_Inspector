#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_system.h>

#include "BoardConfig.h"
#include "CydDisplay.h"

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

String chipModel(const esp_chip_info_t& chipInfo) {
  switch (chipInfo.model) {
    case CHIP_ESP32:
      return "ESP32";
    case CHIP_ESP32S2:
      return "ESP32-S2";
    case CHIP_ESP32S3:
      return "ESP32-S3";
    case CHIP_ESP32C3:
      return "ESP32-C3";
    default:
      return "ESP32 FAMILY";
  }
}

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

String sdDisplayText(const SdStatus& sdStatus) {
  if (!sdStatus.mounted) {
    return "SD: NO CARD OR MOUNT FAIL";
  }

  const uint32_t volumeMiB = static_cast<uint32_t>(
      sdStatus.volumeBytes / (1024ULL * 1024ULL));

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

void showInventory(const esp_chip_info_t& chipInfo,
                   const uint32_t flashBytes,
                   const bool hasPsram,
                   const DisplayProbe& displayProbe,
                   const SdStatus& sdStatus) {
  display.fill(0x0000);

  display.text(10, 8, "CYD BOARD INSPECTOR", 0xFFE0, 2);
  display.text(10, 32, "DISPLAY SPI: WORKING", 0x07FF, 2);
  display.text(
      10,
      54,
      String("DISPLAY IC: ") +
          displayControllerName(displayProbe.controller),
      0x07E0,
      2);
  display.text(10, 76, "BOARD/USB: MANUAL CHECK", 0x07E0, 2);
  display.text(10, 98, sdDisplayText(sdStatus), 0xFBE0, 2);

  display.text(10, 122, "MCU: " + chipModel(chipInfo), 0xFFFF, 2);
  display.text(
      10,
      144,
      "CORES: " + String(chipInfo.cores) +
          " REV: " + String(chipInfo.revision),
      0xFFFF,
      2);
  display.text(
      10,
      166,
      "FLASH: " + String(flashBytes / (1024UL * 1024UL)) + " MB",
      0xFFFF,
      2);
  display.text(
      10,
      188,
      "PSRAM: " + String(hasPsram ? "YES" : "NO"),
      0xFFFF,
      2);

  display.text(10, 220, "SERIAL: 115200", 0x8410, 1);
}

void printInventory(const esp_chip_info_t& chipInfo,
                    const uint32_t flashBytes,
                    const bool hasPsram,
                    const DisplayProbe& displayProbe,
                    const SdStatus& sdStatus) {
  Serial.println("=== CYD BOARD INSPECTOR ===");
  Serial.println("Display initialized using the current SPI profile.");
  Serial.println("Board family and USB connector count require inspection.");

  Serial.printf("Display controller result: %s\n",
                displayControllerName(displayProbe.controller));
  printHexBytes("  RDDID  04", displayProbe.id04,
                sizeof(displayProbe.id04));
  printHexBytes("  RDID4  D3", displayProbe.idD3,
                sizeof(displayProbe.idD3));
  printHexBytes("  RDID1  DA", displayProbe.idDA,
                sizeof(displayProbe.idDA));
  printHexBytes("  RDID2  DB", displayProbe.idDB,
                sizeof(displayProbe.idDB));
  printHexBytes("  RDID3  DC", displayProbe.idDC,
                sizeof(displayProbe.idDC));
  printHexBytes("  MADCTL 0B", displayProbe.madctl,
                sizeof(displayProbe.madctl));
  printHexBytes("  PIXFMT 0C", displayProbe.pixelFormat,
                sizeof(displayProbe.pixelFormat));

  Serial.printf("MCU: %s, cores=%d, revision=%d\n",
                chipModel(chipInfo).c_str(),
                chipInfo.cores,
                chipInfo.revision);

  Serial.printf("Flash: %lu bytes (%lu MB)\n",
                static_cast<unsigned long>(flashBytes),
                static_cast<unsigned long>(
                    flashBytes / (1024UL * 1024UL)));

  Serial.printf("PSRAM: %s\n", hasPsram ? "yes" : "no");

  if (sdStatus.mounted) {
    Serial.printf(
        "SD card: type=%s\n"
        "  Physical card: %llu bytes (%llu MiB)\n"
        "  FAT volume:    %llu bytes (%llu MiB)\n"
        "  Used:          %llu bytes (%llu MiB)\n",
        cardTypeName(sdStatus.cardType),
        static_cast<unsigned long long>(sdStatus.cardBytes),
        static_cast<unsigned long long>(
            sdStatus.cardBytes / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(sdStatus.volumeBytes),
        static_cast<unsigned long long>(
            sdStatus.volumeBytes / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(sdStatus.usedBytes),
        static_cast<unsigned long long>(
            sdStatus.usedBytes / (1024ULL * 1024ULL)));
  } else {
    Serial.println(
        "SD card: no card detected or mount failed using pins "
        "CS=5 SCK=18 MISO=19 MOSI=23.");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  esp_chip_info_t chipInfo{};
  esp_chip_info(&chipInfo);

  uint32_t flashBytes = 0;
  esp_flash_get_size(nullptr, &flashBytes);

  const bool hasPsram = psramFound();

  display.begin();
  const DisplayProbe displayProbe = display.probeController();
  const SdStatus sdStatus = inspectSdCard();

  showInventory(chipInfo,
                flashBytes,
                hasPsram,
                displayProbe,
                sdStatus);
  printInventory(chipInfo,
                 flashBytes,
                 hasPsram,
                 displayProbe,
                 sdStatus);
}

void loop() {
  delay(1000);
}
