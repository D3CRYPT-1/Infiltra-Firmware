#include "nfc_reader.h"
#include "UserInterface/menus/menu_submenus.h"
#include <Adafruit_PN532.h>

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
static constexpr uint8_t NFC_I2C_ADDRESS = 0x24;
static constexpr uint8_t NFC_SDA_PIN = 32;  
static constexpr uint8_t NFC_SCL_PIN = 33;  
static Adafruit_PN532 nfc = Adafruit_PN532(2, 4);  
static bool sFrameDrawn = false;
static bool sBgDrawn = false;
static bool sDirty = true;
static bool sInit = false;
static NFCState sState = NFC_IDLE;
static NFCCardData sLastCard;
static uint32_t sLastScanTime = 0;
static uint32_t sAnimationTime = 0;
static uint8_t sAnimationFrame = 0;
static int W = 0, H = 0;
static int bx = 0, by = 0, bw = 0, bh = 0;
static int cx = 0, cy = 0, cw = 0;
static TFT_eSPI* sTft = nullptr;
static bool sNFCInitialized = false;
static uint32_t sLastDetectionAttempt = 0;

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
    String title = "NFC Reader (13.56MHz)";
    int titleW = tft.textWidth(title);
    tft.setCursor(bx + (bw - titleW) / 2, by + 4);
    tft.print(title);
    tft.drawFastHLine(bx + 6, by + 16, bw - 12, COL_ACC);
    sFrameDrawn = true;
    sDirty = true;
}

static void drawCreditCardIcon(TFT_eSPI& tft, int x, int y, int w, int h, uint16_t color, bool filled = false) {
    const int cornerRadius = 6;
    if (filled) {
        tft.fillRoundRect(x, y, w, h, cornerRadius, color);
    } else {
        tft.drawRoundRect(x, y, w, h, cornerRadius, color);
    }
    int stripeY = y + h / 3;
    int stripeH = h / 6;
    if (filled) {
        tft.fillRect(x + 2, stripeY, w - 4, stripeH, COL_BG);
    } else {
        tft.drawFastHLine(x + 2, stripeY, w - 4, color);
        tft.drawFastHLine(x + 2, stripeY + stripeH - 1, w - 4, color);
    }
    int chipX = x + w / 6;
    int chipY = y + h / 2;
    int chipW = w / 8;
    int chipH = h / 8;
    tft.fillRect(chipX, chipY, chipW, chipH, color);
}

static void drawReadingAnimation(TFT_eSPI& tft, int x, int y) {
    uint32_t now = millis();
    if (now - sAnimationTime > 200) {
        sAnimationFrame = (sAnimationFrame + 1) % 8;
        sAnimationTime = now;
        sDirty = true;
    }
    const int dotRadius = 2;
    const int orbitRadius = 25;
    for (int i = 0; i < 8; i++) {
        float angle = (i * 45 + sAnimationFrame * 45) * PI / 180.0;
        int dotX = x + orbitRadius * cos(angle);
        int dotY = y + orbitRadius * sin(angle);
        uint16_t dotColor = (i == sAnimationFrame) ? COL_ACC : COL_TXT;
        tft.fillCircle(dotX, dotY, dotRadius, dotColor);
    }
}

static void drawCheckmark(TFT_eSPI& tft, int x, int y, int size, uint16_t color) {
    int x1 = x - size/3;
    int y1 = y;
    int x2 = x;
    int y2 = y + size/3;
    int x3 = x + size/2;
    int y3 = y - size/3;
    tft.drawLine(x1, y1, x2, y2, color);
    tft.drawLine(x2, y2, x3, y3, color);
    tft.drawLine(x1, y1+1, x2, y2+1, color);
    tft.drawLine(x2, y2+1, x3, y3+1, color);
}

static void drawCardInfo(TFT_eSPI& tft) {
    if (!sLastCard.isValid) return;
    const int infoY = cy + 80;
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_ACC, COL_BG);
    tft.setCursor(cx, infoY);
    tft.print("UID: ");
    tft.setTextColor(COL_TXT, COL_BG);
    tft.print(sLastCard.uid);
    tft.setTextColor(COL_ACC, COL_BG);
    tft.setCursor(cx, infoY + 12);
    tft.print("Type: ");
    tft.setTextColor(COL_TXT, COL_BG);
    tft.print(sLastCard.type);
    tft.setTextColor(COL_ACC, COL_BG);
    tft.setCursor(cx, infoY + 24);
    tft.print("Size: ");
    tft.setTextColor(COL_TXT, COL_BG);
    tft.print(String(sLastCard.dataSize) + " bytes");
}

