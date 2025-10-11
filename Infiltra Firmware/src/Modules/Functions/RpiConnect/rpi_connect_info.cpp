#include "rpi_connect_info.h"
#include "././UserInterface/menus/menu_submenus.h"  
#include <Arduino.h>
#include <pgmspace.h>

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;

static const char* const kDoc[] PROGMEM = {
"# Infiltra Raspberry Pi Connect",
"",
"**Tagline:** Peer-to-peer control, file transfer, mesh Wi-Fi, and BLE scanning — all powered by **m-Firmware**.",
"",
"**Status:** Beta",
"**Last updated:** August 25, 2025",
"",
"---",
"",
"## Overview",
"",
"**Infiltra Raspberry Pi Connect** (\"RPi Connect\") is a networking feature in **m-Firmware** that uses Wi-Fi and Bluetooth to create secure, ad-hoc links between your Raspberry Pi devices. With RPi Connect you can:",
"",
"* Establish **peer-to-peer (P2P)** connections between Infiltra devices.",
"* Use either device as a **host** or a **controller** — dynamically.",
"* **Send files** quickly between nearby devices.",
"* Spin up a **mesh Wi-Fi** overlay for multi-hop connectivity.",
"* Build a distributed **BLE scanning network**.",
"* Get **terminal access** to any connected Raspberry Pi.",
"* **Discover, manage, and control** fleets of Pis running the Infiltra RPi Connect client.",
"",
"This page covers what it does, how it works, and how to get started.",
"",
"---",
"",
"## Key Capabilities",
"",
"### 1) P2P links over Wi-Fi + Bluetooth",
"* Opportunistic transport selection with Wi-Fi preferred and BLE fallback.",
"* Auto-reconnect with back-off; link health monitoring.",
"",
"### 2) Host / Controller roles",
"* **Host** exposes services (files, shell, sensors, GPIO, etc.).",
"* **Controller** discovers hosts and orchestrates actions.",
"* Devices can **switch roles on the fly** or run **dual-role**.",
"",
"### 3) Fast file transfer",
"* Nearby, encrypted direct transfers.",
"* Queue multiple files; resume on intermittent links.",
"* Optional TTL and auto-delete for drop-box workflows.",
"",
"### 4) Mesh Wi-Fi overlay (ad-hoc)",
"* Self-forming, self-healing mesh for multi-hop reach.",
"* Per-node metrics (ETX/RSSI) to guide routing.",
"* Power-aware participation for battery nodes.",
"",
"### 5) BLE scanning network",
"* Turn any Pi into a **scanner** to crowd-source BLE observations.",
"* Filter by UUID, company ID, or patterns; tag scans by location/role.",
"* Aggregate results to a chosen host or export locally.",
"",
"### 6) Terminal access to the RPi",
"* Secure shell over the active transport (Wi-Fi or BLE UART).",
"* Session logging and clipboard paste support.",
"",
"### 7) Fleet discovery & control",
"* Scan for Pis running the **Infiltra RPi Connect client**.",
"* Group by tags (e.g., `kiosk`, `lab-a`, `field`).",
"* Run commands, push configs, or update modules by group.",
"",
"---",
"",
"## Quick Start",
"",
"> The steps below assume m-Firmware is already installed on your Raspberry Pi.",
"",
"```bash",
"sudo infiltra-connect enable",
"",
"# Discover nearby devices",
"infiltra-connect scan",
"",
"# Pair as controller to a host",
"infiltra-connect pair <device-id> --role controller",
"",
"# Open a terminal session",
"infiltra-connect shell <device-id>",
"",
"# Send a file",
"infiltra-connect send ./build/artifact.tar.gz --to <device-id>",
"",
"# Bring up the mesh",
"infiltra-connect mesh up",
"",
"# Start BLE scanning (optional)",
"infiltra-connect ble scan --uuid FFE0 --tag field-kit",
"```",
"",
"This By Default is Done Automatically on Both Devices",
"",
"---",
"",
"## Feature Summary",
"",
"| Area | What it does | Notes |",
"| ---- | ------------ | ----- |",
"| P2P Link | Direct, encrypted Wi-Fi/BLE link | Auto-reconnect, QoS aware |",
"| Roles | Host / Controller / Dual | Hot-switch roles |",
"| Files | Direct transfer, queue & resume | Optional TTL, drop-box mode |",
"| Mesh  | Ad-hoc multi-hop Wi-Fi overlay | Self-healing, metrics-driven |",
"| BLE Net | Distributed scanning & filtering | Tagged scans, local export |",
"| Terminal | Shell over current transport | Session logs |",
"| Fleet | Discover/control Infiltra Pis | Group actions & updates |",
"",
"---",
"",
"## Architecture",
"* **Connect Client (Agent):** Runs on each Pi; handles discovery, pairing, and transports.",
"* **Control Plane:** Auth, pairing, role negotiation, and command dispatch.",
"* **Data Plane:** File streams, shell sessions, BLE scan results.",
"* **Transports:** Wi-Fi ad-hoc/Direct → primary; BLE (GATT/UART) → fallback.",
"* **Mesh Module:** Forms an ad-hoc SSID and exchanges routing beacons.",
"* **Discovery:** Local broadcast (Wi-Fi) + BLE advertisements; filterable by tag.",
"",
"---",
"",
"## Security",
"* Mutual device authentication during pairing.",
"* Encrypted channels for both control and data.",
"* Per-command permission checks; optional role-based profiles.",
"* Rotating keys and revocation list support.",
"* Audit log of connect, file, and shell events (local ring buffer).",
"",
"> Tip: rotate keys on shared fleet devices and use tags to scope control actions.",
"",
"---",
"",
"## System Requirements",
"* **Hardware:** Raspberry Pi 3B+ or newer with Wi-Fi & BLE. (Pi 02 W Works As Well)",
"* **OS:** Raspberry Pi OS (Bookworm 32/64) or Ubuntu 22.04+/24.04+ ARM.",
"* **Kernel:** 5.15+ recommended.",
"* **Power:** Stable 5V supply; mesh nodes benefit from cooling.",
"",
"---",
"",
"## CLI Reference (Highlights)",
"```bash",
"# List known/nearby devices",
"infiltra-connect scan [--tag <tag>] [--ble] [--wifi]",
"",
"# Pair to a device and set role",
"infiltra-connect pair <device-id> --role <host|controller|dual>",
"",
"# Send/receive files",
"infiltra-connect send <path> --to <device-id> [--ttl 3600]",
"infiltra-connect recv --from <device-id> --out ./incoming",
"",
"# Terminal access",
"infiltra-connect shell <device-id> [--record ./sessions]",
"",
"# Mesh controls",
"infiltra-connect mesh up",
"infiltra-connect mesh status",
"infiltra-connect mesh down",
"",
"# BLE scanning",
"infiltra-connect ble scan [--uuid <id>] [--cid <company>] [--tag <name>]",
"",
"# Group operations",
"infiltra-connect group set <tag> <device-id...>",
"infiltra-connect run --tag <tag> -- \"uptime && vcgencmd measure_temp\"",
"```",
"",
"---",
"",
"## Local API Sketch (for integrations)",
"> Example endpoint sketch — adapt to your implementation.",
"* `GET  /v1/devices` — list nearby/paired devices",
"* `POST /v1/connect` — initiate P2P link {device_id, role}",
"* `POST /v1/files` — start file transfer {to, path, ttl}",
"* `GET  /v1/mesh/status` — current mesh state & metrics",
"* `POST /v1/ble/scan` — start/stop scan {filters, tag}",
"* `POST /v1/shell` — open shell {device_id, record}",
"",
"---",
"",
"## Common Use Cases",
"* **Field service kits:** Controller Pi in a technician’s bag linking to hosts in kiosks.",
"* **Pop-up labs/classrooms:** Share files & shell without infrastructure.",
"* **Sensor sweeps:** Crowd-source BLE scans and aggregate results.",
"* **On-site staging:** Mesh a cluster to pre-load images/configs.",
"",
"---",
"",
"## Performance & Limits (typical)",
"* **Wi-Fi P2P throughput:** medium-high locally; drops with mesh hops.",
"* **BLE fallback:** low throughput; best for control/small files/shell.",
"* **Discovery radius:** room-to-floor; walls/metal reduce range.",
"",
"> For large file bursts, keep hops ≤2 or stage near the host.",
"",
"---",
"",
"## Troubleshooting",
"* **Can’t discover devices:** Check client is enabled & versions match; reduce RF noise.",
"* **Pairing fails:** Clear old keys; ensure time in sync.",
"* **Slow transfers:** Prefer Wi-Fi; reduce hops; verify power.",
"* **Shell not opening:** Check role permissions; firewall; audit log.",
"",
"**Logs:** /var/log/infiltra/connect.log",
"",
"---",
"",
"## Terminology",
"* **Host:** Offers services (files, shell, sensors).",
"* **Controller:** Orchestrates actions on hosts.",
"* **Dual:** Acts as both host and controller.",
"* **Tag:** Label to group devices.",
"",
"---",
"",
"## Legal & Safety",
"* Intended for non-critical use. Respect RF laws/policies.",
"* Don’t expose shell to untrusted networks/users.",
"* Use per-device keys; rotate on role changes.",
};
static const int kDocLines = sizeof(kDoc)/sizeof(kDoc[0]);

