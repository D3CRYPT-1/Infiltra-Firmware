#include <Arduino.h>
#include "buttons.h"

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
  #define ROT_TOP 4
#elif defined(M5STICK_C_PLUS_2)
  #include <M5StickCPlus2.h>
  #define ROT_TOP 2
#elif defined(M5STICK_C_PLUS_1_1)
  #define ROT_TOP 2
#else
  #include <M5StickCPlus2.h>
  #define ROT_TOP 2
#endif

#include "././UserInterface/bitmaps/menu_bitmaps.h"
#include "././UserInterface/menus/menu_submenus.h"
#include "././UserInterface/menus/submenu_options.h"
#include "././UserInterface/menus/menu_enums.h"
#include "././Modules/Functions/Settings/battery_saver.h"
#include "././Modules/Functions/Settings/brightness.h"
#include "././Modules/Functions/Settings/time.h"
#include "././Modules/Functions/Extras/stopwatch.h"
#include "././Modules/Functions/IR/bgone.h"
#include "././Modules/Functions/IR/ir_read.h"
#include "././Modules/Functions/Wifi/wifi_scan.h"
#include "././Modules/Functions/Wifi/packet_scan.h"
#include "././Modules/Functions/Files/integrated_files.h"
#include "././Modules/Functions/RpiConnect/rpi_connect_info.h"
#include "././Modules/Functions/Files/web_files.h"
#include "././Modules/Functions/Bluetooth/ble_scan.h"
#include "././Modules/Functions/Bluetooth/ble_google_adv.h"
#include "././Modules/Functions/Rf/FindFq.h"
#include "././Modules/Functions/BadUsb/monitorPc.h"
#include "././Modules/Core/User Config/Passlock.h"
#include "././Modules/Functions/Extras/InfiChat.h"
#include "././Modules/Functions/Extras/Credits.h"
#include "././Modules/Functions/Settings/pin_config.h"
#include "././Modules/Functions/NFC/nfc_reader.h"
#include <SPIFFS.h>

#ifndef BTN_A_PIN
#define BTN_A_PIN 37
#endif
#ifndef BTN_B_PIN
#define BTN_B_PIN 39
#endif
#ifndef BTN_C_PIN
#define BTN_C_PIN 35
#endif
#if defined(M5CARDPUTER)
#ifndef KEY_ENTER
#define KEY_ENTER 0x28
#endif
#ifndef KEY_SEMICOLON
#define KEY_SEMICOLON 0x33
#endif
#ifndef KEY_DOT
#define KEY_DOT 0x37
#endif
#ifndef KEY_BACKTICK
#define KEY_BACKTICK 0x35
#endif
#endif

static bool sAEdge=false, sBEdge=false, sCEdge=false, sExitEdge=false;
static bool sInBLEScan=false;
static bool sInBLEGoogleAdv=false;
static bool sInBrightness=false;
static bool sInBatterySaver=false;
static bool sInTime=false;
static bool sInIntegrated=false;
static bool sInRpiInfo=false;
static bool sInWebFiles=false;
static bool sInInfiChat=false;
static bool sInCustomIr=false;
static bool sInCredits=false;
static bool sInFindFq=false;
static bool sInMonitorPc=false;
static bool sInPinConfig=false;
static bool sInNfcReader=false;
static bool sSpiffsBegun=false;
static TFT_eSPI* sTft=nullptr;

void initButtons() {
#if defined(M5CARDPUTER)
#elif defined(M5STICK_C_PLUS_2)
  if (BTN_C_PIN >= 0) pinMode(BTN_C_PIN, INPUT_PULLUP);
#elif defined(M5STICK_C_PLUS_1_1)
  pinMode(BTN_A_PIN, INPUT_PULLUP);
  pinMode(BTN_B_PIN, INPUT_PULLUP);
  if (BTN_C_PIN >= 0) pinMode(BTN_C_PIN, INPUT_PULLUP); 
#else
  if (BTN_C_PIN >= 0) pinMode(BTN_C_PIN, INPUT_PULLUP);
#endif
  if(!sSpiffsBegun){ SPIFFS.begin(true); sSpiffsBegun=true; }
}

