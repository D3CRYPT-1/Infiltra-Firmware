#include "packet_scan.h"
#include "UserInterface/menus/menu_submenus.h"
#include <WiFi.h>
extern "C" {
  #include "esp_wifi.h"
  #include "esp_wifi_types.h"
  #include "esp_event.h"
  #ifdef CONFIG_BT_ENABLED
    #include "esp_bt.h"
    #include "esp_bt_main.h"
  #endif
  #ifdef CONFIG_ESP_COEX_SW_COEXIST_ENABLE
    #include "esp_coexist.h"
  #endif
}
#include <string.h>

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;

struct Stats {
  uint32_t total, mgmt, beacon, probe, data, ctrl, misc;
  int32_t  rssiSum; uint32_t rssiCnt;
  uint32_t upBytes, downBytes; 
};
static volatile Stats sIsr;
static Stats          sDraw;
static const uint16_t SAMPLE_MS = 150;
static const int      NSAMPLES  = 64;
static uint16_t sSamples[NSAMPLES];
static int      sSidx = 0;
static uint32_t sPrevTotal = 0;
static unsigned long sLastSample = 0;
static uint32_t sPrevUpBytes = 0, sPrevDownBytes = 0;
static float    sUpKbps = 0.0f,   sDownKbps = 0.0f;
static bool sFrameDrawn = false;
static bool sDirty      = true;
static int W=0,H=0;
static int bx=0,by=0,bw=0,bh=0;   
static int cx=0,cy=0,cw=0;        
static int gx=0,gy=0,gw=0,gh=0;   
static int sy=0,sh=0;             
static const int PILL_H      = 14;
static const int PILL_GAP    = 12;
static const int PILL_W_RATE = 56;  
static const int PILL_W_RSSI = 86;  
static const int PILL_W_KBPS = 90;  
static int  sChannel = 1;
static const int CH_MIN=1, CH_MAX=14;
static esp_err_t sLastErr = ESP_OK;
static bool      sPromiscOn = false;

static String clipToWidth(TFT_eSPI& tft, const String& s, int maxW){
  if (tft.textWidth(s) <= maxW) return s;
  String out = s;
  while (out.length() && tft.textWidth(out + "...") > maxW) out.remove(out.length()-1);
  return out + "...";
}
static void fixedPill(TFT_eSPI& tft,int x,int y,int w,const String& txt,
                      uint16_t edge,uint16_t fill,uint16_t fg){
  tft.fillRoundRect(x, y, w, PILL_H, 6, fill);
  if (edge != fill) tft.drawRoundRect(x, y, w, PILL_H, 6, edge); 
  String inside = txt;
  int maxTxt = w - 10;
  while (inside.length() && tft.textWidth(inside) > maxTxt) inside.remove(inside.length()-1);
  tft.setTextColor(fg, fill);
  tft.setCursor(x + 5, y + 3);
  tft.print(inside);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}
static void lightDivider(TFT_eSPI& tft, int y){ tft.drawFastHLine(bx+6, y, bw-12, TFT_DARKGREY); }
static void clearStats(){
  memset((void*)&sIsr, 0, sizeof(sIsr));
  for (int i=0;i<NSAMPLES;i++) sSamples[i]=0;
  sPrevTotal = 0; sSidx = 0; sLastSample = millis();
  sPrevUpBytes = sPrevDownBytes = 0;
  sUpKbps = sDownKbps = 0.0f;
}
static int avgRssi(const Stats& s){
  return (s.rssiCnt>0) ? (s.rssiSum/(int)s.rssiCnt) : 0;
}
static String fmtKbps(float v){
  if (v < 0) v = 0;
  char b[24]; snprintf(b, sizeof(b), "%.1f kbps", v);
  return String(b);
}

