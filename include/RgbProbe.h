#pragma once

#include <Arduino.h>

#include "BoardProfile.h"

bool beginRgbProbe(BoardProfile profile);
bool rgbProbeSetPin(int pin);
void rgbProbeOff();
bool rgbProbePinIsCandidate(int pin);
int rgbProbeActivePin();
void printRgbProbeConfiguration(BoardProfile profile);
