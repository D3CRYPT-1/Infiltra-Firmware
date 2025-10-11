#include "InfiChat.h"
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <string.h>
#include <SPI.h>
#include <LoRa.h>

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
  static constexpr uint8_t ROT_TOP = 1;   
#else
  static constexpr uint8_t ROT_TOP = 3;   
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 3) & 0x3;

extern void drawOptionsLayerBackground(TFT_eSPI& tft);
static constexpr uint16_t COL_BG        = TFT_BLACK;
static constexpr uint16_t COL_HDR_BG    = TFT_DARKGREY;
static constexpr uint16_t COL_HDR_FG    = TFT_WHITE;
static constexpr uint16_t COL_TXT       = TFT_SILVER;
static constexpr uint16_t COL_TXT_HI    = TFT_WHITE;
static constexpr uint16_t COL_ACCENT    = TFT_WHITE;
static constexpr int HEADER_H        = 16;
static constexpr int LINE_H          = 12;
static constexpr int MAX_TERM_LINES  = 120;
static constexpr int OUTER_RADIUS    = 8;
static constexpr int KBD_CELL_H      = 14;
static constexpr int KBD_PAD         = 4;
static constexpr int KBD_ROWS        = 3;
static constexpr int KBD_HEIGHT      = (KBD_ROWS * KBD_CELL_H + KBD_PAD);
static int W=0, H=0;
static int bx=0, by=0, bw=0, bh=0;
static int cx=0, cy=0, cw=0;
static int sTermMaxW = 0;
static bool sInit         = false;
static bool sBgDrawn      = false;
static bool sHeaderDirty  = true;
static bool sTermDirty    = true;
static bool sKbdDirty     = true;
static bool sPromptDirty  = true;
static TFT_eSPI* sTft = nullptr;
static String sTermLines[MAX_TERM_LINES];
static int    sTermCount = 0;
struct Key { const char* label; char ch; };
static const Key row1[10] = {{"q",'q'},{"w",'w'},{"e",'e'},{"r",'r'},{"t",'t'},{"y",'y'},{"u",'u'},{"i",'i'},{"o",'o'},{"p",'p'}};
static const Key row2[10] = {{"a",'a'},{"s",'s'},{"d",'d'},{"f",'f'},{"g",'g'},{"h",'h'},{"j",'j'},{"k",'k'},{"l",'l'},{"'",'\''}};
static const Key row3[10] = {{"<-",0},{"z",'z'},{"x",'x'},{"c",'c'},{"v",'v'},{"--",' '},{"b",'b'},{"n",'n'},{"m",'m'},{">",0}};
static int  kbIndex = 0;       
static bool sShowKbd = false;
static String sCompose;
static uint32_t sBlinkLast = 0;
static bool     sBlinkOn   = true;
static constexpr uint32_t BLINK_MS = 500;
static int sCurX=0, sCurY=0, sCurW=0;
enum Transport { TR_ESP, TR_LORA };
static Transport sTransport = TR_ESP;
static bool   sEspNowReady   = false;
static uint8_t CHAT_WIFI_CH = 1;
static uint8_t kBroadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static bool   sLoRaReady = false;
static bool   sLoRaTriedInit = false; 
static long   sLoRaFreq  = 915E6;

#ifndef LORA_SS
  #define LORA_SS   -1
#endif
#ifndef LORA_RST
  #define LORA_RST  -1
#endif
#ifndef LORA_DIO0
  #define LORA_DIO0 -1
#endif
#if defined(LORA_SCK) && defined(LORA_MISO) && defined(LORA_MOSI)
  #define HAS_LORA_SPI_PINS 1
#else
  #define HAS_LORA_SPI_PINS 0
#endif

static constexpr int RXQ_MAX = 12;
static String  sRxQ[RXQ_MAX];
static volatile int sRxHead = 0;
static volatile int sRxTail = 0;

static inline bool rxq_push(const String& s) {
  int next = (sRxHead + 1) % RXQ_MAX;
  if (next == sRxTail) return false;
  sRxQ[sRxHead] = s;
  sRxHead = next;
  return true;
}
static inline bool rxq_pop(String& out) {
  if (sRxTail == sRxHead) return false;
  out = sRxQ[sRxTail];
  sRxTail = (sRxTail + 1) % RXQ_MAX;
  return true;
}

enum Mode { MODE_PICK, MODE_LORA_BAND, MODE_BOOT, MODE_CHAT };
static Mode sMode = MODE_PICK;
static int  sPickIdx = 0; 
static int  sBandIdx = 0; 
static uint32_t sLastTick = 0;
static uint32_t sBootLines = 0;
static const char SPN[4] = {'|','-','\\','/'};
static bool sBootUsingLoRa = false;

