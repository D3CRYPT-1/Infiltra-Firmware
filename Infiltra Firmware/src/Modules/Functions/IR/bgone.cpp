#include "bgone.h"
#include "UserInterface/menus/menu_submenus.h"
#include <Arduino.h>
#include <math.h> 
#include "tvbg_sender.h"
#include "../Settings/pin_config.h"
#include "ir_db/tvbg_power.h"
#include "ir_db/tvbg_volume.h"
#include "ir_db/tvbg_channel.h"
#include "ir_db/tvbg_input.h"
#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;
#ifndef IR_TX_PIN
  #warning "IR_TX_PIN not defined! Defaulting to GPIO 9. Change this to match your device."
  #define IR_TX_PIN 9
#endif

static void tvbgBeginOnConfiguredPin() {
  int txPin = getCurrentIrTxPin();
  if (txPin == -1) txPin = IR_TX_PIN;   
  pinMode(txPin, OUTPUT);
  #if defined(TVBG_SENDER_BEGIN_TAKES_PIN)
    tvbgSenderBegin(txPin);
  #elif defined(TVBG_SET_TX_PIN)
    tvbgSetTxPin(txPin);
    tvbgSenderBegin();
  #else
    tvbgSenderBegin();
  #endif
}

enum class BGoneMode : uint8_t { MENU, TV_REMOTE };
static BGoneMode mode = BGoneMode::MENU;
static const char* kItems[] = {
  "<- Back",
  "Add",
  "TV",
  "Proj",
  "LEDs",
  "AC",
  "Fan"
};
static constexpr int kCount = sizeof(kItems)/sizeof(kItems[0]);
static int  selected  = 1;    
static int  pageStart = 0;
static bool dirty     = true; 

static void iconAdd (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c) {
  int cx = x + w/2, cy = y + h/2, r = min(w,h)/3;
  t.drawLine(cx-r, cy, cx+r, cy, c);
  t.drawLine(cx, cy-r, cx, cy+r, c);
}
static void iconTV  (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  t.drawRoundRect(x, y, w, h-4, 3, c);
  t.drawFastHLine(x + w/4, y + h-3, w/2, c);
}
static void iconProj(TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int r = min(w, h) / 4;
  t.drawRoundRect(x, y + 2, w, h-4, 3, c);
  t.drawCircle(x + w/2, y + h/2, r, c);
  t.drawCircle(x + w - r - 5, y + h/2, r/2, c);
}
static void iconLED (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int pad = 6;
  t.drawRoundRect(x, y, w, h, 3, c);
  for (int i=0;i<4;i++) {
    int bx = x + pad + i*( (w-2*pad)/4 );
    t.drawRect(bx, y + h/2 - 4, 8, 8, c);
  }
}
static void iconAC  (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int cx = x + w/2, cy = y + h/2, r = min(w,h)/3;
  t.drawLine(cx-r, cy, cx+r, cy, c);
  t.drawLine(cx, cy-r, cx, cy+r, c);
  t.drawLine(cx-int(r*0.7), cy-int(r*0.7), cx+int(r*0.7), cy+int(r*0.7), c);
  t.drawLine(cx-int(r*0.7), cy+int(r*0.7), cx+int(r*0.7), cy-int(r*0.7), c);
}
static void iconFan (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int cx = x + w/2, cy = y + h/2, r = min(w,h)/4;
  t.drawCircle(cx, cy, 2, c);
  t.drawCircle(cx + r, cy, r, c);
  t.drawCircle(cx - r/2, cy + int(0.866*r), r, c);
  t.drawCircle(cx - r/2, cy - int(0.866*r), r, c);
}

