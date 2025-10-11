#include "pin_config.h"
#include "UserInterface/menus/menu_submenus.h"
#include "Modules/Core/User Config/ConfigManager.h"
#include <Arduino.h>

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
  static constexpr uint8_t ROT_TOP = 1;
#else
  static constexpr uint8_t ROT_TOP = 3;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 3) & 0x3;

extern void drawOptionsLayerBackground(TFT_eSPI& tft);
static constexpr uint16_t COL_BG  = TFT_BLACK;
static constexpr uint16_t COL_TXT = TFT_SILVER;
static constexpr uint16_t COL_ACC = TFT_WHITE;
static TFT_eSPI* sTft = nullptr;
static bool sInit = false, sBgDrawn = false;
static int W=0, H=0, bx=0, by=0, bw=0, bh=0, cx=0, cy=0, cw=0;
static int sMenuLevel = 0; 
static int sSelection = 0;
static int sIrRfMode = 0; 
static int sTxRxMode = 0; 
static bool sTitleDirty = true;
static bool sContentDirty = true;
static String sLastStatus;
static int sCurrentIrTx = -1;
static int sCurrentIrRx = -1;
static int sCurrentRfTx = -1;
static int sCurrentRfRx = -1;
static String sCurrentIrTxName = "Default";
static String sCurrentIrRxName = "Default";
static String sCurrentRfTxName = "Default";
static String sCurrentRfRxName = "Default";

struct PinOption {
    String name;
    int pin;
    String desc;
};

#if defined(M5STICK_C_PLUS_2) || defined(M5STICK_C_PLUS_1_1)
static const PinOption sTxPinOptions[] = {
    {"Default", 19, "Default TX pin"},
    {"GPIO 26", 26, "GPIO 26"},
    {"GPIO 36/25", 25, "GPIO 25"},
    {"Grove 32", 32, "Grove pin 32"},
    {"Grove 33", 33, "Grove pin 33"},
    {"M5 Mod", 32, "M5 Module"}
};
static const PinOption sRxPinOptions[] = {
    {"Default", 33, "Default RX pin"},
    {"GPIO 26", 26, "GPIO 26"},
    {"GPIO 36/25", 25, "GPIO 25"},
    {"Grove 32", 32, "Grove pin 32"},
    {"Grove 33", 33, "Grove pin 33"},
    {"M5 Mod", 33, "M5 Module"}
};
#elif defined(M5CARDPUTER)
static const PinOption sTxPinOptions[] = {
    {"Default", 44, "Default TX pin"},
    {"Grove 1", 1, "Grove port 1"},
    {"Grove 2", 2, "Grove port 2"},
    {"M5 Mod", 2, "M5 Module"}
};
static const PinOption sRxPinOptions[] = {
    {"Default", 1, "Default RX pin"},
    {"Grove 1", 1, "Grove port 1"},
    {"Grove 2", 2, "Grove port 2"},
    {"M5 Mod", 1, "M5 Module"}
};
#endif

static constexpr int PIN_OPTIONS_COUNT = sizeof(sTxPinOptions) / sizeof(sTxPinOptions[0]);

static void loadConfig() {
#if defined(M5STICK_C_PLUS_2) || defined(M5STICK_C_PLUS_1_1)
    sCurrentIrTx = 19; sCurrentIrRx = 33;
    sCurrentRfTx = 19; sCurrentRfRx = 33;
#elif defined(M5CARDPUTER)
    sCurrentIrTx = 44; sCurrentIrRx = 1;
    sCurrentRfTx = 44; sCurrentRfRx = 1;
#endif
    sCurrentIrTxName = "Default";
    sCurrentIrRxName = "Default";
    sCurrentRfTxName = "Default";
    sCurrentRfRxName = "Default";
    
    JsonDocument doc;
    if (ConfigManager::loadConfig(doc)) {
        if (doc["ir_tx_pin"].is<int>()) {
            sCurrentIrTx = doc["ir_tx_pin"];
            sCurrentIrTxName = doc["ir_tx_name"].as<String>();
        }
        if (doc["ir_rx_pin"].is<int>()) {
            sCurrentIrRx = doc["ir_rx_pin"];
            sCurrentIrRxName = doc["ir_rx_name"].as<String>();
        }
        if (doc["rf_tx_pin"].is<int>()) {
            sCurrentRfTx = doc["rf_tx_pin"];
            sCurrentRfTxName = doc["rf_tx_name"].as<String>();
        }
        if (doc["rf_rx_pin"].is<int>()) {
            sCurrentRfRx = doc["rf_rx_pin"];
            sCurrentRfRxName = doc["rf_rx_name"].as<String>();
        }
    }
}

