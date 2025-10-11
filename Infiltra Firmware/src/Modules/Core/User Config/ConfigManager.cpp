#include "ConfigManager.h"

bool ConfigManager::sSpiffsInit = false;

void ConfigManager::init() {
    initSpiffs();
    copyDefaultConfig();
}
void ConfigManager::initSpiffs() {
    if (!sSpiffsInit) {
        SPIFFS.begin(true);
        sSpiffsInit = true;
    }
}
String ConfigManager::getConfigPath() {
    return "/Infiltra.config";
}
void ConfigManager::copyDefaultConfig() {
    if (!SPIFFS.exists(getConfigPath())) {
        JsonDocument doc;
        doc["brightness"] = 70;
        doc["brightness_scale"] = 100;
        doc["battery_saver_mode"] = 0;
        #if defined(M5STICK_C_PLUS_2) || defined(M5STICK_C_PLUS_1_1)
            doc["ir_tx_pin"] = 19;
            doc["ir_tx_name"] = "Default";
            doc["ir_rx_pin"] = 33;
            doc["ir_rx_name"] = "Default";
            doc["rf_tx_pin"] = 19;
            doc["rf_tx_name"] = "Default";
            doc["rf_rx_pin"] = 33;
            doc["rf_rx_name"] = "Default";
        #elif defined(M5CARDPUTER)
            doc["ir_tx_pin"] = 44;
            doc["ir_tx_name"] = "Default";
            doc["ir_rx_pin"] = 1;
            doc["ir_rx_name"] = "Default";
            doc["rf_tx_pin"] = 44;
            doc["rf_tx_name"] = "Default";
            doc["rf_rx_pin"] = 1;
            doc["rf_rx_name"] = "Default";
        #endif
        saveConfig(doc);
    }
}

bool ConfigManager::loadConfig(JsonDocument& doc) {
    initSpiffs();
    if (!SPIFFS.exists(getConfigPath())) {
        copyDefaultConfig();
    }
    fs::File file = SPIFFS.open(getConfigPath(), "r");
    if (!file) {
        return false;
    }
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    return !error;
}

bool ConfigManager::saveConfig(const JsonDocument& doc) {
    initSpiffs();
    fs::File file = SPIFFS.open(getConfigPath(), "w");
    if (!file) {
        return false;
    }
    size_t bytes = serializeJson(doc, file);
    file.close();
    return bytes > 0;
}