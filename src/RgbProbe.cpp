#include "RgbProbe.h"

#include "BoardConfig.h"

namespace {

bool initialized = false;
int activePin = -1;

void prepareOff(const int pin) {
  // Set the latch HIGH before enabling output, preventing a brief active-low
  // flash while the pin changes mode.
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

}  // namespace

bool rgbProbePinIsCandidate(const int pin) {
  return pin == cyd::RGB_CANDIDATE_1 ||
      pin == cyd::RGB_CANDIDATE_2 ||
      pin == cyd::RGB_CANDIDATE_3;
}

bool beginRgbProbe(const BoardProfile profile) {
  if (profile != BoardProfile::ClassicSingleUsb) {
    rgbProbeOff();
    initialized = false;
    activePin = -1;
    return false;
  }

  if (initialized) {
    return true;
  }

  prepareOff(cyd::RGB_CANDIDATE_1);
  prepareOff(cyd::RGB_CANDIDATE_2);
  prepareOff(cyd::RGB_CANDIDATE_3);
  activePin = -1;
  initialized = true;
  return true;
}

void rgbProbeOff() {
  if (!initialized) {
    return;
  }

  digitalWrite(cyd::RGB_CANDIDATE_1, HIGH);
  digitalWrite(cyd::RGB_CANDIDATE_2, HIGH);
  digitalWrite(cyd::RGB_CANDIDATE_3, HIGH);
  activePin = -1;
}

bool rgbProbeSetPin(const int pin) {
  if (!initialized || !rgbProbePinIsCandidate(pin)) {
    return false;
  }

  rgbProbeOff();
  digitalWrite(pin, LOW);
  activePin = pin;
  return true;
}

int rgbProbeActivePin() {
  return activePin;
}

void printRgbProbeConfiguration(const BoardProfile profile) {
  Serial.println("--- RGB LED PROBE ---");

  if (profile != BoardProfile::ClassicSingleUsb) {
    Serial.println(
        "Unavailable: no validated RGB candidate mapping for this profile.");
    return;
  }

  Serial.println("Candidate GPIOs: 4, 16, 17");
  Serial.println("Candidate polarity: active LOW");
  Serial.println("Only one candidate is driven LOW at a time.");

  if (!initialized || activePin < 0) {
    Serial.println("Current state: all candidate channels OFF");
  } else {
    Serial.printf("Current state: GPIO %d ON; other candidates OFF\n",
                  activePin);
  }

  Serial.println("Commands: rgb 4, rgb 16, rgb 17, rgb off");
}
