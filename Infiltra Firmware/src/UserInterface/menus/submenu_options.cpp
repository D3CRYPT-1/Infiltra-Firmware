#include <Arduino.h>
#include <TFT_eSPI.h>
#include "submenu_options.h"
#include "menu_submenus.h"
#include <algorithm>

static const String wifiEntries[] = {
  "<- Back", "Scan Hosts", "Evil Portal", "Packet Scan", "Raw Sniffer", "SSH"
};
static const String btEntries[] = {
  "<- Back", "Scan Dev", "Hijack", "Keyboard", "Google", "iOS", "Android", "Samsung", "Windows"
};
static const String irEntries[] = {
  "<- Back", "B Gone", "Remotes", "Custom", "Read", "IR Interrupt"
};
static const String rfEntries[] = {
  "<- Back", "Find Fq", "RF Visualizer", "RF Jammer", "Custom Sub", "Spectogram"
};
static const String nrfEntries[] = {
  "<- Back", "NRF Jammer", "Spectrogram", "NRF Sniffer", "Receive (RX)", "Transmit (TX)"
};
static const String radioEntries[] = {
  "<- Back", "Spectogram", "Standard Bcst", "Reserved Bcst", "Radio Jam"
};
static const String gpsEntries[] = {
  "<- Back", "Wardrive", "Location", "Track SDR", "Lat/Long"
};
static const String rpiEntries[] = {
  "<- Back", "Info", "Scan Rpi", "Terminal", "Controll", "Host"
};
static const String badusbEntries[] = {
  "<- Back", "Execute", "Bad Ble", "Monitor Pc", "Keyboard", "Mouse"
};
static const String settingsEntries[] = {
  "<- Back", "Theme", "Brightness", "Passlock", "Audio", "Volume", "Time", "Bat Saver", "Config Pins"
};
static const String extrasEntries[] = {
  "<- Back", "Stopwatch", "InfiChat", "Creds", "Schematics"
};
static const String filesEntries[] = {
  "<- Back", "Micro Sd", "Integrated", "Format Sd", "Web Files"
};
static const String rfidEntries[] = {
  "<- Back", "Read 125Khz", "Write 125Khz", "Emulate", "Save 125Khz"
};
static const String nfcEntries[] = {
  "<- Back", "Read 13.56Mhz", "Write 13.56Mhz", "Clone 13.56Mhz", "Extract Key", "Scan Reader", "Wipe Tag"
};

static TFT_eSPI*     tftPtr        = nullptr;
static const String* entries       = nullptr;
static int           entryCount    = 0;
static int           selectedEntry = 0;
void initSubmenuOptions(TFT_eSPI* display) { tftPtr = display; }

void setSubmenuType(SubmenuType type) {
  switch (type) {
    case SUBMENU_WIFI:        entries = wifiEntries;    entryCount = sizeof(wifiEntries)/sizeof(wifiEntries[0]); break;
    case SUBMENU_BLUETOOTH:   entries = btEntries;      entryCount = sizeof(btEntries)/sizeof(btEntries[0]); break;
    case SUBMENU_IR:          entries = irEntries;      entryCount = sizeof(irEntries)/sizeof(irEntries[0]); break;
    case SUBMENU_RF:          entries = rfEntries;      entryCount = sizeof(rfEntries)/sizeof(rfEntries[0]); break;
    case SUBMENU_NRF:         entries = nrfEntries;     entryCount = sizeof(nrfEntries)/sizeof(nrfEntries[0]); break;
    case SUBMENU_RADIO:       entries = radioEntries;   entryCount = sizeof(radioEntries)/sizeof(radioEntries[0]); break;
    case SUBMENU_GPS:         entries = gpsEntries;     entryCount = sizeof(gpsEntries)/sizeof(gpsEntries[0]); break;
    case SUBMENU_RPI:         entries = rpiEntries;     entryCount = sizeof(rpiEntries)/sizeof(rpiEntries[0]); break;
    case SUBMENU_BADUSB:      entries = badusbEntries;  entryCount = sizeof(badusbEntries)/sizeof(badusbEntries[0]); break;
    case SUBMENU_SETTINGS:    entries = settingsEntries;entryCount = sizeof(settingsEntries)/sizeof(settingsEntries[0]); break;
    case SUBMENU_EXTRAS:      entries = extrasEntries;  entryCount = sizeof(extrasEntries)/sizeof(extrasEntries[0]); break;
    case SUBMENU_FILES:       entries = filesEntries;   entryCount = sizeof(filesEntries)/sizeof(filesEntries[0]); break;
    case SUBMENU_RFID:        entries = rfidEntries;    entryCount = sizeof(rfidEntries)/sizeof(rfidEntries[0]); break;
    case SUBMENU_NFC:         entries = nfcEntries;     entryCount = sizeof(nfcEntries)/sizeof(nfcEntries[0]); break;
    default:                  entries = nullptr;        entryCount = 0; break;
  }
  selectedEntry = (entryCount > 1 ? 1 : 0);
}

