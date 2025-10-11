#pragma once
#include <TFT_eSPI.h>
#include <Arduino.h>

enum class BatterySaverMode {
    OFF = 0,
    LIGHT = 1,
    BASIC = 2,
    ECO = 3
};

struct BatterySaverProfile {
    String name;
    String description;
    uint8_t cpuFreqMhz;      
    uint8_t brightnessScale; 
    bool disableWiFi;        
    bool disableBluetooth;   
    bool limitHardware;      
    uint32_t sleepDelay;     
};

void batterySaverReset();
void batterySaverDrawScreen(TFT_eSPI& tft);
void batterySaverHandleInput(bool a, bool b, bool c, bool& requestExit);
void batterySaverApplyMode(BatterySaverMode mode);
BatterySaverMode batterySaverGetCurrentMode();
void batterySaverInitAndApply();
String batterySaverGetModeName(BatterySaverMode mode);
BatterySaverProfile batterySaverGetProfile(BatterySaverMode mode);
bool batterySaverIsHardwareLimited();
void batterySaverSetCpuFrequency(uint8_t freqMhz);
void batterySaverApplyBrightnessScale(uint8_t scalePercent);