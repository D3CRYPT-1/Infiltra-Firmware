#include "time.h"
#include "UserInterface/menus/menu_submenus.h"
#include "Modules/Core/User Config/ConfigManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

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
static bool sFrameDrawn = false;
static bool sBgDrawn = false;
static bool sDirty = true;
static bool sInit = false;
static int sSelection = 0;
static TimeState sState = TIME_CONFIG;
static TimeMode sTimeMode = TIME_OFF;
static int W = 0, H = 0;
static int bx = 0, by = 0, bw = 0, bh = 0;
static int cx = 0, cy = 0, cw = 0;
static TFT_eSPI* sTft = nullptr;
static bool sScreensaverActive = false;
static uint32_t sLastActivity = 0;
static uint32_t sScreensaverTimeout = 30000; 
static uint32_t sLastFactChange = 0;
static int sCurrentFact = 0;
static String sWiFiSSID = "";
static String sWiFiPassword = "";
static bool sWiFiConnected = false;
static int sWiFiScanCount = 0;
static String sWiFiNetworks[20];
static bool sShowKeyboard = false;
static String sPasswordInput = "";
static int sKbIndex = 0;
static uint32_t sBlinkLast = 0;
static bool sBlinkOn = true;
static const String sTimeOptions[] = {
    "Uptime Mode", 
    "Local Time Mode",
    "Screensaver Timeout",
    "<- Back"
};
static const int TIME_OPTION_COUNT = sizeof(sTimeOptions) / sizeof(sTimeOptions[0]);
static const String sInfiltraFacts[] = {
    "Infiltra: Multi-tool security device",
    "ESP32-based penetration testing tool",
    "Features WiFi, Bluetooth, IR, RF modules",
    "Open source hardware and software",
    "Supports M5Stack and custom hardware",
    "Real-time packet analysis capabilities",
    "Built-in LoRa communication support",
    "Customizable pin configuration system",
    "Advanced battery management modes",
    "Integrated file system management"
};
static const int FACT_COUNT = sizeof(sInfiltraFacts) / sizeof(sInfiltraFacts[0]);
struct Key { const char* label; char ch; };
static const Key row1[10] = {{"q",'q'},{"w",'w'},{"e",'e'},{"r",'r'},{"t",'t'},{"y",'y'},{"u",'u'},{"i",'i'},{"o",'o'},{"p",'p'}};
static const Key row2[10] = {{"a",'a'},{"s",'s'},{"d",'d'},{"f",'f'},{"g",'g'},{"h",'h'},{"j",'j'},{"k",'k'},{"l",'l'},{"'",'\''}};
static const Key row3[10] = {{"<-",0},{"z",'z'},{"x",'x'},{"c",'c'},{"v",'v'},{"--",' '},{"b",'b'},{"n",'n'},{"m",'m'},{">",0}};

static void loadConfig() {
    sTimeMode = TIME_OFF; 
    sScreensaverTimeout = 30000; 
    
    JsonDocument doc;
    if (ConfigManager::loadConfig(doc)) {
        if (doc["time_mode"].is<int>()) {
            int mode = doc["time_mode"];
            if (mode >= 0 && mode <= 2) {
                sTimeMode = static_cast<TimeMode>(mode);
            }
        }
        if (doc["screensaver_timeout"].is<int>()) {
            sScreensaverTimeout = doc["screensaver_timeout"];
        }
    }
}

static void saveConfig() {
    JsonDocument doc;
    ConfigManager::loadConfig(doc);
    doc["time_mode"] = static_cast<int>(sTimeMode);
    doc["screensaver_timeout"] = sScreensaverTimeout;
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
    String title = "Time Configuration";
    int titleW = tft.textWidth(title);
    tft.setCursor(bx + (bw - titleW) / 2, by + 4);
    tft.print(title);
    tft.drawFastHLine(bx + 6, by + 16, bw - 12, COL_ACC);
    sFrameDrawn = true;
    sDirty = true;
}