static void IRAM_ATTR snifferCb(void* buf, wifi_promiscuous_pkt_type_t type)
{
  if (!buf) return;
  const wifi_promiscuous_pkt_t* ppkt = (const wifi_promiscuous_pkt_t*)buf;
  const wifi_pkt_rx_ctrl_t& rx = ppkt->rx_ctrl;

  sIsr.total++;
  sIsr.rssiSum += rx.rssi;
  sIsr.rssiCnt++;

  const uint8_t* p = (const uint8_t*)ppkt->payload;
  uint16_t blen = rx.sig_len;
  if (p && blen >= 2) {
    uint16_t fc = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
    bool toDS   = (fc & 0x0100) != 0;
    bool fromDS = (fc & 0x0200) != 0;
    if (toDS && !fromDS) sIsr.upBytes   += blen;   
    if (!toDS && fromDS) sIsr.downBytes += blen;   
  }

  switch (type) {
    case WIFI_PKT_MGMT: {
      sIsr.mgmt++;
      if (p) {
        uint8_t subtype = (p[0] >> 4) & 0x0F;
        if (subtype == 8) sIsr.beacon++;
        if (subtype == 4 || subtype == 5) sIsr.probe++;
      }
      break;
    }
    case WIFI_PKT_DATA: sIsr.data++; break;
    case WIFI_PKT_CTRL: sIsr.ctrl++; break;
    case WIFI_PKT_MISC: sIsr.misc++; break;
    default: break;
  }
}

