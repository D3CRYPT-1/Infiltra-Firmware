#include "battery_saver.h"
#include "UserInterface/menus/menu_submenus.h"
#include "Modules/Core/User Config/ConfigManager.h"
#include "brightness.h"
#include <Arduino.h>
#include <WiFi.h>

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

static constexpr uint8_t ROT_ALT = (ROT_TOP + 3) & 0x3;
static constexpr uint16_t COL_BG = TFT_BLACK;
static constexpr uint16_t COL_TXT = TFT_SILVER;
static constexpr uint16_t COL_ACC = TFT_WHITE;
static constexpr uint16_t COL_SELECTED = TFT_WHITE;
static constexpr uint16_t COL_CURRENT = TFT_WHITE;
static bool sFrameDrawn = false;
static bool sBgDrawn = false;
static bool sDirty = true;
static bool sInit = false;
static int sSelection = 0;
static BatterySaverMode sCurrentMode = BatterySaverMode::OFF;
static BatterySaverMode sOriginalBrightness = BatterySaverMode::OFF;
static int W = 0, H = 0;
static int bx = 0, by = 0, bw = 0, bh = 0;
static int cx = 0, cy = 0, cw = 0;
static TFT_eSPI* sTft = nullptr;

static const BatterySaverProfile sProfiles[] = {
    {
        "Off", 
        "Normal operation",
        240,  
        100,  
        false, false, false, 0
    },
    {
        "Light", 
        "Slight power savings",
        240,  
        85,   
        false, false, false, 300000  
    },
    {
        "Basic", 
        "Moderate power savings",
        160,  
        70,   
        true,  false, false, 180000  
    },
    {
        "Eco Saver", 
        "Maximum power savings",
        80,   
        50,   
        true,  true,  true,  60000   
    }
};

static const int PROFILE_COUNT = sizeof(sProfiles) / sizeof(sProfiles[0]);
static void loadConfig() {
    sCurrentMode = BatterySaverMode::OFF; 
    
    JsonDocument doc;
    if (ConfigManager::loadConfig(doc)) {
        if (doc["battery_saver_mode"].is<int>()) {
            int mode = doc["battery_saver_mode"];
            if (mode >= 0 && mode < PROFILE_COUNT) {
                sCurrentMode = static_cast<BatterySaverMode>(mode);
            }
        }
    }
    sSelection = static_cast<int>(sCurrentMode);
}

static void saveConfig() {
    JsonDocument doc; 
    ConfigManager::loadConfig(doc);
    doc["battery_saver_mode"] = static_cast<int>(sCurrentMode);
    ConfigManager::saveConfig(doc);
}

static void geometry(TFT_eSPI& tft) {
    sTft = &tft;
    tft.setRotation(ROT_TOP);
    W = tft.width();
    H = tft.height();
    const int marginX = 8;
    const int marginTop = 25;
    const int marginBottom = 8;
    bx = marginX;
    by = marginTop;
    bw = W - 2 * marginX;
    bh = H - by - marginBottom;
    cx = bx + 8;
    cy = by + 12;
    cw = bw - 16;
}

static void drawBackground(TFT_eSPI& tft) {
    if (sBgDrawn) return;
    tft.setRotation(ROT_ALT);
    drawOptionsLayerBackground(tft);
    tft.setRotation(ROT_TOP);
    sBgDrawn = true;
}

static void drawFrame(TFT_eSPI& tft) {
    if (sFrameDrawn) return;
    tft.fillRect(bx, by, bw, bh, COL_BG);
    tft.drawRoundRect(bx, by, bw, bh, 6, COL_ACC);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_ACC, COL_BG);
    String title = "Battery Saver";
    int titleW = tft.textWidth(title);
    tft.setCursor(bx + (bw - titleW) / 2, by + 4);
    tft.print(title);
    tft.drawFastHLine(bx + 6, by + 16, bw - 12, COL_ACC);
    sFrameDrawn = true;
    sDirty = true;
}

static String getModeStats(BatterySaverMode mode) {
    const BatterySaverProfile& profile = sProfiles[static_cast<int>(mode)];
    if (mode == BatterySaverMode::OFF) {
        return "Full Performance";
    }
    String stats = "CPU:" + String(profile.cpuFreqMhz) + "MHz ";
    stats += "LCD:" + String(profile.brightnessScale) + "%";
    if (profile.disableWiFi || profile.disableBluetooth || profile.limitHardware) {
        stats += " HW:Limited";
    }
    return stats;
}

