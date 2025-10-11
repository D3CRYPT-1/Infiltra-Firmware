#include "integrated_files.h"
#include "UserInterface/menus/menu_submenus.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>

extern "C" {
  #include "esp_ota_ops.h"
}
#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;
struct IFNode { const char* name; bool isDir; const int16_t* kids; uint8_t kidCount; };
enum : int16_t {
  N_ROOT = 0, N_SRC, N_MAIN_CPP,
  N_MODULES, N_CORE, N_CORE_BUTTONS_CPP, N_CORE_BUTTONS_H,
  N_FUNCS,
  N_BGONE_CPP, N_BGONE_H,
  N_BLE_SCAN_CPP, N_BLE_SCAN_H,
  N_BLE_GOOGLE_ADV_CPP, N_BLE_GOOGLE_ADV_H,
  N_BRIGHTNESS_CPP, N_BRIGHTNESS_H,
  N_IR_READ_CPP, N_IR_READ_H,
  N_PACKET_SCAN_CPP, N_PACKET_SCAN_H,
  N_STOPWATCH_CPP, N_STOPWATCH_H,
  N_WIFI_SCAN_CPP, N_WIFI_SCAN_H,
  N_UI, N_UI_BITMAPS, N_UI_MENUS,
  N_MENU_BITMAPS_CPP, N_MENU_BITMAPS_H,
  N_MENU_SUBMENUS_CPP, N_MENU_SUBMENUS_H,
  N_SUBMENU_OPTIONS_CPP, N_SUBMENU_OPTIONS_H,
  N_MENU_ENUMS_H,
  N_SPIFFS, N_SPIFFS_IR,
  N__COUNT
};

static const int16_t K_ROOT[]    = { N_SRC, N_SPIFFS };
static const int16_t K_SRC[]     = { N_MAIN_CPP, N_MODULES, N_UI };
static const int16_t K_MODULES[] = { N_CORE, N_FUNCS };
static const int16_t K_CORE[]    = { N_CORE_BUTTONS_CPP, N_CORE_BUTTONS_H };
static const int16_t K_FUNCS[]   = {
  N_BGONE_CPP, N_BGONE_H,
  N_BLE_SCAN_CPP, N_BLE_SCAN_H,
  N_BLE_GOOGLE_ADV_CPP, N_BLE_GOOGLE_ADV_H,
  N_BRIGHTNESS_CPP, N_BRIGHTNESS_H,
  N_IR_READ_CPP, N_IR_READ_H,
  N_PACKET_SCAN_CPP, N_PACKET_SCAN_H,
  N_STOPWATCH_CPP, N_STOPWATCH_H,
  N_WIFI_SCAN_CPP, N_WIFI_SCAN_H
};
static const int16_t K_UI[]         = { N_UI_BITMAPS, N_UI_MENUS };
static const int16_t K_UI_BITMAPS[] = { N_MENU_BITMAPS_CPP, N_MENU_BITMAPS_H };
static const int16_t K_UI_MENUS[]   = { N_MENU_SUBMENUS_CPP, N_MENU_SUBMENUS_H, N_SUBMENU_OPTIONS_CPP, N_SUBMENU_OPTIONS_H, N_MENU_ENUMS_H };
static const int16_t K_SPIFFS[]     = { N_SPIFFS_IR };
static const IFNode NODES[N__COUNT] = {
  { "Infiltra Main", true,  K_ROOT,    (uint8_t)(sizeof(K_ROOT)/2) },
  { "src",           true,  K_SRC,     (uint8_t)(sizeof(K_SRC)/2) },
  { "main.cpp",      false, nullptr,   0 },
  { "Modules",       true,  K_MODULES, (uint8_t)(sizeof(K_MODULES)/2) },
  { "Core",          true,  K_CORE,    (uint8_t)(sizeof(K_CORE)/2) },
  { "buttons.cpp",   false, nullptr, 0 },
  { "buttons.h",     false, nullptr, 0 },
  { "Functions",     true,  K_FUNCS,   (uint8_t)(sizeof(K_FUNCS)/2) },
  { "bgone.cpp",         false, nullptr, 0 }, { "bgone.h",         false, nullptr, 0 },
  { "ble_scan.cpp",      false, nullptr, 0 }, { "ble_scan.h",      false, nullptr, 0 },
  { "ble_google_adv.cpp",false, nullptr, 0 }, { "ble_google_adv.h",false, nullptr, 0 },
  { "brightness.cpp",    false, nullptr, 0 }, { "brightness.h",    false, nullptr, 0 },
  { "ir_read.cpp",       false, nullptr, 0 }, { "ir_read.h",       false, nullptr, 0 },
  { "packet_scan.cpp",   false, nullptr, 0 }, { "packet_scan.h",   false, nullptr, 0 },
  { "stopwatch.cpp",     false, nullptr, 0 }, { "stopwatch.h",     false, nullptr, 0 },
  { "wifi_scan.cpp",     false, nullptr, 0 }, { "wifi_scan.h",     false, nullptr, 0 },
  { "UserInterface", true,  K_UI,           (uint8_t)(sizeof(K_UI)/2) },
  { "bitmaps",       true,  K_UI_BITMAPS,   (uint8_t)(sizeof(K_UI_BITMAPS)/2) },
  { "menus",         true,  K_UI_MENUS,     (uint8_t)(sizeof(K_UI_MENUS)/2) },
  { "menu_bitmaps.cpp",   false, nullptr, 0 }, { "menu_bitmaps.h",   false, nullptr, 0 },
  { "menu_submenus.cpp",  false, nullptr, 0 }, { "menu_submenus.h",  false, nullptr, 0 },
  { "submenu_options.cpp",false, nullptr, 0 }, { "submenu_options.h",false, nullptr, 0 },
  { "menu_enums.h",       false, nullptr, 0 },
  { "SPIFFS", true, K_SPIFFS, (uint8_t)(sizeof(K_SPIFFS)/2) },
  { "ir/", true, nullptr, 0 }
};

