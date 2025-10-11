#include "Passlock.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include <string.h>
#include "UserInterface/menus/menu_submenus.h" 
extern TFT_eSPI tft;
#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;   
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3; 
extern "C" int passlock_button_read_blocking(); 
enum { PL_BTN_A = 1, PL_BTN_B = 2, PL_BTN_C = 3 };

#if __has_include(<Preferences.h>)
  #include <Preferences.h>
  static Preferences prefs;
  #define PL_HAS_PREFS 1
#else
  #include <EEPROM.h>
  #define PL_HAS_PREFS 0
#endif
#ifndef TFT_DARKGREY
#define TFT_DARKGREY 0x7BEF
#endif

static int W=0, H=0;
static int bx=0, by=0, bw=0, bh=0; 
static int cx=0, cy=0, cw=0;       
static void drawStaticFrame_NoInnerFill() {
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  W = tft.width(); H = tft.height();
  const int marginX = 10;
  const int marginTop = 16;
  const int marginBottom = 8;
  bx = marginX;
  by = marginTop + 12;
  bw = W - marginX*2;
  bh = H - by - marginBottom;
  tft.setTextSize(1);
  tft.drawRoundRect(bx, by, bw, bh, 8, TFT_DARKGREY);
  tft.drawRoundRect(bx+1, by+1, bw-2, bh-2, 7, TFT_BLACK);
  cx = bx + 10;
  cy = by + 8;
  cw = bw - 20;
}

static void drawStaticFrame() {
  drawStaticFrame_NoInnerFill();
  tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
}

static void normalizeRotationForApp() {
  tft.setRotation(ROT_TOP);
}

static const int MAX_TERM_LINES = 96;
static String sTermLines[MAX_TERM_LINES];
static int    sTermCount = 0;
static int    sTermChan  = 37;
static int    sTermPhase = 0;
static int    sTermMaxW  = 0;
static void termClear() { sTermCount = 0; }
static void addTermLineWrapped(const String& line) {
  
  if (sTermMaxW <= 0) {
    if (sTermCount < MAX_TERM_LINES) sTermLines[sTermCount++] = line;
    return;
  }
  int pxBudget = sTermMaxW;
  int chBudget = max(1, pxBudget / 6);
  int n = line.length(), start = 0;
  while (start < n) {
    int len = min(chBudget, n - start);
    String seg = line.substring(start, start + len);
    if (sTermCount < MAX_TERM_LINES) {
      sTermLines[sTermCount++] = seg;
    } else {
      for (int k=1; k<MAX_TERM_LINES; ++k) sTermLines[k-1] = sTermLines[k];
      sTermLines[MAX_TERM_LINES-1] = seg;
    }
    start += len;
  }
}