static void drawWiFiScan(TFT_eSPI& tft) {
    if (!sDirty) return;
    sDirty = false;
    const int itemY = cy + 6;
    const int itemH = 16;
    const int spacing = 1;
    const int maxVisible = 5;
    tft.fillRect(cx, itemY, cw, bh - 24, COL_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_ACC, COL_BG);
    tft.setCursor(cx, itemY);
    tft.print("WiFi Networks:");
    for (int i = 0; i < min(maxVisible, sWiFiScanCount); i++) {
        int y = itemY + 20 + i * (itemH + spacing);
        bool selected = (i == sSelection);
        if (selected) {
            tft.fillRoundRect(cx, y, cw, itemH, 2, COL_SELECTED);
            tft.setTextColor(COL_BG, COL_SELECTED);
        } else {
            tft.setTextColor(COL_TXT, COL_BG);
        }
        tft.setCursor(cx + 4, y + 3);
        tft.print(sWiFiNetworks[i]);
    }
}

static void drawPasswordEntry(TFT_eSPI& tft) {
    if (!sDirty) return;
    const int kbdHeight = 50;
    const int promptY = by + bh - kbdHeight - 20;
    tft.fillRect(cx, cy + 20, cw, promptY - cy - 20, COL_BG);
    tft.fillRect(cx, promptY, cw, kbdHeight + 20, COL_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_ACC, COL_BG);
    tft.setCursor(cx, cy + 20);
    tft.print("Connecting to: " + sWiFiSSID);
    tft.setTextColor(COL_TXT, COL_BG);
    tft.setCursor(cx, promptY);
    tft.print("Password: ");
    String masked = "";
    for (int i = 0; i < sPasswordInput.length(); i++) {
        masked += "*";
    }
    tft.print(masked);
    uint32_t now = millis();
    if (now - sBlinkLast > 500) {
        sBlinkLast = now;
        sBlinkOn = !sBlinkOn;
    }
    if (sBlinkOn) {
        tft.print("|");
    }
    const int cellW = cw / 10;
    const int cellH = 12;
    const int kbdY = promptY + 15;
    tft.setTextColor(COL_TXT, COL_BG);
    for (int row = 0; row < 3; row++) {
        const Key* keys = (row == 0) ? row1 : (row == 1) ? row2 : row3;
        for (int col = 0; col < 10; col++) {
            int idx = row * 10 + col;
            int x = cx + col * cellW;
            int y = kbdY + row * cellH;
            bool selected = (idx == sKbIndex);
            if (selected) {
                tft.fillRect(x, y, cellW-1, cellH-1, COL_SELECTED);
                tft.setTextColor(COL_BG, COL_SELECTED);
            } else {
                tft.drawRect(x, y, cellW-1, cellH-1, COL_TXT);
                tft.setTextColor(COL_TXT, COL_BG);
            }
            int textW = tft.textWidth(keys[col].label);
            tft.setCursor(x + (cellW - textW) / 2, y + 2);
            tft.print(keys[col].label);
        }
    }
    sDirty = false;
}

