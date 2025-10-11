#include "ir_read.h"
#include "UserInterface/menus/menu_submenus.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../Settings/pin_config.h"

#if __has_include(<IRremote.hpp>)
  #include <IRremote.hpp>
#else
  #include <IRremote.h>
#endif

#ifndef IR_RX_PIN
  #warning "IR_RX_PIN not defined! Defaulting to GPIO 33."
  #define IR_RX_PIN 33
#endif

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;
static constexpr uint16_t C_BG         = TFT_BLACK;
static constexpr uint16_t C_FRAME      = TFT_DARKGREY;
static constexpr uint16_t C_TEXT       = TFT_WHITE;
static constexpr uint16_t C_SUB        = TFT_SILVER;
static constexpr uint16_t C_ACCENT     = 0x39C7;
static constexpr uint16_t C_PILL_BG    = 0x10A2;
static constexpr uint16_t C_PILL_OUT   = 0x3186;
static constexpr uint16_t C_CHIP_IDLE  = 0x9CF3;
static constexpr uint16_t C_CHIP_OK    = 0x07E0;
static constexpr uint16_t C_CHIP_PAUSE = 0xFEA0;
static constexpr uint16_t C_WAVE_HI    = TFT_WHITE;
static constexpr uint16_t C_WAVE_LO    = 0x8410;
static constexpr uint16_t C_WAVE_GRID  = 0x528A;
enum class IRState : uint8_t { WAITING, RECEIVED };
static IRState  sState   = IRState::WAITING;
static bool     sPaused  = false;
static String   sProto = "-";
static uint32_t sAddr  = 0;
static uint32_t sCmd   = 0;
static uint64_t sValue = 0;
static uint16_t sRawBuf[256];
static uint16_t sRawLen = 0;
static bool     sNeedsFullRedraw = true;
static inline String hex32(uint32_t v){ char b[11]; snprintf(b, sizeof(b), "0x%08lX",(unsigned long)v); return String(b); }
static inline String hex64(uint64_t v){ char b[19]; snprintf(b, sizeof(b), "0x%016llX",(unsigned long long)v); return String(b); }
static inline void clearData(){ sProto="-"; sAddr=0; sCmd=0; sValue=0; sRawLen=0; }

static void snapshotFromDecoder() {
  clearData();
#if defined(IRremote_HPP) || __has_include(<IRremote.hpp>)
  auto &d = IrReceiver.decodedIRData;
  sProto = String(getProtocolString(d.protocol));
  sAddr  = d.address;
  sCmd   = d.command;
  sValue = (uint64_t)d.decodedRawData;

  if (d.rawDataPtr && d.rawDataPtr->rawlen > 0) {
    sRawLen = min<uint16_t>(d.rawDataPtr->rawlen, (uint16_t)(sizeof(sRawBuf)/sizeof(sRawBuf[0])));
    for (uint16_t i=0; i<sRawLen; ++i) sRawBuf[i] = d.rawDataPtr->rawbuf[i];
  }
#else
  decode_results r;
  if (IrReceiver.decode(&r)) {
    switch (r.decode_type) {
      case NEC:  sProto = "NEC"; break;
      case SONY: sProto = "SONY"; break;
      case RC5:  sProto = "RC5"; break;
      case RC6:  sProto = "RC6"; break;
      default:   sProto = "UNK"; break;
    }
    sValue = r.value; sAddr = 0; sCmd = 0;
    sRawLen = min<uint16_t>(r.rawlen, (uint16_t)(sizeof(sRawBuf)/sizeof(sRawBuf[0])));
    for (uint16_t i=0; i<sRawLen; ++i) sRawBuf[i] = r.rawbuf[i];
    IrReceiver.resume();
  }
#endif
}

struct Lyt {
  int W,H,L,T,R,B;
  int titleX, titleY;
  int cardX, cardY, cardW, cardH;
  int protoX, protoY, protoW;
  int chipW, chipH, chipX, chipY;
  bool chipWrapped; 
  int gridX, gridY, gridW, rowH, rowGap;
  int waveX, waveY, waveW, waveH;
};
static Lyt L;

