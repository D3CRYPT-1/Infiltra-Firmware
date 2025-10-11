#pragma once
#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

class ConfigManager {
public:
    static void init();
    static bool loadConfig(JsonDocument& doc);
    static bool saveConfig(const JsonDocument& doc);
    static String getConfigPath();
private:
    static bool sSpiffsInit;
    static void initSpiffs();
    static void copyDefaultConfig();
};