static void drawConfigMenu(TFT_eSPI& tft) {
    if (!sDirty) return;
    sDirty = false;
    const int itemY = cy + 6;
    const int itemH = 20;
    const int spacing = 2;
    const int maxVisible = 3; 
    int scrollOffset = 0;
    if (sSelection >= maxVisible) {
        scrollOffset = sSelection - maxVisible + 1;
        if (scrollOffset > TIME_OPTION_COUNT - maxVisible) {
            scrollOffset = TIME_OPTION_COUNT - maxVisible;
        }
    }
    tft.fillRect(cx, itemY, cw, bh - 24, COL_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);
    for (int i = 0; i < maxVisible && (scrollOffset + i) < TIME_OPTION_COUNT; i++) {
        int itemIndex = scrollOffset + i;
        int y = itemY + i * (itemH + spacing);
        bool selected = (itemIndex == sSelection);
        if (selected) {
            tft.fillRoundRect(cx, y, cw, itemH, 3, COL_SELECTED);
            tft.setTextColor(COL_BG, COL_SELECTED);
        } else {
            tft.drawRoundRect(cx, y, cw, itemH, 3, COL_TXT);
            tft.setTextColor(COL_TXT, COL_BG);
        }
        tft.setCursor(cx + 6, y + 6);
        tft.print(sTimeOptions[itemIndex]);
        if (itemIndex == 0) { 
            if (sTimeMode == TIME_UPTIME) {
                tft.setTextColor(selected ? COL_BG : COL_ACC, selected ? COL_SELECTED : COL_BG);
                tft.setCursor(cx + cw - 30, y + 6);
                tft.print("ON");
            } else {
                tft.setTextColor(selected ? COL_BG : COL_TXT, selected ? COL_SELECTED : COL_BG);
                tft.setCursor(cx + cw - 35, y + 6);
                tft.print("OFF");
            }
        } else if (itemIndex == 1) { 
            if (sTimeMode == TIME_LOCAL) {
                tft.setTextColor(selected ? COL_BG : COL_ACC, selected ? COL_SELECTED : COL_BG);
                tft.setCursor(cx + cw - 30, y + 6);
                tft.print("ON");
            } else {
                tft.setTextColor(selected ? COL_BG : COL_TXT, selected ? COL_SELECTED : COL_BG);
                tft.setCursor(cx + cw - 35, y + 6);
                tft.print("OFF");
            }
        } else if (itemIndex == 2) { 
            tft.setTextColor(selected ? COL_BG : COL_TXT, selected ? COL_SELECTED : COL_BG);
            String timeout = String(sScreensaverTimeout / 1000) + "s";
            int timeoutW = tft.textWidth(timeout);
            tft.setCursor(cx + cw - timeoutW - 6, y + 6);
            tft.print(timeout);
        }
    }

    if (TIME_OPTION_COUNT > maxVisible) {
        int indicatorX = cx + cw - 8;
        int indicatorY = itemY + (maxVisible * (itemH + spacing)) / 2;
        tft.setTextColor(COL_TXT, COL_BG);
        String pos = String(sSelection + 1) + "/" + String(TIME_OPTION_COUNT);
        int posW = tft.textWidth(pos);
        tft.setCursor(cx + (cw - posW) / 2, itemY + (maxVisible * (itemH + spacing)) + 5);
        tft.print(pos);
    }
}

String timeGetUptimeString() {
    unsigned long uptimeMs = millis();
    unsigned long seconds = uptimeMs / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24; 
    hours = hours % 24;
    minutes = minutes % 60;
    seconds = seconds % 60;  
    String uptime = "";
    if (days > 0) {
        uptime += String(days) + "d ";
    }
    if (hours > 0 || days > 0) {
        uptime += String(hours) + "h ";
    }
    uptime += String(minutes) + "m " + String(seconds) + "s";    
    return uptime;
}

String timeGetLocalTimeString() {
    if (!sWiFiConnected) {
        return "No WiFi Connection";
    }    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Time Not Synced";
    }    
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    return String(timeStr);
}

static void drawScreensaver(TFT_eSPI& tft) {
    tft.fillScreen(COL_BG);
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(COL_ACC, COL_BG);
    String timeStr;
    if (sTimeMode == TIME_UPTIME) {
        timeStr = "Uptime: " + timeGetUptimeString();
    } else {
        timeStr = "Time: " + timeGetLocalTimeString();
    }    
    int timeW = tft.textWidth(timeStr);
    tft.setCursor((W - timeW) / 2, H / 2 - 20);
    tft.print(timeStr);
    uint32_t now = millis();
    if (now - sLastFactChange > 10000) { 
        sCurrentFact = (sCurrentFact + 1) % FACT_COUNT;
        sLastFactChange = now;
    }
    tft.setTextSize(1);
    tft.setTextColor(COL_TXT, COL_BG);
    String fact = sInfiltraFacts[sCurrentFact];
    int factW = tft.textWidth(fact);
    tft.setCursor((W - factW) / 2, H / 2 + 20);
    tft.print(fact);
}