enum LineKind : uint8_t { LK_TEXT, LK_H1, LK_H2, LK_RULE, LK_BUL, LK_CODE, LK_QUOTE, LK_TABLE };
struct RL { String txt; LineKind k; uint8_t indent; };
static RL   sLines[700];
static int  sLineCount = 0;
static bool sFrameDrawn=false, sDirty=true, sReflow=true;
static int  W=0,H=0;
static int  bx=0,by=0,bw=0,bh=0;  
static int  cx=0,cy=0,cw=0,ch=0;  
static int  sScrollY=0;
static constexpr uint8_t  LINE_H = 12;
static constexpr uint16_t C_FRAME     = TFT_DARKGREY;
static constexpr uint16_t C_TEXT      = TFT_WHITE;
static constexpr uint16_t C_SUB       = TFT_SILVER;
static constexpr uint16_t C_ACCENT    = 0x39C7;  
static constexpr uint16_t C_RULE      = TFT_DARKGREY;
static constexpr uint16_t C_CODE_BG   = 0x0021;  
static constexpr uint16_t C_CODE_ED   = TFT_DARKGREY;
static constexpr uint16_t C_BAR_BG    = 0x0841;  
static constexpr uint16_t C_DOT_R     = 0xE800;  
static constexpr uint16_t C_DOT_Y     = 0xFEC0;  
static constexpr uint16_t C_DOT_G     = 0x07E0;  
static String strip(const String& in){
  int a=0,b=in.length();
  while (a<b && (in[a]==' '||in[a]=='\t')) a++;
  while (b>a && (in[b-1]==' '||in[b-1]=='\t')) b--;
  return in.substring(a,b);
}

