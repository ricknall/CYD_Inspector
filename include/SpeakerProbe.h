#pragma once

#include <Arduino.h>

#include "BoardProfile.h"

bool beginSpeakerProbe(BoardProfile profile);
bool playSpeakerProbeTest();
void stopSpeakerProbe();
void printSpeakerProbeConfiguration(BoardProfile profile);
