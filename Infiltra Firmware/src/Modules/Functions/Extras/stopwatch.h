#pragma once
#include <TFT_eSPI.h>

void resetStopwatch();
void drawStopwatchScreen(TFT_eSPI& tft);     
void drawStopwatchTimeOnly(TFT_eSPI& tft);   
void stopwatchHandleInput(bool a, bool b, bool c, bool& exitReq);
void handleStopwatchInput(bool btnA);
bool isStopwatchRunning();