static void wrapPush(TFT_eSPI& tft, const String& s, LineKind k, uint8_t indentPx){
  if (!s.length()){ sLines[sLineCount++] = { "", k, indentPx }; return; }
  const int maxW = max(10, cw - indentPx);
  String cur; cur.reserve(s.length()+8);
  for (int i=0;i<(int)s.length();){
    String trial = cur + s[i];
    if (tft.textWidth(trial) > maxW){
      if (cur.length()==0){ sLines[sLineCount++] = { String(s[i++]), k, indentPx }; }
      else                { sLines[sLineCount++] = { cur, k, indentPx }; cur=""; }
    }else cur += s[i++];
  }
  if (cur.length()) sLines[sLineCount++] = { cur, k, indentPx };
}

static void reflow(TFT_eSPI& tft){
  sLineCount = 0;
  bool inCode=false;
  for (int i=0; i<kDocLines && sLineCount < (int)(sizeof(sLines)/sizeof(sLines[0])); ++i){
    const char* p = (const char*)pgm_read_ptr(&kDoc[i]);
    String raw = p ? String(p) : String();
    String line = strip(raw);
    if (line.startsWith("```")){ inCode = !inCode; continue; }
    if (inCode){ wrapPush(tft, line, LK_CODE, 8); continue; }
    if (line == "---"){ sLines[sLineCount++] = { "", LK_RULE, 0 }; continue; }
    if (!line.length()){ sLines[sLineCount++] = { "", LK_TEXT, 0 }; continue; }
    if (line.startsWith("## ")){ String t=line.substring(3); wrapPush(tft, t, LK_H2, 0); continue; }
    if (line.startsWith("# ")){  String t=line.substring(2); wrapPush(tft, t, LK_H1, 0); continue; }
    if (line.startsWith("* ")){ String t=line.substring(2); wrapPush(tft, t, LK_BUL, 16); continue; }
    if (line.startsWith("> ")){ String t=line.substring(2); wrapPush(tft, t, LK_QUOTE, 8); continue; }
    if (line.startsWith("|")){ String t=line; t.replace("|","  |  "); wrapPush(tft, t, LK_TABLE, 0); continue; }
    if (line.startsWith("**")){
      int p2 = line.indexOf("**",2);
      int c  = line.indexOf(':', (p2>0)?p2+2:0);
      if (p2>1 && c>p2){
        String label = line.substring(2,p2);
        String rest  = strip(line.substring(c+1));
        wrapPush(tft, label + ":", LK_H2, 0);
        if (rest.length()) wrapPush(tft, rest, LK_TEXT, 0);
        continue;
      }
    }
    wrapPush(tft, line, LK_TEXT, 0);
  }
  sReflow = false;
  if (sScrollY > 0) sScrollY = 0;
  sDirty = true;
}