void timeReset() {
    sFrameDrawn = false;
    sBgDrawn = false;
    sDirty = true;
    sInit = false;
    sState = TIME_CONFIG;
    sSelection = 0;
    sShowKeyboard = false;
    sPasswordInput = "";
    loadConfig();
    sLastActivity = millis();
}

void timeDrawScreen(TFT_eSPI& tft) {
    if (sScreensaverActive) {
        drawScreensaver(tft);
        return;
    }
    
    if (!sInit) {
        geometry(tft);
        drawBackground(tft);
        sInit = true;
    }
    
    if (!sFrameDrawn) {
        drawFrame(tft);
    }
    
    switch (sState) {
        case TIME_CONFIG:
            drawConfigMenu(tft);
            break;
        case TIME_WIFI_SCAN:
            drawWiFiScan(tft);
            break;
        case TIME_WIFI_PASSWORD:
            drawPasswordEntry(tft);
            break;
        default:
            drawConfigMenu(tft);
            break;
    }
}

static void performWiFiScan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    sWiFiScanCount = min(n, 20);
    for (int i = 0; i < sWiFiScanCount; i++) {
        sWiFiNetworks[i] = WiFi.SSID(i);
    }
    if (sWiFiScanCount == 0) {
        sWiFiNetworks[0] = "No networks found";
        sWiFiScanCount = 1;
    }
}

static char getKeyboardChar() {
    if (sKbIndex < 10) return row1[sKbIndex].ch;
    if (sKbIndex < 20) return row2[sKbIndex - 10].ch;
    return row3[sKbIndex - 20].ch;
}

static const char* getKeyboardLabel() {
    if (sKbIndex < 10) return row1[sKbIndex].label;
    if (sKbIndex < 20) return row2[sKbIndex - 10].label;
    return row3[sKbIndex - 20].label;
}

void timeHandleInput(bool a, bool b, bool c, bool& requestExit) {
    requestExit = false;
    sLastActivity = millis();
    if (sScreensaverActive) {
        sScreensaverActive = false;
        sDirty = true;
        return;
    }
    
    if (sState == TIME_CONFIG) {
        if (b) {
            sSelection = (sSelection + 1) % TIME_OPTION_COUNT;
            sDirty = true;
        }
        
        if (c) {
            sSelection = (sSelection - 1 + TIME_OPTION_COUNT) % TIME_OPTION_COUNT;
            sDirty = true;
        }
        
        if (a) {
            switch (sSelection) {
                case 0: 
                    if (sTimeMode == TIME_UPTIME) {
                        
                        sTimeMode = TIME_OFF;
                    } else {
                        
                        sTimeMode = TIME_UPTIME;
                    }
                    saveConfig();
                    sDirty = true;
                    break;
                case 1: 
                    if (sTimeMode == TIME_LOCAL) {
                        
                        sTimeMode = TIME_OFF;
                        saveConfig();
                        sDirty = true;
                    } else {
                        
                        sTimeMode = TIME_LOCAL;
                        saveConfig();
                        
                        sState = TIME_WIFI_SCAN;
                        sSelection = 0;
                        performWiFiScan();
                        sDirty = true;
                        sFrameDrawn = false;
                    }
                    break;
                case 2: 
                    
                    if (sScreensaverTimeout == 15000) sScreensaverTimeout = 30000;
                    else if (sScreensaverTimeout == 30000) sScreensaverTimeout = 60000;
                    else if (sScreensaverTimeout == 60000) sScreensaverTimeout = 120000;
                    else sScreensaverTimeout = 15000;
                    saveConfig();
                    sDirty = true;
                    break;
                case 3: 
                    requestExit = true;
                    sFrameDrawn = false;
                    break;
            }
        }
    }
    else if (sState == TIME_WIFI_SCAN) {
        if (c) {
            sState = TIME_CONFIG;
            sSelection = 1;
            sDirty = true;
            sFrameDrawn = false;
            return;
        }
        
        if (b) {
            sSelection = (sSelection - 1 + sWiFiScanCount) % sWiFiScanCount;
            sDirty = true;
        }
        
        if (a && sSelection < sWiFiScanCount) {
            sWiFiSSID = sWiFiNetworks[sSelection];
            sState = TIME_WIFI_PASSWORD;
            sPasswordInput = "";
            sKbIndex = 0;
            sDirty = true;
            sFrameDrawn = false;
        }
    }
    else if (sState == TIME_WIFI_PASSWORD) {
        if (c) {
            sState = TIME_WIFI_SCAN;
            sSelection = 0;
            sDirty = true;
            sFrameDrawn = false;
            return;
        }
        
        if (b) {
            sKbIndex = (sKbIndex + 1) % 30;
            sDirty = true;
        }
        
        if (a) {
            const char* label = getKeyboardLabel();
            if (strcmp(label, "<-") == 0) {
                
                if (sPasswordInput.length() > 0) {
                    sPasswordInput.remove(sPasswordInput.length() - 1);
                    sDirty = true;
                }
            } else if (strcmp(label, ">") == 0) {
                timeConnectToWiFi(sWiFiSSID, sPasswordInput);
                sState = TIME_CONFIG;
                sSelection = 1;
                sDirty = true;
                sFrameDrawn = false;
            } else if (strcmp(label, "--") == 0) {
                
                sPasswordInput += " ";
                sDirty = true;
            } else {
                char ch = getKeyboardChar();
                if (ch != 0) {
                    sPasswordInput += ch;
                    sDirty = true;
                }
            }
        }
    }
}

