#pragma once
#include <TFT_eSPI.h>
#include <Arduino.h>

void pinConfigReset();
void pinConfigDrawScreen(TFT_eSPI& tft);
void pinConfigHandleInput(bool a, bool b, bool c, bool& requestExit);
int getCurrentIrTxPin();
int getCurrentIrRxPin(); 
int getCurrentRfTxPin();
int getCurrentRfRxPin();