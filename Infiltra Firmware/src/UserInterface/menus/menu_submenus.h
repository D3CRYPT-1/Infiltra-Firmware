#ifndef MENU_SUBMENUS_H
#define MENU_SUBMENUS_H
#include <Arduino.h>
#include <TFT_eSPI.h>

//wifi submenu bitmaps
extern const unsigned char PROGMEM image__16_bits[];
extern const unsigned char PROGMEM image_battery_empty_bits[];
extern const unsigned char PROGMEM image_Layer_9_bits[];
extern const unsigned char PROGMEM image_passport_left_bits[];
 
//bluetooth submenu bitmaps
extern const unsigned char PROGMEM image_bleLayer_2_bits[];
extern const unsigned char PROGMEM image_bleLayer_3_bits[];
extern const unsigned char PROGMEM image_bleLayer_6_bits[];
extern const unsigned char PROGMEM image_bleLayer_8_bits[];

//Ir submenu bitmaps
extern const unsigned char PROGMEM image_irLayer_2_bits[];
extern const unsigned char PROGMEM image_irLayer_3_bits[];
extern const unsigned char PROGMEM image_irLayer_6_bits[];
extern const unsigned char PROGMEM image_irLayer_8_bits[];

//Rf Submenu bitmaps
extern const unsigned char PROGMEM image_rflayer_7_bits[];
extern const unsigned char PROGMEM image_rflayer_8_bits[];
extern const unsigned char PROGMEM image_rflayer_13_bits[];
extern const unsigned char PROGMEM image_rflayer_13_1_bits[];

//NRF Submenu bitmaps
extern const unsigned char PROGMEM image_nrfimage_2_bits[];
extern const unsigned char PROGMEM image_nrfimage_14_bits[];
extern const unsigned char PROGMEM image_nrfimage_15_bits[];
extern const unsigned char PROGMEM image_nrfimage_7_bits[];
extern const unsigned char PROGMEM image_nrfimage_9_bits[];

//radio submenu bitmaps
extern const unsigned char PROGMEM image_RadioLayer_2_bits[];
extern const unsigned char PROGMEM image_RadioLayer_3_bits[];
extern const unsigned char PROGMEM image_RadioLayer_6_bits[];
extern const unsigned char PROGMEM image_RadioLayer_8_bits[];

//gps submenu bitmaps
extern const unsigned char PROGMEM image_GpsLayer_2_bits[];
extern const unsigned char PROGMEM image_GpsLayer_3_bits[];
extern const unsigned char PROGMEM image_GpsLayer_6_bits[];
extern const unsigned char PROGMEM image_GpsLayer_8_bits[];

//Rpi submenu bitmaps
extern const unsigned char PROGMEM image_RpiLayer_2_bits[];
extern const unsigned char PROGMEM image_RpiLayer_3_bits[];
extern const unsigned char PROGMEM image_RpiLayer_6_bits[];
extern const unsigned char PROGMEM image_RpiLayer_8_1_bits[];
extern const unsigned char PROGMEM image_RpiLayer_9_bits[];

//BadUsb submenu bitmaps
extern const unsigned char PROGMEM image_BadLayer_2_bits[];
extern const unsigned char PROGMEM image_BadLayer_3_bits[];
extern const unsigned char PROGMEM image_BadLayer_6_bits[];
extern const unsigned char PROGMEM image_BadLayer_8_bits[];

//Settings submenu bitmaps
extern const unsigned char PROGMEM image_SetLayer_11_bits[];
extern const unsigned char PROGMEM image_SetLayer_12_bits[];
extern const unsigned char PROGMEM image_SetLayer_6_bits[];
extern const unsigned char PROGMEM image_SetLayer_7_bits[];

//extra submenu bitmaps
extern const unsigned char PROGMEM image_exLayer_2_bits[];
extern const unsigned char PROGMEM image_exLayer_3_bits[];
extern const unsigned char PROGMEM image_exLayer_7_bits[];
extern const unsigned char PROGMEM image_exLayer_8_1_bits[];
extern const unsigned char PROGMEM image_exLayer_8_bits[];

//Files submenu bitmaps
extern const unsigned char PROGMEM image_fileLayer_2_bits[];
extern const unsigned char PROGMEM image_fileLayer_3_bits[];
extern const unsigned char PROGMEM image_fileLayer_6_bits[];
extern const unsigned char PROGMEM image_fileLayer_7_bits[];

//Rfid submenu bitmaps
extern const unsigned char PROGMEM image_rfidLayer_2_bits[];
extern const unsigned char PROGMEM image_rfidLayer_3_bits[];
extern const unsigned char PROGMEM image_rfidLayer_6_bits[];
extern const unsigned char PROGMEM image_rfidLayer_8_bits[];

//Nfc submenu bitmaps
extern const unsigned char PROGMEM image_nfcLayer_2_bits[];
extern const unsigned char PROGMEM image_nfcLayer_3_bits[];
extern const unsigned char PROGMEM image_nfcLayer_6_bits[];
extern const unsigned char PROGMEM image_nfcLayer_8_bits[];

// Include T-Embed specific menu support
#include "t_embed/t_embed_menus.h"

void drawWiFisubmenu(void);
void drawBluetoothSubmenu(void);
void drawIrSubmenu(void);
void drawRfSubmenu(void);
void drawNrfSubmenu(void);
void drawRadioSubmenu(void);
void drawGpsSubmenu(void);
void drawRpiSubmenu(void);
void drawBadUsbSubmenu(void);
void drawSettingsSubmenu(void);
void drawExtraSubmenu(void);
void drawFilesSubmenu(void);
void drawRfidSubmenu(void);
void drawNfcSubmenu(void);
void drawOptionsLayerBackground(TFT_eSPI& tft);

#endif