static void stopBluetoothIfAny(){
#ifdef CONFIG_BT_ENABLED
  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) esp_bluedroid_disable();
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) esp_bt_controller_disable();
#endif
}
static void setCountryJP14(){
  wifi_country_t ctry{};
  memcpy(ctry.cc, "JP", 2);
  ctry.schan = 1; ctry.nchan = 14; ctry.max_tx_power = 20;
  ctry.policy = WIFI_COUNTRY_POLICY_MANUAL;
  esp_wifi_set_country(&ctry);
}
static void lowLevelReinit()
{
  esp_wifi_stop();
  esp_wifi_deinit();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  sLastErr = esp_wifi_init(&cfg);
  if (sLastErr != ESP_OK) return;
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_start();
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  setCountryJP14();

#ifdef CONFIG_ESP_COEX_SW_COEXIST_ENABLE
  esp_coex_preference_set(ESP_COEX_PREFER_WIFI);
#endif
}
static void startSniffer(){
  stopBluetoothIfAny();
  lowLevelReinit();
  if (sLastErr != ESP_OK) { sPromiscOn = false; return; }
  esp_wifi_set_promiscuous(false);
  sPromiscOn = false;
  if (sChannel < CH_MIN) sChannel = CH_MIN;
  if (sChannel > CH_MAX) sChannel = CH_MAX;
  esp_wifi_set_channel((uint8_t)sChannel, WIFI_SECOND_CHAN_NONE);
  wifi_promiscuous_filter_t filt{};
  filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT |
                     WIFI_PROMIS_FILTER_MASK_DATA |
                     WIFI_PROMIS_FILTER_MASK_CTRL |
                     WIFI_PROMIS_FILTER_MASK_MISC;
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(snifferCb);
  esp_wifi_set_promiscuous(true);
  sPromiscOn = true;
  esp_wifi_set_channel((uint8_t)sChannel, WIFI_SECOND_CHAN_NONE);
  clearStats();
}
static void stopSniffer(){ esp_wifi_set_promiscuous(false); sPromiscOn = false; }
static void setChannel(int ch){
  if (ch < CH_MIN) ch = CH_MAX;
  if (ch > CH_MAX) ch = CH_MIN;
  sChannel = ch;
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel((uint8_t)sChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel((uint8_t)sChannel, WIFI_SECOND_CHAN_NONE);
  sPromiscOn = true;
  clearStats();
  sDirty = true;
}

static void drawHeader(TFT_eSPI& tft){
  const int headerH=16;
  tft.fillRoundRect(cx, cy, cw, headerH, 3, TFT_DARKGREY);
  tft.drawRoundRect(cx, cy, cw, headerH, 3, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(cx+4, cy+3); tft.print("Packet Sniffer");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char chbuf[24]; snprintf(chbuf,sizeof(chbuf),"CH %d", sChannel);
  int pillW = tft.textWidth(chbuf) + 10;
  int pillX = bx + bw - 10 - pillW;
  tft.fillRect(pillX-2, cy+1, pillW+4, headerH-2, TFT_DARKGREY);
  fixedPill(tft, pillX, cy+1, pillW, chbuf,TFT_DARKGREY,TFT_DARKGREY,TFT_WHITE);
  lightDivider(tft, cy + headerH + 4);
}

static void drawTopPills(TFT_eSPI& tft){
  int rowY = cy + 22;
  tft.fillRect(cx, rowY-2, cw, PILL_H+4, TFT_BLACK);
  uint16_t cur = sSamples[(sSidx + NSAMPLES - 1) % NSAMPLES];
  char rate[24]; snprintf(rate, sizeof(rate), "%u/s", (unsigned)cur);
  fixedPill(tft, cx, rowY, PILL_W_RATE, String(rate), TFT_DARKGREY, 0x0000, TFT_SILVER);
  Stats snap; memcpy(&snap, (const void*)&sIsr, sizeof(Stats));
  String rssiTxt = "RSSI " + String(avgRssi(snap)) + " dBm";
  int rx = cx + cw - PILL_W_RSSI;
  fixedPill(tft, rx, rowY, PILL_W_RSSI, rssiTxt, TFT_DARKGREY, 0x0000, TFT_SILVER);
}

static void drawGraphGrid(TFT_eSPI& tft){
  uint16_t grid = TFT_DARKGREY;
  for (int i=8;i<NSAMPLES;i+=8){
    int x = gx + (i * gw) / NSAMPLES;
    tft.drawFastVLine(x, gy+2, gh-4, grid);
  }
  tft.drawFastHLine(gx, gy + gh*1/3, gw, grid);
  tft.drawFastHLine(gx, gy + gh*2/3, gw, grid);
}

static void drawGraph(TFT_eSPI& tft){
  tft.fillRect(gx, gy, gw, gh, TFT_BLACK);
  drawGraphGrid(tft);
  uint16_t vmax = 1;
  for (int i=0;i<NSAMPLES;i++) if (sSamples[i] > vmax) vmax = sSamples[i];
  tft.drawFastHLine(gx, gy + gh - 1, gw, TFT_DARKGREY);
  int barW = (gw / NSAMPLES); if (barW < 1) barW = 1;
  for (int i=0;i<NSAMPLES;i++){
    int idx = (sSidx + i) % NSAMPLES;
    uint16_t v = sSamples[idx];
    int h = (int)((float)v / (float)vmax * (gh - 3));
    if (h < 1 && v > 0) h = 1;

    int bx0 = gx + i*barW;
    int by0 = gy + gh - h - 1;
    if (h > 0) {
      tft.drawRect(bx0, by0, barW-1, h, TFT_DARKGREY);
      if (h > 2) tft.fillRect(bx0+1, by0+1, barW-3, h-2, TFT_WHITE);
    }
  }
  
}

static void drawStatsPanel(TFT_eSPI& tft){
  tft.fillRoundRect(cx, sy, cw, sh, 6, TFT_BLACK);
  tft.drawRoundRect(cx, sy, cw, sh, 6, TFT_DARKGREY);
  Stats s; memcpy(&s, (const void*)&sIsr, sizeof(Stats));
  int y = sy + 6;
  int xL = cx + 6;
  int xR = cx + cw - 6 - PILL_W_KBPS;
  fixedPill(tft, xL, y, PILL_W_KBPS, "UL " + fmtKbps(sUpKbps),  TFT_DARKGREY, 0x0000, TFT_SILVER);
  fixedPill(tft, xR, y, PILL_W_KBPS, "DL " + fmtKbps(sDownKbps), TFT_DARKGREY, 0x0000, TFT_SILVER);
  y += (PILL_H + 4);
  int x = cx + 6;
  fixedPill(tft, x, y, 84, "Beacon " + String((unsigned long)s.beacon), TFT_DARKGREY, 0x0000, TFT_SILVER);
  x += 84 + PILL_GAP;
  fixedPill(tft, x, y, 84, "Probe " + String((unsigned long)s.probe),   TFT_DARKGREY, 0x0000, TFT_SILVER);
  y += (PILL_H + 4);
  String total = "Total " + String((unsigned long)s.total);
  int TW = 90;
  fixedPill(tft, cx + cw - TW - 6, y, TW, total, TFT_DARKGREY, 0x0000, TFT_SILVER);
}

static void drawFrame(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  W=tft.width(); H=tft.height();
  int marginX=10, marginTop=16, marginBottom=8;
  bx = marginX; by = marginTop + 8; bw = W - marginX*2; bh = H - by - marginBottom;
  tft.setTextSize(1);
  tft.drawRoundRect(bx,   by,   bw,   bh,   8, TFT_DARKGREY);
  tft.drawRoundRect(bx+1, by+1, bw-2, bh-2, 7, TFT_BLACK);
  tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
  cx = bx + 10; cy = by + 8; cw = bw - 20;
  drawHeader(tft);
  gx = cx; gw = cw;
  gy = cy + 42;               
  gh = (bh - (gy - by)) * 3 / 5;
  tft.fillRect(gx, gy, gw, gh, TFT_BLACK);
  sy = gy + gh + 8;
  sh = by + bh - sy - 8;
  tft.fillRoundRect(cx, sy, cw, sh, 6, TFT_BLACK);
  tft.drawRoundRect(cx, sy, cw, sh, 6, TFT_DARKGREY);
  sFrameDrawn = true;
}

void packetScanReset(){
  sFrameDrawn = false; sDirty = true;
  if (sChannel < CH_MIN || sChannel > CH_MAX) sChannel = CH_MIN;
  stopSniffer();
  startSniffer();
}

void packetScanStop(){ stopSniffer(); }
void packetScanDrawScreen(TFT_eSPI& tft){
  if (!sFrameDrawn) { drawFrame(tft); sDirty = true; }

  unsigned long now = millis();
  if (now - sLastSample >= SAMPLE_MS) {
    Stats snap; memcpy(&snap, (const void*)&sIsr, sizeof(Stats));

    uint32_t dPkts = (snap.total >= sPrevTotal) ? (snap.total - sPrevTotal) : 0;
    sPrevTotal = snap.total;
    uint32_t pps = (uint32_t)((1000.0f * dPkts) / (float)SAMPLE_MS);
    if (pps > 20000) pps = 20000;
    sSamples[sSidx] = (uint16_t)pps;
    sSidx = (sSidx + 1) % NSAMPLES;

    uint32_t dUp   = (snap.upBytes   >= sPrevUpBytes)   ? (snap.upBytes   - sPrevUpBytes)   : 0;
    uint32_t dDown = (snap.downBytes >= sPrevDownBytes) ? (snap.downBytes - sPrevDownBytes) : 0;
    sPrevUpBytes   = snap.upBytes;
    sPrevDownBytes = snap.downBytes;
    float secs = (float)SAMPLE_MS / 1000.0f;
    sUpKbps   = (dUp   * 8.0f) / 1000.0f / secs;
    sDownKbps = (dDown * 8.0f) / 1000.0f / secs;

    sLastSample = now;
    sDirty = true;
  }

  if (sDirty) {
    sDirty = false;
    drawHeader(tft);     
    drawTopPills(tft);   
    drawGraph(tft);      
    drawStatsPanel(tft); 
  }
}

void packetScanHandleInput(bool a, bool b, bool c, bool& requestExit){
  requestExit = false;
  if (c) { stopSniffer(); sFrameDrawn=false; requestExit=true; return; }
  if (a) setChannel(sChannel - 1);   
  if (b) setChannel(sChannel + 1);   
}