static void drawMenuGrid(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  const int cols   = 2;
  const int rows   = 2;
  const int tileW  = 100;
  const int tileH  = 38;
  const int gapX   = 12;
  const int gapY   = 12;
  const int startX = 14;
  const int startY = 28;
  const int perPage = cols * rows;
  auto drawTile = [&](int idx, int x, int y) {
    bool sel = (idx == selected);
    uint16_t frame = sel ? TFT_WHITE : TFT_DARKGREY;
    uint16_t label = sel ? TFT_WHITE : TFT_SILVER;
    tft.drawRoundRect(x, y, tileW, tileH, 6, frame);
    if (sel) tft.drawRoundRect(x-1, y-1, tileW+2, tileH+2, 7, frame);
    int iconX = x + 8, iconY = y + 6, iconW = 22, iconH = tileH - 12;
    switch (idx) {
      case 0: break; 
      case 1: iconAdd (tft, iconX, iconY, iconW, iconH, frame); break;
      case 2: iconTV  (tft, iconX, iconY, iconW, iconH, frame); break;
      case 3: iconProj(tft, iconX, iconY, iconW, iconH, frame); break;
      case 4: iconLED (tft, iconX, iconY, iconW, iconH, frame); break;
      case 5: iconAC  (tft, iconX, iconY, iconW, iconH, frame); break;
      case 6: iconFan (tft, iconX, iconY, iconW, iconH, frame); break;
    }
    tft.setTextColor(label, TFT_BLACK);
    tft.setTextSize(2);
    String name = kItems[idx];
    int lw = tft.textWidth(name);
    tft.setCursor(x + tileW - lw - 8, y + (tileH - 16)/2);
    tft.print(name);
  };
  const int end = min(pageStart + perPage, kCount);
  int visibleIdx = 0;
  for (int i = pageStart; i < end; ++i, ++visibleIdx) {
    int r = visibleIdx / cols;
    int c = visibleIdx % cols;
    int x = startX + c*(tileW + gapX);
    int y = startY + r*(tileH + gapY);
    drawTile(i, x, y);
  }
}
static void ensureMenuSelectionVisible(){
  const int perPage = 4;
  if (selected < pageStart || selected >= pageStart + perPage) {
    pageStart = (selected / perPage) * perPage;
    dirty = true;
  }
}

enum TVIdx {
  TV_POWER = 0,
  TV_INPUT,
  TV_VOL_UP, TV_VOL_DOWN,
  TV_CH_UP,  TV_CH_DOWN,
  TV_COUNT
};
static int tvSel = TV_POWER;
static void drawTVRemote(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  const int bezelX = 18;
  const int bezelY = 30;
  const int bezelW = 204;
  const int bezelH = 100;
  tft.drawRoundRect(bezelX, bezelY, bezelW, bezelH, 8, TFT_DARKGREY);
  const int colGap = 16;
  const int keyW   = 84;
  const int keyH   = 22;
  const int rowGap = 8;
  const int leftX  = bezelX + (bezelW - (keyW*2 + colGap))/2;
  const int rightX = leftX + keyW + colGap;
  const int contentH = keyH*3 + rowGap*2;
  const int topPad   = (bezelH - contentH)/2;
  int rowY = bezelY + topPad;
  auto key = [&](int idx, int x, int y, const char* label){
    bool sel = (tvSel == idx);
    uint16_t frame = sel ? TFT_WHITE : TFT_DARKGREY;
    uint16_t fill  = TFT_BLACK;
    uint16_t text  = sel ? TFT_WHITE : TFT_SILVER;
    tft.fillRoundRect(x, y, keyW, keyH, 6, fill);
    tft.drawRoundRect(x, y, keyW, keyH, 6, frame);
    if (sel) tft.drawRoundRect(x-1, y-1, keyW+2, keyH+2, 7, frame);
    tft.setTextColor(text, fill);
    tft.setTextSize(2);
    int lw = tft.textWidth(label);
    tft.setCursor(x + (keyW - lw)/2, y + (keyH - 16)/2);
    tft.print(label);
  };

  key(TV_POWER, leftX,  rowY, "Power");
  key(TV_INPUT, rightX, rowY, "Input");
  rowY += keyH + rowGap;
  key(TV_VOL_UP, leftX,  rowY, "Vol+");
  key(TV_CH_UP,  rightX, rowY, "Ch+");
  rowY += keyH + rowGap;
  key(TV_VOL_DOWN, leftX,  rowY, "Vol-");
  key(TV_CH_DOWN,  rightX, rowY, "Ch-");
}

static bool        overlayOn      = false;
static bool        overlayDirty   = false;
static uint32_t    lastOverlayDrawMs = 0;
static const uint16_t OVERLAY_MIN_REDRAW_MS = 120; 
static uint32_t    overlayDoneStamp = 0;
static const uint16_t OVERLAY_AUTO_HIDE_MS = 700;  
static const int IR_MAX_LINES = 40;
static String irLines[IR_MAX_LINES];
static int    irCount = 0;
static int n_cx=0, n_cy=0, n_cw=0;        
static int n_bx=0, n_by=0, n_bw=0, n_bh=0; 
static unsigned    jobTotal = 0;
static unsigned    jobDone  = 0;
enum class JobKind : uint8_t { NONE, POWER, INPUT_SRC, VOL_UP, VOL_DOWN, CH_UP, CH_DOWN };
static JobKind    jobKind    = JobKind::NONE;
static bool       jobPending = false;
enum class NotifState : uint8_t { INIT, SENDING, SUCCESS };
static NotifState notifState = NotifState::INIT;
static const char SPIN[4] = {'|','/','-','\\'};
static uint8_t    spinIdx  = 0;

