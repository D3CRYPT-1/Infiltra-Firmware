#include "t_embed_submenu_options.h"
#include "../submenu_options.h"

extern TFT_eSPI* submenuDisplay;
static int t_embed_current_option = 0;
static SubmenuType t_embed_current_submenu = SUBMENU_WIFI;

// T-Embed specific submenu options data
static const char* t_embed_wifi_options[] = {
    "WiFi Scan",
    "Deauth Attack", 
    "Beacon Spam",
    "Evil Portal",
    "Packet Monitor",
    "Probe Sniff",
    "WiFi Jammer"
};

static const char* t_embed_bluetooth_options[] = {
    "BLE Scan",
    "BLE Advertise",
    "Device Spoof",
    "BLE Exploits",
    "Beacon Flood",
    "Apple Spam"
};

static const char* t_embed_ir_options[] = {
    "TV-B-Gone",
    "Read Signal",
    "Send Signal", 
    "IR Database",
    "Raw Capture",
    "Learning Mode"
};

static const char* t_embed_rf_options[] = {
    "Sub-GHz Scan",
    "Record Signal",
    "Replay Signal",
    "RF Jammer",
    "Spectrum Analyzer",
    "Frequency Hopper",
    "ASK/OOK Decode"
};

static const char* t_embed_nrf_options[] = {
    "NRF24 Scan",
    "Mouse Jammer",
    "Packet Sniffer",
    "Beacon Flood",
    "Address Scan"
};

static const char* t_embed_radio_options[] = {
    "FM Transmitter",
    "Spectrum View",
    "Signal Generator",
    "APRS Beacon"
};

static const char* t_embed_gps_options[] = {
    "GPS Tracker",
    "GPS Spoof",
    "Location Info",
    "NMEA Parser"
};

static const char* t_embed_rpi_options[] = {
    "Connect RPI",
    "Remote Control", 
    "File Transfer",
    "SSH Terminal"
};

static const char* t_embed_badusb_options[] = {
    "Keyboard Attack",
    "Mouse Attack",
    "HID Attack",
    "Load Payloads",
    "Rubber Ducky",
    "Custom Script"
};

static const char* t_embed_settings_options[] = {
    "Configure Pins",
    "Brightness",
    "System Config",
    "About Device",
    "Reset Settings"
};

static const char* t_embed_extras_options[] = {
    "Credits",
    "InfiChat",
    "Stopwatch",
    "Calculator",
    "QR Generator",
    "Text Editor"
};

static const char* t_embed_files_options[] = {
    "File Explorer",
    "File Manager",
    "Text Viewer",
    "Hex Editor"
};

static const char* t_embed_rfid_options[] = {
    "Read RFID",
    "Write RFID", 
    "Clone Card",
    "Emulate Card",
    "Card Database"
};

static const char* t_embed_nfc_options[] = {
    "Read NFC",
    "Write NFC",
    "Emulate Tag", 
    "Clone Tag",
    "NDEF Editor"
};

// Get options array and count for current submenu
static void getTEmbedSubmenuOptions(const char*** options, int* count) {
    switch (t_embed_current_submenu) {
        case SUBMENU_WIFI:
            *options = t_embed_wifi_options;
            *count = sizeof(t_embed_wifi_options) / sizeof(t_embed_wifi_options[0]);
            break;
        case SUBMENU_BLUETOOTH:
            *options = t_embed_bluetooth_options;
            *count = sizeof(t_embed_bluetooth_options) / sizeof(t_embed_bluetooth_options[0]);
            break;
        case SUBMENU_IR:
            *options = t_embed_ir_options;
            *count = sizeof(t_embed_ir_options) / sizeof(t_embed_ir_options[0]);
            break;
        case SUBMENU_RF:
            *options = t_embed_rf_options;
            *count = sizeof(t_embed_rf_options) / sizeof(t_embed_rf_options[0]);
            break;
        case SUBMENU_NRF:
            *options = t_embed_nrf_options;
            *count = sizeof(t_embed_nrf_options) / sizeof(t_embed_nrf_options[0]);
            break;
        case SUBMENU_RADIO:
            *options = t_embed_radio_options;
            *count = sizeof(t_embed_radio_options) / sizeof(t_embed_radio_options[0]);
            break;
        case SUBMENU_GPS:
            *options = t_embed_gps_options;
            *count = sizeof(t_embed_gps_options) / sizeof(t_embed_gps_options[0]);
            break;
        case SUBMENU_RPI:
            *options = t_embed_rpi_options;
            *count = sizeof(t_embed_rpi_options) / sizeof(t_embed_rpi_options[0]);
            break;
        case SUBMENU_BADUSB:
            *options = t_embed_badusb_options;
            *count = sizeof(t_embed_badusb_options) / sizeof(t_embed_badusb_options[0]);
            break;
        case SUBMENU_SETTINGS:
            *options = t_embed_settings_options;
            *count = sizeof(t_embed_settings_options) / sizeof(t_embed_settings_options[0]);
            break;
        case SUBMENU_EXTRAS:
            *options = t_embed_extras_options;
            *count = sizeof(t_embed_extras_options) / sizeof(t_embed_extras_options[0]);
            break;
        case SUBMENU_FILES:
            *options = t_embed_files_options;
            *count = sizeof(t_embed_files_options) / sizeof(t_embed_files_options[0]);
            break;
        case SUBMENU_RFID:
            *options = t_embed_rfid_options;
            *count = sizeof(t_embed_rfid_options) / sizeof(t_embed_rfid_options[0]);
            break;
        case SUBMENU_NFC:
            *options = t_embed_nfc_options;
            *count = sizeof(t_embed_nfc_options) / sizeof(t_embed_nfc_options[0]);
            break;
        default:
            *options = t_embed_wifi_options;
            *count = sizeof(t_embed_wifi_options) / sizeof(t_embed_wifi_options[0]);
            break;
    }
}