static void termDrawHeader(const char* title) {
  tft.setTextSize(1);
  const int headerH = 16;
  tft.fillRoundRect(cx, cy, cw, headerH, 3, TFT_DARKGREY);
  tft.drawRoundRect (cx, cy, cw, headerH, 3, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(cx + 4, cy + 3);
  tft.print(title ? title : "infiltra terminal — /secure");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static void termDrawBody() {
  tft.setTextSize(1);
  const int headerH = 16;
  const int termTop = cy + headerH + 2;
  const int lineH   = 12;
  const int availH = (by + bh - 8) - termTop;
  const int maxVis = max(3, availH / lineH);
  const int total  = sTermCount;
  const int start  = (total > maxVis) ? (total - maxVis) : 0;
  tft.fillRect(cx, termTop, cw, maxVis * lineH, TFT_BLACK);
  tft.setTextColor(TFT_SILVER, TFT_BLACK);
  int y = termTop;
  for (int i=0; i<maxVis && (start + i) < total; ++i) {
    tft.setCursor(cx, y);
    tft.print(sTermLines[start + i]);
    y += lineH;
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static void termDraw(const char* headerTitle) {
  termDrawHeader(headerTitle);
  termDrawBody();
}

static void termTick_likeScan(const char* headerTitle) {
  String chStr = String(sTermChan);
  sTermChan = (sTermChan == 39) ? 37 : (sTermChan + 1);
  const char sp[4] = {'|','/','-','\\'};
  static int spIdx = 0;
  char spc = sp[(spIdx++) & 3];
  String msg;
  switch ((sTermPhase++) % 4) {
    case 0: msg = "["+String(spc)+"] root@infiltra:/# apply --stage=prepare"; break;
    case 1: msg = "["+String(spc)+"] root@infiltra:/# write --secure-config"; break;
    case 2: msg = "["+String(spc)+"] root@infiltra:/# verify --checksum"; break;
    default: msg = "["+String(spc)+"] root@infiltra:/# finalize"; break;
  }
  addTermLineWrapped(msg);
  termDraw(headerTitle);
}

using namespace Core;
static const char* NS         = "passlock";
static const char* KEY_EN     = "enabled";
static const char* KEY_TYPE   = "type";
static const char* KEY_NUM    = "num4";
static const char* KEY_PAT    = "pattern";
static const size_t PAT_MAX   = 16;
static bool         g_enabled = false;
static PasslockType g_type    = PasslockType::None;
static char         g_numeric[5] = {0};     
static std::vector<uint8_t> g_pattern;      
static void loadState() {
#if PL_HAS_PREFS
  prefs.begin(NS,true);
  g_enabled = prefs.getBool(KEY_EN, false);
  g_type    = static_cast<PasslockType>(prefs.getUChar(KEY_TYPE, 0));
  String n  = prefs.getString(KEY_NUM, "");
  if (n.length() == 4) { memcpy(g_numeric, n.c_str(), 4); g_numeric[4] = 0; }
  else { memset(g_numeric, 0, sizeof(g_numeric)); }
  size_t plen = prefs.getBytesLength(KEY_PAT);
  g_pattern.clear();
  if (plen && plen <= PAT_MAX) {
    g_pattern.resize(plen);
    prefs.getBytes(KEY_PAT, g_pattern.data(), plen);
  }
  prefs.end();
#else
  EEPROM.begin(256);
  EEPROM.end();
#endif
}

static void saveState() {
#if PL_HAS_PREFS
  prefs.begin(NS,false);
  prefs.putBool(KEY_EN, g_enabled);
  prefs.putUChar(KEY_TYPE, static_cast<uint8_t>(g_type));
  prefs.putString(KEY_NUM, g_numeric);
  if (!g_pattern.empty()) prefs.putBytes(KEY_PAT, g_pattern.data(), g_pattern.size());
  else prefs.remove(KEY_PAT);
  prefs.end();
#endif
}

static inline int read_button_blocking() {
  return passlock_button_read_blocking(); 
}

static void drawLockScaffold(const char* title) {
  drawStaticFrame(); 
  tft.setTextSize(1);
  const int headerH = 16;
  tft.fillRoundRect(cx, cy, cw, headerH, 3, TFT_DARKGREY);
  tft.drawRoundRect (cx, cy, cw, headerH, 3, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(cx + 4, cy + 3);
  tft.print(title ? title : "Enter Passcode");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static void drawPlaceholders(int filledCount) {
  const int y = cy + 26;
  const int slots = 4;
  const int slotW = 16;  
  const int gap   = (cw - slots*slotW) / (slots + 1);
  int x = cx + gap;
  tft.setTextSize(2);
  for (int i=0; i<slots; ++i) {
    bool filled = (i < filledCount);
    tft.setCursor(x, y);
    if (filled) {
      tft.print("\x95"); 
    } else {
      tft.print("_");
    }
    x += slotW + gap;
  }
  tft.setTextSize(1);
}

static void drawNiceNumberRow(int curDigit) {
  const int y = cy + 26 + 30;
  const int count = 10;
  int centers[10];
  for (int i=0;i<count;++i) {
    int left   = cx + 8;
    int right  = cx + cw - 8;
    centers[i] = left + ((right - left) * i) / (count - 1);
  }
  tft.setTextSize(2);
  for (int d=0; d<=9; ++d) {
    int tw = 12; 
    int tx = centers[d] - tw/2;
    int ty = y;
    if (d == curDigit) {
      int pillW = 18, pillH = 20;
      tft.fillRoundRect(centers[d]-pillW/2, ty-2, pillW, pillH, 5, TFT_WHITE);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
    } else {
      tft.setTextColor(TFT_SILVER, TFT_BLACK);
    }
    tft.setCursor(tx, ty);
    tft.print(d);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  const int hintY = y + 26;
  tft.setCursor(cx, hintY);
  tft.setTextColor(TFT_SILVER, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static void drawWrongBanner() {
  const int headerH = 16;
  const int y = cy + headerH + 2;
  tft.fillRect(cx, y, cw, 14, TFT_BLACK);
  tft.fillRoundRect(cx, y, cw, 14, 3, TFT_RED);
  tft.drawRoundRect (cx, y, cw, 14, 3, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setCursor(cx + 4, y + 2);
  tft.print("Wrong passcode");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  delay(650);
  tft.fillRect(cx, y, cw, 14, TFT_BLACK);
}

static bool unlockNumericFlow(const char expect4[5]) {
  int entered = 0;
  int curDigit = 0;
  char buf[5] = {0};
  while (true) {
    drawLockScaffold("Enter Passcode");
    drawPlaceholders(entered);
    drawNiceNumberRow(curDigit);
    int btn = read_button_blocking();
    if      (btn == PL_BTN_B) curDigit = (curDigit + 1) % 10; 
    else if (btn == PL_BTN_C) curDigit = (curDigit + 9) % 10; 
    else if (btn == PL_BTN_A) {
      buf[entered++] = char('0' + curDigit);
      if (entered == 4) {
        bool ok = (memcmp(buf, expect4, 4) == 0);
        if (ok) return true;
        drawWrongBanner();
        entered = 0;
        memset(buf, 0, sizeof(buf));
      }
    }
    delay(90);
  }
}

static void drawTitleAndTerminal(const char* title) {
  drawStaticFrame(); 
  termClear();
  sTermPhase = 0; sTermChan = 37; sTermMaxW = cw;
  termDraw(title);
}

static void drawNumericOverlaySetup(int pos, int curDigit) {
  const int headerH = 16;
  const int lineH   = 12;
  const int infoY   = cy + headerH + 2;
  const int digitsY = infoY + lineH + 2;
  tft.fillRect(cx, infoY, cw, lineH, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(cx + 2, infoY + 1);
  char hdr[64];
  snprintf(hdr, sizeof(hdr), "Enter digit %d/4   (B/C, A=Select)", pos+1); 
  tft.print(hdr);
  tft.fillRect(cx, digitsY, cw, 26, TFT_BLACK);
  int centers[10];
  for (int i=0;i<10;++i) {
    int left = cx + 8, right = cx + cw - 8;
    centers[i] = left + ((right - left) * i) / 9;
  }

  tft.setTextSize(2);
  for (int d=0; d<=9; ++d) {
    int tw = 12;
    int tx = centers[d] - tw/2;
    int ty = digitsY + 2;
    if (d == curDigit) {
      int pillW = 18, pillH = 20;
      tft.fillRoundRect(centers[d]-pillW/2, ty-2, pillW, pillH, 5, TFT_WHITE);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
    } else {
      tft.setTextColor(TFT_SILVER, TFT_BLACK);
    }
    tft.setCursor(tx, ty);
    tft.print(d);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
}

static bool uiGetNumeric(char outDigits[5]) {
  memset(outDigits, 0, 5);
  int pos = 0, cur = 0;
  drawTitleAndTerminal("infiltra terminal — /secure");
  while (pos < 4) {
    drawNumericOverlaySetup(pos, cur);
    int btn = read_button_blocking();
    if      (btn == PL_BTN_B) cur = (cur + 1) % 10; 
    else if (btn == PL_BTN_C) cur = (cur + 9) % 10; 
    else if (btn == PL_BTN_A) { outDigits[pos++] = char('0' + cur); cur = 0; }
    delay(90);
  }
  outDigits[4] = 0;
  return true;
}

static void drawPatternPrompt(size_t count) {
  const int headerH = 16;
  const int infoY1  = cy + headerH + 2;
  const int infoY2  = infoY1 + 12;
  tft.fillRect(cx, infoY1, cw, 24, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(cx + 2, infoY1 + 1);
  tft.print("Pattern Lock");
  tft.setCursor(cx + 2, infoY2 + 1);
  tft.print("A=Finish   B/C");
  tft.print((int)count);
}

static bool uiGetPattern(std::vector<uint8_t>& out) {
  out.clear();
  drawTitleAndTerminal("infiltra terminal — /secure");
  while (true) {
    drawPatternPrompt(out.size());
    int btn = read_button_blocking();
    if (btn == PL_BTN_A) {
      if (out.size() >= 3) return true;     
    } else if (btn == PL_BTN_B || btn == PL_BTN_C) {
      if (out.size() < PAT_MAX) out.push_back((uint8_t)btn);
      addTermLineWrapped(String("append ") + (btn==PL_BTN_B?"B":"C"));
      termDraw("infiltra terminal — /secure");
    }
    delay(90);
  }
}

static bool uiConfirmNumeric(const char expect4[5]) {
  char attempt[5] = {0};
  if (!uiGetNumeric(attempt)) return false;
  return memcmp(attempt, expect4, 4) == 0;
}
static bool uiConfirmPattern(const std::vector<uint8_t>& expect) {
  std::vector<uint8_t> attempt;
  if (!uiGetPattern(attempt)) return false;
  return attempt == expect;
}

static void terminalAndReboot(const char* headerTitle) {
  drawTitleAndTerminal(headerTitle);
  addTermLineWrapped("Applying secure configuration…");
  termDraw(headerTitle); delay(180);
  termTick_likeScan(headerTitle);     delay(160);
  addTermLineWrapped("Writing passlock data…");
  termDraw(headerTitle); delay(180);
  termTick_likeScan(headerTitle);     delay(160);
  addTermLineWrapped("Verifying integrity…");
  termDraw(headerTitle); delay(180);
  termTick_likeScan(headerTitle);     delay(160);
  addTermLineWrapped("Done. Rebooting…");
  termDraw(headerTitle); delay(520);
  ESP.restart();
}

void Core::Passlock::begin() { loadState(); }
bool Core::Passlock::isEnabled() { return g_enabled; }
PasslockType Core::Passlock::type() { return g_type; }
bool Core::Passlock::promptForUnlock() {
  if (!g_enabled) return true;
  if (g_type == PasslockType::Numeric4) {
    bool ok = unlockNumericFlow(g_numeric);
    normalizeRotationForApp();
    return ok;
  }
  else if (g_type == PasslockType::Pattern) {
    std::vector<uint8_t> attempt;
    while (true) {
      drawLockScaffold("Enter Pattern");
      int count = (int)attempt.size();
      drawPlaceholders(min(count, 4));
      tft.setCursor(cx, cy + 26 + 30);
      tft.setTextColor(TFT_SILVER, TFT_BLACK);
      tft.print("B/C = add step,  A = finish (>=3)");
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      int btn = read_button_blocking();
      if (btn == PL_BTN_A) {
        if (attempt.size() >= 3) {
          bool ok = (attempt == g_pattern);
          if (ok) { normalizeRotationForApp(); return true; }
          drawWrongBanner();
          attempt.clear();
        }
      } else if (btn == PL_BTN_B || btn == PL_BTN_C) {
        if (attempt.size() < PAT_MAX) attempt.push_back((uint8_t)btn);
      }
      delay(90);
    }
  }
  normalizeRotationForApp();
  return true;
}

static int choose2(const char* a, const char* b) {
  drawTitleAndTerminal("infiltra terminal — /secure");
  int sel = 0;
  while (true) {
    tft.setTextSize(1);
    const int y = cy + 16 + 2;
    tft.fillRect(cx, y, cw, 14, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(cx + 2, y + 2);
    tft.print("[B] "); tft.print(a); if (sel==0) tft.print(" *  "); else tft.print("    ");
    tft.print("[C] "); tft.print(b); if (sel==1) tft.print(" *");  else tft.print("");
    int btn = passlock_button_read_blocking();
    if      (btn == PL_BTN_B) sel = 0;
    else if (btn == PL_BTN_C) sel = 1;
    else if (btn == PL_BTN_A) return sel;
    delay(90);
  }
}

bool Core::Passlock::runSettingsFlow() {
  if (g_enabled) {
    int act = choose2("Remove", "Change");
    if (act == 0) {
      bool ok = false;
      if (g_type == PasslockType::Numeric4) ok = uiConfirmNumeric(g_numeric);
      else if (g_type == PasslockType::Pattern) ok = uiConfirmPattern(g_pattern);
      if (!ok) { normalizeRotationForApp(); return false; }
      g_enabled = false;
      g_type    = PasslockType::None;
      memset(g_numeric, 0, sizeof(g_numeric));
      g_pattern.clear();
      saveState();
      terminalAndReboot("infiltra terminal — /secure");
      return true; 
    }
    
  }

  int tsel = choose2("Numeric", "Pattern");
  if (tsel == 0) {
    char first[5]={0}, second[5]={0};
    if (!uiGetNumeric(first))  { normalizeRotationForApp(); return false; }
    if (!uiGetNumeric(second)) { normalizeRotationForApp(); return false; }
    if (memcmp(first, second, 4) != 0)     { normalizeRotationForApp(); return false; }
    memcpy(g_numeric, first, 4);
    g_type = PasslockType::Numeric4;
    g_enabled = true;
    g_pattern.clear();
    saveState();
    terminalAndReboot("infiltra terminal — /secure");
    return true;
  } else {
    std::vector<uint8_t> first, second;
    if (!uiGetPattern(first))  { normalizeRotationForApp(); return false; }
    if (!uiGetPattern(second)) { normalizeRotationForApp(); return false; }
    if (first != second)       { normalizeRotationForApp(); return false; }
    g_pattern = first;
    g_type = PasslockType::Pattern;
    g_enabled = true;
    memset(g_numeric, 0, sizeof(g_numeric));
    saveState();
    terminalAndReboot("infiltra terminal — /secure");
    return true;
  }
}