enum class Mode : uint8_t { ROOT, BROWSE };
static Mode   sMode        = Mode::ROOT;
static int    sSel         = 2;
static int    sPageStart   = 0;
static int16_t sStack[12];
static int     sDepth      = 0;
static int16_t sCur        = N_ROOT;
static bool   sDynMode     = false;
static volatile bool g_openWebReq = false;
static String sDynPath     = "/integrated";
static bool   sOpenWebReq  = false;
static bool    sFrameDrawn = false;
static bool    sDirty      = true;
static unsigned long sMsgUntil = 0;
static String        sMsgText;
static bool          sMsgWasActive = false;
static int W=0, H=0;
static int bx=0, by=0, bw=0, bh=0;
static int cx=0, cy=0, cw=0;
static int lx=0, lw=0;
static int rx=0, rw=0;
static constexpr uint16_t C_FRAME     = TFT_DARKGREY;
static constexpr uint16_t C_FRAME_IN  = TFT_BLACK;
static constexpr uint16_t C_TEXT      = TFT_WHITE;
static constexpr uint16_t C_SUB       = TFT_SILVER;
static constexpr uint16_t C_ROW       = 0x0000;
static constexpr uint16_t C_ROW_ALT   = 0x0000;
static constexpr uint16_t C_HILITE    = 0x39C7;
static constexpr uint16_t C_OUTLINE   = TFT_WHITE;
static constexpr uint16_t C_WARN_BG   = 0xA2A0;
static constexpr uint16_t C_WARN_TXT  = TFT_WHITE;
static constexpr uint16_t C_SLIDER_TR = 0x03A0;
static constexpr uint16_t C_SLIDER_FI = 0x07E0;

static uint32_t getAppCapacity(){
  const esp_partition_t* p = esp_ota_get_running_partition();
  return p ? p->size : 0;
}
static uint32_t getAppUsed(){
#if defined(ARDUINO_ARCH_ESP32)
  return (uint32_t)ESP.getSketchSize();
#else
  return 0;
#endif
}
static String fmtMBPrec(uint32_t b, uint8_t prec){
  float mb=b/1048576.0f; char buf[24];
  if(prec==2) snprintf(buf,sizeof(buf),"%.2fMB",mb);
  else if(prec==1) snprintf(buf,sizeof(buf),"%.1fMB",mb);
  else snprintf(buf,sizeof(buf),"%.0fMB",mb);
  return String(buf);
}
static String fmtMBInt(uint32_t b){ uint32_t mb=(b+524288)/1048576; return String(mb)+"MB"; }
static inline bool msgActiveNow(){ return sMsgUntil && (long)(sMsgUntil - millis()) > 0; }
static void msg(const String& m, uint16_t ms=2000){ sMsgText=m; sMsgUntil=millis()+ms; sDirty=true; }
static void drawStaticFrame(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  tft.setTextSize(1);
  W = tft.width(); H = tft.height();
  const int marginX = 10, marginTop = 16, marginBottom = 8;
  bx = marginX; by = marginTop + 12; bw = W - marginX*2; bh = H - by - marginBottom;
  tft.drawRoundRect(bx, by, bw, bh, 8, C_FRAME);
  tft.drawRoundRect(bx+1, by+1, bw-2, bh-2, 7, C_FRAME_IN);
  tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
  cx = bx + 8; cy = by + 6; cw = bw - 16;
  lw = 90; lx = cx;
  rx = lx + lw + 6; rw = cw - lw - 6;
}

