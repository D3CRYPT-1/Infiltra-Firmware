#pragma once
#include <TFT_eSPI.h>

void bgoneReset();
void bgoneDrawScreen(TFT_eSPI& tft);
void bgoneHandleInput(bool aPressed, bool bPressed, bool cPressed, bool& exitRequested);