static void drawModeSelection(TFT_eSPI& tft) {
    if (!sDirty) return;
    sDirty = false;
    const int itemY = cy + 6;
    const int itemH = 24;
    const int spacing = 3;
    const int maxVisible = 3; 
    int scrollOffset = 0;
    if (sSelection >= maxVisible) {
        scrollOffset = sSelection - maxVisible + 1;
        if (scrollOffset > PROFILE_COUNT - maxVisible) {
            scrollOffset = PROFILE_COUNT - maxVisible;
        }
    }

    tft.fillRect(cx, itemY, cw, bh - 24, COL_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);

    for (int i = 0; i < maxVisible && (scrollOffset + i) < PROFILE_COUNT; i++) {
        int itemIndex = scrollOffset + i;
        const BatterySaverProfile& profile = sProfiles[itemIndex];
        BatterySaverMode mode = static_cast<BatterySaverMode>(itemIndex);        
        int y = itemY + i * (itemH + spacing);
        bool selected = (itemIndex == sSelection);
        bool current = (mode == sCurrentMode);       
        if (selected) {
            tft.fillRoundRect(cx, y, cw, itemH, 3, COL_SELECTED);
            tft.setTextColor(COL_BG, COL_SELECTED);
        } else {
            tft.drawRoundRect(cx, y, cw, itemH, 3, COL_TXT);
            tft.setTextColor(COL_TXT, COL_BG);
        }
        tft.setCursor(cx + 6, y + 3);
        tft.print(profile.name);
        if (current) {
            tft.setTextColor(selected ? COL_BG : COL_CURRENT, selected ? COL_SELECTED : COL_BG);
            tft.setCursor(cx + cw - 20, y + 3);
            tft.print("ON");
        }
        String stats = getModeStats(mode);
        tft.setTextColor(selected ? COL_BG : COL_TXT, selected ? COL_SELECTED : COL_BG);
        tft.setCursor(cx + 6, y + 13);
        tft.print(stats);
    }
    if (PROFILE_COUNT > maxVisible) {
        int indicatorX = cx + cw - 8;
        int indicatorY = itemY + (maxVisible * (itemH + spacing)) / 2;
    }
    
}

void batterySaverReset() {
    sFrameDrawn = false;
    sBgDrawn = false;
    sDirty = true;
    sInit = false;
    loadConfig();
    sSelection = static_cast<int>(sCurrentMode);
}

void batterySaverDrawScreen(TFT_eSPI& tft) {
    if (!sInit) {
        geometry(tft);
        drawBackground(tft);
        sInit = true;
    }    
    if (!sFrameDrawn) {
        drawFrame(tft);
    }    
    drawModeSelection(tft);
}

void batterySaverHandleInput(bool a, bool b, bool c, bool& requestExit) {
    requestExit = false;   
    if (c) {
        requestExit = true;
        sFrameDrawn = false;
        return;
    }
    if (b) {
        sSelection = (sSelection - 1 + PROFILE_COUNT) % PROFILE_COUNT;
        sDirty = true;
    }  
    if (a) {
        BatterySaverMode newMode = static_cast<BatterySaverMode>(sSelection);
        batterySaverApplyMode(newMode);
        sCurrentMode = newMode;
        saveConfig();
        sDirty = true;
    }
}

void batterySaverApplyMode(BatterySaverMode mode) {
    const BatterySaverProfile& profile = sProfiles[static_cast<int>(mode)];
    batterySaverSetCpuFrequency(profile.cpuFreqMhz);
    batterySaverApplyBrightnessScale(profile.brightnessScale);
    if (profile.disableWiFi && WiFi.getMode() != WIFI_OFF) {
        WiFi.mode(WIFI_OFF);
    }
    sCurrentMode = mode;
}

BatterySaverMode batterySaverGetCurrentMode() {
    return sCurrentMode;
}

void batterySaverInitAndApply() {
    loadConfig();
    batterySaverApplyMode(sCurrentMode);
}

String batterySaverGetModeName(BatterySaverMode mode) {
    return sProfiles[static_cast<int>(mode)].name;
}

BatterySaverProfile batterySaverGetProfile(BatterySaverMode mode) {
    return sProfiles[static_cast<int>(mode)];
}

bool batterySaverIsHardwareLimited() {
    const BatterySaverProfile& profile = sProfiles[static_cast<int>(sCurrentMode)];
    return profile.limitHardware;
}

void batterySaverSetCpuFrequency(uint8_t freqMhz) {   
    setCpuFrequencyMhz(freqMhz);
}

void batterySaverApplyBrightnessScale(uint8_t scalePercent) {
    JsonDocument doc;
    ConfigManager::loadConfig(doc);
    doc["brightness_scale"] = scalePercent;
    ConfigManager::saveConfig(doc);
}