#pragma once

#include <Arduino.h>

#include "BoardProfile.h"

struct TouchSample {
  bool supported = false;
  bool irqAsserted = false;
  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t z1 = 0;
  uint16_t z2 = 0;
  uint16_t screenX = 0;
  uint16_t screenY = 0;
};

struct TouchCalibration {
  int32_t rawTop = 500;
  int32_t rawBottom = 3600;
  int32_t rawLeft = 500;
  int32_t rawRight = 3600;
};

bool beginTouchProbe(BoardProfile profile);
TouchSample readTouchSample();
void setTouchCalibration(const TouchCalibration& calibration);
TouchCalibration getTouchCalibration();
bool touchCalibrationIsValid(const TouchCalibration& calibration);
bool saveTouchCalibration(const TouchCalibration& calibration);
bool loadTouchCalibration();
bool clearTouchCalibration();
void mapTouchCoordinates(uint16_t rawX,
                         uint16_t rawY,
                         uint16_t& screenX,
                         uint16_t& screenY);
void printTouchConfiguration(BoardProfile profile);
