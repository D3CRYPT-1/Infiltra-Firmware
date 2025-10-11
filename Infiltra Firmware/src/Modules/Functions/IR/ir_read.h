#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

void irReadReset();                         
void irReadDrawScreen(TFT_eSPI& tft);       
void irReadHandleInput(bool a, bool b, bool c, bool& requestExit);
                                            