static void drawFittingStorageText(TFT_eSPI& tft, int sx, int sy, int sw, uint32_t used, uint32_t cap){
  String s;
  s = fmtMBPrec(used,2) + " / " + fmtMBPrec(cap,2);
  if (tft.textWidth(s) <= sw) { tft.setCursor(sx, sy); tft.print(s); return; }
  s = fmtMBPrec(used,1) + " / " + fmtMBPrec(cap,1);
  if (tft.textWidth(s) <= sw) { tft.setCursor(sx, sy); tft.print(s); return; }
  s = fmtMBPrec(used,0) + " / " + fmtMBPrec(cap,0);
  if (tft.textWidth(s) <= sw) { tft.setCursor(sx, sy); tft.print(s); return; }
  { String u=fmtMBPrec(used,1); u.replace("MB",""); String t=fmtMBPrec(cap,1); s=u+"/"+t; if (tft.textWidth(s) <= sw) { tft.setCursor(sx, sy); tft.print(s); return; } }
  { String u=fmtMBInt(used); u.replace("MB",""); String t=fmtMBInt(cap); s=u+"/"+t; if (tft.textWidth(s) <= sw) { tft.setCursor(sx, sy); tft.print(s); return; } }
  String u=fmtMBPrec(used,1), t=fmtMBPrec(cap,1);
  tft.setCursor(sx, sy);       tft.print("Used: ");  tft.print(u);
  tft.setCursor(sx, sy + 10);  tft.print("Total: "); tft.print(t);
}

static void drawStoragePanel(TFT_eSPI& tft){
  const int y = cy;
  const int h = bh - (cy - by) - 10;
  tft.drawRoundRect(lx, y, lw, h, 5, C_OUTLINE);
  tft.setTextColor(C_TEXT, TFT_BLACK);
  tft.setCursor(lx + 4, y + 2);  tft.print("Integrated");
  tft.setTextColor(C_SUB, TFT_BLACK);
  tft.setCursor(lx + 4, y + 12); tft.print("Storage");
  uint32_t cap = getAppCapacity(); if (!cap) cap = 3*1024*1024;
  uint32_t used = getAppUsed(); if (!used || used>cap) used = cap/2;
  const int sx = lx + 4, sw = lw - 8, sy = y + 24, sh = 8;
  tft.fillRoundRect(sx, sy, sw, sh, 3, C_SLIDER_TR);
  int fillw = (int)((uint64_t)sw * used / cap); if (fillw < 1) fillw = 1;
  tft.fillRoundRect(sx, sy, fillw, sh, 3, C_SLIDER_FI);
  tft.drawRoundRect(sx, sy, sw, sh, 3, C_OUTLINE);
  tft.setTextColor(C_SUB, TFT_BLACK);
  drawFittingStorageText(tft, sx, sy + 12, sw, used, cap);
  tft.setCursor(sx, sy + 26); tft.print("A: select");
  tft.setCursor(sx, sy + 36); tft.print("B/C: nav");
}

static String clipToWidth(TFT_eSPI& tft, const String& s, int maxW){
  if (tft.textWidth(s) <= maxW) return s;
  String out = s;
  while (out.length() && tft.textWidth(out + "...") > maxW) out.remove(out.length()-1);
  return out + "...";
}

static int listIntegrated(String basePath, String* out, int maxItems){
  if (!basePath.startsWith("/integrated")) basePath="/integrated";
  if (!SPIFFS.begin(false)) { }
  SPIFFS.mkdir(basePath);
  int n=0;
  fs::File root = SPIFFS.open(basePath);
  if (!root || !root.isDirectory()) return 0;
  fs::File f = root.openNextFile();
  while (f && n < maxItems){
    String name = String(f.name());
    if (name.startsWith(basePath)) name = name.substring(basePath.length());
    if (name.startsWith("/")) name.remove(0,1);
    int slash=name.lastIndexOf('/');
    if (slash>=0) name = name.substring(slash+1);
    if (name.length()){
      out[n++] = name + (f.isDirectory()?"/":"");
    }
    f = root.openNextFile();
  }
  return n;
}

