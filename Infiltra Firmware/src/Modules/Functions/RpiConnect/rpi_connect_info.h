#pragma once
#include <TFT_eSPI.h>

void rpiInfoReset();
void rpiInfoDrawScreen(TFT_eSPI& tft);
void rpiInfoHandleInput(bool a, bool b, bool c, bool& exitReq);
