# 🧠 Infiltra Firmware

<p align="center">
<img src="https://socialify.git.ci/D3CRYPT-1/Infiltra-Firmware/image?custom_language=C%2B%2B&description=1&font=JetBrains+Mono&forks=1&issues=1&language=1&name=1&pattern=Circuit+Board&pulls=1&stargazers=1&theme=Dark" alt="Infiltra-Firmware" width="640" height="320" />
</p>

Welcome to **Infiltra Firmware** — a **next-generation multi-tool firmware platform** built for **wireless security research, RF experimentation, device emulation, and embedded hacking**.

Designed to rival and surpass commercial pentesting gadgets, Infiltra transforms supported hardware into a **modular, extensible, and professional-grade cyber toolkit**.

Built by the **Infiltra Network** community of developers, hackers, and engineers.

---

## ⚡ What Is Infiltra?

Infiltra Firmware turns supported devices into a **Swiss-Army knife for wireless, RF, BLE, IR, and IoT security testing**, providing:

- Real-time wireless scanning  
- Packet analysis and transmission  
- Protocol emulation  
- Signal capture and replay  
- Attack simulation & security auditing  
- Modular extensibility across platforms  

**Goal:** Make Infiltra a **Flipper-class (or better)** open-source security platform.

---

## 🧩 Supported Hardware

Infiltra supports (and continues expanding):

- **M5StickC Plus / Plus2**
- **M5Stack Cardputer**
- **LilyGO T-Embed CC1101**
- **ESP32 / ESP32-WROOM / ESP8266**
- **RTL8720DN BW16 (5GHz Wi-Fi Deauther)**
- **CYD / WIO Terminal**
- **NRF24L01+ modules**
- **Headless ESP32 Mode**
- **Future multi-board support planned**

---

## 📶 Wi-Fi Toolkit (PRODUCTION-READY)

### Passive Features
- Network Scanner  
- Raw Packet Sniffer (Promiscuous Mode)
- Beacon & Probe Monitor  
- Real-time RSSI & Channel Tracking  
- Auto Channel Hopping  
- Deduplicated Network Database  
- Signal Strength Indicators  
- Memory-bounded buffers  
- Stable Active Manager system  

### Active Features
- **Wi-Fi Deauthentication**
  - Continuous mode  
  - Burst mode  
  - Auto-stop safety  
  - Adjustable timing  
  - Real-time counters  
- **Beacon Spam**
  - Preset SSID lists  
  - Random SSID generation  
  - Adjustable TX rate  
  - Channel lock / hop  
  - Live transmit stats  

### Infrastructure
- Shared Wi-Fi Channel Control  
- Active Manager to prevent conflicts  
- Safe TX/RX transitions  
- Watchdog-safe loops  
- Clean task teardown  

---

## 🟦 Bluetooth (BLE) Toolkit *(In Progress / Expanding)*

- BLE Scanner  
- Device Discovery  
- RSSI Tracking  
- Device Metadata Viewer  
- GATT Service Enumeration  
- Device Filtering & Sorting  
- Live RSSI Monitoring  

---

## 📡 Sub-GHz / RF (CC1101)

- Signal Capture & Replay  
- Protocol-aware RX/TX  
- RF Packet Analysis  
- Frequency Profiles  
- Spectrum Scanning *(planned)*  
- Multi-remote Emulation *(planned)*  

---

## 📺 Infrared (IR)

- IR Receive & Transmit  
- Protocol Decode (NEC, Sony, RC5, etc.)  
- Saved Remote Profiles  
- Batch IR Replay  
- Favorites Mode  
- TV-B-Gone-style mass transmit  

---

## 🧿 NRF24L01+ (2.4 GHz)

- Packet Capture  
- Wireless Device Interaction  
- IoT Simulation  
- Mesh Testing *(planned)*  
- Security Analysis *(planned)*  

---

## 📛 RFID & NFC *(Planned Expansion)*

- Card Scanning  
- Tag Emulation  
- Clone Simulation  
- NFC Protocol Debugging  

---

## ⚙️ System Features

- Modular Menu System  
- Device-wide Active Manager  
- Crash Recovery  
- Non-blocking Task Scheduling  
- Persistent Settings (NVS / Flash)  
- Memory-bounded buffers  
- Watchdog-safe loops  
- UI Error Messaging  
- Clean Exit Handling  
- Performance-optimized firmware core  

---

## 📦 Installation

### ⚠️ Always back up your device before flashing

### Step 1 — Download Firmware
Grab the latest release:  
👉 https://github.com/D3CRYPT-1/Infiltra-Firmware/releases  

### Step 2 — Flash Firmware
Use:  
👉 **Infiltra WebFlasher** — https://infiltra.co  

Or flash manually using ESP tools, Arduino IDE, or PlatformIO.

---

## 🛠 Development

Clone the repo:
```bash
git clone https://github.com/D3CRYPT-1/Infiltra-Firmware
