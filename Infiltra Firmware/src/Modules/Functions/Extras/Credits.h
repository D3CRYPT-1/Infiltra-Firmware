#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

void creditsReset();
void creditsDrawScreen(TFT_eSPI& tft);
void creditsHandleInput(bool btnA, bool btnB, bool btnC, bool& exitReq);