static void computeLayout(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  L.W = tft.width();  L.H = tft.height();
  L.L = 6;            L.T = 6;
  L.R = L.W - 6;      L.B = L.H - 6;
  L.titleX = L.L + 4;
  L.titleY = L.T + 2;
  L.cardW = L.R - L.L - 12;
  L.cardX = L.L + ((L.R - L.L) - L.cardW)/2;
  L.cardY = L.T + 18;
  tft.setTextSize(1);
  tft.setTextColor(C_ACCENT, C_BG);
  String p = sProto;
  while (tft.textWidth(p) > (L.cardW - 40) && p.length() > 3) p.remove(p.length()-1);
  L.protoX = L.cardX + 10;
  L.protoY = L.cardY + 8;
  L.protoW = tft.textWidth(p);
  const char* chipTxt = sPaused ? "PAUSED" : (sState==IRState::RECEIVED ? "RECEIVED" : "LISTENING");
  int chipTxtW = tft.textWidth(chipTxt);
  L.chipH = 15;
  L.chipW = chipTxtW + 16; 
  L.chipWrapped = false;
  L.chipX = L.cardX + L.cardW - 10 - L.chipW;
  L.chipY = L.protoY - 2;
  int rightEdgeOfProto = L.protoX + L.protoW;
  if (rightEdgeOfProto + 8 > L.chipX) {
    int protoMaxW = max(0, L.chipX - L.protoX - 8);
    while (tft.textWidth(p) > protoMaxW && p.length() > 3) p.remove(p.length()-1);
    L.protoW = tft.textWidth(p);
    rightEdgeOfProto = L.protoX + L.protoW;
    if (rightEdgeOfProto + 8 > L.chipX) {
      L.chipWrapped = true;
      L.chipX = L.cardX + L.cardW - 10 - L.chipW;
      L.chipY = L.protoY + 14; 
    }
  }

  int headerBottom = max(L.protoY + 10, L.chipY + L.chipH);
  int afterHeaderY = headerBottom + 4;
  L.gridX  = L.cardX + 8;
  L.gridY  = afterHeaderY + 4;
  L.gridW  = L.cardW - 16;
  L.rowH   = 14;
  L.rowGap = 4;
  int rowsBlockH = 2*L.rowH + 1*L.rowGap + 10;
  L.cardH = (L.gridY - L.cardY) + rowsBlockH;
  int safeGap = 8;
  L.waveX = L.L + 10;
  L.waveW = L.R - L.L - 20;
  L.waveY = L.cardY + L.cardH + safeGap;
  int availableH = (L.B - safeGap) - L.waveY;
  L.waveH = max(24, min(availableH, 56));
  if (availableH < 24) {
    int need = 24 - availableH;
    int minCardH = 58; 
    if (L.cardH - need >= minCardH) {
      L.cardH -= need;
      L.waveY = L.cardY + L.cardH + safeGap;
      L.waveH = max(24, min((L.B - safeGap) - L.waveY, 56));
    } else {
      
      L.waveH = 24;
      L.waveY = min(L.waveY, L.B - safeGap - L.waveH);
    }
  }
}

static void drawStatusChip(TFT_eSPI& tft){
  uint16_t chipCol = sPaused ? C_CHIP_PAUSE
                    : (sState==IRState::RECEIVED ? C_CHIP_OK : C_CHIP_IDLE);
  const char* chipTxt = sPaused ? "PAUSED" : (sState==IRState::RECEIVED ? "RECEIVED" : "LISTENING");
  tft.fillRoundRect(L.chipX, L.chipY, L.chipW, L.chipH, 5, chipCol);
  tft.drawRoundRect(L.chipX, L.chipY, L.chipW, L.chipH, 5, C_FRAME);
  tft.setTextColor(TFT_BLACK, chipCol);
  tft.setTextDatum(CC_DATUM);
  tft.drawString(chipTxt, L.chipX + L.chipW/2, L.chipY + L.chipH/2, 1);
  tft.setTextDatum(TL_DATUM);
}

static void drawFieldRow(TFT_eSPI& tft, int x, int y, const char* label, const String& value){
  tft.fillRoundRect(x, y, L.gridW, L.rowH, 4, C_PILL_BG);
  tft.drawRoundRect(x, y, L.gridW, L.rowH, 4, C_PILL_OUT);
  tft.setTextSize(1);
  tft.setTextColor(C_SUB, C_PILL_BG);
  tft.setCursor(x+6, y+3);
  tft.print(label);
  String v = value;
  int maxW = L.gridW - 12 - 48;
  while (tft.textWidth(v) > maxW && v.length() > 3) v.remove(v.length()-1);
  tft.setTextColor(C_TEXT, C_PILL_BG);
  tft.setCursor(x + L.gridW - tft.textWidth(v) - 6, y+3);
  tft.print(v);
}

static void drawWaveGrid(TFT_eSPI& tft) {
  int mid = L.waveY + L.waveH/2;
  tft.drawFastHLine(L.waveX, mid, L.waveW, C_WAVE_GRID);
  int q = L.waveW / 4;
  for (int i=1; i<4; ++i) {
    int x = L.waveX + i*q;
    for (int yy=L.waveY; yy<L.waveY+L.waveH; yy+=2) tft.drawPixel(x, yy, C_WAVE_GRID);
  }
}

