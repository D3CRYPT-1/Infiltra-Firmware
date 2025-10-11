#pragma once
#include <Arduino.h>
class TFT_eSPI;

void infiChatReset();
void infiChatDrawScreen(TFT_eSPI &tft);
void infiChatHandleInput(bool aPressed, bool bPressed, bool cPressed, bool &exitRequested);
void infiChatStop();
