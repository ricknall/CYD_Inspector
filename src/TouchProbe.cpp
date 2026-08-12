#include "TouchProbe.h"

#include <Preferences.h>

#include "BoardConfig.h"

namespace {

bool initialized = false;

// Initial calibration measured on Rick's classic single-USB CYD.
// Raw X runs from the top to the bottom of the landscape display; raw Y runs
// from left to right. Values outside the measured range are clipped.
TouchCalibration activeCalibration;

constexpr char PREFERENCES_NAMESPACE[] = "cyd_touch";
constexpr uint8_t CALIBRATION_VERSION = 1;

uint16_t mapClipped(const uint16_t raw,
                    const int32_t rawMinimum,
                    const int32_t rawMaximum,
                    const int32_t screenMaximum) {
  const int32_t clipped = constrain(
      static_cast<int32_t>(raw), rawMinimum, rawMaximum);
  return static_cast<uint16_t>(
      map(clipped, rawMinimum, rawMaximum, 0, screenMaximum));
}

uint8_t transferByte(const uint8_t output) {
  uint8_t input = 0;

  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(cyd::TOUCH_MOSI, (output >> bit) & 1U);
    digitalWrite(cyd::TOUCH_SCLK, HIGH);
    input = static_cast<uint8_t>(
        (input << 1U) | (digitalRead(cyd::TOUCH_MISO) ? 1U : 0U));
    digitalWrite(cyd::TOUCH_SCLK, LOW);
  }

  return input;
}

uint16_t readChannel(const uint8_t command) {
  digitalWrite(cyd::TOUCH_CS, LOW);
  transferByte(command);
  const uint8_t high = transferByte(0x00);
  const uint8_t low = transferByte(0x00);
  digitalWrite(cyd::TOUCH_CS, HIGH);

  return static_cast<uint16_t>(
      ((static_cast<uint16_t>(high) << 8U) | low) >> 3U);
}

}  // namespace

bool beginTouchProbe(const BoardProfile profile) {
  initialized = false;

  if (profile != BoardProfile::ClassicSingleUsb) {
    return false;
  }

  pinMode(cyd::TOUCH_MOSI, OUTPUT);
  pinMode(cyd::TOUCH_MISO, INPUT);
  pinMode(cyd::TOUCH_SCLK, OUTPUT);
  pinMode(cyd::TOUCH_CS, OUTPUT);
  // GPIO 36 is input-only and has no ESP32 internal pull-up. The CYD touch
  // circuit supplies the IRQ pull-up.
  pinMode(cyd::TOUCH_IRQ, INPUT);

  digitalWrite(cyd::TOUCH_MOSI, LOW);
  digitalWrite(cyd::TOUCH_SCLK, LOW);
  digitalWrite(cyd::TOUCH_CS, HIGH);
  initialized = true;
  return true;
}

TouchSample readTouchSample() {
  TouchSample sample;
  sample.supported = initialized;

  if (!initialized) {
    return sample;
  }

  sample.irqAsserted = digitalRead(cyd::TOUCH_IRQ) == LOW;

  if (sample.irqAsserted) {
    sample.x = readChannel(0xD0);
    sample.y = readChannel(0x90);
    sample.z1 = readChannel(0xB0);
    sample.z2 = readChannel(0xC0);
    mapTouchCoordinates(
        sample.x, sample.y, sample.screenX, sample.screenY);
  }

  return sample;
}

void setTouchCalibration(const TouchCalibration& calibration) {
  activeCalibration = calibration;
}

TouchCalibration getTouchCalibration() {
  return activeCalibration;
}

bool touchCalibrationIsValid(const TouchCalibration& calibration) {
  const int32_t verticalSpan =
      calibration.rawBottom - calibration.rawTop;
  const int32_t horizontalSpan =
      calibration.rawRight - calibration.rawLeft;

  return calibration.rawTop >= -1000 &&
      calibration.rawBottom <= 5000 &&
      calibration.rawLeft >= -1000 &&
      calibration.rawRight <= 5000 &&
      verticalSpan >= 1000 && verticalSpan <= 5000 &&
      horizontalSpan >= 1000 && horizontalSpan <= 5000;
}

bool saveTouchCalibration(const TouchCalibration& calibration) {
  if (!touchCalibrationIsValid(calibration)) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const bool success =
      preferences.putInt("top", calibration.rawTop) == sizeof(int32_t) &&
      preferences.putInt("bottom", calibration.rawBottom) == sizeof(int32_t) &&
      preferences.putInt("left", calibration.rawLeft) == sizeof(int32_t) &&
      preferences.putInt("right", calibration.rawRight) == sizeof(int32_t) &&
      preferences.putUChar("version", CALIBRATION_VERSION) == sizeof(uint8_t);
  preferences.end();
  return success;
}

bool loadTouchCalibration() {
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, true)) {
    return false;
  }

  if (preferences.getUChar("version", 0) != CALIBRATION_VERSION) {
    preferences.end();
    return false;
  }

  TouchCalibration calibration;
  calibration.rawTop = preferences.getInt("top", 500);
  calibration.rawBottom = preferences.getInt("bottom", 3600);
  calibration.rawLeft = preferences.getInt("left", 500);
  calibration.rawRight = preferences.getInt("right", 3600);
  preferences.end();

  if (!touchCalibrationIsValid(calibration)) {
    return false;
  }

  setTouchCalibration(calibration);
  return true;
}

bool clearTouchCalibration() {
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  const bool success = preferences.clear();
  preferences.end();
  setTouchCalibration(TouchCalibration{});
  return success;
}

void mapTouchCoordinates(const uint16_t rawX,
                         const uint16_t rawY,
                         uint16_t& screenX,
                         uint16_t& screenY) {
  screenX = mapClipped(rawY,
                       activeCalibration.rawLeft,
                       activeCalibration.rawRight,
                       319);
  screenY = mapClipped(rawX,
                       activeCalibration.rawTop,
                       activeCalibration.rawBottom,
                       239);
}

void printTouchConfiguration(const BoardProfile profile) {
  Serial.println("--- TOUCH ---");

  if (profile != BoardProfile::ClassicSingleUsb) {
    Serial.println(
        "Touch probe unavailable: no validated candidate mapping for this "
        "board profile.");
    return;
  }

  Serial.println("Controller: XPT2046-compatible (confirmed)");
  Serial.printf("Confirmed pins: MOSI=%d MISO=%d SCLK=%d CS=%d IRQ=%d\n",
                cyd::TOUCH_MOSI,
                cyd::TOUCH_MISO,
                cyd::TOUCH_SCLK,
                cyd::TOUCH_CS,
                cyd::TOUCH_IRQ);
  Serial.println("Axis mapping: raw X=top/bottom, raw Y=left/right");
  Serial.printf("Active raw range: top=%ld bottom=%ld left=%ld right=%ld\n",
                static_cast<long>(activeCalibration.rawTop),
                static_cast<long>(activeCalibration.rawBottom),
                static_cast<long>(activeCalibration.rawLeft),
                static_cast<long>(activeCalibration.rawRight));
  Serial.println("Mapped touch monitor active. Type 'touch' again to stop.");
}
