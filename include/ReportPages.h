#pragma once

#include "CydDisplay.h"
#include "PeripheralReport.h"
#include "SystemReport.h"

void showOverviewPage(CydDisplay& display,
                      const SystemReport& system,
                      const DisplayProbe& probe,
                      const SdStatus& sd);
void showSystemPage(CydDisplay& display, const SystemReport& system);
void showDisplayPage(CydDisplay& display, const DisplayProbe& probe);
void showPeripheralPage(CydDisplay& display, const SdStatus& sd);
void showTouchPage(CydDisplay& display);