static void drawWave(TFT_eSPI& tft){
  if (L.waveH <= 0 || L.waveW <= 0) return;
  tft.drawRoundRect(L.waveX-2, L.waveY-2, L.waveW+4, L.waveH+4, 5, C_FRAME);
  tft.fillRect(L.waveX, L.waveY, L.waveW, L.waveH, C_BG);
  if (sRawLen < 2) { drawWaveGrid(tft); return; }
  uint32_t sumTicks = 0;
  for (uint16_t i=1;i<sRawLen;++i) sumTicks += sRawBuf[i];
  if (!sumTicks) { drawWaveGrid(tft); return; }
  drawWaveGrid(tft);
  float pxPerTick = float(L.waveW) / float(sumTicks);
  int   hiY = L.waveY + 2;
  int   loY = L.waveY + (L.waveH/2) + 2;
  int   hiH = max(2, (L.waveH/2 - 4));
  int   loH = max(2, (L.waveH/3 - 3));
  int   cx  = L.waveX;
  bool  mark = true;
  for (uint16_t i=1; i<sRawLen; ++i) {
    int pw = max(1, int(sRawBuf[i] * pxPerTick));
    int remaining = (L.waveX + L.waveW) - cx;
    if (remaining <= 0) break;
    int drawW = max(1, min(pw, remaining));
    int y0 = mark ? hiY : loY;
    int h  = mark ? hiH : loH;
    if (y0 + h > L.waveY + L.waveH) h = (L.waveY + L.waveH) - y0;
    tft.fillRect(cx, y0, drawW, h, mark ? C_WAVE_HI : C_WAVE_LO);
    if (cx + drawW < L.waveX + L.waveW) tft.drawFastVLine(cx + drawW - 1, y0, h, C_FRAME);
    cx += drawW;
    if (cx < L.waveX + L.waveW) cx += 1; 
    if (cx >= L.waveX + L.waveW) break;
    mark = !mark;
  }
}

static void drawFull(TFT_eSPI& tft){
  computeLayout(tft);
  tft.setTextSize(1);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.setCursor(L.titleX, L.titleY);
  tft.fillRoundRect(L.cardX, L.cardY, L.cardW, L.cardH, 10, C_BG);
  tft.drawRoundRect(L.cardX, L.cardY, L.cardW, L.cardH, 10, C_FRAME);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.setTextSize(1);
  String p = sProto;
  while (tft.textWidth(p) > (L.cardW - 40) && p.length() > 3) p.remove(p.length()-1);
  while (tft.textWidth(p) > L.protoW && p.length() > 3) p.remove(p.length()-1);
  tft.setCursor(L.protoX, L.protoY);
  tft.print(p);
  drawStatusChip(tft);
  int headerBottom = max(L.protoY + 10, L.chipY + L.chipH);
  tft.drawFastHLine(L.cardX+6, headerBottom + 4 - 4, L.cardW-12, C_FRAME); 
  int y = L.gridY;
  drawFieldRow(tft, L.gridX, y, "Cmd ",  hex32(sCmd));   y += L.rowH + L.rowGap;
  drawFieldRow(tft, L.gridX, y, "Value", hex64(sValue));
  drawWave(tft);
}

void irReadReset() {
  int rxPin = getCurrentIrRxPin();
  if (rxPin == -1) rxPin = IR_RX_PIN; // Fallback to default
  
#if defined(IRremote_HPP) || __has_include(<IRremote.hpp>)
  IrReceiver.begin(rxPin, ENABLE_LED_FEEDBACK);
#else
  IrReceiver.begin(rxPin, ENABLE_LED_FEEDBACK);
#endif
  sState = IRState::WAITING;
  sPaused = false;
  clearData();
  sNeedsFullRedraw = true;
}

void irReadDrawScreen(TFT_eSPI& tft) {
  if (!sNeedsFullRedraw) return;
  drawFull(tft);
  sNeedsFullRedraw = false;
}

void irReadHandleInput(bool a, bool b, bool c, bool& requestExit) {
  requestExit = false;

  if (a) {                    
    clearData();
    sState = IRState::WAITING;
    sNeedsFullRedraw = true;
  }
  if (b) {                    
    sPaused = !sPaused;
    
  }
  if (c) {                    
    requestExit = true;
    return;
  }

  if (!sPaused) {
#if defined(IRremote_HPP) || __has_include(<IRremote.hpp>)
    if (IrReceiver.decode()) {
      snapshotFromDecoder();
      IrReceiver.resume();
      sState = IRState::RECEIVED;
      sNeedsFullRedraw = true;
    }
#else
    decode_results r;
    if (IrReceiver.decode(&r)) {
      snapshotFromDecoder();
      IrReceiver.resume();
      sState = IRState::RECEIVED;
      sNeedsFullRedraw = true;
    }
#endif
  }
}
