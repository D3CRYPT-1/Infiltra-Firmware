#include "Credits.h"
#include "UserInterface/menus/menu_submenus.h"

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
static int sScrollOffset = 0;
static const int sLineHeight = 12;
static const int sMaxVisible = 8; 
static unsigned long sLastScrollTime = 0;
static const unsigned long sScrollDelay = 150;
static int W = 0, H = 0;
static int bx = 0, by = 0, bw = 0, bh = 0;
static int cx = 0, cy = 0, cw = 0;
static TFT_eSPI* sTft = nullptr;

static const String sCreditsText[] = {
  "Thanks to the Infiltra Community,", 
  "M5Stack, ESP8266 Deauther Team,",
  "and others for their contributions",
  "to this project.",
  "",
  "Team:",
  "",
  "Hecker (D3CRYPT) (@D3CRYPT-1)",
  "Owner, CEO & Founder of",
  "Infiltra Network & Firmware",
  "",
  "FTW Luke (@d1goat0134)",
  "Main Developer & Co-Owner",
  "of Infiltra",
  "",
  "SgtFlixy (@sgtflixy)",
  "Assisted with website",
  "",
  "Abdul (@ab9l)",
  "Website Developer & Designer",
  "",
  "Fourier (9dl)",
  "Website Engineer & UI Maker",
  "",
  "Msi(sadecemsi)",
  "Headless Dev & html/css dev",
  "",
  "Support Us:",
  "",
  "If you appreciate Infiltra",
  "Firmware, consider starring",
  "our GitHub repo!",
  "",
  "Thank you for using Infiltra!"
};

static const int sCreditsTextCount = sizeof(sCreditsText) / sizeof(sCreditsText[0]);
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
    String title = "Credits & Acknowledgments";
    int titleW = tft.textWidth(title);
    tft.setCursor(bx + (bw - titleW) / 2, by + 4);
    tft.print(title);    
    tft.drawFastHLine(bx + 6, by + 16, bw - 12, COL_ACC);    
    sFrameDrawn = true;
    sDirty = true;
}

void creditsReset() {
    sFrameDrawn = false;
    sBgDrawn = false;
    sDirty = true;
    sInit = false;
    sScrollOffset = 0;
    sLastScrollTime = 0;
}

void creditsDrawScreen(TFT_eSPI& tft) {
    if (!sInit) {
        geometry(tft);
        drawBackground(tft);
        sInit = true;
    }
    if (!sFrameDrawn) {
        drawFrame(tft);
    }
    if (!sDirty) return;
    sDirty = false;
    const int startY = cy + 6;
    const int spacing = 2;
    int totalLines = sCreditsTextCount;
    int maxScroll = max(0, totalLines - sMaxVisible);
    if (sScrollOffset > maxScroll) sScrollOffset = maxScroll;
    tft.fillRect(cx, startY, cw, bh - 30, COL_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);
    for (int i = 0; i < sMaxVisible && (sScrollOffset + i) < totalLines; i++) {
        int lineIndex = sScrollOffset + i;
        int y = startY + i * (sLineHeight + spacing);        
        tft.setCursor(cx + 4, y + 2);
        if (sCreditsText[lineIndex].indexOf("Team:") >= 0 || 
            sCreditsText[lineIndex].indexOf("Support Us") >= 0) {
            tft.setTextColor(COL_ACC, COL_BG);
        } else if (sCreditsText[lineIndex].indexOf("(@") >= 0) {
            tft.setTextColor(COL_ACC, COL_BG);
        } else if (sCreditsText[lineIndex].length() > 0) {
            tft.setTextColor(COL_TXT, COL_BG);
        } else {
            continue; 
        }        
        tft.print(sCreditsText[lineIndex]);
    }

    const int btnY = by + bh - 20;
    const int btnW = 50;
    const int btnH = 14;
    const int btnX = cx + (cw - btnW) / 2;    
    tft.drawRoundRect(btnX, btnY, btnW, btnH, 3, COL_TXT);
    tft.setTextColor(COL_TXT, COL_BG);
    String backText = "C: Back";
    int backW = tft.textWidth(backText);
    tft.setCursor(btnX + (btnW - backW) / 2, btnY + 3);
    tft.print(backText);
    if (totalLines > sMaxVisible) {
        const int indX = cx + cw - 20;
        tft.setTextColor(COL_TXT, COL_BG);
        String pos = String(min(sScrollOffset + sMaxVisible, totalLines)) + "/" + String(totalLines);
        int posW = tft.textWidth(pos);
        tft.setCursor(cx + (cw - posW) / 2, btnY - 15);
        tft.print(pos);
    }
}

void creditsHandleInput(bool btnA, bool btnB, bool btnC, bool& exitReq) {
    unsigned long currentTime = millis();   
    if (btnC) {
        exitReq = true;
        sFrameDrawn = false;
        return;
    }    
    int maxScroll = max(0, sCreditsTextCount - sMaxVisible);    
    if (btnB && (currentTime - sLastScrollTime) > sScrollDelay) {
        if (sScrollOffset < maxScroll) {
            sScrollOffset++;
            sLastScrollTime = currentTime;
            sDirty = true;
        }
    }        
    if (btnA && (currentTime - sLastScrollTime) > sScrollDelay) {
        if (sScrollOffset > 0) {
            sScrollOffset--;
            sLastScrollTime = currentTime;
            sDirty = true;
        }
    }
}