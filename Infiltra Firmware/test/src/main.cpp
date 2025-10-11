#include <Arduino.h>
#ifdef M5STICK_1
#include <M5StickCPlus.h>
#endif
#ifdef M5STICK_2
#include <M5StickCPlus2.h>
#endif
#ifdef CARDPUTER
#include <M5Cardputer.h>
#endif

#ifdef LILYGO_TWATCH_S3
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif
#ifdef LILYGO_T_DONGLE_S3
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif
#ifdef LILYGO_T_DECK
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif
#ifdef LILYGO_T_EMBED
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif
#if defined(CYD_2432S028) || defined(CYD_2432S032) || defined(CYD_8048S050) || defined(CYD_8048S070)
#include <esp32_smartdisplay.h>
#endif

void initDisplay()
{
  Serial.println("Initializing display...");
#ifdef M5STICK_1
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  Serial.println("M5Stick C Plus v1 display initialized");
#endif

#ifdef M5STICK_2
  M5.begin();
  M5.Display.setRotation(1);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  Serial.println("M5Stick C Plus v2 display initialized");
#endif

#ifdef CARDPUTER
  M5.begin();
  M5.Display.setRotation(1);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  Serial.println("M5 Cardputer display initialized");
#endif

#ifdef LILYGO_TWATCH_S3
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  Serial.println("LILYGO T-Watch S3 display initialized");
#endif

#ifdef LILYGO_T_DONGLE_S3
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  Serial.println("LILYGO T-Dongle S3 display initialized");
#endif

#ifdef LILYGO_T_DECK
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  Serial.println("LILYGO T-Deck display initialized");
#endif

#ifdef LILYGO_T_EMBED
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  Serial.println("LILYGO T-Embed display initialized");
#endif

#if defined(CYD_2432S028) || defined(CYD_2432S032)
  smartdisplay_init();
  smartdisplay_lcd_set_backlight(1.0);
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "");
  Serial.println("CYD 2.8\"/3.2\" display initialized");
#endif

#if defined(CYD_8048S050) || defined(CYD_8048S070)
  smartdisplay_init();
  smartdisplay_lcd_set_backlight(1.0);
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "");
  Serial.println("CYD 5.0\"/7.0\" display initialized");
#endif
}

void displayInfiltra()
{
  Serial.println("Displaying 'infiltra' on screen...");
#ifdef M5STICK_1
  M5.Lcd.setCursor(10, 30);
  M5.Lcd.println("infiltra");
#endif

#ifdef M5STICK_2
  M5.Display.setCursor(10, 30);
  M5.Display.println("infiltra");
#endif

#ifdef CARDPUTER
  M5.Display.setCursor(40, 60);
  M5.Display.println("infiltra");
#endif

#ifdef LILYGO_TWATCH_S3
  tft.setCursor(60, 100);
  tft.println("infiltra");
#endif

#ifdef LILYGO_T_DONGLE_S3
  tft.setCursor(10, 30);
  tft.println("infiltra");
#endif

#ifdef LILYGO_T_DECK
  tft.setCursor(100, 120);
  tft.println("infiltra");
#endif

#ifdef LILYGO_T_EMBED
  tft.setCursor(20, 50);
  tft.println("infiltra");
#endif

#if defined(CYD_2432S028) || defined(CYD_2432S032) || defined(CYD_8048S050) || defined(CYD_8048S070)
  lv_obj_t *scr = lv_scr_act();
  lv_obj_clean(scr);
  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_text(label, "infiltra");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_center(label);
  lv_task_handler();
#endif
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== INFILTRA Display Test ===");
  Serial.println("Initializing device...");
#ifdef M5STICK_1
  Serial.println("Device: M5Stick C Plus v1");
#endif
#ifdef M5STICK_2
  Serial.println("Device: M5Stick C Plus v2");
#endif
#ifdef CARDPUTER
  Serial.println("Device: M5 Cardputer");
#endif
#ifdef LILYGO_TWATCH_S3
  Serial.println("Device: LILYGO T-Watch S3");
#endif
#ifdef LILYGO_T_DONGLE_S3
  Serial.println("Device: LILYGO T-Dongle S3");
#endif
#ifdef LILYGO_T_DECK
  Serial.println("Device: LILYGO T-Deck");
#endif
#ifdef LILYGO_T_EMBED
  Serial.println("Device: LILYGO T-Embed");
#endif
#ifdef LILYGO_T_EMBED_CC1101
  Serial.println("Device: LILYGO T-Embed CC1101");
#endif
#ifdef CYD_2432S028
  Serial.println("Device: CYD 2432S028");
#endif
#ifdef CYD_2432S032
  Serial.println("Device: CYD 2432S032");
#endif
#ifdef CYD_8048S050
  Serial.println("Device: CYD 8048S050");
#endif
#ifdef CYD_8048S070
  Serial.println("Device: CYD 8048S070");
#endif

  initDisplay();
  delay(500);
  displayInfiltra();

  Serial.println("Display test complete!");
}

void loop()
{

#if defined(M5STICK_1) || defined(M5STICK_2) || defined(CARDPUTER)
  M5.update();
#endif

#if defined(CYD_2432S028) || defined(CYD_2432S032) || defined(CYD_8048S050) || defined(CYD_8048S070)
  lv_task_handler();
#endif

  delay(100);
}