void updateButtons() {
#if defined(M5CARDPUTER)
  M5Cardputer.update();
  static bool lastARaw=false, lastBRaw=false, lastCRaw=false, lastExitRaw=false;
  bool aRaw =
      M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) ||
      M5Cardputer.Keyboard.isKeyPressed('\r');
  bool bRaw =
      M5Cardputer.Keyboard.isKeyPressed(';') ||
      M5Cardputer.Keyboard.isKeyPressed(KEY_SEMICOLON);
  bool cRaw =
      M5Cardputer.Keyboard.isKeyPressed('.') ||
      M5Cardputer.Keyboard.isKeyPressed(KEY_DOT);
  bool exitRaw =
      M5Cardputer.Keyboard.isKeyPressed('`') ||
      M5Cardputer.Keyboard.isKeyPressed(KEY_BACKTICK);
  sAEdge    = aRaw   && !lastARaw;
  sBEdge    = bRaw   && !lastBRaw;
  sCEdge    = cRaw   && !lastCRaw;
  sExitEdge = exitRaw&& !lastExitRaw;
  lastARaw = aRaw;
  lastBRaw = bRaw;
  lastCRaw = cRaw;
  lastExitRaw = exitRaw;
#elif defined(M5STICK_C_PLUS_2)
  M5.update();
  sAEdge = M5.BtnA.wasPressed();
  sBEdge = M5.BtnB.wasPressed();
  static bool lastCRaw=false;
  bool cRaw = (BTN_C_PIN >= 0) ? !digitalRead(BTN_C_PIN) : false; 
  sCEdge = cRaw && !lastCRaw;
  lastCRaw = cRaw;
  sExitEdge = false;
#elif defined(M5STICK_C_PLUS_1_1)
  static bool lastA=false, lastB=false, lastC=false;
  bool a = !digitalRead(BTN_A_PIN);
  bool b = !digitalRead(BTN_B_PIN);
  bool c = (BTN_C_PIN >= 0) ? !digitalRead(BTN_C_PIN) : false;
  sAEdge = a && !lastA;
  sBEdge = b && !lastB;
  sCEdge = c && !lastC;
  lastA = a; lastB = b; lastC = c;
  sExitEdge = false;
#else
  M5.update();
  sAEdge = M5.BtnA.wasPressed();
  sBEdge = M5.BtnB.wasPressed();
  static bool lastCRaw=false;
  bool cRaw = (BTN_C_PIN >= 0) ? !digitalRead(BTN_C_PIN) : false;
  sCEdge = cRaw && !lastCRaw;
  lastCRaw = cRaw;
  sExitEdge = false;
#endif

  // Update screensaver
  timeUpdateScreensaver();
}

bool btnAPressed(){ return sAEdge; }
bool btnBPressed(){ return sBEdge; }
bool btnCPressed(){ return sCEdge; }
static bool btnExitSpecialPressed(){ return sExitEdge; }
void finalizeButtons() { }