static void saveConfig() {
    JsonDocument doc;
    ConfigManager::loadConfig(doc);
    doc["ir_tx_pin"] = sCurrentIrTx;
    doc["ir_tx_name"] = sCurrentIrTxName;
    doc["ir_rx_pin"] = sCurrentIrRx;
    doc["ir_rx_name"] = sCurrentIrRxName;
    doc["rf_tx_pin"] = sCurrentRfTx;
    doc["rf_tx_name"] = sCurrentRfTxName;
    doc["rf_rx_pin"] = sCurrentRfRx;
    doc["rf_rx_name"] = sCurrentRfRxName;
    ConfigManager::saveConfig(doc);
}

static void geometry(TFT_eSPI& tft) {
    sTft = &tft;
    tft.setRotation(ROT_TOP);
    W = tft.width(); H = tft.height();
    const int marginX = 6, marginTop = 6, marginBottom = 6;
    bx = marginX; by = marginTop; bw = W - 2*marginX; bh = H - by - marginBottom;
    cx = bx; cy = by; cw = bw;
}

static void drawBackground(TFT_eSPI& tft) {
    if (sBgDrawn) return;
    tft.setRotation(ROT_ALT);
    drawOptionsLayerBackground(tft);
    tft.setRotation(ROT_TOP);
    sBgDrawn = true;
}

static void drawTitle(TFT_eSPI& tft, const String& subtitle) {
    if (!sTitleDirty && subtitle == sLastStatus) return;
    sLastStatus = subtitle;    
    tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextColor(COL_ACC, COL_BG);
    tft.fillRect(cx, cy, cw, 14, COL_BG);   
    if (subtitle.length()) {
        tft.setTextColor(COL_TXT, COL_BG);
        int w = tft.textWidth(subtitle);
        tft.setCursor(cx+cw-w-2, cy+1); 
        tft.print(subtitle);
    }
    sTitleDirty = false;
}

static void drawMainSelection(TFT_eSPI& tft) {
    if (!sContentDirty) return;
    sContentDirty = false;
    const int contentTop = cy + 18;
    const int optionHeight = 25;
    tft.fillRect(cx, contentTop, cw, bh-18, COL_BG);
    tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextColor(COL_TXT, COL_BG);
    int y = contentTop + 8;
    String irInfo = "IR: TX=" + sCurrentIrTxName + " RX=" + sCurrentIrRxName;
    String rfInfo = "RF: TX=" + sCurrentRfTxName + " RX=" + sCurrentRfRxName;
    int irW = tft.textWidth(irInfo);
    int rfW = tft.textWidth(rfInfo);
    tft.setCursor(cx + (cw - irW)/2, y);
    tft.print(irInfo);
    y += 12;
    tft.setCursor(cx + (cw - rfW)/2, y);
    tft.print(rfInfo);
    y += 20;    
    const char* options[] = {"Configure IR Pins", "Configure RF Pins"};
    for (int i = 0; i < 2; i++) {
        bool selected = (i == sSelection);
        if (selected) {
            tft.fillRoundRect(cx+8, y-3, cw-16, optionHeight, 3, COL_ACC);
            tft.setTextColor(COL_BG, COL_ACC);
        } else {
            tft.drawRoundRect(cx+8, y-3, cw-16, optionHeight, 3, COL_ACC);
            tft.setTextColor(COL_TXT, COL_BG);
        }
        int optW = tft.textWidth(options[i]);
        tft.setCursor(cx + (cw - optW)/2, y+7);
        tft.print(options[i]);        
        y += optionHeight + 8;
    }
}

