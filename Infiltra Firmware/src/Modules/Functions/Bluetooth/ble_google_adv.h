#pragma once
#include <TFT_eSPI.h>

void bleGoogleAdvReset();
void bleGoogleAdvDrawScreen(TFT_eSPI& tft);
void bleGoogleAdvHandleInput(bool a, bool b, bool c, bool& requestExit);
void bleGoogleAdvStop();
