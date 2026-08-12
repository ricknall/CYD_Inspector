#include "LightProbe.h"

#include "BoardConfig.h"

namespace {

bool initialized = false;
constexpr uint8_t SAMPLE_COUNT = 32;

}  // namespace

bool beginLightProbe(const BoardProfile profile) {
  initialized = false;

  if (profile != BoardProfile::ClassicSingleUsb) {
    return false;
  }

  // GPIO 34 is input-only and has no internal pull-up or pull-down.
  pinMode(cyd::LIGHT_SENSOR_PIN, INPUT);
  initialized = true;
  return true;
}

LightSample readLightSample() {
  LightSample sample;
  sample.supported = initialized;

  if (!initialized) {
    return sample;
  }

  uint32_t rawTotal = 0;
  uint32_t millivoltTotal = 0;
  sample.rawMinimum = 4095;
  sample.rawMaximum = 0;

  for (uint8_t index = 0; index < SAMPLE_COUNT; ++index) {
    const uint16_t raw = analogRead(cyd::LIGHT_SENSOR_PIN);
    const uint16_t millivolts = analogReadMilliVolts(cyd::LIGHT_SENSOR_PIN);
    rawTotal += raw;
    millivoltTotal += millivolts;
    sample.rawMinimum = min(sample.rawMinimum, raw);
    sample.rawMaximum = max(sample.rawMaximum, raw);
    delayMicroseconds(250);
  }

  sample.sampleCount = SAMPLE_COUNT;
  sample.rawAverage = static_cast<uint16_t>(rawTotal / SAMPLE_COUNT);
  sample.millivolts =
      static_cast<uint16_t>(millivoltTotal / SAMPLE_COUNT);
  return sample;
}

void printLightProbeConfiguration(const BoardProfile profile) {
  Serial.println("--- LIGHT SENSOR PROBE ---");

  if (profile != BoardProfile::ClassicSingleUsb) {
    Serial.println(
        "Unavailable: no validated light-sensor mapping for this profile.");
    return;
  }

  Serial.printf("Confirmed analog input: GPIO %d (input-only)\n",
                cyd::LIGHT_SENSOR_PIN);
  Serial.println("Response: darker = higher raw reading; brighter = lower");
  Serial.println("Sampling: 32 raw ADC and calibrated-millivolt readings");
}

void printLightSample(const LightSample& sample) {
  if (!sample.supported) {
    Serial.println("Light sample unavailable.");
    return;
  }

  Serial.printf(
      "LIGHT GPIO=34 RAW_AVG=%u RAW_MIN=%u RAW_MAX=%u MV=%u SAMPLES=%u\n",
      sample.rawAverage,
      sample.rawMinimum,
      sample.rawMaximum,
      sample.millivolts,
      sample.sampleCount);
}
