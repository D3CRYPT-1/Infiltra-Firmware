#include "stopwatch.h"
#include "UserInterface/menus/menu_submenus.h"

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4; 
#else
  static constexpr uint8_t ROT_TOP = 2; 
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;
static unsigned long sStartMs       = 0;
static unsigned long sPausedElapsed = 0;   
static bool          sRunning       = false;
static unsigned long sLastDrawMs    = 0;
static const uint16_t DRAW_EVERY_MS = 40;

void resetStopwatch() {
  sStartMs        = millis();
  sPausedElapsed  = 0;
  sRunning        = false;
  sLastDrawMs     = 0;
}

static void fmtTime(char* out, size_t n, unsigned long ms){
  unsigned long cs = (ms / 10) % 100;      
  unsigned long s  = (ms / 1000) % 60;
  unsigned long m  = (ms / 60000);
  snprintf(out, n, "%02lu:%02lu.%02lu", m, s, cs);
}

static unsigned long elapsedNow(){
  return sRunning ? (millis() - sStartMs) : sPausedElapsed;
}

static void drawPlayButton(TFT_eSPI& tft, int x, int y, int w, int h, bool active) {
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);
  tft.fillRoundRect(x, y, w, h, 8, TFT_BLACK);
  int cx = x + w/2, cy = y + h/2;
  int half = h/3;
  tft.fillTriangle(cx - half, cy - half, cx - half, cy + half, cx + half, cy, TFT_WHITE);
  if (active) tft.drawRoundRect(x-2, y-2, w+4, h+4, 10, TFT_WHITE);
}

static void drawPauseButton(TFT_eSPI& tft, int x, int y, int w, int h, bool active) {
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);
  tft.fillRoundRect(x, y, w, h, 8, TFT_BLACK);
  int barW = w / 6;
  int barH = h * 3/5;
  int barY = y + (h - barH)/2;
  tft.fillRect(x + w/3 - barW, barY, barW, barH, TFT_WHITE);
  tft.fillRect(x + 2*w/3,      barY, barW, barH, TFT_WHITE);
  if (active) tft.drawRoundRect(x-2, y-2, w+4, h+4, 10, TFT_WHITE);
}

void drawStopwatchScreen(TFT_eSPI& tft) {
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  tft.fillRoundRect(6, 18, tft.width()-12, tft.height()-26, 6, TFT_BLACK);
  tft.drawRoundRect(6, 18, tft.width()-12, tft.height()-26, 6, TFT_DARKGREY);
  tft.setTextFont(1); tft.setTextSize(1);
  tft.setTextColor(TFT_SILVER, TFT_BLACK);
  tft.setCursor(12, 22); tft.print("Stopwatch");
  tft.drawLine(10, 32, tft.width()-10, 32, TFT_DARKGREY);
  char buf[16]; fmtTime(buf, sizeof(buf), elapsedNow());
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int w = tft.textWidth(buf);
  int x = (tft.width() - w) / 2;
  int y = 46;
  tft.setCursor(x, y); tft.print(buf);
  int btnW = 48, btnH = 48;
  int btnX = (tft.width() - btnW) / 2;
  int btnY = y + 34;
  if (sRunning) drawPauseButton(tft, btnX, btnY, btnW, btnH, true);
  else          drawPlayButton (tft, btnX, btnY, btnW, btnH, true);
  tft.setTextSize(1); tft.setTextColor(TFT_SILVER, TFT_BLACK);
  tft.setCursor(12, tft.height()-12);
}

void drawStopwatchTimeOnly(TFT_eSPI& tft) {
  unsigned long now = millis();
  if (now - sLastDrawMs < DRAW_EVERY_MS) return;
  sLastDrawMs = now;
  tft.setRotation(ROT_ALT);
  char buf[16]; fmtTime(buf, sizeof(buf), elapsedNow());
  tft.setTextSize(3);
  int w = tft.textWidth(buf);
  int x = (tft.width() - w) / 2;
  int y = 46;
  tft.fillRect(x - 6, y - 4, w + 12, 36, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(x, y); tft.print(buf);
}

void stopwatchHandleInput(bool a, bool b, bool c, bool& exitReq){
  exitReq = false;
  if (c) { exitReq = true; return; }
  if (a) {
    if (sRunning) {
      sRunning       = false;
      sPausedElapsed = millis() - sStartMs;
    } else {
      sRunning       = true;
      sStartMs       = millis() - sPausedElapsed;
    }
  }
  if (b) {
    sPausedElapsed = 0;
    if (sRunning) sStartMs = millis();
  }
}

void handleStopwatchInput(bool btnA){
  bool dummyExit=false;
  stopwatchHandleInput(btnA,false,false, dummyExit);
}

bool isStopwatchRunning(){ return sRunning; }
