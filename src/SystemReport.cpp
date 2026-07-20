#include "SystemReport.h"

#include <Esp.h>
#include <esp_chip_info.h>
#include <esp_system.h>

namespace {

const char* flashModeName(const FlashMode_t mode) {
  switch (mode) {
    case FM_QIO:
      return "QIO";
    case FM_QOUT:
      return "QOUT";
    case FM_DIO:
      return "DIO";
    case FM_DOUT:
      return "DOUT";
    case FM_FAST_READ:
      return "FAST_READ";
    case FM_SLOW_READ:
      return "SLOW_READ";
    default:
      return "UNKNOWN";
  }
}

const char* resetReasonName(const esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:
      return "power-on";
    case ESP_RST_EXT:
      return "external reset";
    case ESP_RST_SW:
      return "software reset";
    case ESP_RST_PANIC:
      return "exception/panic";
    case ESP_RST_INT_WDT:
      return "interrupt watchdog";
    case ESP_RST_TASK_WDT:
      return "task watchdog";
    case ESP_RST_WDT:
      return "other watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deep-sleep wake";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "SDIO reset";
    default:
      return "unknown";
  }
}

}  // namespace

SystemReport collectSystemReport() {
  esp_chip_info_t chipInfo{};
  esp_chip_info(&chipInfo);

  SystemReport report;
  report.chipModel = ESP.getChipModel();
  report.chipRevision = chipInfo.revision;
  report.coreCount = chipInfo.cores;
  report.cpuFrequencyMhz = ESP.getCpuFreqMHz();

  report.flashSizeBytes = ESP.getFlashChipSize();
  report.flashSpeedHz = ESP.getFlashChipSpeed();
  report.flashMode = flashModeName(ESP.getFlashChipMode());

  report.psramFound = psramFound();
  report.psramSizeBytes = ESP.getPsramSize();
  report.freePsramBytes = ESP.getFreePsram();

  report.heapSizeBytes = ESP.getHeapSize();
  report.freeHeapBytes = ESP.getFreeHeap();
  report.minimumFreeHeapBytes = ESP.getMinFreeHeap();
  report.largestFreeHeapBlockBytes = ESP.getMaxAllocHeap();

  report.resetReason = resetReasonName(esp_reset_reason());
  report.arduinoCoreVersion = ESP.getCoreVersion();
  report.espIdfVersion = ESP.getSdkVersion();

  return report;
}

void printSystemReport(const SystemReport& report) {
  Serial.println("--- SYSTEM ---");
  Serial.printf("MCU: %s, cores=%u, revision=%u, CPU=%lu MHz\n",
                report.chipModel.c_str(),
                static_cast<unsigned>(report.coreCount),
                static_cast<unsigned>(report.chipRevision),
                static_cast<unsigned long>(report.cpuFrequencyMhz));
  Serial.printf("Flash: %lu bytes (%lu MiB), %lu MHz, mode=%s\n",
                static_cast<unsigned long>(report.flashSizeBytes),
                static_cast<unsigned long>(
                    report.flashSizeBytes / (1024UL * 1024UL)),
                static_cast<unsigned long>(report.flashSpeedHz / 1000000UL),
                report.flashMode.c_str());
  Serial.printf("PSRAM: %s, total=%lu bytes, free=%lu bytes\n",
                report.psramFound ? "yes" : "no",
                static_cast<unsigned long>(report.psramSizeBytes),
                static_cast<unsigned long>(report.freePsramBytes));
  Serial.printf(
      "Heap: total=%lu, free=%lu, minimum-free=%lu, largest-block=%lu bytes\n",
      static_cast<unsigned long>(report.heapSizeBytes),
      static_cast<unsigned long>(report.freeHeapBytes),
      static_cast<unsigned long>(report.minimumFreeHeapBytes),
      static_cast<unsigned long>(report.largestFreeHeapBlockBytes));
  Serial.printf("Reset reason: %s\n", report.resetReason.c_str());
  Serial.printf("Arduino-ESP32: %s\n", report.arduinoCoreVersion.c_str());
  Serial.printf("ESP-IDF: %s\n", report.espIdfVersion.c_str());
}