static void drawTxRxSelection(TFT_eSPI& tft) {
    if (!sContentDirty) return;
    sContentDirty = false;
    const int contentTop = cy + 18;
    const int optionHeight = 25;
    tft.fillRect(cx, contentTop, cw, bh-18, COL_BG);
    String mode = sIrRfMode == 0 ? "IR" : "RF";
    String txName = sIrRfMode == 0 ? sCurrentIrTxName : sCurrentRfTxName;
    String rxName = sIrRfMode == 0 ? sCurrentIrRxName : sCurrentRfRxName;
    int y = contentTop + 15;    
    tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextColor(COL_TXT, COL_BG);
    String current = mode + " Current: TX=" + txName + " RX=" + rxName;
    int currentW = tft.textWidth(current);
    tft.setCursor(cx + (cw - currentW)/2, y);
    tft.print(current);
    y += 25;
    const char* options[] = {"Configure TX Pin", "Configure RX Pin"};
    for (int i = 0; i < 2; i++) {
        bool selected = (i == sSelection);
        if (selected) {
            tft.fillRoundRect(cx+8, y-3, cw-16, optionHeight, 3, COL_ACC);
            tft.setTextColor(COL_BG, COL_ACC);
        } else {
            tft.drawRoundRect(cx+8, y-3, cw-16, optionHeight, 3, COL_ACC);
            tft.setTextColor(COL_TXT, COL_BG);
        }
        int optW = tft.textWidth(options[i]);
        tft.setCursor(cx + (cw - optW)/2, y+7);
        tft.print(options[i]);        
        y += optionHeight + 8;
    }
}

static void drawPinSelection(TFT_eSPI& tft) {
    if (!sContentDirty) return;
    sContentDirty = false;
    const int contentTop = cy + 18;
    const int optionHeight = 18;
    const int spacing = 3;
    const int maxVisible = 3; 
    tft.fillRect(cx, contentTop, cw, bh-18, COL_BG);
    String mode = sIrRfMode == 0 ? "IR" : "RF";
    String txrx = sTxRxMode == 0 ? "TX" : "RX";
    tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextColor(COL_ACC, COL_BG);
    String title = mode + " " + txrx + " Pin";
    int titleW = tft.textWidth(title);
    int titleY = contentTop + 5;
    tft.setCursor(cx + (cw - titleW)/2, titleY);
    tft.print(title);
    int scrollOffset = 0;
    if (PIN_OPTIONS_COUNT > maxVisible) {
        if (sSelection >= maxVisible) {
            scrollOffset = sSelection - maxVisible + 1;            
            if (scrollOffset > PIN_OPTIONS_COUNT - maxVisible) {
                scrollOffset = PIN_OPTIONS_COUNT - maxVisible;
            }
        }
    }

    int startY = titleY + 20; 
    const PinOption* options = sTxRxMode == 0 ? sTxPinOptions : sRxPinOptions;    
    for (int i = 0; i < maxVisible && (scrollOffset + i) < PIN_OPTIONS_COUNT; i++) {
        int itemIndex = scrollOffset + i;
        int y = startY + i * (optionHeight + spacing);        
        bool selected = (itemIndex == sSelection);       
        if (selected) {
            tft.fillRoundRect(cx+4, y-2, cw-8, optionHeight, 2, COL_ACC);
            tft.setTextColor(COL_BG, COL_ACC);
        } else {
            tft.drawRoundRect(cx+4, y-2, cw-8, optionHeight, 2, COL_ACC);
            tft.setTextColor(COL_TXT, COL_BG);
        }        
        tft.setCursor(cx+8, y+2);
        tft.print(options[itemIndex].name);         
        String gpio = "GPIO" + String(options[itemIndex].pin);
        int gpioW = tft.textWidth(gpio);
        tft.setCursor(cx + cw - gpioW - 8, y+2);
        tft.print(gpio);
    }
    
    if (PIN_OPTIONS_COUNT > maxVisible) {
        int indicatorX = cx + cw - 6;
        int indicatorY = titleY + 10;
        if (scrollOffset > 0) {
            tft.setTextColor(COL_ACC, COL_BG);
            tft.setCursor(indicatorX, indicatorY);
            tft.print("^");
        }
        
        if (scrollOffset + maxVisible < PIN_OPTIONS_COUNT) {
            tft.setTextColor(COL_ACC, COL_BG);
            tft.setCursor(indicatorX, indicatorY + (maxVisible * (optionHeight + spacing)) + 5);
            tft.print("v");
        }

        tft.setTextColor(COL_TXT, COL_BG);
        String pos = String(sSelection + 1) + "/" + String(PIN_OPTIONS_COUNT);
        int posW = tft.textWidth(pos);
        tft.setCursor(cx + (cw - posW)/2, startY + (maxVisible * (optionHeight + spacing)) + 10);
        tft.print(pos);
    }
}

