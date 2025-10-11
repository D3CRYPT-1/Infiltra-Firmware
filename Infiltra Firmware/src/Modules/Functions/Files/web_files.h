#pragma once
#include <TFT_eSPI.h>

void webFilesReset();                              
void webFilesDrawScreen(TFT_eSPI& tft);            
void webFilesHandleInput(bool a, bool b, bool c, bool& requestExit); 
void webFilesStop();                               