static void drawStaticFrame(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);  
  tft.setRotation(ROT_ALT);
  tft.setTextSize(1);
  W = tft.width(); H = tft.height();
  const int marginX = 10;
  const int marginTop = 16;
  const int marginBottom = 8;
  bx = marginX; by = marginTop + 12;
  bw = W - marginX*2;
  bh = H - by - marginBottom;
  tft.drawRoundRect(bx, by, bw, bh, 8, C_FRAME);
  tft.drawRoundRect(bx+1, by+1, bw-2, bh-2, 7, C_FRAME);
  tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
  cx = bx + 8; cy = by + 6; cw = bw - 16; ch = bh - 12;
}

static int drawH1(TFT_eSPI& tft, int x, int y, const String& txt){
  tft.setTextColor(C_ACCENT, TFT_BLACK);
  tft.setCursor(x, y); tft.print(txt);
  tft.drawFastHLine(x, y+10, min(cw-4, tft.textWidth(txt)+4), C_RULE);
  tft.setTextColor(C_TEXT, TFT_BLACK);
  return LINE_H + 2;
}
static int drawH2(TFT_eSPI& tft, int x, int y, const String& txt){
  int w = tft.textWidth(txt) + 6;
  tft.drawRoundRect(x-1, y-1, w+2, 11, 3, C_ACCENT);
  tft.setTextColor(C_ACCENT, TFT_BLACK);
  tft.setCursor(x+2, y); tft.print(txt);
  tft.setTextColor(C_TEXT, TFT_BLACK);
  return LINE_H;
}
static int drawRule(TFT_eSPI& tft, int x, int y){
  tft.drawFastHLine(cx, y+5, cw, C_RULE);
  return 8;
}
static int drawBul(TFT_eSPI& tft, int x, int y, const String& txt){
  tft.setTextColor(C_TEXT, TFT_BLACK);
  tft.setCursor(cx+2, y); tft.print("•");
  tft.setCursor(x, y);    tft.print(txt);
  return LINE_H;
}
static int drawQuote(TFT_eSPI& tft, int x, int y, const String& txt){
  tft.drawFastVLine(cx+2, y-1, LINE_H, C_RULE);
  tft.setTextColor(C_SUB, TFT_BLACK);
  tft.setCursor(x, y); tft.print(txt);
  tft.setTextColor(C_TEXT, TFT_BLACK);
  return LINE_H;
}
static int drawText(TFT_eSPI& tft, int x, int y, const String& txt){
  tft.setTextColor(C_TEXT, TFT_BLACK);
  tft.setCursor(x, y); tft.print(txt);
  return LINE_H;
}

