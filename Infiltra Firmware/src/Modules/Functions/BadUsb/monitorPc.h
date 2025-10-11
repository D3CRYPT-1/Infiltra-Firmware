#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

void monitorPcReset();
void monitorPcDrawScreen(TFT_eSPI& tft);
void monitorPcHandleInput(bool a, bool b, bool c, bool& exitReq);
void monitorPcStop();