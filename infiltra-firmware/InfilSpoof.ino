#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEAdvertising.h>
#include <vector>
#include <map>
#include <string>
#include <math.h>

/**
 * InfilSpoof - Universal BLE Location Spoofer
 * Advanced location spoofing for multiple ESP32 devices
 * Supports: M5StickC Plus, LilyGO T-Embed, ESP32 WROOM, etc.
 */

class UniversalLocationSpoofer {
private:
    #if defined(ESP32)
        BLEScan* pScanner;
        BLEAdvertising* pAdvertising;
    #elif defined(ARDUINO_AMB23_BW16)
        // Ameba BLE objects
    #elif defined(ARDUINO_ARCH_SAMD)
        // rpcBLE objects  
    #endif

    struct Location {
        double latitude;
        double longitude;
        double altitude;
        uint8_t locationData[15];
    };
    
    Location currentLocation;
    std::map<std::string, Location> locationPresets;
    bool isLocationSpoofing;

public:
    UniversalLocationSpoofer() : isLocationSpoofing(false) {
        #if defined(ESP32)
            pScanner = nullptr;
            pAdvertising = nullptr;
        #endif
        initializeLocationPresets();
    }

    bool begin() {
        #if defined(ESP32)
            if (!BLEDevice::getInitialized()) {
                BLEDevice::init("InfilSpoof");
            }
            pAdvertising = BLEDevice::getAdvertising();
            return true;
            
        #elif defined(ESP8266)
            Serial.println("ESP8266: No BLE hardware");
            return false;
            
        #elif defined(ARDUINO_AMB23_BW16)
            Serial.println("RTL8720DN: BLE support needs implementation");
            return false;
            
        #elif defined(ARDUINO_ARCH_SAMD)
            Serial.println("WIO Terminal: BLE support needs implementation");
            return false;
            
        #else
            Serial.println("Unsupported platform");
            return false;
        #endif
    }

    void initializeLocationPresets() {
        locationPresets["apple_park"] = {37.3349, -122.0090, 80.0};
        locationPresets["googleplex"] = {37.4220, -122.0841, 30.0};
        locationPresets["white_house"] = {38.8977, -77.0365, 20.0};
        locationPresets["eiffel_tower"] = {48.8584, 2.2945, 324.0};
        locationPresets["times_square"] = {40.7580, -73.9855, 25.0};
        locationPresets["area_51"] = {37.2350, -115.8111, 1350.0};
        locationPresets["silicon_valley"] = {37.3875, -122.0575, 50.0};
    }

    void setLocation(double lat, double lon, double alt = 0.0) {
        currentLocation.latitude = lat;
        currentLocation.longitude = lon;
        currentLocation.altitude = alt;
        updateLocationData();
        Serial.printf("Location set to: Lat: %.6f, Lon: %.6f, Alt: %.2fm\n", lat, lon, alt);
    }

    void setLocationPreset(const std::string& presetName) {
        if (locationPresets.find(presetName) != locationPresets.end()) {
            currentLocation = locationPresets[presetName];
            updateLocationData();
            Serial.printf("Location preset loaded: %s\n", presetName.c_str());
        } else {
            Serial.println("Location preset not found");
        }
    }

    void updateLocationData() {
        int32_t latMicro = (int32_t)(currentLocation.latitude * 1000000);
        int32_t lonMicro = (int32_t)(currentLocation.longitude * 1000000);
        int16_t altCm = (int16_t)(currentLocation.altitude * 100);
        
        currentLocation.locationData[0] = (latMicro >> 24) & 0xFF;
        currentLocation.locationData[1] = (latMicro >> 16) & 0xFF;
        currentLocation.locationData[2] = (latMicro >> 8) & 0xFF;
        currentLocation.locationData[3] = latMicro & 0xFF;
        
        currentLocation.locationData[4] = (lonMicro >> 24) & 0xFF;
        currentLocation.locationData[5] = (lonMicro >> 16) & 0xFF;
        currentLocation.locationData[6] = (lonMicro >> 8) & 0xFF;
        currentLocation.locationData[7] = lonMicro & 0xFF;
        
        currentLocation.locationData[8] = (altCm >> 8) & 0xFF;
        currentLocation.locationData[9] = altCm & 0xFF;
    }