static int listSpiffsFolder(const String& basePath, String* out, int maxItems){
  if (!SPIFFS.begin(false)) { }
  int n=0;
  fs::File root = SPIFFS.open(basePath);
  if (!root || !root.isDirectory()) return 0;
  fs::File f = root.openNextFile();
  while (f && n < maxItems){
    String name = String(f.name());
    if (name.startsWith(basePath)) name = name.substring(basePath.length());
    if (name.startsWith("/")) name.remove(0,1);
    int slash=name.lastIndexOf('/');
    if (slash>=0) name = name.substring(slash+1);
    if (name.length()){
      out[n++] = name + (f.isDirectory()?"/":"");
    }
    f = root.openNextFile();
  }
  return n;
}

static void drawMessageBar(TFT_eSPI& tft, int panelY, int panelH){
  if (!msgActiveNow()) return;
  const int bhh = 14;
  const int x  = rx + 4;
  const int w  = rw - 8;
  const int y  = panelY + panelH - bhh - 3;
  tft.fillRoundRect(x, y, w, bhh, 3, C_WARN_BG);
  tft.drawRoundRect(x, y, w, bhh, 3, C_OUTLINE);
  tft.setTextColor(C_WARN_TXT, C_WARN_BG);
  tft.setTextDatum(CC_DATUM);
  tft.drawString(sMsgText, x + w/2, y + bhh/2, 1);
  tft.setTextDatum(TL_DATUM);
}

static void drawDirectoryPanel(TFT_eSPI& tft){
  const int y = cy;
  const int h = bh - (cy - by) - 10;
  tft.drawRoundRect(rx, y, rw, h, 5, C_OUTLINE);
  tft.fillRect(rx+1, y+1, rw-2, h-2, TFT_BLACK);
  tft.setTextColor(C_TEXT, TFT_BLACK);
  tft.setCursor(rx + 4, y + 2);
  tft.print(sMode == Mode::ROOT ? "Integrated Files" : "Infiltra Main");
  const int listX = rx + 4;
  const int listY = y + 14;
  const int listW = rw - 8;
  const int rowH  = 12;
  const int available = h - (listY - y);
  const int vis   = max(3, available / rowH);
  String items[60]; int count=0;
  if (sMode == Mode::ROOT){
    items[count++] = "<- Back";
    items[count++] = "+ Add";
    items[count++] = "Infiltra Main";
    items[count++] = "SPIFFS/ir";
  } else {
    const IFNode& n = NODES[sCur];
    items[count++] = "<- Back";
    if (n.kids && n.kidCount){
      for (int i=0;i<n.kidCount;i++){
        const IFNode& k = NODES[n.kids[i]];
        items[count++] = String(k.name) + (k.isDir ? "/" : "");
      }
    }

    if (sCur == N_ROOT){
      items[count++] = "— integrated —";
      int added = listIntegrated("/integrated", items+count, 60-count);
      count += added;
    }
    if (sCur == N_SPIFFS){
      items[count++] = "— SPIFFS —";
      int added = listSpiffsFolder("/", items+count, 60-count);
      count += added;
    }
    if (sCur == N_SPIFFS_IR){
      items[count++] = "— /ir —";
      int added = listSpiffsFolder("/ir", items+count, 60-count);
      count += added;
    }
  }
  if (sSel < sPageStart) sPageStart = sSel;
  if (sSel >= sPageStart + vis) sPageStart = sSel - vis + 1;
  for (int i=0;i<vis;i++){
    const int idx = sPageStart + i;
    const int ry  = listY + i*rowH;
    int rh = rowH-1;
    int bottom = y + h - 1;
    if (ry + rh > bottom) rh = bottom - ry;
    if (rh <= 0) break;
    tft.fillRect(listX, ry, listW, rh, C_ROW);
    if (idx >= count) continue;
    bool sel = (idx == sSel);
    String txt = items[idx];
    if (txt.startsWith("— ")) {
      tft.drawLine(listX, ry+rh/2, listX+listW, ry+rh/2, C_SUB);
      continue;
    }
    String drawTxt = clipToWidth(tft, txt, listW - 6);
    if (sel){
      int pillH = max(3, rh - 2);
      tft.fillRoundRect(listX+1, ry+1, listW-2, pillH, 3, C_HILITE);
      tft.drawRoundRect(listX+1, ry+1, listW-2, pillH, 3, C_OUTLINE);
      tft.setTextColor(C_TEXT, C_HILITE);
    } else {
      tft.setTextColor(C_TEXT, C_ROW);
    }
    tft.setCursor(listX + 3, ry + 1);
    tft.print(drawTxt);
  }
  drawMessageBar(tft, y, h);
}

