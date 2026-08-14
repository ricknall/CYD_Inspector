#include "PeripheralReport.h"

#include <SPI.h>

#include "BoardConfig.h"

namespace {

// The display uses VSPI. Give the SD card the other hardware SPI controller.
SPIClass sdSpi(HSPI);

}  // namespace

const char* sdCardTypeName(const uint8_t type) {
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

void printPeripheralReport(const SdStatus& status,
                           const BoardProfile profile) {
  Serial.println("--- PERIPHERALS ---");
  Serial.printf("SD SPI: MOSI=%d MISO=%d SCLK=%d CS=%d at %lu MHz\n",
                cyd::SD_MOSI,
                cyd::SD_MISO,
                cyd::SD_SCLK,
                cyd::SD_CS,
                static_cast<unsigned long>(
                    cyd::SD_SPI_FREQUENCY / 1000000UL));

  if (status.mounted) {
    Serial.printf(
        "SD card: type=%s\n"
        "  Physical card: %llu bytes (%llu MiB)\n"
        "  FAT volume:    %llu bytes (%llu MiB)\n"
        "  Used:          %llu bytes (%llu MiB)\n",
        sdCardTypeName(status.cardType),
        static_cast<unsigned long long>(status.cardBytes),
        static_cast<unsigned long long>(
            status.cardBytes / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(status.volumeBytes),
        static_cast<unsigned long long>(
            status.volumeBytes / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(status.usedBytes),
        static_cast<unsigned long long>(
            status.usedBytes / (1024ULL * 1024ULL)));
  } else {
    Serial.println("SD card: no card detected or mount failed.");
  }

  Serial.println("Touch mapping: reported in the separate TOUCH section.");

  if (profile == BoardProfile::ClassicSingleUsb) {
    Serial.println(
        "Confirmed RGB: GPIO 4=RED, 16=GREEN, 17=BLUE, active LOW.");
    Serial.println(
        "Confirmed light sensor: GPIO 34, darker=higher raw ADC.");
    Serial.println(
        "Confirmed speaker: GPIO 26 / DAC2 -> SC8002B -> P4.");
  } else if (profile == BoardProfile::Cyd2Usb) {
    Serial.println("CYD2USB RGB mapping: NOT TESTED.");
    Serial.println("CYD2USB light-sensor mapping: NOT TESTED.");
    Serial.println("CYD2USB speaker mapping: NOT TESTED.");
    Serial.println("Classic-board results are deliberately not applied.");
  } else {
    Serial.println("Peripheral mappings: select a board profile first.");
  }
}
