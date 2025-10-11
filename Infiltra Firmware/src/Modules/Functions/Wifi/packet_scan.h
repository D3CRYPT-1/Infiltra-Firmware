#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

void packetScanReset();                    
void packetScanDrawScreen(TFT_eSPI& tft);  
void packetScanHandleInput(bool a, bool b, bool c, bool& requestExit); 
void packetScanStop();                     
