#include "FindFq.h"
#include "../Settings/pin_config.h"

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
  static constexpr uint8_t ROT_TOP = 1;
#else
  static constexpr uint8_t ROT_TOP = 3;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 3) & 0x3;

extern void drawOptionsLayerBackground(TFT_eSPI& tft);
static TFT_eSPI* sTft = nullptr;
static bool sInit=false, sBgDrawn=false;
static int W=0,H=0,bx=0,by=0,bw=0,bh=0,cx=0,cy=0,cw=0;
static constexpr uint16_t COL_BG  = TFT_BLACK;
static constexpr uint16_t COL_TXT = TFT_SILVER;
static constexpr uint16_t COL_ACC = TFT_WHITE;
static volatile uint32_t sLastEdgeUs=0;
static volatile uint8_t  sLastLvl=0;
static volatile uint16_t sDurBuf[256];
static volatile uint8_t  sLvlBuf[256];
static volatile uint16_t sHead=0, sTail=0;
static uint16_t sVisDur[512];
static uint8_t  sVisLvl[512];
static uint16_t sVisLen=0;
static uint32_t sVisTotalUs=0;
static bool  sFound=false;     
static float sFoundGHz=0.0f;   
static int   sCurrentRxPin=GROOVE_RX_PIN; 
static bool   sTitleDirty=true;
static bool   sDigitsDirty=true;
static bool   sBurstDirty=true;
static String sLastTitleRight;
static String sLastDigits;

static inline bool ring_push(uint16_t dur, uint8_t lvl){
  uint16_t n = (sHead + 1) & 255;
  if (n == sTail) return false;
  sDurBuf[sHead]=dur; sLvlBuf[sHead]=lvl; sHead=n; return true;
}
static inline bool ring_pop(uint16_t& dur, uint8_t& lvl){
  if (sTail == sHead) return false;
  dur = sDurBuf[sTail]; lvl = sLvlBuf[sTail]; sTail = (sTail + 1) & 255; return true;
}

static void IRAM_ATTR isr_rx(){
  const uint32_t now   = micros();
  const uint32_t d     = now - sLastEdgeUs;
  const uint8_t  newLv = (uint8_t)digitalRead(sCurrentRxPin);
  ring_push((uint16_t)min<uint32_t>(65535, d), sLastLvl);
  sLastLvl    = newLv;
  sLastEdgeUs = now;
}

static void geometry(TFT_eSPI& tft){
  sTft = &tft;
  tft.setRotation(ROT_TOP);
  W=tft.width(); H=tft.height();
  const int marginX=6, marginTop=6, marginBottom=6;
  bx=marginX; by=marginTop; bw=W-2*marginX; bh=H-by-marginBottom;
  cx=bx;      cy=by;        cw=bw;
}
static void drawBackground(TFT_eSPI& tft){
  if (sBgDrawn) return;
  tft.setRotation(ROT_ALT);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_TOP);
  sBgDrawn = true;
}

static void drawTitle(TFT_eSPI& tft, const String& rightHint){
  if (!sTitleDirty && rightHint == sLastTitleRight) return;
  sLastTitleRight = rightHint;
  tft.setTextFont(1); tft.setTextSize(1);
  tft.setTextColor(COL_TXT, COL_BG);
  tft.fillRect(cx, cy, cw, 12, COL_BG);
  if (rightHint.length()){
    int w = tft.textWidth(rightHint);
    tft.setCursor(cx+cw-w-2, cy+1); tft.print(rightHint);
  }
  sTitleDirty=false;
}

static void drawDigits(TFT_eSPI& tft, const String& s){
  if (!sDigitsDirty && s == sLastDigits) return;
  sLastDigits = s;
  const int top = cy + (bh * 34) / 100;   
  tft.setTextFont(1);
  tft.setTextSize(4);
  const int fh = tft.fontHeight();         
  const int tw = tft.textWidth(s);
  const int x  = cx + (cw - tw)/2;
  const int underlineGap = 2;              
  const int clearW = tw + 16;
  const int clearH = fh + underlineGap + 14;
  const int clearX = max(cx, x - 8);
  const int clearY = max(cy, top - 6);
  tft.fillRect(clearX, clearY, min(clearW, cx+cw-clearX), clearH, COL_BG);
  tft.setTextColor(COL_ACC, COL_BG);
  tft.setCursor(x, top);
  tft.print(s);
  const int undY  = top + fh + underlineGap;
  const int charW = tft.textWidth("0");
  const int dotW  = tft.textWidth(".");
  int uX = x;
  for (int i=0;i<(int)s.length();++i){
    if (s[i]=='.'){ uX += dotW; continue; }
    const int segW = max(2, charW-2);
    tft.fillRect(uX, undY, segW, 2, COL_ACC); 
    uX += charW;
  }

  tft.setTextFont(1);
  tft.setTextSize(1);
  const char* unitsStr = "Units: MHz";
  const int uw = tft.textWidth(unitsStr);
  const int unitsY = undY + 8;
  tft.fillRect(cx + (cw-uw)/2 - 2, unitsY - 2, uw + 4, 12, COL_BG);
  tft.setTextColor(COL_TXT, COL_BG);
  tft.setCursor(cx + (cw-uw)/2, unitsY);
  tft.print(unitsStr);
  sDigitsDirty=false;
}