    void spoofLocation(double lat, double lon, double alt = 0.0) {
        #if defined(ESP32)
            setLocation(lat, lon, alt);
            spamLocationBeacon();
        #elif defined(ESP8266)
            spoofWiFiLocation(lat, lon);
        #endif
    }

    void startLocationSpoofing(uint32_t durationMs = 0) {
        if (!begin()) return;
        
        isLocationSpoofing = true;
        uint32_t startTime = millis();
        
        Serial.println("Starting location spoofing...");
        
        while (isLocationSpoofing && (durationMs == 0 || (millis() - startTime) < durationMs)) {
            #if defined(ESP32)
                spamLocationBeacon();
            #elif defined(ESP8266)
                spoofWiFiLocation(currentLocation.latitude, currentLocation.longitude);
            #endif
            delay(100);
        }
        stopLocationSpoofing();
    }

    void spamLocationBeacon() {
        #if defined(ESP32)
            if (!pAdvertising) return;
            
            BLEAdvertisementData advertisementData;
            std::vector<uint8_t> locationData = getLocationBeaconData();
            
            std::vector<uint8_t> beaconData = {
                0x4C, 0x00, 0x02, 0x15,
                0x4C, 0x6F, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E,
                0x42, 0x65, 0x61, 0x63, 0x6F, 0x6E, 0x30, 0x31
            };
            
            beaconData.insert(beaconData.end(), locationData.begin(), locationData.end());
            beaconData.push_back(0xC5);
            
            advertisementData.setManufacturerData(std::string(beaconData.begin(), beaconData.end()));
            advertisementData.setName("Location Beacon");
            advertisementData.setFlags(0x06);
            
            pAdvertising->setAdvertisementData(advertisementData);
            pAdvertising->start();
        #endif
    }

    std::vector<uint8_t> getLocationBeaconData() {
        std::vector<uint8_t> data;
        for (int i = 0; i < 10; i++) {
            data.push_back(currentLocation.locationData[i]);
        }
        return data;
    }

    void spoofWiFiLocation(double lat, double lon) {
        #if defined(ESP8266)
            Serial.printf("ESP8266: WiFi location spoofing - Lat: %.6f, Lon: %.6f\n", lat, lon);
        #endif
    }

    void stopLocationSpoofing() {
        isLocationSpoofing = false;
        #if defined(ESP32)
            if (pAdvertising) {
                pAdvertising->stop();
            }
        #endif
        Serial.println("Location spoofing stopped");
    }

    void listLocationPresets() {
        Serial.println("Available location presets:");
        for (const auto& preset : locationPresets) {
            Serial.printf("  - %s: %.4f, %.4f\n", preset.first.c_str(), preset.second.latitude, preset.second.longitude);
        }
    }
};

UniversalLocationSpoofer spoofer;

void setup() {
    Serial.begin(115200);
    
    #ifdef ARDUINO_M5Stick_C_Plus
        Serial.println("M5StickC Plus - InfilSpoof Ready");
    #elif defined(ARDUINO_LILYGO_T_EMBED)
        Serial.println("LilyGO T-Embed - InfilSpoof Ready");
    #elif defined(ESP8266)
        Serial.println("ESP8266 - InfilSpoof Ready (WiFi mode)");
    #else
        Serial.println("ESP32 - InfilSpoof Ready");
    #endif

    spoofer.listLocationPresets();
    Serial.println("Commands: apple, google, whitehouse, stop");
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readString();
        command.trim();
        
        if (command == "apple") {
            spoofer.setLocationPreset("apple_park");
            spoofer.startLocationSpoofing(30000);
        } else if (command == "google") {
            spoofer.setLocationPreset("googleplex");
            spoofer.startLocationSpoofing(30000);
        } else if (command == "whitehouse") {
            spoofer.setLocationPreset("white_house");
            spoofer.startLocationSpoofing(30000);
        } else if (command == "stop") {
            spoofer.stopLocationSpoofing();
        } else if (command == "list") {
            spoofer.listLocationPresets();
        }
    }
    delay(100);
}