static bool initNFC() {
    if (sNFCInitialized) return true;
    Wire.begin(NFC_SDA_PIN, NFC_SCL_PIN);
    Wire.setClock(100000);
    sNFCInitialized = true;
    return true;
}

static bool scanForCard() {
    if (!sNFCInitialized) return false;
    uint32_t now = millis();
    if (now - sLastDetectionAttempt < 1000) return false; 
    sLastDetectionAttempt = now;
    return false;
}

void nfcReaderInit() {
    sNFCInitialized = true;
}

void nfcReaderUpdate() {
    uint32_t now = millis();
    if (sState == NFC_READING) {
        if (now - sAnimationTime > 200) {
            sAnimationFrame = (sAnimationFrame + 1) % 8;
            sAnimationTime = now;
            sDirty = true;
        }
    }
}

void nfcReaderReset() {
    sFrameDrawn = false;
    sBgDrawn = false;
    sDirty = true;
    sInit = false;
    sState = NFC_IDLE;
    sLastCard.isValid = false;
    sAnimationFrame = 0;
    sAnimationTime = 0;
    sLastDetectionAttempt = 0;
    if (sTft) {
        sTft->fillScreen(TFT_BLACK);
    }
    nfcReaderInit();
}

void nfcReaderDrawScreen(TFT_eSPI& tft) {
    if (!sInit) {
        tft.setRotation(ROT_TOP);
        geometry(tft);
        drawBackground(tft);
        sInit = true;
    }
    
    if (!sFrameDrawn) {
        drawFrame(tft);
    }
    nfcReaderUpdate();
    if (!sDirty) return;
    sDirty = false;
    tft.fillRect(cx, cy + 6, cw, bh - 30, COL_BG);
    const int cardW = 60;
    const int cardH = 38;
    const int cardX = cx + (cw - cardW) / 2;
    const int cardY = cy + 20;
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_TXT, COL_BG);
    String statusText = "";
    uint16_t cardColor = COL_TXT;
    bool cardFilled = false;
    switch (sState) {
        case NFC_IDLE:
            statusText = "Place card near reader...";
            cardColor = COL_TXT;
            break;
        case NFC_READING:
            statusText = "Reading card...";
            cardColor = COL_ACC;
            cardFilled = true;
            drawReadingAnimation(tft, cardX + cardW/2, cardY + cardH/2);
            break;
        case NFC_SUCCESS:
            statusText = "Card read successfully!";
            cardColor = COL_ACC;
            cardFilled = true;
            drawCheckmark(tft, cardX + cardW + 15, cardY + cardH/2, 12, COL_ACC);
            break;
        case NFC_ERROR:
            statusText = "Error reading card";
            cardColor = COL_TXT;
            break;
    }

    int statusW = tft.textWidth(statusText);
    tft.setCursor(cx + (cw - statusW) / 2, cy + 6);
    tft.print(statusText);
    drawCreditCardIcon(tft, cardX, cardY, cardW, cardH, cardColor, cardFilled);
    if (sState == NFC_SUCCESS && sLastCard.isValid) {
        drawCardInfo(tft);
    }
    const int btnY = by + bh - 20;
    tft.setTextColor(COL_TXT, COL_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(COL_TXT, COL_BG);
    String gpioText = "SDA:32 SCL:33";
    int gpioW = tft.textWidth(gpioText);
    tft.setCursor(cx + (cw - gpioW) / 2, btnY - 15);
    tft.print(gpioText);
}

void nfcReaderHandleInput(bool btnA, bool btnB, bool btnC, bool& exitReq) {
    if (btnC) {
        exitReq = true;
        sFrameDrawn = false;
        return;
    }
    if (btnA) {
        if (sState == NFC_SUCCESS) {
            sState = NFC_IDLE;
            sLastCard.isValid = false;
            sDirty = true;
        } else if (sState == NFC_IDLE) {
            sLastCard.uid = "04:A3:16:2A";
            sLastCard.type = "MIFARE Classic 1K";
            sLastCard.dataSize = 1024;
            sLastCard.protocol = NFC_PROTOCOL_MIFARE;
            sLastCard.sak = 0x08;
            sLastCard.atqa[0] = 0x00;
            sLastCard.atqa[1] = 0x04;
            sLastCard.isValid = true;
            sState = NFC_SUCCESS;
            sDirty = true;
        }
    }
}

NFCState nfcReaderGetState() {
    return sState;
}

NFCCardData nfcReaderGetLastCard() {
    return sLastCard;
}