static String norm(String s){
  s.toLowerCase();
  String r; r.reserve(s.length());
  for (size_t i=0;i<s.length();++i){
    char c = s[i];
    if ((c>='a'&&c<='z')||(c>='0'&&c<='9')) r += c;
  }
  return r;
}
static bool labelIsWifiScan(const String& label){
  String n = norm(label);
  if (n.indexOf("wifiscan")>=0) return true;
  if (n.indexOf("wifiscanner")>=0) return true;
  if (n.indexOf("wifinetworkscan")>=0) return true;
  return (n.indexOf("wifi")>=0 && (n.indexOf("scan")>=0 || n.indexOf("scanner")>=0));
}
static bool labelIsPacketScan(const String& label){
  String n = norm(label);
  if (n.indexOf("packetscan")>=0 || n.indexOf("packetscanner")>=0) return true;
  if (n.indexOf("pktscan")>=0 || n.indexOf("sniffer")>=0) return true;
  return ( (n.indexOf("packet")>=0 || n.indexOf("pkt")>=0 )
        && (n.indexOf("scan")>=0 || n.indexOf("sniff")>=0) );
}
static bool labelIsBLEScan(const String& label){
  String n = norm(label);
  bool hasBle = (n.indexOf("ble")>=0 || n.indexOf("bluetooth")>=0);
  bool hasScan = (n.indexOf("scan")>=0 || n.indexOf("scanner")>=0 || n.indexOf("discover")>=0);
  return hasBle && hasScan;
}
static bool labelIsBLEGoogleAdv(const String& label){
  String n = norm(label);
  if (n.indexOf("google")>=0) return true;
  if ((n.indexOf("ble")>=0 || n.indexOf("bluetooth")>=0) &&
      (n.indexOf("adv")>=0 || n.indexOf("advert")>=0 || n.indexOf("advertise")>=0))
    return true;
  return false;
}
static bool labelIsBrightness(const String& label){
  String n = norm(label);
  return (n.indexOf("bright")>=0 || n.indexOf("display")>=0);
}
static bool labelIsBatterySaver(const String& label){
  String n = norm(label);
  return (n.indexOf("bat")>=0 && n.indexOf("saver")>=0);
}
static bool labelIsTime(const String& label){
  String n = norm(label);
  return (n.indexOf("time")>=0);
}
static bool labelIsIntegrated(const String& label){
  String n = norm(label);
  return (n.indexOf("integrated")>=0);
}
static bool labelIsRpiInfo(const String& label){
  String n = norm(label);
  if (n == "info") return true;
  bool hasInfo = (n.indexOf("info") >= 0 || n.indexOf("about") >= 0 || n.indexOf("help") >= 0);
  bool hasRpi  = (n.indexOf("rpi") >= 0 || n.indexOf("raspberrypi") >= 0 || n.indexOf("raspberry") >= 0 || n.indexOf("pi") >= 0);
  bool hasConn = (n.indexOf("connect") >= 0);
  return hasInfo && (hasRpi || hasConn);
}
static bool labelIsPasslock(const String& label){
  String n = norm(label);
  return (n.indexOf("passlock")>=0 || n.indexOf("passkey")>=0 || n.indexOf("password")>=0);
}
static bool labelIsInfiChat(const String& label){
  String n = norm(label);
  if (n.indexOf("infichat")>=0) return true;
  if (n.indexOf("chat")>=0)     return true;
  return false;
}
static bool labelIsCustomIr(const String& label){
  String n = norm(label);
  if(n.indexOf("custom")>=0 && (n.indexOf("ir")>=0||n.indexOf("remote")>=0)) return true;
  return false;
}
static bool labelIsCredits(const String& label){
  String n = norm(label);
  if (n.indexOf("credits")>=0) return true;
  if (n.indexOf("acknowledgments")>=0) return true;
  if (n.indexOf("about")>=0) return true;
  if (n.indexOf("team")>=0) return true;
  return false;
}
static bool labelIsPinConfig(const String& label){
  String n = norm(label);
  if (n.indexOf("configpins")>=0) return true;
  if (n.indexOf("pinconfig")>=0) return true;
  if (n.indexOf("pins")>=0 && n.indexOf("config")>=0) return true;
  return false;
}
static bool labelIsNfcReader(const String& label){
  String n = norm(label);
  if (n.indexOf("read") >= 0 && n.indexOf("1356") >= 0) return true;
  if (n.indexOf("read") >= 0 && n.indexOf("mhz") >= 0) return true;
  if (n.indexOf("nfc") >= 0 && n.indexOf("read") >= 0) return true;
  return false;
}

static void openInfiChatFromHere(TFT_eSPI* tft) {
  tft->fillScreen(TFT_BLACK);
#if defined(M5CARDPUTER)
  drawOptionsLayerBackground(*tft);
#endif
  infiChatReset();
  infiChatDrawScreen(*tft);
  sInInfiChat = true;
}

