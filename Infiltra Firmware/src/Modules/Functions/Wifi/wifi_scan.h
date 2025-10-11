#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

void wifiScanReset();                   
void wifiScanDrawScreen(TFT_eSPI& tft); 
void wifiScanHandleInput(bool a, bool b, bool c, bool& requestExit);
