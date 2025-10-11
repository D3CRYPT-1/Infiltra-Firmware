#pragma once
#include <Arduino.h>
class TFT_eSPI;

enum TimeMode {
    TIME_OFF = 0,
    TIME_UPTIME = 1,
    TIME_LOCAL = 2
};
enum TimeState {
    TIME_CONFIG,
    TIME_WIFI_SCAN,
    TIME_WIFI_PASSWORD,
    TIME_SCREENSAVER
};

void timeReset();
void timeDrawScreen(TFT_eSPI& tft);
void timeHandleInput(bool aPressed, bool bPressed, bool cPressed, bool& exitRequested);
void timeInitScreensaver();
void timeUpdateScreensaver();
bool timeIsScreensaverActive();
void timeDeactivateScreensaver();
void timeSetScreensaverTimeout(uint32_t timeoutMs);
void timeSetMode(TimeMode mode);
TimeMode timeGetMode();
bool timeIsConfigured();
void timeConnectToWiFi(const String& ssid, const String& password);
bool timeIsWiFiConnected();
void timeSyncWithNTP();
String timeGetLocalTimeString();
String timeGetUptimeString();
void timeDrawMainMenuDisplay(TFT_eSPI& tft);