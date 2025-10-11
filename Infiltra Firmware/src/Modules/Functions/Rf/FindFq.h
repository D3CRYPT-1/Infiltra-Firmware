#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

#ifndef GROOVE_RX_PIN
#error "GROOVE_RX_PIN must be defined by the board json or build flags."
#endif

void findFqReset();
void findFqDrawScreen(TFT_eSPI& tft);
void findFqHandleInput(bool a, bool b, bool c, bool& exitReq);
void findFqStop();