void initTEmbedSubmenuOptions(TFT_eSPI* display) {
    submenuDisplay = display;
    t_embed_current_option = 0;
}

void setTEmbedSubmenuType(SubmenuType type) {
    t_embed_current_submenu = type;
    t_embed_current_option = 0;
}

void drawTEmbedSubmenuOptions() {
    if (!submenuDisplay) return;
    
    const char** options;
    int option_count;
    getTEmbedSubmenuOptions(&options, &option_count);
    
    // Clear the options area (T-Embed optimized)
    submenuDisplay->fillRect(0, T_EMBED_SUBMENU_START_Y, 170, 260, TFT_BLACK);
    
    // Calculate scroll offset for T-Embed display
    int start_index = 0;
    if (t_embed_current_option >= T_EMBED_SUBMENU_MAX_VISIBLE) {
        start_index = t_embed_current_option - T_EMBED_SUBMENU_MAX_VISIBLE + 1;
    }
    
    // Draw visible options
    for (int i = 0; i < T_EMBED_SUBMENU_MAX_VISIBLE && (start_index + i) < option_count; i++) {
        int option_index = start_index + i;
        int y_pos = T_EMBED_SUBMENU_START_Y + (i * T_EMBED_SUBMENU_ITEM_HEIGHT);
        bool is_selected = (option_index == t_embed_current_option);
        
        drawTEmbedSubmenuOption(option_index, options[option_index], is_selected, y_pos);
    }
    
    // Draw scroll indicator if needed
    if (option_count > T_EMBED_SUBMENU_MAX_VISIBLE) {
        drawTEmbedSubmenuScrollIndicator(t_embed_current_option, option_count);
    }
}

void drawTEmbedSubmenuOption(int index, const char* text, bool selected, int y_position) {
    if (!submenuDisplay) return;
    
    if (selected) {
        // Draw selection background (T-Embed optimized)
        submenuDisplay->fillRect(T_EMBED_SUBMENU_TEXT_X - 5, y_position - 2, 150, T_EMBED_SUBMENU_ITEM_HEIGHT - 2, TFT_BLUE);
        submenuDisplay->setTextColor(TFT_WHITE);
        
        // Draw selection indicator
        submenuDisplay->drawString(">", 5, y_position);
    } else {
        submenuDisplay->setTextColor(TFT_WHITE);
    }
    
    // Draw option text
    submenuDisplay->setTextSize(1);
    submenuDisplay->drawString(text, T_EMBED_SUBMENU_TEXT_X, y_position);
}

void nextTEmbedSubmenuOption() {
    const char** options;
    int option_count;
    getTEmbedSubmenuOptions(&options, &option_count);
    
    t_embed_current_option++;
    if (t_embed_current_option >= option_count) {
        t_embed_current_option = 0; // Wrap to beginning
    }
}

void previousTEmbedSubmenuOption() {
    const char** options;
    int option_count;
    getTEmbedSubmenuOptions(&options, &option_count);
    
    t_embed_current_option--;
    if (t_embed_current_option < 0) {
        t_embed_current_option = option_count - 1; // Wrap to end
    }
}

int getTEmbedSubmenuOptionIndex() {
    return t_embed_current_option;
}

String getTEmbedSubmenuOptionText() {
    const char** options;
    int option_count;
    getTEmbedSubmenuOptions(&options, &option_count);
    
    if (t_embed_current_option >= 0 && t_embed_current_option < option_count) {
        return String(options[t_embed_current_option]);
    }
    return "";
}

void drawTEmbedSubmenuScrollIndicator(int current, int total) {
    if (!submenuDisplay) return;
    
    // Draw scroll bar on right side (T-Embed optimized for 170px width)
    int scroll_x = 165;
    int scroll_y = T_EMBED_SUBMENU_START_Y;
    int scroll_height = T_EMBED_SUBMENU_MAX_VISIBLE * T_EMBED_SUBMENU_ITEM_HEIGHT;
    
    // Draw scroll track
    submenuDisplay->drawLine(scroll_x, scroll_y, scroll_x, scroll_y + scroll_height, TFT_DARKGREY);
    
    // Calculate scroll thumb position
    int thumb_height = max(4, (T_EMBED_SUBMENU_MAX_VISIBLE * scroll_height) / total);
    int thumb_y = scroll_y + (current * (scroll_height - thumb_height)) / (total - 1);
    
    // Draw scroll thumb
    submenuDisplay->fillRect(scroll_x - 1, thumb_y, 3, thumb_height, TFT_WHITE);
}

void drawTEmbedSubmenuSelection(int y_position) {
    if (!submenuDisplay) return;
    submenuDisplay->drawRect(T_EMBED_SUBMENU_TEXT_X - 5, y_position - 2, 150, T_EMBED_SUBMENU_ITEM_HEIGHT - 2, TFT_YELLOW);
}

void clearTEmbedSubmenuSelection(int y_position) {
    if (!submenuDisplay) return;
    submenuDisplay->drawRect(T_EMBED_SUBMENU_TEXT_X - 5, y_position - 2, 150, T_EMBED_SUBMENU_ITEM_HEIGHT - 2, TFT_BLACK);
}