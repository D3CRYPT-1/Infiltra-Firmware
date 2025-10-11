#pragma once
#include <TFT_eSPI.h>
#include <Arduino.h>

void brightnessReset();
void brightnessDrawScreen(TFT_eSPI& tft);
void brightnessHandleInput(bool a, bool b, bool c, bool& requestExit);
void brightnessInitAndApply();