static void addTermRaw(const String& line) {
  if (sTermCount < MAX_TERM_LINES) sTermLines[sTermCount++] = line;
  else {
    for (int i=1;i<MAX_TERM_LINES;++i) sTermLines[i-1] = sTermLines[i];
    sTermLines[MAX_TERM_LINES-1] = line;
  }
  sTermDirty = true;
}

static void addTermWrapped(const String& line) {
  if (!sTft) return;
  if (sTermMaxW <= 0) { addTermRaw(line); return; }
  int n = line.length();
  int start = 0;
  while (start < n) {
    int bestEnd = start, lastSpace = -1;
    String seg;
    for (int i = start; i < n; ++i) {
      char c = line.charAt(i);
      String trial = seg + c;
      if (sTft->textWidth(trial) > sTermMaxW) break;
      seg += c; bestEnd = i + 1;
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
    if (seg.length()) addTermRaw(seg);
    start = bestEnd;
  }
}

static void macToString(const uint8_t* mac, char* out) {
  sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x",
          mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

static void drawBackground(TFT_eSPI& tft) {
  if (sBgDrawn) return;
  tft.setRotation(ROT_ALT);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_TOP);
  sBgDrawn = true;
  tft.drawRoundRect(bx, by, bw, bh, OUTER_RADIUS, COL_HDR_BG);
  tft.drawRoundRect(bx+1, by+1, bw-2, bh-2, OUTER_RADIUS-1, TFT_BLACK);
}

static void geometrySetup(TFT_eSPI& tft) {
  sTft = &tft;
  tft.setRotation(ROT_TOP);
  W = tft.width(); H = tft.height();
  const int marginX = 10, marginTop = 16, marginBottom = 8;
  bx = marginX; by = marginTop + 12;
  bw = W - marginX * 2; bh = H - by - marginBottom;
  cx = bx + 10; cy = by + 8; cw = bw - 20;
  tft.setTextFont(1);
  tft.setTextSize(1);
  sTermMaxW = cw - 2;
  sHeaderDirty = sTermDirty = sKbdDirty = true;
  sPromptDirty = true;
}

static void drawHeader(TFT_eSPI& tft, const String& left, const String& right) {
  if (!sHeaderDirty) return;
  tft.setTextFont(1); tft.setTextSize(1);
  tft.fillRoundRect(cx, cy, cw, HEADER_H, 3, COL_HDR_BG);
  tft.drawRoundRect (cx, cy, cw, HEADER_H, 3, TFT_BLACK);
  tft.setTextColor(COL_HDR_FG, COL_HDR_BG);
  tft.setCursor(cx + 4, cy + 3); tft.print(left);
  if (right.length()) {
    int w = tft.textWidth(right);
    tft.setCursor(cx + cw - w - 4, cy + 3); tft.print(right);
  }
  tft.setTextColor(COL_TXT, COL_BG);
  sHeaderDirty = false;
}

static int termTopY() { return cy + HEADER_H + 2; }
static int kbdHeight() { return sShowKbd ? KBD_HEIGHT : 0; }
static int promptY() {
  const int kbTop  = by + bh - 8 - kbdHeight();
  return kbTop - LINE_H;
}

static void drawPromptLine(TFT_eSPI& tft) {
  if (!(sMode == MODE_CHAT && sShowKbd)) return;
  const int pY = promptY();
  tft.setTextFont(1); tft.setTextSize(1);
  tft.setTextColor(COL_TXT_HI, COL_BG);
  tft.fillRect(cx, pY, cw, LINE_H, COL_BG);
  tft.setCursor(cx, pY); tft.print("> "); tft.print(sCompose);
  sCurW = sTft->textWidth("|");
  sCurX = cx + sTft->textWidth(String("> ") + sCompose);
  sCurY = pY;
  if (sBlinkOn) { tft.setCursor(sCurX, sCurY); tft.print("|"); }
  else          { tft.fillRect(sCurX, sCurY, sCurW, LINE_H, COL_BG); }
}

static void blinkCursorIfNeeded(TFT_eSPI& tft) {
  if (!(sMode == MODE_CHAT && sShowKbd)) return;
  uint32_t now = millis();
  if (now - sBlinkLast < BLINK_MS) return;
  sBlinkLast = now; sBlinkOn = !sBlinkOn;
  if (sBlinkOn) {
    tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextColor(COL_TXT_HI, COL_BG);
    tft.setCursor(sCurX, sCurY); tft.print("|");
  } else {
    tft.fillRect(sCurX, sCurY, sCurW, LINE_H, COL_BG);
  }
}

static void drawTerminal(TFT_eSPI& tft) {
  if (!sTermDirty) return;
  tft.setTextFont(1); tft.setTextSize(1);
  const int top    = termTopY();
  const int pY     = promptY();
  const int availH = max(0, pY - top);
  const int maxVis = max(5, availH / LINE_H);
  const int total  = sTermCount;
  const int start  = (total > maxVis) ? (total - maxVis) : 0;
  tft.fillRect(cx, top, cw, maxVis * LINE_H, COL_BG);
  tft.setTextColor(COL_TXT, COL_BG);
  int y = top;
  for (int i=0; i<maxVis && (start + i) < total; ++i) {
    tft.setCursor(cx, y); tft.print(sTermLines[start + i]); y += LINE_H;
  }
  drawPromptLine(tft);
  sPromptDirty = false;
  sTermDirty = false;
}

static void drawKeyboard(TFT_eSPI& tft) {
  if (!sShowKbd || !sKbdDirty) return;
  tft.setTextFont(1); tft.setTextSize(1);
  const int totalH = kbdHeight();
  const int kbTop  = by + bh - 8 - totalH;
  tft.fillRect(cx, kbTop, cw, totalH, COL_BG);
  const int gridTop = kbTop + 2;
  const int cellW   = cw / 10;
  const int cellH   = KBD_CELL_H;
  auto drawRow = [&](const Key* row, int y, int baseIdx) {
    for (int c=0;c<10;c++) {
      int idx = baseIdx + c;
      int x   = cx + c*cellW;
      bool sel = (idx == kbIndex);
      if (sel) { sTft->fillRoundRect(x, y, cellW-1, cellH-1, 3, COL_ACCENT);
                 sTft->setTextColor(TFT_BLACK, COL_ACCENT); }
      else     { sTft->drawRoundRect(x, y, cellW-1, cellH-1, 3, COL_ACCENT);
                 sTft->setTextColor(COL_TXT, COL_BG); }
      int w = sTft->textWidth(row[c].label);
      int tx = x + (cellW - w)/2; int ty = y + 3;
      sTft->setCursor(tx, ty); sTft->print(row[c].label);
    }
  };
  drawRow(row1, gridTop + 0*cellH,  0);
  drawRow(row2, gridTop + 1*cellH, 10);
  drawRow(row3, gridTop + 2*cellH, 20);
  sKbdDirty = false;
  drawPromptLine(tft);
  sPromptDirty = false;
}

static void kbNext(){ kbIndex = (kbIndex + 1) % 30; sKbdDirty = true; }
static void kbPrev(){ kbIndex = (kbIndex + 29) % 30; sKbdDirty = true; }
static const char* kbLabel(){
  if (kbIndex < 10) return row1[kbIndex].label;
  if (kbIndex < 20) return row2[kbIndex-10].label;
  return row3[kbIndex-20].label;
}
static char kbChar(){
  if (kbIndex < 10) return row1[kbIndex].ch;
  if (kbIndex < 20) return row2[kbIndex-10].ch;
  return row3[kbIndex-20].ch;
}

static void onSendCB(const uint8_t*, esp_now_send_status_t){ }
static void onRecvCB(const uint8_t* mac, const uint8_t* data, int len){
  if (!data || len <= 0 || !mac) return;
  char macStr[18]; macToString(mac, macStr);
  String msg; msg.reserve(len + 24);
  msg += "<"; msg += macStr; msg += "> ";
  for (int i=0;i<len;i++) { char c = (char)data[i];
    if ((c >= 32 && c <= 126) || c=='\t') msg += c; }
  rxq_push(msg);
}

static void espNowInit() {
  if (sEspNowReady) return;
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(CHAT_WIFI_CH, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() == ESP_OK) {
    esp_now_register_send_cb(onSendCB);
    esp_now_register_recv_cb(onRecvCB);
    esp_now_peer_info_t peer{}; memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, kBroadcast, 6);
    peer.channel = CHAT_WIFI_CH; peer.encrypt = false;
    esp_err_t pe = esp_now_add_peer(&peer);
    if (pe != ESP_OK && pe != ESP_ERR_ESPNOW_EXIST) addTermWrapped("[error] add_peer failed");
    sEspNowReady = true;
  } else {
    addTermWrapped("[error] esp_now_init failed");
  }
}

