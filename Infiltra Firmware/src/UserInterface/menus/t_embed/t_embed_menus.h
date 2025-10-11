#ifndef T_EMBED_MENUS_H
#define T_EMBED_MENUS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// T-Embed display size: 170x320 (different from M5Stick 135x240)
// Menu implementations optimized for T-Embed screen resolution

// T-Embed submenu background bitmaps (placeholders)
extern const unsigned char PROGMEM t_embed_submenu_bg_layer_1_bits[];
extern const unsigned char PROGMEM t_embed_submenu_bg_layer_2_bits[];
extern const unsigned char PROGMEM t_embed_submenu_bg_layer_3_bits[];
extern const unsigned char PROGMEM t_embed_options_layer_bg_bits[];

// WiFi submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_wifi_sub_scan_bits[];
extern const unsigned char PROGMEM t_embed_wifi_sub_deauth_bits[];
extern const unsigned char PROGMEM t_embed_wifi_sub_beacon_bits[];
extern const unsigned char PROGMEM t_embed_wifi_sub_evil_portal_bits[];
extern const unsigned char PROGMEM t_embed_wifi_sub_packet_monitor_bits[];

// Bluetooth submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_bt_sub_scan_bits[];
extern const unsigned char PROGMEM t_embed_bt_sub_advertise_bits[];
extern const unsigned char PROGMEM t_embed_bt_sub_spoof_bits[];
extern const unsigned char PROGMEM t_embed_bt_sub_exploit_bits[];

// IR submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_ir_sub_tv_bgone_bits[];
extern const unsigned char PROGMEM t_embed_ir_sub_read_bits[];
extern const unsigned char PROGMEM t_embed_ir_sub_send_bits[];
extern const unsigned char PROGMEM t_embed_ir_sub_database_bits[];

// RF submenu bitmaps for T-Embed (CC1101 specific)
extern const unsigned char PROGMEM t_embed_rf_sub_scan_bits[];
extern const unsigned char PROGMEM t_embed_rf_sub_record_bits[];
extern const unsigned char PROGMEM t_embed_rf_sub_replay_bits[];
extern const unsigned char PROGMEM t_embed_rf_sub_jammer_bits[];
extern const unsigned char PROGMEM t_embed_rf_sub_analyzer_bits[];

// NRF submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_nrf_sub_scan_bits[];
extern const unsigned char PROGMEM t_embed_nrf_sub_mouse_jammer_bits[];
extern const unsigned char PROGMEM t_embed_nrf_sub_sniffer_bits[];
extern const unsigned char PROGMEM t_embed_nrf_sub_beacon_bits[];

// Radio submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_radio_sub_fm_bits[];
extern const unsigned char PROGMEM t_embed_radio_sub_spectrum_bits[];
extern const unsigned char PROGMEM t_embed_radio_sub_transmit_bits[];

// GPS submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_gps_sub_tracker_bits[];
extern const unsigned char PROGMEM t_embed_gps_sub_spoof_bits[];
extern const unsigned char PROGMEM t_embed_gps_sub_info_bits[];

// RPI submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_rpi_sub_connect_bits[];
extern const unsigned char PROGMEM t_embed_rpi_sub_remote_bits[];
extern const unsigned char PROGMEM t_embed_rpi_sub_file_transfer_bits[];

// BadUSB submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_badusb_sub_keyboard_bits[];
extern const unsigned char PROGMEM t_embed_badusb_sub_mouse_bits[];
extern const unsigned char PROGMEM t_embed_badusb_sub_hid_bits[];
extern const unsigned char PROGMEM t_embed_badusb_sub_payloads_bits[];

// Settings submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_settings_sub_pins_bits[];
extern const unsigned char PROGMEM t_embed_settings_sub_brightness_bits[];
extern const unsigned char PROGMEM t_embed_settings_sub_config_bits[];
extern const unsigned char PROGMEM t_embed_settings_sub_about_bits[];

// Extras submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_extras_sub_credits_bits[];
extern const unsigned char PROGMEM t_embed_extras_sub_infichat_bits[];
extern const unsigned char PROGMEM t_embed_extras_sub_stopwatch_bits[];
extern const unsigned char PROGMEM t_embed_extras_sub_calculator_bits[];

// Files submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_files_sub_explorer_bits[];
extern const unsigned char PROGMEM t_embed_files_sub_manager_bits[];
extern const unsigned char PROGMEM t_embed_files_sub_viewer_bits[];

// RFID submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_rfid_sub_read_bits[];
extern const unsigned char PROGMEM t_embed_rfid_sub_write_bits[];
extern const unsigned char PROGMEM t_embed_rfid_sub_clone_bits[];
extern const unsigned char PROGMEM t_embed_rfid_sub_emulate_bits[];

// NFC submenu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_nfc_sub_read_bits[];
extern const unsigned char PROGMEM t_embed_nfc_sub_write_bits[];
extern const unsigned char PROGMEM t_embed_nfc_sub_emulate_bits[];
extern const unsigned char PROGMEM t_embed_nfc_sub_clone_bits[];

// T-Embed submenu function declarations
void drawTEmbedWiFiSubmenu(void);
void drawTEmbedBluetoothSubmenu(void);
void drawTEmbedIrSubmenu(void);
void drawTEmbedRfSubmenu(void);
void drawTEmbedNrfSubmenu(void);
void drawTEmbedRadioSubmenu(void);
void drawTEmbedGpsSubmenu(void);
void drawTEmbedRpiSubmenu(void);
void drawTEmbedBadUsbSubmenu(void);
void drawTEmbedSettingsSubmenu(void);
void drawTEmbedExtrasSubmenu(void);
void drawTEmbedFilesSubmenu(void);
void drawTEmbedRfidSubmenu(void);
void drawTEmbedNfcSubmenu(void);

// T-Embed utility functions for menu drawing
void drawTEmbedOptionsLayerBackground(TFT_eSPI& tft);
void drawTEmbedSubmenuHeader(const char* title);
void drawTEmbedSubmenuBorder();

// T-Embed device detection for menus
bool isTEmbedDeviceForMenus();
void setTEmbedMenuDisplayMode();

#endif