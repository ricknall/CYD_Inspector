#pragma once

#include <Arduino.h>

#include "BoardProfile.h"

struct LightSample {
  bool supported = false;
  uint16_t rawAverage = 0;
  uint16_t rawMinimum = 0;
  uint16_t rawMaximum = 0;
  uint16_t millivolts = 0;
  uint8_t sampleCount = 0;
};

bool beginLightProbe(BoardProfile profile);
LightSample readLightSample();
void printLightProbeConfiguration(BoardProfile profile);
void printLightSample(const LightSample& sample);
