#pragma once
#include <TFT_eSPI.h>

void bleScanReset();
void bleScanDrawScreen(TFT_eSPI& tft);
void bleScanHandleInput(bool a, bool b, bool c, bool& requestExit);
void bleScanStop();
