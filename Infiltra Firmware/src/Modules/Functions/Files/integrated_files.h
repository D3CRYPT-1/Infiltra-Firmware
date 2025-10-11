#pragma once
#include <TFT_eSPI.h>

void integratedFilesReset();
void integratedFilesDrawScreen(TFT_eSPI& tft);
void integratedFilesHandleInput(bool a, bool b, bool c, bool& exitReq);
bool integratedFilesConsumeOpenWebRequest();
