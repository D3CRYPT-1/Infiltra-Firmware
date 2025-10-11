#include "ble_google_adv.h"
#include "UserInterface/menus/menu_submenus.h"
#if defined(CONFIG_BT_NIMBLE_ENABLED)
  #include <NimBLEDevice.h>
  using AdvDataT      = NimBLEAdvertisementData;
  using AdvertisingT  = NimBLEAdvertising;
  using UUIDT         = NimBLEUUID;
  #define BLEDEV      NimBLEDevice
#else
  #include <BLEDevice.h>
  #include <BLEUtils.h>
  #include <BLEAdvertising.h>
  using AdvDataT      = BLEAdvertisementData;
  using AdvertisingT  = BLEAdvertising;
  using UUIDT         = BLEUUID;
  #define BLEDEV      BLEDevice
#endif
#include <WiFi.h>
#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;
static bool sFrameDrawn=false;
static bool sDirty=true;          
static bool sAdvOn=false;         
static int  W=0,H=0;
static int  bx=0,by=0,bw=0,bh=0;  
static int  cx=0,cy=0,cw=0;       
static const char* sNames[] = { "Infiltra", "Infiltra-01", "Infiltra-Lab", "Infiltra-Dev" };
static int         sNameIdx = 0;
static UUIDT         sUUID("12345678-1234-5678-9abc-def012345678"); 
static AdvertisingT* sAdv = nullptr;
static uint32_t      sCounter = 0;  
static const int HEADER_H = 16;

static String clipToWidth(TFT_eSPI& tft, const String& s, int maxW){
  if (tft.textWidth(s) <= maxW) return s;
  String out = s;
  while (out.length() && tft.textWidth(out + "...") > maxW) out.remove(out.length()-1);
  return out + "...";
}

