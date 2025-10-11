#ifndef T_EMBED_BITMAPS_H
#define T_EMBED_BITMAPS_H

#include <Arduino.h>

// T-Embed display size: 170x320 (different from M5Stick 135x240)
// These bitmaps are optimized for the T-Embed screen resolution

// Main menu background bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_main_menu_bg_bits[];

// Submenu background bitmaps for T-Embed  
extern const unsigned char PROGMEM t_embed_submenu_bg_bits[];

// WiFi menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_wifi_icon_bits[];
extern const unsigned char PROGMEM t_embed_wifi_scan_bits[];
extern const unsigned char PROGMEM t_embed_wifi_deauth_bits[];
extern const unsigned char PROGMEM t_embed_wifi_beacon_bits[];
extern const unsigned char PROGMEM t_embed_wifi_evil_portal_bits[];
extern const unsigned char PROGMEM t_embed_wifi_packet_monitor_bits[];

// Bluetooth menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_bt_icon_bits[];
extern const unsigned char PROGMEM t_embed_bt_scan_bits[];
extern const unsigned char PROGMEM t_embed_bt_advertise_bits[];
extern const unsigned char PROGMEM t_embed_bt_spoof_bits[];
extern const unsigned char PROGMEM t_embed_bt_exploit_bits[];

// IR menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_ir_icon_bits[];
extern const unsigned char PROGMEM t_embed_ir_tv_bgone_bits[];
extern const unsigned char PROGMEM t_embed_ir_read_bits[];
extern const unsigned char PROGMEM t_embed_ir_send_bits[];
extern const unsigned char PROGMEM t_embed_ir_database_bits[];

// RF menu bitmaps for T-Embed (CC1101 specific)
extern const unsigned char PROGMEM t_embed_rf_icon_bits[];
extern const unsigned char PROGMEM t_embed_rf_scan_bits[];
extern const unsigned char PROGMEM t_embed_rf_record_bits[];
extern const unsigned char PROGMEM t_embed_rf_replay_bits[];
extern const unsigned char PROGMEM t_embed_rf_jammer_bits[];
extern const unsigned char PROGMEM t_embed_cc1101_icon_bits[];

// NRF menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_nrf_icon_bits[];
extern const unsigned char PROGMEM t_embed_nrf_scan_bits[];
extern const unsigned char PROGMEM t_embed_nrf_mouse_jammer_bits[];
extern const unsigned char PROGMEM t_embed_nrf_sniffer_bits[];

// Radio menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_radio_icon_bits[];
extern const unsigned char PROGMEM t_embed_radio_fm_bits[];
extern const unsigned char PROGMEM t_embed_radio_spectrum_bits[];

// GPS menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_gps_icon_bits[];
extern const unsigned char PROGMEM t_embed_gps_tracker_bits[];
extern const unsigned char PROGMEM t_embed_gps_spoof_bits[];

// RPI menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_rpi_icon_bits[];
extern const unsigned char PROGMEM t_embed_rpi_connect_bits[];
extern const unsigned char PROGMEM t_embed_rpi_remote_bits[];

// BadUSB menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_badusb_icon_bits[];
extern const unsigned char PROGMEM t_embed_badusb_keyboard_bits[];
extern const unsigned char PROGMEM t_embed_badusb_mouse_bits[];
extern const unsigned char PROGMEM t_embed_badusb_hid_bits[];

// Settings menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_settings_icon_bits[];
extern const unsigned char PROGMEM t_embed_settings_pins_bits[];
extern const unsigned char PROGMEM t_embed_settings_brightness_bits[];
extern const unsigned char PROGMEM t_embed_settings_config_bits[];

// Extras menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_extras_icon_bits[];
extern const unsigned char PROGMEM t_embed_extras_credits_bits[];
extern const unsigned char PROGMEM t_embed_extras_infichat_bits[];
extern const unsigned char PROGMEM t_embed_extras_stopwatch_bits[];

// Files menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_files_icon_bits[];
extern const unsigned char PROGMEM t_embed_files_explorer_bits[];
extern const unsigned char PROGMEM t_embed_files_folder_bits[];
extern const unsigned char PROGMEM t_embed_files_file_bits[];

// RFID menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_rfid_icon_bits[];
extern const unsigned char PROGMEM t_embed_rfid_read_bits[];
extern const unsigned char PROGMEM t_embed_rfid_write_bits[];
extern const unsigned char PROGMEM t_embed_rfid_clone_bits[];

// NFC menu bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_nfc_icon_bits[];
extern const unsigned char PROGMEM t_embed_nfc_read_bits[];
extern const unsigned char PROGMEM t_embed_nfc_write_bits[];
extern const unsigned char PROGMEM t_embed_nfc_emulate_bits[];

// Utility bitmaps for T-Embed
extern const unsigned char PROGMEM t_embed_battery_icon_bits[];
extern const unsigned char PROGMEM t_embed_signal_icon_bits[];
extern const unsigned char PROGMEM t_embed_lock_icon_bits[];
extern const unsigned char PROGMEM t_embed_unlock_icon_bits[];

// Function declarations for T-Embed specific drawing
void drawTEmbedWiFiMenu();
void drawTEmbedBluetoothMenu();
void drawTEmbedIRMenu();
void drawTEmbedRFMenu();
void drawTEmbedNRFMenu();
void drawTEmbedRadioMenu();
void drawTEmbedGPSMenu();
void drawTEmbedRPIMenu();
void drawTEmbedBadUSBMenu();
void drawTEmbedSettingsMenu();
void drawTEmbedExtrasMenu();
void drawTEmbedFilesMenu();
void drawTEmbedRfidMenu();
void drawTEmbedNfcMenu();

// Utility functions
bool isTEmbedDevice();
void setTEmbedDisplayMode();

#endif