static void loRaSetupPins() {
#if HAS_LORA_SPI_PINS
  SPI.end(); 
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, (LORA_SS>=0 ? LORA_SS : SS));
  delay(2);
#endif
  if (LORA_SS >= 0 && LORA_RST >= 0 && LORA_DIO0 >= 0) {
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  }
}

static void loRaInit() {
  if (sLoRaReady) return;
  loRaSetupPins();
  delay(5); yield();
  bool ok = LoRa.begin(sLoRaFreq);
  if (!ok) {
    addTermWrapped("[error] LoRa begin failed (pins/frequency/power?)");
    sLoRaReady = false;
    return;
  }
  LoRa.setTxPower(17);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.enableCrc();
  delay(10); yield();
  sLoRaReady = true;
}

static void loRaPollRx() {
  if (!sLoRaReady) return;
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;
  String in; in.reserve(packetSize + 8);
  while (LoRa.available()) in += (char)LoRa.read();
  int rssi = LoRa.packetRssi(); float snr = LoRa.packetSnr();
  String line; line.reserve(in.length() + 32);
  line += "<LoRa rssi:"; line += String(rssi);
  line += " snr:"; line += String(snr,1); line += "> ";
  line += in;
  rxq_push(line);
}

static void loRaSend(const String& s) {
  if (!sLoRaReady || s.length()==0) return;
  LoRa.beginPacket();
  LoRa.write((const uint8_t*)s.c_str(), s.length());
  LoRa.endPacket(true);
}