static const char* jobPathFor(JobKind k){
  switch (k){
    case JobKind::POWER:     return "/ir/tv/power";
    case JobKind::INPUT_SRC: return "/ir/tv/input";
    case JobKind::VOL_UP:    return "/ir/tv/vol_up";
    case JobKind::VOL_DOWN:  return "/ir/tv/vol_down";
    case JobKind::CH_UP:     return "/ir/tv/ch_up";
    case JobKind::CH_DOWN:   return "/ir/tv/ch_down";
    default:                 return "/ir/tv";
  }
}

static void irAddLineWrapped(TFT_eSPI& tft, const String& line){
  int maxW = n_cw;
  if (maxW <= 0){
    if (irCount < IR_MAX_LINES) irLines[irCount++] = line;
    else { for (int k=1;k<IR_MAX_LINES;k++) irLines[k-1]=irLines[k]; irLines[IR_MAX_LINES-1]=line; }
    overlayDirty = true; return;
  }

  tft.setTextSize(1);
  int n = line.length(), start = 0;
  while (start < n) {
    int bestEnd = start, lastSpace = -1;
    String seg;
    for (int i = start; i < n; ++i) {
      char c = line.charAt(i);
      String trial = seg + c;
      if (tft.textWidth(trial) > maxW) break;
      seg += c; bestEnd = i + 1; if (c==' ') lastSpace = i;
    }
    if (bestEnd < n && lastSpace >= start) { seg = line.substring(start, lastSpace); bestEnd = lastSpace + 1; }
    if (seg.length() == 0 && bestEnd < n) { seg = line.substring(start, start+1); bestEnd = start + 1; }

    if (seg.length()){
      if (irCount < IR_MAX_LINES) irLines[irCount++] = seg;
      else { for (int k=1;k<IR_MAX_LINES;k++) irLines[k-1]=irLines[k]; irLines[IR_MAX_LINES-1]=seg; }
    }
    start = bestEnd;
  }
  overlayDirty = true;
}

static void drawArcPoly(TFT_eSPI& tft, int cx,int cy,int r,int a0,int a1,uint16_t col){
  float step = 12.0f;
  for (float a=a0;a<a1;a+=step){
    float r1=a*0.0174533f, r2=(a+step)*0.0174533f;
    int x1=cx+int(cos(r1)*r), y1=cy+int(sin(r1)*r);
    int x2=cx+int(cos(r2)*r), y2=cy+int(sin(r2)*r);
    tft.drawLine(x1,y1,x2,y2,col);
  }
}
static void drawIRGlyph(TFT_eSPI& tft, int x, int y, uint16_t col){
  tft.drawLine(x, y+6, x+6, y+6, col);       
  tft.drawLine(x+3, y+2, x+3, y+6, col);     
  tft.drawCircle(x+8, y+5, 2, col);          
  drawArcPoly(tft, x+8, y+5, 6, 300, 20, col);
  drawArcPoly(tft, x+8, y+5, 9, 300, 20, col);
}