static void handleSubmenuAction(
  MenuState currentMenu,
  int idx,
  TFT_eSPI* tft,
  bool& inStopwatch,
  bool& inOptionScreen,
  bool& inBGone,
  bool& inIRRead,
  bool& inWiFiScan,
  bool& inPacketScan
) {
  if (idx > 0) {
    String label = getSubmenuOptionText();
    if (labelIsInfiChat(label)) {
      openInfiChatFromHere(tft);
      return;
    }
  }

  if (currentMenu == EXTRAS_SUBMENU && idx == 1) {
    resetStopwatch();
    drawStopwatchScreen(*tft);
    inStopwatch = true;
    return;
  }
  if (currentMenu == EXTRAS_SUBMENU && idx == 3) {
    tft->fillScreen(TFT_BLACK);
#if defined(M5CARDPUTER)
    drawOptionsLayerBackground(*tft);
#endif
    creditsReset();
    creditsDrawScreen(*tft);
    sInCredits = true;
    return;
  }
  if (currentMenu == EXTRAS_SUBMENU && idx > 0) {
    String label = getSubmenuOptionText();
    if (labelIsCredits(label)) {
      tft->fillScreen(TFT_BLACK);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      creditsReset();
      creditsDrawScreen(*tft);
      sInCredits = true;
      return;
    }
  }

  if (currentMenu == IR_SUBMENU && idx == 1) {
    bgoneReset();
    bgoneDrawScreen(*tft);
    inBGone = true;
    return;
  }

  if (currentMenu == IR_SUBMENU && idx == 4) {
    irReadReset();
    irReadDrawScreen(*tft);
    inIRRead = true;
    return;
  }

  if (currentMenu == WIFI_SUBMENU && idx > 0) {
    if (idx == 1) { wifiScanReset();   wifiScanDrawScreen(*tft);   inWiFiScan   = true; return; }
    if (idx == 2) { packetScanReset(); packetScanDrawScreen(*tft); inPacketScan = true; return; }
    String label = getSubmenuOptionText();
    if (labelIsPacketScan(label)) { packetScanReset(); packetScanDrawScreen(*tft); inPacketScan = true; return; }
    if (labelIsWifiScan(label))   { wifiScanReset();   wifiScanDrawScreen(*tft);   inWiFiScan   = true; return; }
  }

  if (currentMenu == BLUETOOTH_SUBMENU && idx > 0) {
    if (idx == 1) { bleScanReset();      bleScanDrawScreen(*tft);      sInBLEScan = true;      return; }
    if (idx == 2) { bleGoogleAdvReset(); bleGoogleAdvDrawScreen(*tft); sInBLEGoogleAdv = true; return; }
    String label = getSubmenuOptionText();
    if (labelIsBLEScan(label))        { bleScanReset();      bleScanDrawScreen(*tft);      sInBLEScan = true;      return; }
    if (labelIsBLEGoogleAdv(label))   { bleGoogleAdvReset(); bleGoogleAdvDrawScreen(*tft); sInBLEGoogleAdv = true; return; }
  }

  if (currentMenu == SETTINGS_SUBMENU && idx > 0) {
    String label = getSubmenuOptionText();
    if (labelIsBrightness(label)) {
      brightnessReset();
      brightnessDrawScreen(*tft);
      sInBrightness = true;
      return;
    }
    if (labelIsBatterySaver(label)) {
      batterySaverReset();
      batterySaverDrawScreen(*tft);
      sInBatterySaver = true;
      return;
    }
    if (labelIsTime(label)) {
      timeReset();
      timeDrawScreen(*tft);
      sInTime = true;
      return;
    }
    if (labelIsPasslock(label)) {
      Core::Passlock::runSettingsFlow();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawSettingsSubmenu();
      delay(30);
      return;
    }
    if (labelIsPinConfig(label)) {
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      pinConfigReset();
      pinConfigDrawScreen(*tft);
      sInPinConfig = true;
      return;
    }
  }

  if (currentMenu == FILES_SUBMENU && idx > 0) {
    String label = getSubmenuOptionText();
    if (labelIsIntegrated(label) || idx == 2) {
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      integratedFilesReset();
      integratedFilesDrawScreen(*tft);
      sInIntegrated = true;
      return;
    }
  }

  if (currentMenu == RPI_SUBMENU && idx > 0) {
    String label = getSubmenuOptionText();
    if (labelIsRpiInfo(label)) {
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      rpiInfoReset();
      rpiInfoDrawScreen(*tft);
      sInRpiInfo = true;
      return;
    }
  }

  if (currentMenu == RF_SUBMENU && idx > 0) {
    if (idx == 1) {
      tft->fillScreen(TFT_BLACK);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      findFqReset();
      findFqDrawScreen(*tft);
      sInFindFq = true;
      return;
    }
  }

  if (currentMenu == BADUSB_SUBMENU && idx > 0) {
    if (idx == 3) {
      tft->fillScreen(TFT_BLACK);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      monitorPcReset();
      monitorPcDrawScreen(*tft);
      sInMonitorPc = true;
      return;
    }
  }

  if (currentMenu == NFC_SUBMENU && idx > 0) {
    String label = getSubmenuOptionText();
    if (labelIsNfcReader(label)) {
      tft->fillScreen(TFT_BLACK);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      nfcReaderReset();
      nfcReaderDrawScreen(*tft);
      sInNfcReader = true;
      return;
    }
  }

#if defined(M5CARDPUTER)
  drawOptionsLayerBackground(*tft);
#endif
  inOptionScreen = true;
}

