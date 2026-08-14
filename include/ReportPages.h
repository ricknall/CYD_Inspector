#pragma once

#include "BoardProfile.h"
#include "CydDisplay.h"
#include "PeripheralReport.h"
#include "SystemReport.h"

void showOverviewPage(CydDisplay& display,
                      const SystemReport& system,
                      const DisplayProbe& probe,
                      const SdStatus& sd,
                      BoardProfile profile);
void showProfilePage(CydDisplay& display, BoardProfile profile);
void showSystemPage(CydDisplay& display, const SystemReport& system);
void showDisplayPage(CydDisplay& display, const DisplayProbe& probe);
void showPeripheralPage(CydDisplay& display,
                        const SdStatus& sd,
                        BoardProfile profile);
void showTouchPage(CydDisplay& display, BoardProfile profile);
void showTouchCandidatePage(CydDisplay& display,
                            BoardProfile profile,
                            bool rawProbe);
void showCalibrationTarget(CydDisplay& display, uint8_t targetIndex);
void showCalibrationResult(CydDisplay& display,
                           int32_t rawTop,
                           int32_t rawBottom,
                           int32_t rawLeft,
                           int32_t rawRight,
                           bool persisted);
void showRgbProbePage(CydDisplay& display,
                      BoardProfile profile,
                      int activePin);
void showLightProbePage(CydDisplay& display,
                        BoardProfile profile,
                        uint16_t rawAverage,
                        uint16_t rawMinimum,
                        uint16_t rawMaximum,
                        uint16_t millivolts);
void showSpeakerProbePage(CydDisplay& display,
                          BoardProfile profile,
                          bool testPlayed);