static void renderPickToTerminal() {
  sTermCount = 0;
  addTermRaw("");
  addTermRaw(String(sPickIdx==0?"> ":"  ") + "[ESP-NOW]");
  addTermRaw(String(sPickIdx==1?"> ":"  ") + "[LoRa]");
  addTermRaw("");
}

static void renderBandPickTerminal() {
  sTermCount = 0;
  addTermRaw("Select LoRa region:");
  addTermRaw("");
  addTermRaw(String(sBandIdx==0?"> ":"  ") + "US-915 (902–928 MHz)");
  addTermRaw(String(sBandIdx==1?"> ":"  ") + "EU-868 (863–870 MHz)");
  addTermRaw("");
  addTermRaw("A=Select  B=Toggle  C=Back");
}

static void bootTickIfNeeded() {
  if (sMode != MODE_BOOT) return;
  if (sBootUsingLoRa && !sLoRaReady && !sLoRaTriedInit) {
    sLoRaTriedInit = true;
    loRaInit();
    if (!sLoRaReady) {
      addTermWrapped("[fatal] LoRa init failed. Returning to picker.");
      sMode = MODE_PICK;
      renderPickToTerminal();
      sHeaderDirty = sTermDirty = true;
      return;
    }
  }

  uint32_t now = millis();
  if (now - sLastTick < 180) return;
  sLastTick = now;
  char sp = SPN[(sBootLines) & 3];
  String msg;
  if (sBootUsingLoRa) {
    switch (sBootLines % 4) {
      case 0: msg = String("[") + sp + "] root@infiltra:/# lora init @" + String((long)(sLoRaFreq/1000000)) + "MHz"; break;
      case 1: msg = String("[") + sp + "] root@infiltra:/# lora set sf=7 bw=125k crc=on"; break;
      case 2: msg = String("[") + sp + "] root@infiltra:/# chatd start --mode=lora"; break;
      default:msg = String("[") + sp + "] root@infiltra:/# peers listen"; break;
    }
  } else {
    switch (sBootLines % 4) {
      case 0: msg = String("[") + sp + "] root@infiltra:/# espnow init"; break;
      case 1: msg = String("[") + sp + "] root@infiltra:/# chat bind --iface wlan0"; break;
      case 2: msg = String("[") + sp + "] root@infiltra:/# peers discover --timeout=150ms"; break;
      default:msg = String("[") + sp + "] root@infiltra:/# chatd start --mode=broadcast"; break;
    }
  }
  sBootLines++;
  addTermWrapped(msg);
  if (sBootLines >= 10) {
    addTermRaw("");
    addTermRaw("ready.");
    sMode = MODE_CHAT;
    sHeaderDirty = sTermDirty = true;
  } else {
    sTermDirty = true;
  }
}

void infiChatReset() {
  sInit = false; sBgDrawn = false;
  sHeaderDirty = sTermDirty = sKbdDirty = true;
  sPromptDirty = true; sShowKbd = false;
  sCompose = ""; sTermCount = 0;
  sMode = MODE_PICK; sPickIdx = 0; sBandIdx = 0;
  sLastTick = 0; sBootLines = 0;
  sBlinkLast = 0; sBlinkOn = true; sCurX = sCurY = sCurW = 0;
  sTransport = TR_ESP;
  sBootUsingLoRa = false;
  sLoRaReady = false; sLoRaTriedInit = false;
  sRxHead = sRxTail = 0;
}

