#include "wifi_scan.h"
#include "UserInterface/menus/menu_submenus.h"
#include <WiFi.h>
#include <math.h>

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;   
#else
  static constexpr uint8_t ROT_TOP = 2;   
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;

struct NetInfo {
  String ssid;
  String bssid;
  int32_t rssi;
  int32_t channel;
  wifi_auth_mode_t enc; 
  bool hidden;
};

static NetInfo sList[64];
static int     sCount = 0;
static int     sSel   = 0;
static bool    sBusy          = false;
static bool    sFrameDrawn    = false; 
static bool    sDirty         = true;  
static bool    sScanTextDrawn = false;  
static int W=0,H=0;
static int bx=0,by=0,bw=0,bh=0; 
static int cx=0,cy=0,cw=0;          
static unsigned long sLastTermMs   = 0;
static const uint16_t TERM_INTERVAL_MS = 120;
static const int MAX_TERM_LINES = 40;    
static String sTermLines[MAX_TERM_LINES];
static int    sTermCount = 0;          
static int    sTermChan  = 1;             
static int    sTermPhase = 0;             
static int    sTermMaxW  = 0;             
static bool          sScanCompleteFlash = false;
static bool          sScanCompleteShown = false;  
static unsigned long sScanCompleteMs    = 0;

static void addTermLineWrapped(TFT_eSPI& tft, const String& line) {
  if (sTermMaxW <= 0) {
    if (sTermCount < MAX_TERM_LINES) sTermLines[sTermCount++] = line;
    return;
  }
  int n = line.length();
  int start = 0;
  while (start < n) {
    int bestEnd = start;
    int lastSpace = -1;
    String seg;
    for (int i = start; i < n; ++i) {
      char c = line.charAt(i);
      String trial = seg + c;
      if (tft.textWidth(trial) > sTermMaxW) break;
      seg += c;
      bestEnd = i + 1;
      if (c == ' ') lastSpace = i;
    }
    if (bestEnd < n && lastSpace >= start) {
      seg = line.substring(start, lastSpace);  
      bestEnd = lastSpace + 1;                 
    }
    if (seg.length() == 0 && bestEnd < n) {
      seg = line.substring(start, start+1);   
      bestEnd = start + 1;
    }
    if (seg.length()) {
      if (sTermCount < MAX_TERM_LINES) {
        sTermLines[sTermCount++] = seg;
      } else {
        for (int k=1;k<MAX_TERM_LINES;k++) sTermLines[k-1] = sTermLines[k];
        sTermLines[MAX_TERM_LINES-1] = seg;
      }
    }
    start = bestEnd;
  }
}

