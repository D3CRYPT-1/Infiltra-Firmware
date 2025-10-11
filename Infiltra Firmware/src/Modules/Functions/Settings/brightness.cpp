#include "brightness.h"
#include "UserInterface/menus/menu_submenus.h"
#include "Modules/Core/User Config/ConfigManager.h"
#include <Arduino.h>

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
  static constexpr uint8_t ROT_TOP = 1;
#elif defined(M5STICK_C_PLUS_2)
  #include <M5StickCPlus2.h>
  static constexpr uint8_t ROT_TOP = 3;
#elif defined(M5STICK_C_PLUS_1_1)
  static constexpr uint8_t ROT_TOP = 3;
#else
  #include <M5StickCPlus2.h>
  static constexpr uint8_t ROT_TOP = 3;
#endif

#ifndef TFT_BL
  #define TFT_BL 27
#endif
#ifndef TFT_BACKLIGHT_ON
  #define TFT_BACKLIGHT_ON 1
#endif

static constexpr uint8_t ROT_ALT = (ROT_TOP + 3) & 0x3;
static constexpr uint16_t COL_BG = TFT_BLACK;
static constexpr uint16_t COL_TXT = TFT_SILVER;
static constexpr uint16_t COL_ACC = TFT_WHITE;
static constexpr uint16_t COL_SLIDER_FILL = TFT_WHITE;
static constexpr uint16_t COL_SLIDER_BG = TFT_DARKGREY;
static bool sFrameDrawn = false;
static bool sBgDrawn = false;
static bool sDirty = true;
static bool sInit = false;
static int sPct = 70;
static int W = 0, H = 0;
static int bx = 0, by = 0, bw = 0, bh = 0;
static int cx = 0, cy = 0, cw = 0;
static TFT_eSPI* sTft = nullptr;
static bool sPwmInit = false; 
static void loadConfig() {
    sPct = 70; 
    JsonDocument doc;
    if (ConfigManager::loadConfig(doc)) {
        if (doc["brightness"].is<int>()) {
            sPct = doc["brightness"];
            if (sPct < 0) sPct = 0;
            if (sPct > 100) sPct = 100;
        }
        if (doc["brightness_scale"].is<int>()) {
            int scale = doc["brightness_scale"];
            if (scale > 0 && scale < 100) {
                sPct = (sPct * scale) / 100;
                if (sPct < 10) sPct = 10; 
            }
        }
    }
}

static void saveConfig() {
    JsonDocument doc;
    ConfigManager::loadConfig(doc);
    doc["brightness"] = sPct;
    ConfigManager::saveConfig(doc);
}

static void applyBrightness(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    uint8_t hw = map(pct, 0, 100, 50, 255);
    
#if defined(M5CARDPUTER)
    M5Cardputer.Display.setBrightness(hw);
#elif defined(M5STICK_C_PLUS_2)
    M5.Lcd.setBrightness(hw);
    M5.Display.setBrightness(hw);
#elif defined(M5STICK_C_PLUS_1_1)
    if (!sPwmInit) {
        ledcAttachPin(TFT_BL, 1);
        ledcSetup(1, 12000, 8);
        sPwmInit = true;
        delay(10); 
    }
    #if TFT_BACKLIGHT_ON
        ledcWrite(1, hw);
    #else
        ledcWrite(1, 255 - hw);
    #endif
#else    
    M5.Lcd.setBrightness(hw);
#endif
}

static void geometry(TFT_eSPI& tft) {
    sTft = &tft;
    tft.setRotation(ROT_TOP);
    W = tft.width();
    H = tft.height();
    const int marginX = 10;
    const int marginTop = 30;
    const int marginBottom = 10;
    bx = marginX;
    by = marginTop;
    bw = W - 2 * marginX;
    bh = H - by - marginBottom;
    cx = bx + 10;
    cy = by + 15;
    cw = bw - 20;
}

static void drawBackground(TFT_eSPI& tft) {
    if (sBgDrawn) return;
    tft.setRotation(ROT_ALT);
    drawOptionsLayerBackground(tft);
    tft.setRotation(ROT_TOP);    
    sBgDrawn = true;
}

void brightnessReset() {
    sFrameDrawn = false;
    sBgDrawn = false;
    sDirty = true;
    sInit = false;
    loadConfig(); 
    applyBrightness(sPct);
}

static void drawFrame(TFT_eSPI& tft) {
    if (sFrameDrawn) return;
    tft.fillRect(bx, by, bw, bh, COL_BG);
    tft.drawRoundRect(bx, by, bw, bh, 8, COL_SLIDER_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_ACC, COL_BG);
    String title = "Display Brightness";
    int titleW = tft.textWidth(title);
    tft.setCursor(bx + (bw - titleW) / 2, by + 5);
    tft.print(title);
    tft.drawFastHLine(bx + 8, by + 18, bw - 16, COL_SLIDER_BG);  
    sFrameDrawn = true;
    sDirty = true;
}

static void drawSlider(TFT_eSPI& tft) {
    if (!sDirty) return;
    const int sliderY = cy + 20;
    const int sliderH = 16;
    const int pctTextY = sliderY + sliderH + 8;
    const int instructionY = by + bh - 15;
    tft.fillRect(cx, sliderY - 5, cw, 50, COL_BG);
    tft.drawRoundRect(cx, sliderY, cw, sliderH, 4, COL_SLIDER_BG);
    tft.fillRoundRect(cx + 1, sliderY + 1, cw - 2, sliderH - 2, 3, COL_BG);
    int fillW = ((cw - 4) * sPct) / 100;
    if (fillW > 0) {
        tft.fillRoundRect(cx + 2, sliderY + 2, fillW, sliderH - 4, 2, COL_SLIDER_FILL);
    }

    tft.setTextFont(1);
    tft.setTextSize(1);
    String pctText = String(sPct) + "%";
    int pctW = tft.textWidth(pctText);
    int pctX = cx + (cw - pctW) / 2;
    tft.fillRect(pctX - 5, pctTextY - 2, pctW + 10, 12, COL_BG);
    tft.setTextColor(COL_TXT, COL_BG);
    tft.setCursor(pctX, pctTextY);
    tft.print(pctText);
    String instruction = "A=Exit  B=Up  C=Down";
    int instrW = tft.textWidth(instruction);
    int instrX = cx + (cw - instrW) / 2;
    tft.fillRect(instrX - 5, instructionY - 2, instrW + 10, 12, COL_BG);
    tft.setTextColor(COL_TXT, COL_BG);
    tft.setCursor(instrX, instructionY);
    tft.print(instruction); 
    sDirty = false;
}

void brightnessDrawScreen(TFT_eSPI& tft) {
    if (!sInit) {
        geometry(tft);
        drawBackground(tft);
        sInit = true;
    }
    if (!sFrameDrawn) {
        drawFrame(tft);
    }
    drawSlider(tft);
}

void brightnessHandleInput(bool a, bool b, bool c, bool& requestExit) {
    requestExit = false;
    if (a) {
        saveConfig(); 
        requestExit = true;
        sFrameDrawn = false;
        return;
    }
    if (b) {
        sPct += 5;
        if (sPct > 100) sPct = 100;
        applyBrightness(sPct);
        sDirty = true;
    }
    if (c) {
        sPct -= 5;
        if (sPct < 0) sPct = 0;
        applyBrightness(sPct);
        sDirty = true;
    }
}

void brightnessInitAndApply() {
    loadConfig();
    if (sPct < 10) sPct = 70; 
    applyBrightness(sPct);
}