void pinConfigReset() {
    sInit = false; sBgDrawn = false;
    sMenuLevel = 0;
    sSelection = 0;
    sTitleDirty = true;
    sContentDirty = true;
    sLastStatus = "";
    loadConfig();
}

void pinConfigDrawScreen(TFT_eSPI& tft) {
    if (!sInit) { geometry(tft); drawBackground(tft); sInit = true; }
    if (sMenuLevel == 0) {
        drawMainSelection(tft);
    } else if (sMenuLevel == 1) {
        drawTxRxSelection(tft);
    } else if (sMenuLevel == 2) {
        drawPinSelection(tft);
    }
}

void pinConfigHandleInput(bool a, bool b, bool c, bool& requestExit) {
    requestExit = false;
    if (c) {
        if (sMenuLevel == 0) {
            requestExit = true;
            return;
        } else if (sMenuLevel == 1) {
            sMenuLevel = 0;
            sSelection = sIrRfMode;
        } else if (sMenuLevel == 2) {
            sMenuLevel = 1;
            sSelection = sTxRxMode;
        }
        sTitleDirty = true;
        sContentDirty = true;
        return;
    }
    
    if (b) {
        if (sMenuLevel == 0) {
            sSelection = (sSelection + 1) % 2;
        } else if (sMenuLevel == 1) {
            sSelection = (sSelection + 1) % 2;
        } else if (sMenuLevel == 2) {
            sSelection = (sSelection + 1) % PIN_OPTIONS_COUNT;
        }
        sContentDirty = true;
        return;
    }
    
    if (a) {
        if (sMenuLevel == 0) {
            sIrRfMode = sSelection;
            sMenuLevel = 1;
            sSelection = 0;
        } else if (sMenuLevel == 1) {
            sTxRxMode = sSelection;
            sMenuLevel = 2;
            sSelection = 0;
        } else if (sMenuLevel == 2) {
            const PinOption* options = sTxRxMode == 0 ? sTxPinOptions : sRxPinOptions;
            int newPin = options[sSelection].pin;
            String newName = options[sSelection].name;
            
            if (sIrRfMode == 0) { 
                if (sTxRxMode == 0) { 
                    sCurrentIrTx = newPin;
                    sCurrentIrTxName = newName;
                } else { 
                    sCurrentIrRx = newPin;
                    sCurrentIrRxName = newName;
                }
            } else { 
                if (sTxRxMode == 0) { 
                    sCurrentRfTx = newPin;
                    sCurrentRfTxName = newName;
                } else { 
                    sCurrentRfRx = newPin;
                    sCurrentRfRxName = newName;
                }
            }
            saveConfig();
            sMenuLevel = 1;
            sSelection = sTxRxMode;
        }
        sTitleDirty = true;
        sContentDirty = true;
    }
}

int getCurrentIrTxPin() {
    if (sCurrentIrTx == -1) loadConfig();
    return sCurrentIrTx;
}

int getCurrentIrRxPin() {
    if (sCurrentIrRx == -1) loadConfig();
    return sCurrentIrRx;
}

int getCurrentRfTxPin() {
    if (sCurrentRfTx == -1) loadConfig();
    return sCurrentRfTx;
}

int getCurrentRfRxPin() {
    if (sCurrentRfRx == -1) loadConfig();
    return sCurrentRfRx;
}