static void drawFrozenBurst(TFT_eSPI& tft){
  if (!sBurstDirty) return;
  sBurstDirty = false;
  const int waveH   = 16;
  const int bottomY = cy + bh - 4;
  const int waveTop = bottomY - waveH;
  tft.fillRect(cx, waveTop, cw, waveH, COL_BG);
  if (sVisLen == 0 || sVisTotalUs == 0) return;
  const int base = bottomY - 2;
  int x = 0;
  const int r = 4;
  for (int i=0;i<(int)sVisLen;i++){
    int px = (int)((uint64_t)sVisDur[i] * (uint64_t)cw / (uint64_t)sVisTotalUs);
    if (px < 1) px = 1;
    const int h = sVisLvl[i] ? (waveH - 6) : 6;
    int drawW = min(px, cw - x);
    if (drawW <= 0) break;
    tft.fillRoundRect(cx + x, base - h, drawW, h, r, COL_ACC);
    x += px;
    if (x >= cw) break;
  }
}

static float guessBandGHz(uint16_t medianUs){
  if (medianUs < 300)  return 0.315f;
  if (medianUs < 900)  return 0.434f;  
  if (medianUs < 1600) return 0.390f;
  if (medianUs < 4000) return 0.868f;
  return 0.915f;
}

static void harvestBurstFromRing(){
  static uint16_t curDur[512];
  static uint8_t  curLvl[512];
  static int      curLen = 0;
  const uint16_t GAP_US = 6000;
  uint16_t d; uint8_t lvl;
  bool newBurst=false;
  while (ring_pop(d,lvl)){
    if (d >= GAP_US){
      if (curLen >= 4){
        sVisLen = (uint16_t)min(curLen, 512);
        uint32_t total=0;
        for (int i=0;i<sVisLen;i++){ sVisDur[i]=curDur[i]; sVisLvl[i]=curLvl[i]; total += sVisDur[i]; }
        sVisTotalUs = total;
        newBurst = true;
      }
      curLen = 0;
    } else {
      if (curLen < 512){ curDur[curLen]=d; curLvl[curLen]=lvl; curLen++; }
    }
  }
  if (newBurst) sBurstDirty = true;
}

static bool lockFromCurrentBurst(){
  if (sVisLen < 8 || sVisTotalUs == 0) return false;
  uint16_t tmp[512]; int cnt=0;
  for (int i=0;i<(int)sVisLen && cnt<512;i++){
    uint16_t d = sVisDur[i];
    if (d>20 && d<20000) tmp[cnt++] = d;
  }
  if (cnt < 12) return false;
  for (int i=1;i<cnt;i++){
    uint16_t v = tmp[i]; int j=i-1; while (j>=0 && tmp[j] > v){ tmp[j+1]=tmp[j]; j--; } tmp[j+1]=v;
  }
  const uint16_t med = tmp[cnt/2];
  sFoundGHz = guessBandGHz(med);
  return true;
}

void findFqReset(){
  sInit=false; sBgDrawn=false;
  sFound=false; sFoundGHz=0.0f;
  sVisLen=0; sVisTotalUs=0;
  sCurrentRxPin = getCurrentRfRxPin();
  if (sCurrentRxPin == -1) sCurrentRxPin = GROOVE_RX_PIN; 
  sHead=sTail=0;
  pinMode(sCurrentRxPin, INPUT);
  sLastEdgeUs = micros();
  sLastLvl    = (uint8_t)digitalRead(sCurrentRxPin);
  attachInterrupt(digitalPinToInterrupt(sCurrentRxPin), isr_rx, CHANGE);
  sTitleDirty  = true;
  sDigitsDirty = true;
  sBurstDirty  = true;
  sLastTitleRight = "";
  sLastDigits  = "";
}

void findFqDrawScreen(TFT_eSPI& tft){
  if (!sInit){ geometry(tft); drawBackground(tft); sInit=true; }
  if (!sFound){
    harvestBurstFromRing();
    if (lockFromCurrentBurst()){
      sFound = true;
      detachInterrupt(digitalPinToInterrupt(sCurrentRxPin));
      sDigitsDirty = true;
      sBurstDirty  = true;
      sTitleDirty  = true;
    }
  }
  String disp;
  if (sFound){
    const int mhz_thou = (int)(sFoundGHz * 1000000.0f + 0.5f); 
    const int int3  = mhz_thou / 1000;   
    const int frac3 = mhz_thou % 1000;   
    char buf[16];
    sprintf(buf, "%03d.%03d", int3, frac3);
    disp = String(buf);
  } else {
    disp = "000.000";
  }
  drawDigits(tft, disp);

  if (sFound) drawFrozenBurst(tft);
}

void findFqHandleInput(bool a, bool, bool c, bool& exitReq){
  exitReq = false;
  if (a){
    sFound=false; sFoundGHz=0.0f; sVisLen=0; sVisTotalUs=0;
    sHead=sTail=0;
    sLastEdgeUs = micros();
    sLastLvl    = (uint8_t)digitalRead(sCurrentRxPin);
    attachInterrupt(digitalPinToInterrupt(sCurrentRxPin), isr_rx, CHANGE);
    sTitleDirty  = true;
    sDigitsDirty = true;
    sBurstDirty  = true;
    sLastTitleRight = "";
    sLastDigits  = "";
  }
  if (c){
    exitReq = true;
  }
}

void findFqStop(){
  detachInterrupt(digitalPinToInterrupt(sCurrentRxPin));
}