static int drawCodeBlock(TFT_eSPI& tft, int x, int y, int& idx){
  int start = idx;
  int count = 0;
  while (start + count < sLineCount && sLines[start + count].k == LK_CODE) count++;
  if (count <= 0) return 0;
  const int padX = 6;
  const int padY = 4;
  const int barH = 10;
  const int innerW = cw - 8;
  const int boxX = cx + 4;
  int boxH = barH + padY + count*LINE_H + padY;
  tft.fillRoundRect(boxX-2, y-2, innerW+4, boxH+4, 3, C_CODE_BG);
  tft.drawRoundRect(boxX-2, y-2, innerW+4, boxH+4, 3, C_CODE_ED);
  tft.fillRoundRect(boxX-2, y-2, innerW+4, barH, 3, C_BAR_BG);
  tft.fillCircle(boxX+6,  y+3, 2, C_DOT_R);
  tft.fillCircle(boxX+13, y+3, 2, C_DOT_Y);
  tft.fillCircle(boxX+20, y+3, 2, C_DOT_G);
  tft.setTextColor(C_SUB, C_BAR_BG);
  tft.setCursor(boxX+28, y+1); tft.print("bash");
  tft.setTextColor(C_SUB, C_CODE_BG);
  int ty = y + barH + padY;
  for (int i=0;i<count;i++){
    tft.setCursor(boxX + padX, ty);
    tft.print(sLines[start + i].txt);
    ty += LINE_H;
  }

  idx = start + count - 1;
  return boxH + 4;
}

void rpiInfoReset(){
  sFrameDrawn = false;
  sDirty = true;
  sReflow = true;
  sLineCount = 0;
  sScrollY = 0;
}

void rpiInfoDrawScreen(TFT_eSPI& tft){
  if (!sFrameDrawn){
    drawStaticFrame(tft);
    sFrameDrawn = true;
    sDirty = true;
  }
  if (sReflow){
    reflow(tft);
    sDirty = true;
  }
  if (!sDirty) return;
  tft.fillRect(bx+2, by+2, bw-4, bh-4, TFT_BLACK);
  int y = cy - sScrollY;           
  const int yMin = cy;
  const int yMax = cy + ch;
  for (int i=0; i<sLineCount; ++i){
    if (y + LINE_H < yMin){     
      if (sLines[i].k == LK_CODE){
        int j=i, blockH = 0;
        
        int cnt=0; while (j < sLineCount && sLines[j].k == LK_CODE){ cnt++; j++; }
        blockH = (10 + 4 + cnt*LINE_H + 4) + 4; 
        y += blockH + 2;
        i = j - 1;
        continue;
      } else {
        y += LINE_H;
        continue;
      }
    }

    if (y > yMax) break;
    int x = cx + sLines[i].indent;
    if (sLines[i].k == LK_CODE){
      int used = drawCodeBlock(tft, x, y, i);
      y += used + 2;
      continue;
    }
    int used = 0;
    switch (sLines[i].k){
      case LK_H1:   used = drawH1(tft, x, y, sLines[i].txt); break;
      case LK_H2:   used = drawH2(tft, x, y, sLines[i].txt); break;
      case LK_RULE: used = drawRule(tft, x, y); break;
      case LK_BUL:  used = drawBul(tft, x, y, sLines[i].txt); break;
      case LK_QUOTE:used = drawQuote(tft, x, y, sLines[i].txt); break;
      case LK_TABLE:
      case LK_TEXT:
      default:      used = drawText(tft, x, y, sLines[i].txt); break;
    }
    y += used;
  }

  sDirty = false;
}

void rpiInfoHandleInput(bool a, bool b, bool c, bool& exitReq){
  exitReq = false;
  const int step = 36; 
  if (b){ sScrollY += step; sDirty = true; }
  if (c){ sScrollY -= step; if (sScrollY < 0) sScrollY = 0; sDirty = true; }
  if (a){ exitReq = true; return; } 
  int totalH = LINE_H * sLineCount + 96; 
  int maxScroll = max(0, totalH - (ch));
  if (sScrollY > maxScroll) sScrollY = maxScroll;
}