void handleAllButtonLogic(
  TFT_eSPI* tft,
  bool& inOptionScreen,
  bool& inStopwatch,
  bool& inBGone,
  bool& inIRRead,
  bool& inWiFiScan,
  bool& inPacketScan,
  MenuState& currentMenu
) {
  unsigned long now = millis();

  if (sInInfiChat) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    infiChatHandleInput(btnAPressed(), btnBPressed(),
                        btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    infiChatHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInInfiChat = false;
      infiChatStop();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawExtrasSubmenu();
      delay(20);
      return;
    }
    infiChatDrawScreen(*tft);
    delay(20);
    return;
  }

  if (sInCredits) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    creditsHandleInput(btnAPressed(), btnBPressed(),
                       btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    creditsHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInCredits = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawExtrasSubmenu();
      delay(20);
      return;
    }
    creditsDrawScreen(*tft);
    delay(20);
    return;
  }

  if (sInNfcReader) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    nfcReaderHandleInput(btnAPressed(), btnBPressed(),
                         btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    nfcReaderHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInNfcReader = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawNfcSubmenu();
      delay(20);
      return;
    }
    nfcReaderDrawScreen(*tft);
    delay(20);
    return;
  }

  if (sInBLEGoogleAdv) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    bleGoogleAdvHandleInput(btnAPressed(), btnBPressed(),
                            btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    bleGoogleAdvHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInBLEGoogleAdv = false;
      bleGoogleAdvStop();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawBluetoothSubmenu();
      delay(30);
      return;
    }
    bleGoogleAdvDrawScreen(*tft);
    delay(30);
    return;
  }


  if (sInBLEScan) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    bleScanHandleInput(btnAPressed(), btnBPressed(),
                       btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    bleScanHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInBLEScan = false;
      bleScanStop();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawBluetoothSubmenu();
      delay(30);
      return;
    }
    bleScanDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInBrightness) {
    bool exitReq = false;
    brightnessHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
    if (exitReq) {
      sInBrightness = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawSettingsSubmenu();
      delay(30);
      return;
    }
    brightnessDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInBatterySaver) {
    bool exitReq = false;
    batterySaverHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
    if (exitReq) {
      sInBatterySaver = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawSettingsSubmenu();
      delay(30);
      return;
    }
    batterySaverDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInTime) {
    bool exitReq = false;
    timeHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
    if (exitReq) {
      sInTime = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawSettingsSubmenu();
      delay(30);
      return;
    }
    timeDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInPinConfig) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    pinConfigHandleInput(btnAPressed(), btnBPressed(),
                         btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    pinConfigHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInPinConfig = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawSettingsSubmenu();
      delay(30);
      return;
    }
    pinConfigDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInIntegrated) {
    bool exitReq = false;
    integratedFilesHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
    if (exitReq) {
      extern bool integratedFilesConsumeOpenWebRequest();
      if (integratedFilesConsumeOpenWebRequest()) {
        sInIntegrated = false;
        tft->fillScreen(TFT_BLACK);
        tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
        drawOptionsLayerBackground(*tft);
#endif
        webFilesReset();
        webFilesDrawScreen(*tft);
        sInWebFiles = true;
        delay(20);
        return;
      }
      sInIntegrated = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawFilesSubmenu();
      delay(30);
      return;
    }
    integratedFilesDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInRpiInfo) {
    bool exitReq = false;
    rpiInfoHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
    if (exitReq) {
      sInRpiInfo = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawRpiSubmenu();
      delay(20);
      return;
    }
    rpiInfoDrawScreen(*tft);
    delay(20);
    return;
  }

  if (sInWebFiles) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    webFilesHandleInput(btnAPressed(), btnBPressed(),
                        btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    webFilesHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInWebFiles = false;
      webFilesStop();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawFilesSubmenu();
      delay(30);
      return;
    }
    webFilesDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInFindFq) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    findFqHandleInput(btnAPressed(), btnBPressed(),
                      btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    findFqHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInFindFq = false;
      findFqStop();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawRfSubmenu();
      delay(30);
      return;
    }
    findFqDrawScreen(*tft);
    delay(30);
    return;
  }

  if (sInMonitorPc) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    monitorPcHandleInput(btnAPressed(), btnBPressed(),
                         btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    monitorPcHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      sInMonitorPc = false;
      monitorPcStop();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawBadUsbSubmenu();
      delay(30);
      return;
    }
    monitorPcDrawScreen(*tft);
    delay(30);
    return;
  }

  if (inStopwatch) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    stopwatchHandleInput(btnAPressed(), btnBPressed(),
                         btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    stopwatchHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      inStopwatch = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawExtrasSubmenu();
      delay(20);
      return;
    }
    drawStopwatchTimeOnly(*tft);
    delay(20);
    return;
  }

  if (inWiFiScan) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    wifiScanHandleInput(btnAPressed(), btnBPressed(),
                        btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    wifiScanHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      inWiFiScan = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawWiFisubmenu();
      delay(30);
      return;
    }
    wifiScanDrawScreen(*tft);
    delay(30);
    return;
  }

  if (inPacketScan) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    packetScanHandleInput(btnAPressed(), btnBPressed(),
                          btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    packetScanHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      inPacketScan = false;
      packetScanStop();
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawWiFisubmenu();
      delay(30);
      return;
    }
    packetScanDrawScreen(*tft);
    delay(30);
    return;
  }

  if (inIRRead) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    irReadHandleInput(btnAPressed(), btnBPressed(),
                      btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    irReadHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      inIRRead = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawIrSubmenu();
      delay(30);
      return;
    }
    irReadDrawScreen(*tft);
    delay(30);
    return;
  }

  if (inBGone) {
    bool exitReq = false;
    bgoneHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
    if (exitReq) {
      inBGone = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawIrSubmenu();
      delay(30);
      return;
    }
    bgoneDrawScreen(*tft);
    delay(30);
    return;
  }

  if (inOptionScreen && btnCPressed()) {
    inOptionScreen = false;
    tft->fillScreen(TFT_BLACK);
    tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
    drawOptionsLayerBackground(*tft);
#endif
    switch(currentMenu) {
      case WIFI_SUBMENU:       drawWiFisubmenu(); break;
      case BLUETOOTH_SUBMENU:  drawBluetoothSubmenu(); break;
      case IR_SUBMENU:         drawIrSubmenu(); break;
      case RF_SUBMENU:         drawRfSubmenu(); break;
      case NRF_SUBMENU:        drawNrfSubmenu(); break;
      case RADIO_SUBMENU:      drawRadioSubmenu(); break;
      case GPS_SUBMENU:        drawGpsSubmenu(); break;
      case RPI_SUBMENU:        drawRpiSubmenu(); break;
      case BADUSB_SUBMENU:     drawBadUsbSubmenu(); break;
      case SETTINGS_SUBMENU:   drawSettingsSubmenu(); break;
      case EXTRAS_SUBMENU:     drawExtrasSubmenu(); break;
      case FILES_SUBMENU:      drawFilesSubmenu(); break;
      case RFID_SUBMENU:       drawRfidSubmenu(); break;
      case NFC_SUBMENU:        drawNfcSubmenu(); break;
      default: break;
    }
    delay(10);
    return;
  }

  if (!inOptionScreen && !inStopwatch && !inIRRead && !inBGone &&
      !inWiFiScan && !inPacketScan && !sInBLEScan && !sInBLEGoogleAdv &&
      !sInBrightness && !sInBatterySaver && !sInTime && !sInIntegrated && !sInRpiInfo && !sInWebFiles && !sInInfiChat && !sInCustomIr && !sInCredits && !sInFindFq && !sInMonitorPc && !sInPinConfig) {
    switch(currentMenu) {
      case WIFI_SUBMENU: case BLUETOOTH_SUBMENU: case IR_SUBMENU:
      case RF_SUBMENU: case NRF_SUBMENU: case RADIO_SUBMENU:
      case GPS_SUBMENU: case RPI_SUBMENU: case BADUSB_SUBMENU:
      case SETTINGS_SUBMENU: case EXTRAS_SUBMENU: case FILES_SUBMENU:
      case RFID_SUBMENU: case NFC_SUBMENU:
        if (btnBPressed()) { nextSubmenuOption();     drawSubmenuOptions(); }
        if (btnCPressed()) { previousSubmenuOption(); drawSubmenuOptions(); }
        if (btnAPressed()) {
          int idx = getSubmenuOptionIndex();
          if (idx == 0) {
            switch(currentMenu) {
              case WIFI_SUBMENU:       currentMenu = WIFI_MENU;       drawWiFiMenu(); break;
              case BLUETOOTH_SUBMENU:  currentMenu = BLUETOOTH_MENU;  drawBluetoothMenu(); break;
              case IR_SUBMENU:         currentMenu = IR_MENU;         drawIRMenu(); break;
              case RF_SUBMENU:         currentMenu = RF_MENU;         drawRFMenu(); break;
              case NRF_SUBMENU:        currentMenu = NRF_MENU;        drawNRFMenu(); break;
              case RADIO_SUBMENU:      currentMenu = RADIO_MENU;      drawRadioMenu(); break;
              case GPS_SUBMENU:        currentMenu = GPS_MENU;        drawGPSMenu(); break;
              case RPI_SUBMENU:        currentMenu = RPI_MENU;        drawRPIMenu(); break;
              case BADUSB_SUBMENU:     currentMenu = BADUSB_MENU;     drawBadUSBMenu(); break;
              case SETTINGS_SUBMENU:   currentMenu = SETTINGS_MENU;   drawSettingsMenu(); break;
              case EXTRAS_SUBMENU:     currentMenu = EXTRAS_MENU;     drawExtrasMenu(); break;
              case FILES_SUBMENU:      currentMenu = FILES_MENU;      drawFilesMenu(); break;
              case RFID_SUBMENU:       currentMenu = RFID_MENU;       drawRfidMenu(); break;
              case NFC_SUBMENU:        currentMenu = NFC_MENU;        drawNfcMenu(); break;
              default: break;
            }
          } else {
            handleSubmenuAction(currentMenu, idx, tft,
                                inStopwatch, inOptionScreen, inBGone, inIRRead,
                                inWiFiScan, inPacketScan);
          }
        }
        return;
      default: break;
    }
  }

  static unsigned long last = 0;
  if (!inOptionScreen && !inStopwatch && !inIRRead && !inBGone &&
      !inWiFiScan && !inPacketScan && !sInBLEScan && !sInBLEGoogleAdv &&
      !sInBrightness && !sInBatterySaver && !sInTime && !sInIntegrated && !sInRpiInfo && !sInWebFiles && !sInInfiChat && !sInCustomIr && !sInCredits && !sInFindFq && !sInMonitorPc && !sInPinConfig) {
    if (now - last > 80) {
      if (btnBPressed()) {
        last = now;
        switch(currentMenu) {
          case WIFI_MENU:        currentMenu=NFC_MENU;         drawNfcMenu();        break;
          case BLUETOOTH_MENU:   currentMenu=WIFI_MENU;        drawWiFiMenu();       break;
          case IR_MENU:          currentMenu=BLUETOOTH_MENU;   drawBluetoothMenu();  break;
          case RF_MENU:          currentMenu=IR_MENU;          drawIRMenu();         break;
          case NRF_MENU:         currentMenu=RF_MENU;          drawRFMenu();         break;
          case RADIO_MENU:       currentMenu=NRF_MENU;         drawNRFMenu();        break;
          case GPS_MENU:         currentMenu=RADIO_MENU;       drawRadioMenu();      break;
          case RPI_MENU:         currentMenu=GPS_MENU;         drawGPSMenu();        break;
          case BADUSB_MENU:      currentMenu=RPI_MENU;         drawRPIMenu();        break;
          case SETTINGS_MENU:    currentMenu=BADUSB_MENU;      drawBadUSBMenu();     break;
          case EXTRAS_MENU:      currentMenu=SETTINGS_MENU;    drawSettingsMenu();   break;
          case FILES_MENU:       currentMenu=EXTRAS_MENU;      drawExtrasMenu();     break;
          case RFID_MENU:        currentMenu=FILES_MENU;       drawFilesMenu();      break;
          case NFC_MENU:         currentMenu=RFID_MENU;        drawRfidMenu();       break;
          default: break;
        }
      }
      if (btnCPressed()) {
        last = now;
        switch(currentMenu) {
          case WIFI_MENU:        currentMenu=BLUETOOTH_MENU;   drawBluetoothMenu();  break;
          case BLUETOOTH_MENU:   currentMenu=IR_MENU;          drawIRMenu();         break;
          case IR_MENU:          currentMenu=RF_MENU;          drawRFMenu();         break;
          case RF_MENU:          currentMenu=NRF_MENU;         drawNRFMenu();        break;
          case NRF_MENU:         currentMenu=RADIO_MENU;       drawRadioMenu();      break;
          case RADIO_MENU:       currentMenu=GPS_MENU;         drawGPSMenu();        break;
          case GPS_MENU:         currentMenu=RPI_MENU;         drawRPIMenu();        break;
          case RPI_MENU:         currentMenu=BADUSB_MENU;      drawBadUSBMenu();     break;
          case BADUSB_MENU:      currentMenu=SETTINGS_MENU;    drawSettingsMenu();   break;
          case SETTINGS_MENU:    currentMenu=EXTRAS_MENU;      drawExtrasMenu();     break;
          case EXTRAS_MENU:      currentMenu=FILES_MENU;       drawFilesMenu();      break;
          case FILES_MENU:       currentMenu=RFID_MENU;        drawRfidMenu();       break;
          case RFID_MENU:        currentMenu=NFC_MENU;         drawNfcMenu();        break;
          case NFC_MENU:         currentMenu=WIFI_MENU;        drawWiFiMenu();       break;
          default: break;
        }
      }
    }
  }

  if (!inOptionScreen && !inStopwatch && !inIRRead && !inBGone &&
      !inWiFiScan && !inPacketScan && !sInBLEScan && !sInBLEGoogleAdv &&
      !sInBrightness && !sInBatterySaver && !sInTime && !sInIntegrated && !sInRpiInfo && !sInWebFiles && !sInInfiChat && !sInCustomIr && !sInCredits && !sInFindFq && !sInMonitorPc && btnAPressed()) {
    tft->fillScreen(TFT_BLACK);
    tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
    drawOptionsLayerBackground(*tft);
#endif
    switch (currentMenu) {
      case WIFI_MENU:        currentMenu = WIFI_SUBMENU;       setSubmenuType(SUBMENU_WIFI);       drawWiFisubmenu();       break;
      case BLUETOOTH_MENU:   currentMenu = BLUETOOTH_SUBMENU;  setSubmenuType(SUBMENU_BLUETOOTH);  drawBluetoothSubmenu();  break;
      case IR_MENU:          currentMenu = IR_SUBMENU;         setSubmenuType(SUBMENU_IR);         drawIrSubmenu();         break;
      case RF_MENU:          currentMenu = RF_SUBMENU;         setSubmenuType(SUBMENU_RF);         drawRfSubmenu();         break;
      case NRF_MENU:         currentMenu = NRF_SUBMENU;        setSubmenuType(SUBMENU_NRF);        drawNrfSubmenu();        break;
      case RADIO_MENU:       currentMenu = RADIO_SUBMENU;      setSubmenuType(SUBMENU_RADIO);      drawRadioSubmenu();      break;
      case GPS_MENU:         currentMenu = GPS_SUBMENU;        setSubmenuType(SUBMENU_GPS);        drawGpsSubmenu();        break;
      case RPI_MENU:         currentMenu = RPI_SUBMENU;        setSubmenuType(SUBMENU_RPI);        drawRpiSubmenu();        break;
      case BADUSB_MENU:      currentMenu = BADUSB_SUBMENU;     setSubmenuType(SUBMENU_BADUSB);     drawBadUsbSubmenu();     break;
      case SETTINGS_MENU:    currentMenu = SETTINGS_SUBMENU;   setSubmenuType(SUBMENU_SETTINGS);   drawSettingsSubmenu();   break;
      case EXTRAS_MENU:      currentMenu = EXTRAS_SUBMENU;     setSubmenuType(SUBMENU_EXTRAS);     drawExtrasSubmenu();     break;
      case FILES_MENU:       currentMenu = FILES_SUBMENU;      setSubmenuType(SUBMENU_FILES);      drawFilesSubmenu();      break;
      case RFID_MENU:        currentMenu = RFID_SUBMENU;       setSubmenuType(SUBMENU_RFID);       drawRfidSubmenu();       break;
      case NFC_MENU:         currentMenu = NFC_SUBMENU;        setSubmenuType(SUBMENU_NFC);        drawNfcSubmenu();        break;
      default: break;
    }
  }
}

extern "C" int passlock_button_read_blocking() {
  for (;;) {
    updateButtons();
    if (btnAPressed()) return 1;
    if (btnBPressed()) return 2;
    if (btnCPressed()) return 3;
    delay(5);
  }
}
