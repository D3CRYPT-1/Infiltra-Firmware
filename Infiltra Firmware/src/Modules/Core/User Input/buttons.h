#pragma once
#include "././UserInterface/menus/menu_enums.h"

class TFT_eSPI;

void initButtons();
void updateButtons();
bool btnAPressed();
bool btnBPressed();
bool btnCPressed();
void finalizeButtons();

void handleAllButtonLogic(
    TFT_eSPI *tft,
    bool &inOptionScreen,
    bool &inStopwatch,
    bool &inBGone,
    bool &inIRRead,
    bool &inWiFiScan,
    bool &inPacketScan,
    MenuState &currentMenu);