void infiChatDrawScreen(TFT_eSPI& tft) {
  if (!sInit) {
    geometrySetup(tft);
    drawBackground(tft);
    drawHeader(tft, "infiltra terminal — /root", "");
    renderPickToTerminal();
    sInit = true;
  }
  loRaPollRx();
  String incoming;
  while (rxq_pop(incoming)) addTermWrapped(incoming);
  bootTickIfNeeded();
  switch (sMode) {
    case MODE_PICK:       drawHeader(tft, "infiltra terminal — /root", "pick"); break;
    case MODE_LORA_BAND:  drawHeader(tft, "infiltra terminal — /lora", "region"); break;
    case MODE_BOOT:       drawHeader(tft, sBootUsingLoRa ? "infiltra terminal — /lora" : "infiltra terminal — /chat", "starting…"); break;
    case MODE_CHAT:       drawHeader(tft, sTransport==TR_LORA ? "infiltra terminal — /lora" : "infiltra terminal — /chat", sShowKbd ? "typing…" : ""); break;
  }
  if (sTermDirty)  drawTerminal(tft);
  if (sKbdDirty)   drawKeyboard(tft);
  if (sPromptDirty){ drawPromptLine(tft); sPromptDirty = false; }
  blinkCursorIfNeeded(tft);
}

void infiChatHandleInput(bool a, bool b, bool c, bool& exitReq) {
  exitReq = false;
  if (sShowKbd) {
    if (b) { kbNext(); }
    if (c) { kbPrev(); }
    if (a) {
      const char* lab = kbLabel();
      if (strcmp(lab,"<-")==0) {
        if (sCompose.length()) sCompose.remove(sCompose.length()-1);
        sPromptDirty = true;
      } else if (strcmp(lab,">")==0) {
        if (sCompose.length()) {
          addTermRaw(String("> ") + sCompose);
          if (sTransport == TR_ESP && sEspNowReady) {
            esp_now_send(kBroadcast, (const uint8_t*)sCompose.c_str(), sCompose.length());
          } else if (sTransport == TR_LORA && sLoRaReady) {
            loRaSend(sCompose);
          }
          sCompose = "";
        }
        sShowKbd = false;
        sHeaderDirty = true; sTermDirty = true; sKbdDirty = true; sPromptDirty = true;
      } else if (strcmp(lab,"--")==0) {
        sCompose += ' '; sPromptDirty = true;
      } else {
        char ch = kbChar(); if (ch) { sCompose += ch; sPromptDirty = true; }
      }
    }
    return;
  }

  if (sMode == MODE_PICK) {
    if (b) { sPickIdx ^= 1; renderPickToTerminal(); }
    if (a) {
      if (sPickIdx == 0) { 
        sTransport = TR_ESP; sBootUsingLoRa = false;
        espNowInit();
        sMode = MODE_BOOT; sLastTick = 0; sBootLines = 0; sTermCount = 0;
        addTermRaw("root@infiltra:/# starting chat …");
        sHeaderDirty = sTermDirty = true;
      } else {             
        sMode = MODE_LORA_BAND;
        renderBandPickTerminal();
        sHeaderDirty = sTermDirty = true;
      }
    }
    if (c) { exitReq = true; }
    return;
  }

  if (sMode == MODE_LORA_BAND) {
    if (b) { sBandIdx ^= 1; renderBandPickTerminal(); return; }
    if (c) { sMode = MODE_PICK; renderPickToTerminal(); sHeaderDirty = sTermDirty = true; return; }
    if (a) {
      sLoRaFreq = (sBandIdx == 0) ? 915E6 : 868E6;
      sTransport = TR_LORA; sBootUsingLoRa = true;
      sLoRaReady = false; sLoRaTriedInit = false; 
      sMode = MODE_BOOT; sLastTick = 0; sBootLines = 0; sTermCount = 0;
      addTermRaw("root@infiltra:/# starting LoRa …");
      sHeaderDirty = sTermDirty = true;
      return;
    }
  }

  if (sMode == MODE_BOOT) {
    if (c) { exitReq = true; return; }
    if (a) {
      if ((sTransport==TR_ESP && sEspNowReady) || (sTransport==TR_LORA && sLoRaReady)) {
        sMode = MODE_CHAT; sHeaderDirty = sTermDirty = true;
      }
    }
    return;
  }
  if (a) { sShowKbd = true; sKbdDirty = true; sPromptDirty = true; sHeaderDirty = true; return; }
  if (c) { exitReq = true; return; }
  if (b) {
  }
}
void infiChatStop() {
}