static void termDraw(TFT_eSPI& tft) {
  const int headerH = 16;
  const int termTop = cy + headerH + 2;
  const int lineH   = 12;
  tft.fillRoundRect(cx, cy, cw, headerH, 3, TFT_DARKGREY);
  tft.drawRoundRect(cx, cy, cw, headerH, 3, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(cx + 4, cy + 3);
  tft.print("infiltra terminal — /root");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int availH = (by + bh - 8) - termTop; 
  int maxVis = max(3, availH / lineH);
  int total = sTermCount;
  int start = (total > maxVis) ? (total - maxVis) : 0;
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

static void termTick(TFT_eSPI& tft) {
  String chStr;
  if (sTermChan <= 13) {
    chStr = String(sTermChan);
    sTermChan++;
  } else {
    int ch5 = 36 + ((sTermChan - 14) * 4);
    if (ch5 > 165) { sTermChan = 1; chStr = "1"; }
    else { chStr = String(ch5); sTermChan++; }
  }
  const char sp[4] = {'|','/','-','\\'};
  static int spIdx = 0;
  char spc = sp[(spIdx++) & 3];
  String msg;
  switch ((sTermPhase++) % 4) {
    case 0: msg = "["+String(spc)+"] root@infiltra:/# ifconfig wlan0 up"; break;
    case 1: msg = "["+String(spc)+"] root@infiltra:/# iw scan ch " + chStr; break;
    case 2: msg = "["+String(spc)+"] root@infiltra:/# sniff --beacons --short --timeout=150ms"; break;
    default: msg = "["+String(spc)+"] root@infiltra:/# collect --results --dedupe"; break;
  }
  addTermLineWrapped(tft, msg);
  termDraw(tft);
}

static String encToText(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN:            return "Open";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    case WIFI_AUTH_WAPI_PSK:        return "WAPI";
    default:                        return "?";
  }
}
static String bandFromChannel(int ch) {
  if (ch <= 14) return "2.4G";
  if (ch >= 36 && ch <= 177) return "5G";
  return "?G";
}
static String clipToWidth(TFT_eSPI& tft, const String& s, int maxW) {
  if (tft.textWidth(s) <= maxW) return s;
  String out = s;
  while (out.length() && tft.textWidth(out + "...") > maxW) out.remove(out.length()-1);
  return out + "...";
}
static void drawBadge(TFT_eSPI& tft, int x, int y, const String& txt, uint16_t edge, uint16_t fill, uint16_t fg) {
  int tw = tft.textWidth(txt) + 8;
  int th = 14;
  tft.fillRoundRect(x, y, tw, th, 4, fill);
  tft.drawRoundRect(x, y, tw, th, 4, edge);
  tft.setTextColor(fg, fill);
  tft.setCursor(x+4, y+3);
  tft.print(txt);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static void drawSignalBars(TFT_eSPI& tft, int centerX, int baselineY, int rssi, uint16_t col=TFT_WHITE) {
  int bars = 0;
  if      (rssi >= -55) bars = 4;
  else if (rssi >= -65) bars = 3;
  else if (rssi >= -75) bars = 2;
  else if (rssi >= -85) bars = 1;

  const int w=5, gap=3;              
  const int total = 4*w + 3*gap;
  int x0 = centerX - total/2;

  for (int i=0;i<4;i++) {
    int bh = 3 + i*3;               
    int bx = x0 + i*(w+gap);
    int by = baselineY - bh;        
    tft.drawRect(bx, by, w, bh, col);
    if (i < bars) tft.fillRect(bx+1, by+1, w-2, bh-2, col);
  }
}

static void drawStaticFrame(TFT_eSPI& tft) {
  
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  W = tft.width(); H = tft.height();
  int marginX = 10;
  int marginTop = 16;
  int marginBottom = 8;        
  bx = marginX;
  by = marginTop + 12;         
  bw = W - marginX*2;
  bh = H - by - marginBottom;  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(8, 4);
  tft.drawRoundRect(bx, by, bw, bh, 8, TFT_DARKGREY);
  tft.drawRoundRect(bx+1, by+1, bw-2, bh-2, 7, TFT_BLACK);
  cx = bx + 10;
  cy = by + 8;
  cw = bw - 20;
  sTermMaxW = cw;
  tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
}

static void startAsyncScan() {
  sBusy = true; sCount = 0; sSel = 0;
  sScanTextDrawn = false;
  sDirty = true;
  sTermCount = 0;
  sTermChan  = 1;
  sTermPhase = 0;
  sLastTermMs = millis();
  sScanCompleteFlash = false;
  sScanCompleteShown = false;
  sScanCompleteMs    = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(20);
  WiFi.scanDelete();
  WiFi.scanNetworks(/*async*/true,true);
}

static void fetchResultsIfReady(TFT_eSPI& tft) {
  int res = WiFi.scanComplete();
  if (res == -1) return;                
  if (res <  0)  { sBusy=false; return; } 

  sCount = (res > 64) ? 64 : res;
  for (int i=0; i<sCount; ++i) {
    sList[i].ssid    = WiFi.SSID(i);
    sList[i].bssid   = WiFi.BSSIDstr(i);
    sList[i].rssi    = WiFi.RSSI(i);
    sList[i].channel = WiFi.channel(i);
    sList[i].enc     = WiFi.encryptionType(i);
    sList[i].hidden  = (WiFi.SSID(i).length() == 0);
  }

  addTermLineWrapped(tft, "[✓] root@infiltra:/# scan complete");
  sScanCompleteFlash = true;
  sScanCompleteShown = false;           
  sScanCompleteMs    = millis();
  sBusy  = false;
  sDirty = true;                        
  WiFi.scanDelete();
}

void wifiScanReset() { startAsyncScan(); }
void wifiScanDrawScreen(TFT_eSPI& tft) {
  if (!sFrameDrawn) {
    drawStaticFrame(tft);
    sFrameDrawn = true;
    sDirty = true;
  }

  if (sScanCompleteFlash) {
    if (!sScanCompleteShown) {
      termDraw(tft);                    
      sScanCompleteShown = true;
    }
    if (millis() - sScanCompleteMs < 650) {
      return;                           
    }
    sScanCompleteFlash = false;
    sDirty = true;                      
  }

  if (sBusy) {
    if (!sScanTextDrawn) {
      tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
      sScanTextDrawn = true;
      sLastTermMs = 0;                  
      sTermCount = 0;
    }

    unsigned long now = millis();
    if (now - sLastTermMs >= TERM_INTERVAL_MS) {
      termTick(tft);
      sLastTermMs = now;
    }
    fetchResultsIfReady(tft);
    return;
  }

  if (sDirty) {
    tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);

    if (sCount <= 0) {
      tft.setTextColor(TFT_SILVER, TFT_BLACK);
      tft.setCursor(cx, cy);
      tft.print("No networks found.");
      sDirty = false;
      return;
    }

    const NetInfo& n = sList[sSel];
    int y = cy;                 

    {
      String name = n.ssid.length() ? n.ssid : String("<hidden>");
      String left = "SSID: " + clipToWidth(tft, name, cw - 60);
      tft.setCursor(cx, y); tft.print(left);
      char buf[24]; snprintf(buf, sizeof(buf), "#%d/%d", sSel+1, sCount);
      int rightX = bx + bw - 10 - tft.textWidth(buf);
      tft.setCursor(rightX, y); tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.print(buf);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      y += 14;
      tft.drawFastHLine(bx+6, y, bw-12, TFT_DARKGREY);
      y += 6;
    }

    {
      String line = "BSSID: " + clipToWidth(tft, n.bssid, cw - tft.textWidth("BSSID: "));
      tft.setCursor(cx, y); tft.print(line);
      y += 14;
    }

    {
      String enc = encToText(n.enc);
      bool isOpen = (enc == "Open");
      uint16_t edge = isOpen ? TFT_YELLOW : TFT_DARKGREY;
      uint16_t fill = isOpen ? (uint16_t)0x2104 : (uint16_t)0x0842;
      uint16_t fg   = isOpen ? TFT_YELLOW : TFT_WHITE;

      drawBadge(tft, cx, y, "ENC " + enc, edge, fill, fg);

      
      String chBand = "CH " + String(n.channel) + " • " + bandFromChannel(n.channel);
      int encW = tft.textWidth("ENC " + enc) + 8;
      int x2 = cx + encW + 10;
      drawBadge(tft, x2, y, chBand, TFT_DARKGREY, 0x0000, TFT_WHITE);

      y += 20;
    }

    {
      String rssiTxt = "RSSI: " + String(n.rssi) + " dBm";
      tft.setCursor(cx, y); tft.print(rssiTxt);
    }

    {
      const int barsLineY   = by + bh - 20;  
      tft.drawFastHLine(bx+6, barsLineY, bw-12, TFT_DARKGREY);

      const int barsBaseline = barsLineY + 18; 
      drawSignalBars(tft, bx + bw/2, barsBaseline, n.rssi);
      
    }

    sDirty = false;
  }
}

void wifiScanHandleInput(bool a, bool b, bool c, bool& requestExit) {
  requestExit = false;
  if (c) {
    sFrameDrawn = false;  
    requestExit = true;
    return;
  }
  if (a) { startAsyncScan(); return; } 
  if (b && !sBusy && sCount > 0) {
    sSel = (sSel + 1) % sCount;
    sDirty = true; 
  }
}