static void sectionTitle(TFT_eSPI& tft, int y, const String& title){
  int x = cx, padX = 6, padY = 2;
  int tw = tft.textWidth(title) + padX*2;
  int th = 14;
  tft.fillRoundRect(x, y, tw, th, 4, TFT_DARKGREY);
  tft.drawRoundRect(x, y, tw, th, 4, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(x + padX, y + padY + 1);
  tft.print(title);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static int drawKVRow(TFT_eSPI& tft, int y, const String& key, const String& value){
  tft.setCursor(cx, y);
  tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.print(key);
  tft.setTextColor(TFT_WHITE,  TFT_BLACK); tft.print(value);
  return y + 14;
}

static int drawWrappedValue(TFT_eSPI& tft, int y, const String& key, const String& value, int maxW){
  tft.setCursor(cx, y);
  tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.print(key);
  int keyW = tft.textWidth(key);
  int lineH = 12;
  String remaining = value;
  int vx = cx + keyW;
  int vy = y;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  while (remaining.length()){
    String seg; int n = remaining.length();
    int bestEnd=0, lastSpace=-1;
    for (int i=0;i<n;++i){
      char c = remaining[i];
      String trial = seg + c;
      if (tft.textWidth(trial) > (maxW - keyW)) break;
      seg += c; bestEnd = i+1;
      if (c==' ') lastSpace = i;
    }
    if (bestEnd < n && lastSpace >= 0){ seg = remaining.substring(0,lastSpace); bestEnd = lastSpace+1; }
    if (!seg.length()){ seg = remaining.substring(0,1); bestEnd = 1; }
    tft.setCursor(vx, vy); tft.print(seg);
    remaining.remove(0, bestEnd);
    keyW = 0; vx = cx; vy += lineH;
  }
  return vy + 2;
}
static void lightDivider(TFT_eSPI& tft, int y){
  tft.drawFastHLine(bx+6, y, bw-12, TFT_DARKGREY);
}

static void drawFrame(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  W = tft.width(); H = tft.height();
  int marginX=10, marginTop=16, marginBottom=8;
  bx = marginX; by = marginTop + 8; bw = W - marginX*2; bh = H - by - marginBottom;
  tft.setTextSize(1);
  tft.drawRoundRect(bx,   by,   bw,   bh,   8, TFT_DARKGREY);
  tft.drawRoundRect(bx+1, by+1, bw-2, bh-2, 7, TFT_BLACK);
  tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
  cx = bx + 10; cy = by + 8; cw = bw - 20;
  tft.fillRoundRect(cx, cy, cw, HEADER_H, 3, TFT_DARKGREY);
  tft.drawRoundRect(cx, cy, cw, HEADER_H, 3, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(cx+4, cy+3);
  tft.print("BLE Google Adv (lab)");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  lightDivider(tft, cy + HEADER_H + 4);
}
static void redraw(TFT_eSPI& tft){
  tft.setTextSize(1);
  int statusY = cy + HEADER_H + 8;

  tft.fillRect(bx+3, statusY-2, bw-6, 14, TFT_BLACK);
  {
    tft.setCursor(cx, statusY);
    tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.print("Status: ");
    tft.setTextColor(TFT_WHITE,  TFT_BLACK); tft.print(sAdvOn ? "Advertising" : "Stopped");
  }

  int topY = statusY + 16;
  int bottomReserved = 8; 
  int usableH = (by + bh) - bottomReserved - topY;
  tft.fillRect(bx+3, topY, bw-6, usableH, TFT_BLACK);
  int y = topY;
  sectionTitle(tft, y, "Device");
  y += 18;
  y = drawKVRow(tft, y, "Name:   ", clipToWidth(tft, String(sNames[sNameIdx]), cw - tft.textWidth("Name:   ")));
  y = drawKVRow(tft, y, "Mode:   ", String("Neutral (custom UUID)"));
  y += 6; lightDivider(tft, y); y += 8;
  sectionTitle(tft, y, "Service");
  y += 18;
  y = drawWrappedValue(tft, y, "UUID:   ", " " + String(sUUID.toString().c_str()), cw);
  y += 6; lightDivider(tft, y); y += 8;
  sectionTitle(tft, y, "Payload");
  y += 18;
  {
    char buf[64];
    snprintf(buf, sizeof(buf), " INFT + #%lu (8 bytes)", (unsigned long)sCounter);
    y = drawWrappedValue(tft, y, "Data:   ", String(buf), cw);
  }
}

static void buildAdv(AdvDataT& adv, AdvDataT& resp){
  adv = AdvDataT();
  resp = AdvDataT();
  adv.setFlags(0x06);
  adv.setName(sNames[sNameIdx]);
  resp.setName(sNames[sNameIdx]);
  adv.setCompleteServices(sUUID);
  uint8_t payload[8];
  payload[0]='I'; payload[1]='N'; payload[2]='F'; payload[3]='T';
  uint32_t c = sCounter;
  payload[4]= (uint8_t)(c & 0xFF);
  payload[5]= (uint8_t)((c>>8) & 0xFF);
  payload[6]= (uint8_t)((c>>16)& 0xFF);
  payload[7]= (uint8_t)((c>>24)& 0xFF);
  adv.setServiceData(sUUID, std::string((char*)payload, sizeof(payload)));
}

static void advStart(){
  WiFi.mode(WIFI_OFF);  
  delay(10);
  if (!sAdv){
    BLEDEV::init(sNames[sNameIdx]);          
    sAdv = BLEDEV::getAdvertising();
  }
  AdvDataT advData, scanResp;
  buildAdv(advData, scanResp);
  sAdv->setAdvertisementData(advData);
  sAdv->setScanResponseData(scanResp);
  sAdv->start();
  sAdvOn = true;
}

static void advStop(){
  if (sAdv) sAdv->stop();
  sAdvOn = false;
}

void bleGoogleAdvReset(){
  sFrameDrawn = false;
  sDirty      = true;
  sAdvOn      = false;
  sCounter    = 0;
}

void bleGoogleAdvDrawScreen(TFT_eSPI& tft){
  if (!sFrameDrawn){
    drawFrame(tft);
    sFrameDrawn = true;
    sDirty = true;
  }
  if (sDirty){
    sDirty = false;
    redraw(tft);      
  }
}

void bleGoogleAdvHandleInput(bool a, bool b, bool c, bool& requestExit){
  requestExit = false;

  if (c){
    advStop();
    sFrameDrawn = false;   
    requestExit = true;
    return;
  }

  if (a){
    if (sAdvOn){
      advStop();
    } else {
      sCounter++;          
      advStart();
    }
    sDirty = true;         
  }

  if (b){
    sNameIdx = (sNameIdx + 1) % (int)(sizeof(sNames)/sizeof(sNames[0]));
    if (sAdvOn){
      advStop();
      advStart();
    }
    sDirty = true;
  }
}

void bleGoogleAdvStop(){
  advStop();
}
