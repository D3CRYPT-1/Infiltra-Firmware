#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "UserInterface/menus/menu_enums.h"
#include "UserInterface/bitmaps/menu_bitmaps.h"
#include "UserInterface/menus/menu_submenus.h"
#include "UserInterface/menus/submenu_options.h"
#include "Modules/Core/User Input/buttons.h"
#include "Modules/Functions/Extras/stopwatch.h"
#include "Modules/Core/User Config/Passlock.h"
#include "Modules/Functions/Settings/brightness.h"
#include "Modules/Functions/Settings/battery_saver.h"
#include "Modules/Core/User Config/ConfigManager.h"

#if defined(M5CARDPUTER)
#include <M5Cardputer.h>
static constexpr uint8_t ROT_TOP = 4;
#elif defined(M5STICK_C_PLUS_2)
#include <M5StickCPlus2.h>
static constexpr uint8_t ROT_TOP = 2;
#elif defined(M5STICK_C_PLUS_1_1)
static constexpr uint8_t ROT_TOP = 2;
#else
#include <M5StickCPlus2.h>
static constexpr uint8_t ROT_TOP = 2;
#endif
#ifndef TFT_BL
#define TFT_BL 27
#endif
#ifndef TFT_BACKLIGHT_ON
#define TFT_BACKLIGHT_ON 1
#endif

TFT_eSPI tft;
bool inOptionScreen = false;
bool inStopwatch = false;
bool inBGone = false;
bool inIRRead = false;
bool inWiFiScan = false;
bool inPacketScan = false;
MenuState currentMenu = WIFI_MENU;

#if defined(M5STICK_C_PLUS_1_1)
static inline void axpWrite(uint8_t reg, uint8_t val)
{
  Wire.beginTransmission(0x34);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static void axp192_init_for_stickc_plus11()
{
  Wire.begin(21, 22);
  axpWrite(0x12, 0x4D);
  axpWrite(0x28, 0xCC);
  delay(10);

#if (defined(TFT_BL) && (TFT_BL >= 0))
  pinMode(TFT_BL, OUTPUT);
#if TFT_BACKLIGHT_ON
  digitalWrite(TFT_BL, HIGH);
#else
  digitalWrite(TFT_BL, LOW);
#endif
#endif
}
#endif

void setup()
{
#if defined(M5CARDPUTER)
  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(ROT_TOP);
  M5Cardputer.Display.setBrightness(180);
#elif defined(M5STICK_C_PLUS_2)
  M5.begin();
  M5.Display.setRotation(ROT_TOP);
  M5.Display.setBrightness(200);
#elif defined(M5STICK_C_PLUS_1_1)
  axp192_init_for_stickc_plus11();
#else
  M5.begin();
  M5.Display.setRotation(ROT_TOP);
  M5.Display.setBrightness(200);
#endif
  tft.init();
  tft.setRotation(ROT_TOP);
  tft.fillScreen(TFT_BLACK);
  delay(80);

  initButtons();
  initSubmenuOptions(&tft);
  ConfigManager::init(); 
  batterySaverInitAndApply(); 
  Core::Passlock::begin();
  Core::Passlock::promptForUnlock();
  drawWiFiMenu();
  delay(100); 
  brightnessInitAndApply(); 
}

void loop()
{
  updateButtons();
  handleAllButtonLogic(&tft,
                       inOptionScreen,
                       inStopwatch,
                       inBGone,
                       inIRRead,
                       inWiFiScan,
                       inPacketScan,
                       currentMenu);
}