#if defined(M5CARDPUTER)
  static constexpr int screenW        = 240;
  static constexpr int screenH        = 135;
  static constexpr int maxVisible     = 3;
  static constexpr int lineH          = 32;
  static constexpr int verticalOffset = 10;
  static constexpr int textSzSel      = 2;
  static constexpr int charWSel       = 6 * textSzSel;
  static constexpr int padSelX        = 10;
  static constexpr int padSelY        = 6;
  static constexpr int boxHSel        = (8 * textSzSel) + padSelY * 2;
  static constexpr int textSzUns      = 2;
  static constexpr int charWUns       = 6 * textSzUns;
  static constexpr int padUnsX        = 6;
  static constexpr int padUnsY        = 4;
  static constexpr int boxHUns        = (8 * textSzUns) + padUnsY * 2;
  static constexpr int sbW            = 6;
  static constexpr int sbRightMargin  = 2;
  static constexpr int sbMarginTop    = 6;
  static constexpr int sbMarginBottom = 6;
#else
  
  static constexpr int screenW        = 240;
  static constexpr int screenH        = 135;
  static constexpr int maxVisible     = 3;
  static constexpr int lineH          = 32;
  static constexpr int verticalOffset = 10;
  static constexpr int textSzSel      = 2;
  static constexpr int charWSel       = 6 * textSzSel;
  static constexpr int padSelX        = 10;
  static constexpr int padSelY        = 6;
  static constexpr int boxHSel        = (8 * textSzSel) + padSelY * 2;
  static constexpr int textSzUns      = 2;
  static constexpr int charWUns       = 6 * textSzUns;
  static constexpr int padUnsX        = 6;
  static constexpr int padUnsY        = 4;
  static constexpr int boxHUns        = (8 * textSzUns) + padUnsY * 2;
  static constexpr int sbW            = 6;
  static constexpr int sbRightMargin  = 2;
  static constexpr int sbMarginTop    = 6;
  static constexpr int sbMarginBottom = 6;
#endif


static void drawEntry(int idx, int bubbleX, int bubbleW, int y0, bool sel) {
  const int txtSz = sel ? textSzSel : textSzUns;
  const int chW   = sel ? charWSel  : charWUns;
  const int padY  = sel ? padSelY   : padUnsY;
  const int boxH  = sel ? boxHSel   : boxHUns;
  const int boxY  = y0 + (lineH - boxH)/2;
  tftPtr->drawRoundRect(bubbleX, boxY, bubbleW, boxH, 8, TFT_WHITE);
  if (sel) {
    tftPtr->drawRoundRect(bubbleX+1, boxY+1, bubbleW-2, boxH-2, 7, TFT_WHITE);
  }
  tftPtr->setTextSize(txtSz);
  tftPtr->setTextColor(TFT_WHITE, TFT_BLACK);
  const int textW = entries[idx].length() * chW;
  const int tx    = bubbleX + (bubbleW - textW)/2;
  const int ty    = boxY + (boxH - (8*txtSz))/2;
  tftPtr->setCursor(tx, ty);
  tftPtr->print(entries[idx]);
}

void drawSubmenuOptions() {
  if (!tftPtr || !entries) return;
#if defined(M5CARDPUTER)
  static constexpr uint8_t SUBMENU_ROT = 1;  
#else
  static constexpr uint8_t SUBMENU_ROT = 3;  
#endif
  const int orig = tftPtr->getRotation();
  tftPtr->setRotation(SUBMENU_ROT);
  const int totalH = lineH * maxVisible;
  const int startY = ((screenH - totalH) / 2) + verticalOffset;
  int maxLen = 0;
  for (int i = 0; i < entryCount; ++i) {
    maxLen = std::max(maxLen, (int)entries[i].length());
  }
  const int bubbleWsel = maxLen * charWSel + padSelX * 2;
  const int bubbleWuns = maxLen * charWUns + padUnsX * 2;
  const int bubbleXsel = (screenW - bubbleWsel) / 2;
  const int bubbleXuns = (screenW - bubbleWuns) / 2;
  tftPtr->fillRect(bubbleXsel - 4, startY - 2, bubbleWsel + 8, totalH + 4, TFT_BLACK);
  for (int off = -1; off <= 1; ++off) {
    const int idx = (selectedEntry + off + entryCount) % entryCount;
    const int y0  = startY + (off + 1) * lineH;
    const bool sel = (off == 0);
    const int bw   = sel ? bubbleWsel : bubbleWuns;
    const int bx   = sel ? bubbleXsel : bubbleXuns;
    drawEntry(idx, bx, bw, y0, sel);
  }
  const int sbX = screenW - sbW - sbRightMargin;
  const int sbY = startY + sbMarginTop;
  const int sbH = totalH - (sbMarginTop + sbMarginBottom);
  tftPtr->fillRoundRect(sbX, sbY, sbW, sbH, 2, 0x2104);
  if (entryCount > 1) {
    const float pos = float(selectedEntry) / (entryCount - 1);
    const int th = std::max(sbH * maxVisible / entryCount, sbW);
    const int ty = sbY + int((sbH - th) * pos);
    tftPtr->fillRoundRect(sbX, ty, sbW, th, 2, TFT_WHITE);
  }
  tftPtr->setRotation(orig);
}

void nextSubmenuOption()     { if (entries) selectedEntry = (selectedEntry + 1) % entryCount; }
void previousSubmenuOption() { if (entries) selectedEntry = (selectedEntry - 1 + entryCount) % entryCount; }
int  getSubmenuOptionIndex() { return selectedEntry; }
String getSubmenuOptionText(){ return entries ? entries[selectedEntry] : String(); }