void timeInitScreensaver() {   
    if (sTimeMode != TIME_OFF && !sScreensaverActive && (millis() - sLastActivity > sScreensaverTimeout)) {
        sScreensaverActive = true;
        sLastFactChange = millis();
        sCurrentFact = 0;
    }
}

void timeUpdateScreensaver() {
    timeInitScreensaver();
}

bool timeIsScreensaverActive() {
    return sScreensaverActive;
}

void timeDeactivateScreensaver() {
    sScreensaverActive = false;
    sLastActivity = millis();
}

void timeSetScreensaverTimeout(uint32_t timeoutMs) {
    sScreensaverTimeout = timeoutMs;
    saveConfig();
}

void timeSetMode(TimeMode mode) {
    sTimeMode = mode;
    saveConfig();
}

TimeMode timeGetMode() {
    return sTimeMode;
}

bool timeIsConfigured() {
    return true; 
}


void timeConnectToWiFi(const String& ssid, const String& password) {
    sWiFiSSID = ssid;
    sWiFiPassword = password;
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    sWiFiConnected = (WiFi.status() == WL_CONNECTED);
    if (sWiFiConnected) {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        timeSyncWithNTP();
    }
}

bool timeIsWiFiConnected() {
    return sWiFiConnected && (WiFi.status() == WL_CONNECTED);
}

void timeSyncWithNTP() {
    if (!sWiFiConnected) return;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) { 
    }
}

void timeDrawMainMenuDisplay(TFT_eSPI& tft) {
    if (sTimeMode == TIME_OFF) return;
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_TXT, COL_BG);
    String timeStr = "";
    if (sTimeMode == TIME_UPTIME) {
        timeStr = "Up: " + timeGetUptimeString();
    } else if (sTimeMode == TIME_LOCAL) {
        timeStr = timeGetLocalTimeString();
    }
    const int timeX = 5;
    const int timeY = 15;
    int timeW = tft.textWidth(timeStr);
    tft.fillRect(timeX, timeY - 2, timeW + 4, 10, COL_BG);
    tft.setCursor(timeX, timeY);
    tft.print(timeStr);
}