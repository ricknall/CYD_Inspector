#include "SpeakerProbe.h"

#include "BoardConfig.h"

namespace {

bool initialized = false;

constexpr int8_t SINE_16[] = {
    0, 49, 90, 117, 127, 117, 90, 49,
    0, -49, -90, -117, -127, -117, -90, -49};
constexpr uint8_t DAC_CENTER = 128;
constexpr uint8_t TEST_AMPLITUDE = 20;

void playDacTone(const uint16_t frequencyHz, const uint16_t durationMs) {
  uint32_t cycles =
      (static_cast<uint32_t>(frequencyHz) * durationMs) / 1000U;
  uint32_t sampleDelayUs = 1000000UL / frequencyHz / 16U;

  if (cycles == 0) {
    cycles = 1;
  }
  if (sampleDelayUs == 0) {
    sampleDelayUs = 1;
  }

  for (uint32_t cycle = 0; cycle < cycles; ++cycle) {
    for (const int8_t point : SINE_16) {
      const int16_t value =
          DAC_CENTER +
          (static_cast<int16_t>(point) * TEST_AMPLITUDE) / 127;
      dacWrite(cyd::SPEAKER_DAC_PIN, static_cast<uint8_t>(value));
      delayMicroseconds(sampleDelayUs);
    }
  }
}

}  // namespace

bool beginSpeakerProbe(const BoardProfile profile) {
  stopSpeakerProbe();

  if (profile != BoardProfile::ClassicSingleUsb) {
    return false;
  }

  // Begin at the DAC midpoint to avoid a full-scale step into the amplifier.
  dacWrite(cyd::SPEAKER_DAC_PIN, DAC_CENTER);
  delay(10);
  initialized = true;
  return true;
}

bool playSpeakerProbeTest() {
  if (!initialized) {
    return false;
  }

  for (uint8_t beep = 0; beep < 3; ++beep) {
    playDacTone(880, 180);
    dacWrite(cyd::SPEAKER_DAC_PIN, DAC_CENTER);
    delay(100);
  }

  stopSpeakerProbe();
  return true;
}

void stopSpeakerProbe() {
  if (initialized) {
    dacWrite(cyd::SPEAKER_DAC_PIN, DAC_CENTER);
    delay(10);
  }

  dacDisable(cyd::SPEAKER_DAC_PIN);
  pinMode(cyd::SPEAKER_DAC_PIN, INPUT);
  initialized = false;
}

void printSpeakerProbeConfiguration(const BoardProfile profile) {
  Serial.println("--- SPEAKER PROBE ---");

  if (profile != BoardProfile::ClassicSingleUsb) {
    Serial.println(
        "Unavailable: no validated speaker mapping for this profile.");
    return;
  }

  Serial.printf("Confirmed audio output: GPIO %d (ESP32 DAC2)\n",
                cyd::SPEAKER_DAC_PIN);
  Serial.println("Confirmed path: GPIO26 -> SC8002B amplifier -> P4 output");
  Serial.println("P4 is a bridged output: connect speaker between VO1 and VO2.");
  Serial.println("Do not connect either P4 pin to ground.");
  Serial.println("First test: place about 100 ohms in series with the speaker.");
  Serial.println("Test: three 180 ms, low-amplitude 880 Hz DAC beeps");
  Serial.println("GPIO26 returns to high impedance immediately after the test.");
}
