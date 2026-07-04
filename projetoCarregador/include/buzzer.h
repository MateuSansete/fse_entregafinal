#pragma once

#include <stdint.h>

void buzzerInit();
void buzzerUpdate();
void buzzerBeep(uint16_t durationMs);
void buzzerPatternHrAlert();
void buzzerPatternMotion();
void buzzerPatternSos();