static int resolveChildCount(int16_t node){
  const IFNode& n=NODES[node];
  return n.kids?n.kidCount:0;
}
static int16_t childAt(int16_t node,int idx){
  const IFNode& n=NODES[node];
  if(!n.kids||idx<0||idx>=n.kidCount) return -1;
  return n.kids[idx];
}

void integratedFilesReset(){
  sMode = Mode::ROOT;
  sSel = 2;
  sPageStart = 0;
  sDepth = 0;
  sCur = N_ROOT;
  sDynMode = false;
  sDynPath = "/integrated";
  sOpenWebReq = false;
  sFrameDrawn = false;
  sDirty = true;
  sMsgUntil = 0;
  sMsgText = "";
  sMsgWasActive = false;
}

void integratedFilesRequestOpenWeb() {
  g_openWebReq = true;
}

bool integratedFilesConsumeOpenWebRequest() {
  bool was = g_openWebReq;
  g_openWebReq = false;
  return was;
}

void integratedFilesDrawScreen(TFT_eSPI& tft){
  if (!sFrameDrawn){
    drawStaticFrame(tft);
    sFrameDrawn = true;
    sDirty = true;
  }
  bool nowActive = msgActiveNow();
  if (sMsgWasActive && !nowActive) sDirty = true;
  sMsgWasActive = nowActive;
  if (!sDirty && !nowActive) return;
  drawStoragePanel(tft);
  drawDirectoryPanel(tft);
  sDirty = false;
}

void integratedFilesHandleInput(bool a, bool b, bool c, bool& exitReq){
  exitReq = false;
  if (b) { sSel++; sDirty = true; }
  if (c) { sSel--; sDirty = true; }
  if (sSel < 0) sSel = 0;
  if (sMode == Mode::ROOT){
    if (sSel > 3) sSel = 0;
    if (a){
      if (sSel == 0){ exitReq = true; return; }
      if (sSel == 1){
        sOpenWebReq = true;
        exitReq = true;
        return;
      }
      if (sSel == 2){ sMode = Mode::BROWSE; sDynMode=false; sSel=1; sPageStart=0; sDepth=0; sCur=N_ROOT; sDirty=true; return; }
      if (sSel == 3){ sMode = Mode::BROWSE; sDynMode=false; sSel=1; sPageStart=0; sDepth=0; sCur=N_SPIFFS; sDirty=true; return; }
    }
    return;
  }
  if (!sDynMode){
    String tmp[80]; int count=0;
    tmp[count++] = "<- Back";
    int kidc = resolveChildCount(sCur);
    for(int i=0;i<kidc;i++){
      int16_t ch = childAt(sCur,i);
      const IFNode& kn = NODES[ch];
      tmp[count++] = String(kn.name) + (kn.isDir?"/":"");
    }
    bool atRoot = (sCur==N_ROOT);
    bool atSpiffs = (sCur==N_SPIFFS);
    bool atSpiffsIr = (sCur==N_SPIFFS_IR);
    if (atRoot){
      tmp[count++] = "— integrated —";
      int added = listIntegrated("/integrated", tmp+count, 80-count);
      count += added;
    }
    if (atSpiffs){
      tmp[count++] = "— SPIFFS —";
      int added = listSpiffsFolder("/", tmp+count, 80-count);
      count += added;
    }
    if (atSpiffsIr){
      tmp[count++] = "— /ir —";
      SPIFFS.mkdir("/ir");
      int added = listSpiffsFolder("/ir", tmp+count, 80-count);
      count += added;
    }
    if (sSel > count-1) sSel = 0;
    if (a){
      if (sSel == 0){
        sMode=Mode::ROOT; sSel=2; sPageStart=0; sDirty=true; return;
      }
      int rel = sSel - 1;
      if (rel < kidc){
        int16_t next = childAt(sCur, rel);
        const IFNode& ch = NODES[next];
        if (ch.isDir){
          if (sDepth < (int)(sizeof(sStack)/sizeof(sStack[0]))) sStack[sDepth++] = sCur;
          sCur = next;
          sSel = 1; sPageStart = 0; sDirty = true;
        } else {
          msg("cant edit main", 1500);
        }
        return;
      }
      int base = 1 + kidc;
      if (atRoot && sSel==base-1) return;
      if (atRoot && sSel>=base){
        String name = tmp[sSel];
        if (name.endsWith("/")){
          sDynMode=true;
          sDynPath="/integrated/"+name.substring(0,name.length()-1);
          sSel=1; sPageStart=0; sDirty=true;
        } else {
          msg("cant edit main", 1500);
        }
        return;
      }
      if (atSpiffs && sSel==base-1) return;
      if (atSpiffsIr && sSel==base-1) return;
    return;
  }
}

}