static void drawStatusPill(TFT_eSPI& tft, int rightX, int y){
  const char* txt   = (notifState==NotifState::SUCCESS) ? "SUCCESS"
                    : (notifState==NotifState::INIT)    ? "INITIALIZING"
                                                        : "SENDING";
  uint16_t fillCol  = (notifState==NotifState::SUCCESS) ? 0x07E0
                    : (notifState==NotifState::INIT)    ? 0x4208
                                                        : 0x39C7;
  uint16_t fgCol    = TFT_BLACK;
  int tw = tft.textWidth(txt) + 10;
  int th = 12;
  int x  = rightX - tw;
  tft.fillRoundRect(x, y, tw, th, 6, fillCol);
  tft.drawRoundRect(x, y, tw, th, 6, TFT_BLACK);
  tft.setTextColor(fgCol, fillCol);
  tft.setCursor(x+5, y+2);
  tft.print(txt);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static void drawMiniTerminal(TFT_eSPI& tft, const char* dirPath){
  int W = tft.width();
  int H = tft.height();
  int notifW = min(196, W - 24);
  int headerH = 18;         
  int bodyH   = 48;         
  int notifH  = headerH + bodyH;
  int notifX = (W - notifW) / 2;
  int notifY = (H - notifH) / 2;
  n_bx = notifX; n_by = notifY; n_bw = notifW; n_bh = notifH;
  n_cx = notifX + 8; n_cy = notifY + headerH + 2; n_cw = notifW - 16;
  tft.fillRoundRect(n_bx+2, n_by+2, n_bw, n_bh, 8, 0x0842);
  tft.fillRoundRect(n_bx, n_by, n_bw, n_bh, 8, TFT_BLACK);
  tft.drawRoundRect(n_bx, n_by, n_bw, n_bh, 8, TFT_DARKGREY);
  tft.fillRoundRect(n_bx+1, n_by+1, n_bw-2, headerH, 7, TFT_DARKGREY);
  tft.drawRoundRect(n_bx+1, n_by+1, n_bw-2, headerH, 7, TFT_BLACK);
  uint16_t hfg = TFT_WHITE;
  drawIRGlyph(tft, n_bx + 6, n_by + 4, hfg);
  char spin = SPIN[(spinIdx++) & 3];
  tft.setTextColor(hfg, TFT_DARKGREY);
  tft.setTextSize(1);
  tft.setCursor(n_bx + 22, n_by + 4);
  String path = dirPath ? String(dirPath) : String("/ir/tv");
  String head = String(spin) + " infiltra — " + path;
  while (tft.textWidth(head) > (n_bw - 90) && path.length() > 3) { 
    path.remove(path.length()-1);
    head = String(spin) + " infiltra — " + path;
  }
  tft.print(head);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawStatusPill(tft, n_bx + n_bw - 6, n_by + 3);
  int barX = n_bx + 6;
  int barY = n_by + headerH + 4;
  int barW = n_bw - 12;
  int barH = 6;
  tft.drawRect(barX, barY, barW, barH, TFT_SILVER);
  uint32_t fillW = (jobTotal > 0) ? ((jobDone * (uint32_t)(barW-2)) / jobTotal) : 0;
  if ((int)fillW > 0) tft.fillRect(barX+1, barY+1, (int)fillW, barH-2, TFT_WHITE);
  const int lineH = 12;
  const int maxVis = 4;
  int total = irCount;
  int start = (total > maxVis) ? (total - maxVis) : 0;
  int textTop = n_cy + 6;
  tft.fillRect(n_cx, textTop, n_cw, maxVis * lineH, TFT_BLACK);
  tft.setTextColor(TFT_SILVER, TFT_BLACK);
  tft.setTextSize(1);
  int y = textTop;
  for (int i=0; i<maxVis && (start + i) < total; ++i) {
    tft.setCursor(n_cx, y);
    tft.print(irLines[start + i]);
    y += lineH;
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  lastOverlayDrawMs = millis();
  overlayDirty = false;
}

enum class NotifMsg : uint8_t { INIT, SEND, DONE };
static void logLine(TFT_eSPI& tft, NotifMsg kind, const char* a=nullptr, const char* b=nullptr){
  switch (kind){
    case NotifMsg::INIT: irAddLineWrapped(tft, String("[…] initializing")); break;
    case NotifMsg::SEND: irAddLineWrapped(tft, String("$ send ") + a + " " + b); break;
    case NotifMsg::DONE: irAddLineWrapped(tft, String("[✓] success")); break;
  }
}

template<typename T>
static void runSendList(TFT_eSPI& tft, const T* list, size_t n, JobKind kind){
  overlayOn    = true;
  overlayDirty = true;
  notifState   = NotifState::INIT;
  jobTotal     = (unsigned)n;
  jobDone      = 0;
  irCount      = 0;
  const char* dirPath = jobPathFor(kind);
  drawMiniTerminal(tft, dirPath);
  tvbgBeginOnConfiguredPin();    
  logLine(tft, NotifMsg::INIT);
  drawMiniTerminal(tft, dirPath);
  notifState = NotifState::SENDING;
  for (size_t i=0;i<n;i++){
    const auto& tok = list[i];
    logLine(tft, NotifMsg::SEND, tok.brand, tok.label);
    uint32_t now = millis();
    if (overlayDirty && (now - lastOverlayDrawMs >= OVERLAY_MIN_REDRAW_MS)){
      drawMiniTerminal(tft, dirPath);
    }
    tvbgSend(tok);         
    jobDone++;
    overlayDirty = true;
    now = millis();
    if (overlayDirty && (now - lastOverlayDrawMs >= OVERLAY_MIN_REDRAW_MS)){
      drawMiniTerminal(tft, dirPath);
    }
  }
  tvbgSenderEnd();
  notifState = NotifState::SUCCESS;
  logLine(tft, NotifMsg::DONE);
  drawMiniTerminal(tft, dirPath);   
  overlayDoneStamp = millis();      
  (void)dirPath;
}

static void runPendingJob(TFT_eSPI& tft){
  if (!jobPending || jobKind == JobKind::NONE) return;
  switch (jobKind){
    case JobKind::POWER:
      runSendList(tft, TVBG_POWER_LIST, TVBG_POWER_COUNT, JobKind::POWER);
      break;
    case JobKind::INPUT_SRC:
      runSendList(tft, TVBG_INPUT_LIST, TVBG_INPUT_COUNT, JobKind::INPUT_SRC);
      break;
    case JobKind::VOL_UP:
      runSendList(tft, TVBG_VOL_UP_LIST, TVBG_VOL_UP_COUNT, JobKind::VOL_UP);
      break;
    case JobKind::VOL_DOWN:
      runSendList(tft, TVBG_VOL_DOWN_LIST, TVBG_VOL_DOWN_COUNT, JobKind::VOL_DOWN);
      break;
    case JobKind::CH_UP:
      runSendList(tft, TVBG_CH_UP_LIST, TVBG_CH_UP_COUNT, JobKind::CH_UP);
      break;
    case JobKind::CH_DOWN:
      runSendList(tft, TVBG_CH_DOWN_LIST, TVBG_CH_DOWN_COUNT, JobKind::CH_DOWN);
      break;
    default: break;
  }
  jobPending = false;
}

void bgoneReset(){
  mode = BGoneMode::MENU;
  selected = 1;
  pageStart = 0;
  tvSel = TV_POWER;
  dirty = true;
  overlayOn = false;
  overlayDirty = false;
  irCount = 0;
  jobTotal = 0;
  jobDone = 0;
  jobKind = JobKind::NONE;
  jobPending = false;
  lastOverlayDrawMs = 0;
  overlayDoneStamp = 0;
  notifState = NotifState::INIT;
  spinIdx = 0;
}

static void renderIfDirty(TFT_eSPI& tft){
  if (!dirty) return;
  if (mode == BGoneMode::MENU) drawMenuGrid(tft);
  else drawTVRemote(tft);
  dirty = false;
}

void bgoneDrawScreen(TFT_eSPI& tft){
  renderIfDirty(tft);
  if (jobPending) {
    runPendingJob(tft);
  }

  if (overlayOn && overlayDoneStamp){
    if (millis() - overlayDoneStamp >= OVERLAY_AUTO_HIDE_MS){
      overlayOn = false;
      overlayDoneStamp = 0;
      overlayDirty = false;
      dirty = true; 
    }
  }

  if (overlayOn && overlayDirty){
    if (millis() - lastOverlayDrawMs >= OVERLAY_MIN_REDRAW_MS){
      drawMiniTerminal(tft, jobPathFor(jobKind));
    }
  }
}

void bgoneHandleInput(bool aPressed,bool bPressed,bool cPressed,bool& exitRequested){
  exitRequested = false;  
  if (overlayOn && cPressed){
    overlayOn = false;
    overlayDirty = false;
    overlayDoneStamp = 0;
    dirty = true;
    return;
  }

  if (mode == BGoneMode::MENU){
    if (bPressed){ selected = (selected+1)%kCount; ensureMenuSelectionVisible(); dirty=true; }
    if (cPressed){ if (selected==0){exitRequested=true;return;} selected=(selected-1+kCount)%kCount; ensureMenuSelectionVisible(); dirty=true; }
    if (aPressed){
      if (selected==0){ exitRequested = true; return; }
      if (selected==2){ mode = BGoneMode::TV_REMOTE; tvSel = TV_POWER; dirty = true; return; }
      
    }
    return;
  }

  if (mode == BGoneMode::TV_REMOTE){
    if (bPressed){ tvSel = (tvSel + 1) % TV_COUNT; dirty = true; }
    if (cPressed){ mode = BGoneMode::MENU; dirty = true; return; }
    if (aPressed){
      switch (tvSel){
        case TV_POWER:    jobKind = JobKind::POWER;     break;
        case TV_INPUT:    jobKind = JobKind::INPUT_SRC; break; 
        case TV_VOL_UP:   jobKind = JobKind::VOL_UP;    break;
        case TV_VOL_DOWN: jobKind = JobKind::VOL_DOWN;  break;
        case TV_CH_UP:    jobKind = JobKind::CH_UP;     break;
        case TV_CH_DOWN:  jobKind = JobKind::CH_DOWN;   break;
        default:          jobKind = JobKind::NONE;      break;
      }
      if (jobKind != JobKind::NONE){
        jobPending    = true;
        overlayOn     = true;
        overlayDirty  = true;
        overlayDoneStamp = 0;
        irCount = 0; jobTotal = 0; jobDone = 0;
        notifState = NotifState::INIT;
        dirty = true;   
      }
    }
  }
}
