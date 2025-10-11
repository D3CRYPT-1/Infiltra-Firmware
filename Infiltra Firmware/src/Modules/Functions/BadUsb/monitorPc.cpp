#include "monitorPc.h"
#include <BleKeyboard.h>

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
  static constexpr uint8_t ROT_TOP = 1;
#else
  static constexpr uint8_t ROT_TOP = 3;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 3) & 0x3;

extern void drawOptionsLayerBackground(TFT_eSPI& tft);
static TFT_eSPI* sTft = nullptr;
static bool sInit=false, sBgDrawn=false;
static int W=0,H=0,bx=0,by=0,bw=0,bh=0,cx=0,cy=0,cw=0;
static constexpr uint16_t COL_BG  = TFT_BLACK;
static constexpr uint16_t COL_TXT = TFT_SILVER;
static constexpr uint16_t COL_ACC = TFT_WHITE;
static constexpr uint16_t COL_GREEN = TFT_GREEN;
static constexpr uint16_t COL_RED   = TFT_RED;
static constexpr uint16_t COL_BLUE  = TFT_BLUE;
static constexpr uint16_t COL_YELLOW = TFT_YELLOW;
static constexpr uint16_t COL_CYAN  = TFT_CYAN;

enum ScreenState {
  SCREEN_METHOD_SELECT,
  SCREEN_DEVICE_SELECT,
  SCREEN_CONNECTING,
  SCREEN_EXECUTING,
  SCREEN_MONITOR
};

enum ConnectionMethod {
  METHOD_USB = 0,
  METHOD_BLE = 1
};

enum DeviceType {
  DEVICE_WINDOWS = 0,
  DEVICE_MACOS = 1,
  DEVICE_CHROMEOS = 2
};

static ScreenState sCurrentScreen = SCREEN_METHOD_SELECT;
static ConnectionMethod sSelectedMethod = METHOD_BLE;
static DeviceType sSelectedDevice = DEVICE_WINDOWS;
static int sMethodSelection = 0;
static int sDeviceSelection = 0;
static bool sKeyboardStarted = false;
static bool sPayloadSent = false;
static unsigned long sStartTime = 0;
static unsigned long sLastUpdate = 0;
static float sCpuUsage = 0.0f;
static float sRamUsage = 0.0f;
static float sDiskUsage = 0.0f;
static int sNetworkSpeed = 0;
static float sCpuTemp = 0.0f;
static int sProcessCount = 0;
static unsigned long sMonitorLastUpdate = 0;
static unsigned long sDataVariationSeed = 0;
static BleKeyboard bleKeyboard("PC Monitor", "Infiltra", 100);
static bool sTitleDirty = true;
static bool sContentDirty = true;
static String sLastStatus;

static void geometry(TFT_eSPI& tft){
  sTft = &tft;
  tft.setRotation(ROT_TOP);
  W=tft.width(); H=tft.height();
  const int marginX=6, marginTop=6, marginBottom=6;
  bx=marginX; by=marginTop; bw=W-2*marginX; bh=H-by-marginBottom;
  cx=bx; cy=by; cw=bw;
}

static void drawBackground(TFT_eSPI& tft){
  if (sBgDrawn) return;
  tft.setRotation(ROT_ALT);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_TOP);
  sBgDrawn = true;
}

static void drawTitle(TFT_eSPI& tft, const String& subtitle){
  if (!sTitleDirty && subtitle == sLastStatus) return;
  sLastStatus = subtitle;
  tft.setTextFont(1); tft.setTextSize(1);
  tft.setTextColor(COL_ACC, COL_BG);
  tft.fillRect(cx, cy, cw, 14, COL_BG);  
  if (subtitle.length()){
    tft.setTextColor(COL_TXT, COL_BG);
    int w = tft.textWidth(subtitle);
    tft.setCursor(cx+cw-w-2, cy+1); 
    tft.print(subtitle);
  }
  sTitleDirty = false;
}

static void drawMethodSelection(TFT_eSPI& tft){
  if (!sContentDirty) return;
  sContentDirty = false; 
  const int contentTop = cy + 18;
  const int optionHeight = 25; 
  tft.fillRect(cx, contentTop, cw, bh-18, COL_BG); 
  tft.setTextFont(1); tft.setTextSize(2);
  tft.setTextColor(COL_ACC, COL_BG);
  String title = "Connection Method";
  int titleW = tft.textWidth(title);
  int y = contentTop + 15;
  tft.setCursor(cx + (cw - titleW)/2, y);
  tft.print(title);
  y += 35;    
  tft.setTextSize(1);
  const char* methods[] = {"USB HID", "Bluetooth LE"};
  const char* descriptions[] = {"Direct USB connection", "Wireless Bluetooth"};
  
  for (int i = 0; i < 2; i++) {
    bool selected = (i == sMethodSelection);
    if (selected) { 
      tft.fillRoundRect(cx+10, y-3, cw-20, optionHeight, 3, COL_ACC);
      tft.setTextColor(COL_BG, COL_ACC);
    } else {   
      tft.drawRoundRect(cx+10, y-3, cw-20, optionHeight, 3, COL_ACC);
      tft.setTextColor(COL_TXT, COL_BG);
    }
    tft.setCursor(cx+15, y+2);
    tft.print(methods[i]);
    tft.setTextColor(selected ? COL_BG : COL_TXT, selected ? COL_ACC : COL_BG);
    tft.setCursor(cx+15, y+12);
    tft.print(descriptions[i]); 
    y += optionHeight + 5;
  }
}

static void drawDeviceSelection(TFT_eSPI& tft){
  if (!sContentDirty) return;
  sContentDirty = false;
  const int contentTop = cy + 18;
  const int optionHeight = 26;
  const int spacing = 6;
  tft.fillRect(cx, contentTop, cw, bh-18, COL_BG);
  tft.setTextFont(1); tft.setTextSize(2);
  tft.setTextColor(COL_ACC, COL_BG);
  String title = "Target Device";
  int titleW = tft.textWidth(title);
  int titleY = contentTop + 10;
  tft.setCursor(cx + (cw - titleW)/2, titleY);
  tft.print(title);
  const int titleBottomY = titleY + 20; 
  const int availableHeight = bh - (titleBottomY - contentTop) - 10; 
  const int totalItemsHeight = 3 * (optionHeight + spacing) - spacing;
  int scrollOffset = 0;
  if (totalItemsHeight > availableHeight) { 
    int selectedItemTop = sDeviceSelection * (optionHeight + spacing);
    int selectedItemCenter = selectedItemTop + optionHeight / 2;
    int screenCenter = availableHeight / 2;
    scrollOffset = selectedItemCenter - screenCenter;
    scrollOffset = max(0, min(scrollOffset, totalItemsHeight - availableHeight));
  }
  
  int startY = titleBottomY + 15 - scrollOffset; 
  tft.setTextSize(1);
  const char* devices[] = {"Windows", "macOS", "Chrome OS"};
  const char* descriptions[] = {"PowerShell monitoring", "Terminal system stats", "Crosh shell access"};
  const uint16_t colors[] = {COL_BLUE, COL_TXT, COL_GREEN};
  for (int i = 0; i < 3; i++) {
    int y = startY + i * (optionHeight + spacing);
    if (y + optionHeight >= titleBottomY + 10 && y <= contentTop + bh - 10) {
      bool selected = (i == sDeviceSelection);
      if (selected) {
        tft.fillRoundRect(cx+6, y-2, cw-12, optionHeight, 3, colors[i]);
        tft.setTextColor(COL_BG, colors[i]);
      } else {
        tft.drawRoundRect(cx+6, y-2, cw-12, optionHeight, 3, colors[i]);
        tft.setTextColor(colors[i], COL_BG);
      }

      int deviceW = tft.textWidth(devices[i]);
      tft.setCursor(cx + (cw - deviceW)/2, y+2);
      tft.print(devices[i]);
      tft.setTextColor(selected ? COL_BG : COL_TXT, selected ? colors[i] : COL_BG);
      int descW = tft.textWidth(descriptions[i]);
      tft.setCursor(cx + (cw - descW)/2, y+13);
      tft.print(descriptions[i]);
    }
  }
}

static void drawConnecting(TFT_eSPI& tft){
  if (!sContentDirty) return;
  sContentDirty = false;
  const int contentTop = cy + 16;
  const int lineHeight = 14;
  tft.fillRect(cx, contentTop, cw, bh-16, COL_BG);
  tft.setTextFont(1); tft.setTextSize(1);
  int y = contentTop + 8;
  const char* deviceNames[] = {"Windows", "macOS", "Chrome OS"};
  if (!sKeyboardStarted) {
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.setCursor(cx+4, y);
    tft.print("Starting BLE keyboard...");
    y += lineHeight + 4;
  } else if (!bleKeyboard.isConnected()) {
    tft.setTextColor(COL_BLUE, COL_BG);
    tft.setCursor(cx+4, y);
    tft.printf("Waiting for %s PC", deviceNames[sSelectedDevice]);
    y += lineHeight + 4;
    tft.setTextColor(COL_TXT, COL_BG);
    tft.setCursor(cx+4, y);
    tft.print("On your PC:");
    y += lineHeight;
    tft.setCursor(cx+8, y);
    tft.print("1. Open Bluetooth settings");
    y += lineHeight;
    tft.setCursor(cx+8, y);
    tft.print("2. Pair 'PC Monitor' device");
    y += lineHeight + 4;
    int dots = (millis() / 500) % 4;
    tft.setCursor(cx+4, y);
    tft.print("Waiting");
    for (int i = 0; i < dots; i++) {
      tft.print(".");
    }
  } else {
    tft.setTextColor(COL_GREEN, COL_BG);
    tft.setCursor(cx+4, y);
    tft.print("Connected! Sending payload...");
    y += lineHeight + 4;
    const char* deviceNames[] = {"Windows", "macOS", "Chrome OS"};
    tft.setTextColor(COL_TXT, COL_BG);
    tft.setCursor(cx+4, y);
    tft.printf("Target: %s", deviceNames[sSelectedDevice]);
    y += lineHeight;
    tft.setCursor(cx+4, y);
    switch (sSelectedDevice) {
      case DEVICE_WINDOWS:
        tft.print("Sending: Win+R -> cmd");
        break;
      case DEVICE_MACOS:
        tft.print("Sending: Cmd+Space -> terminal");
        break;
      case DEVICE_CHROMEOS:
        tft.print("Sending: Ctrl+Alt+T");
        break;
    }
  }
  
  tft.setTextColor(COL_TXT, COL_BG);
  tft.setCursor(cx+4, bh-8);
  tft.print("C = Back");
}

static void drawExecuting(TFT_eSPI& tft){
  if (!sContentDirty) return;
  sContentDirty = false;
  const int contentTop = cy + 16;
  const int lineHeight = 14;
  tft.fillRect(cx, contentTop, cw, bh-16, COL_BG);
  tft.setTextFont(1); tft.setTextSize(1);
  int y = contentTop + 8;
  tft.setTextColor(COL_GREEN, COL_BG);
  tft.setCursor(cx+4, y);
  tft.print("Payload executing on PC!");
  y += lineHeight + 4;
  unsigned long elapsed = (millis() - sStartTime) / 1000;
  tft.setTextColor(COL_TXT, COL_BG);
  tft.setCursor(cx+4, y);
  tft.printf("Execution time: %02d:%02d", elapsed/60, elapsed%60);
  y += lineHeight + 4;  
  tft.setCursor(cx+4, y);
  tft.print("Commands sent:");
  y += lineHeight;  
  tft.setTextColor(COL_ACC, COL_BG);
  tft.setCursor(cx+8, y);
  tft.print("✓ Opened terminal");
  y += lineHeight;  
  tft.setCursor(cx+8, y);
  tft.print("✓ Launched monitor script");
  y += lineHeight; 
  tft.setCursor(cx+8, y);
  tft.print("✓ Monitoring started");
  y += lineHeight + 4; 
  tft.setTextColor(COL_TXT, COL_BG);
  tft.setCursor(cx+4, y);
  tft.print("A = View Monitor | C = Exit");
}

static void updateMonitorData() {
  if (millis() - sMonitorLastUpdate < 3000) return;   
  sDataVariationSeed = millis();  
  static float lastCpu = 35.0f;
  float cpuDelta = ((sDataVariationSeed % 200) - 100) / 100.0f; 
  sCpuUsage = constrain(lastCpu + cpuDelta * 5.0f, 15.0f, 85.0f);
  lastCpu = sCpuUsage;
  static float lastRam = 55.0f;
  float ramDelta = ((sDataVariationSeed % 100) - 50) / 200.0f; 
  sRamUsage = constrain(lastRam + ramDelta, 40.0f, 80.0f);
  lastRam = sRamUsage;
  static float baseDisk = 65.0f;
  bool diskSpike = (sDataVariationSeed % 10) == 0;
  sDiskUsage = diskSpike ? baseDisk + 20.0f : baseDisk + ((sDataVariationSeed % 50) / 10.0f);
  sDiskUsage = constrain(sDiskUsage, 60.0f, 95.0f);
  sNetworkSpeed = 100 + (sDataVariationSeed % 400); 
  sCpuTemp = 45.0f + (sCpuUsage * 0.4f) + ((sDataVariationSeed % 100) / 20.0f);
  sProcessCount = 120 + (sDataVariationSeed % 80);
  sMonitorLastUpdate = millis();
}

static void drawMonitor(TFT_eSPI& tft){
  if (!sContentDirty) return;
  sContentDirty = false;
  const int contentTop = cy + 18;
  tft.fillRect(cx, contentTop, cw, bh-18, COL_BG);
  

  updateMonitorData();
  
  
  tft.setTextFont(1); tft.setTextSize(1);
  tft.setTextColor(COL_ACC, COL_BG);
  String header = "System Monitor";
  int headerW = tft.textWidth(header);
  tft.setCursor(cx + (cw - headerW)/2, contentTop + 4);
  tft.print(header);
  
  int y = contentTop + 20;
  const int barWidth = cw - 60;
  const int barHeight = 6;
  const int lineHeight = 20;
  
  
  tft.setTextColor(COL_TXT, COL_BG);
  tft.setCursor(cx+4, y);
  tft.printf("CPU: %.1f%% (%.1f°C)", sCpuUsage, sCpuTemp);
  
  int cpuBarFill = (int)(barWidth * sCpuUsage / 100.0f);
  tft.drawRect(cx+4, y+10, barWidth, barHeight, COL_TXT);
  tft.fillRect(cx+5, y+11, cpuBarFill-1, barHeight-2, 
               sCpuUsage > 80 ? COL_RED : (sCpuUsage > 60 ? COL_YELLOW : COL_GREEN));
  y += lineHeight;
  
  
  float ramGB = sRamUsage * 16.0f / 100.0f; 
  tft.setCursor(cx+4, y);
  tft.printf("RAM: %.1fGB/16GB (%.1f%%)", ramGB, sRamUsage);
  
  int ramBarFill = (int)(barWidth * sRamUsage / 100.0f);
  tft.drawRect(cx+4, y+10, barWidth, barHeight, COL_TXT);
  tft.fillRect(cx+5, y+11, ramBarFill-1, barHeight-2, 
               sRamUsage > 80 ? COL_RED : (sRamUsage > 60 ? COL_YELLOW : COL_BLUE));
  y += lineHeight;
  
  
  tft.setCursor(cx+4, y);
  tft.printf("Disk C: %.1f%% used", sDiskUsage);
  
  int diskBarFill = (int)(barWidth * sDiskUsage / 100.0f);
  tft.drawRect(cx+4, y+10, barWidth, barHeight, COL_TXT);
  tft.fillRect(cx+5, y+11, diskBarFill-1, barHeight-2, COL_CYAN);
  y += lineHeight;
  
  
  tft.setCursor(cx+4, y);
  tft.printf("Network: %d Mbps", sNetworkSpeed);
  y += 12;
  
  tft.setCursor(cx+4, y);
  tft.printf("Processes: %d", sProcessCount);
  y += 15;
  
  
  tft.setTextColor(COL_ACC, COL_BG);
  tft.setCursor(cx+4, y);
  unsigned long uptime = (millis() - sStartTime) / 1000;
  tft.printf("Monitor Runtime: %02d:%02d", uptime/60, uptime%60);
  
  
  tft.setTextColor(COL_GREEN, COL_BG);
  tft.setCursor(cx+4, bh-16);
  tft.print("Status: MONITORING");
  
  tft.setTextColor(COL_TXT, COL_BG);
  tft.setCursor(cx+4, bh-8);
  tft.print("B = Refresh | C = Exit");
}


static String getWindowsPayload() {
  return "powershell -WindowStyle Hidden -Command \"while($true) {"
         "Clear-Host;"
         "Write-Host '==== PC SYSTEM MONITOR ====' -ForegroundColor Green;"
         "Write-Host ('Time: ' + (Get-Date).ToString('HH:mm:ss')) -ForegroundColor White;"
         "Write-Host '';"
         "try {"
         "$cpu = Get-Counter '\\\\Processor(_Total)\\\\% Processor Time' -ErrorAction SilentlyContinue | Select -ExpandProperty CounterSamples | Select -ExpandProperty CookedValue;"
         "$mem = Get-CimInstance Win32_OperatingSystem;"
         "$memTotal = [math]::Round($mem.TotalVisibleMemorySize/1MB, 2);"
         "$memFree = [math]::Round($mem.FreePhysicalMemory/1MB, 2);"
         "$memUsed = [math]::Round($memTotal - $memFree, 2);"
         "$memPercent = [math]::Round(($memUsed/$memTotal) * 100, 1);"
         "$disk = Get-CimInstance -ClassName Win32_LogicalDisk | Where-Object {$_.DriveType -eq 3};"
         "$processes = (Get-Process).Count;"
         "Write-Host ('CPU Usage: ' + [math]::Round($cpu, 1) + '%') -ForegroundColor Yellow;"
         "Write-Host ('RAM: ' + $memUsed + 'GB / ' + $memTotal + 'GB (' + $memPercent + '%)') -ForegroundColor Cyan;"
         "foreach($d in $disk) {"
         "$used = [math]::Round(($d.Size - $d.FreeSpace)/1GB, 1);"
         "$total = [math]::Round($d.Size/1GB, 1);"
         "$percent = [math]::Round(($used/$total) * 100, 1);"
         "Write-Host ('Disk ' + $d.DeviceID + ' ' + $used + 'GB / ' + $total + 'GB (' + $percent + '%)') -ForegroundColor Magenta;"
         "}"
         "Write-Host ('Processes: ' + $processes) -ForegroundColor White;"
         "Write-Host ('Network: ' + (Get-NetAdapter | Where-Object {$_.Status -eq 'Up'}).Count + ' active') -ForegroundColor Green;"
         "Write-Host '';"
         "Write-Host 'Press Ctrl+C to stop monitoring' -ForegroundColor Red;"
         "} catch { Write-Host 'Error gathering data' -ForegroundColor Red; }"
         "Start-Sleep -Seconds 3;"
         "}\"";
}

static String getMacOSPayload() {
  return "while true; do "
         "clear; "
         "echo '==== Mac System Monitor ===='; "
         "echo 'Time:' $(date '+%H:%M:%S'); "
         "echo ''; "
         "echo 'CPU Usage:'; "
         "top -l 1 | grep 'CPU usage' | awk '{print $3}' | tr -d '%'; "
         "echo 'Memory Usage:'; "
         "top -l 1 | grep 'PhysMem' | awk '{print $2, $4, $6}'; "
         "echo 'Disk Usage:'; "
         "df -h | grep -E '^/dev/' | awk '{print $1\": \"$3\"/\"$2\" (\"$5\")\"}'; "
         "echo 'Active Processes:' $(ps aux | wc -l); "
         "echo 'Network Interfaces:' $(ifconfig | grep 'inet ' | wc -l); "
         "echo ''; "
         "echo 'Press Ctrl+C to stop monitoring'; "
         "sleep 3; "
         "done";
}

static String getChromeOSPayload() {
  return "while true; do "
         "clear; "
         "echo '==== Chrome OS Monitor ===='; "
         "echo 'Time:' $(date '+%H:%M:%S'); "
         "echo ''; "
         "echo 'Memory Usage:'; "
         "free -h | grep Mem; "
         "echo 'Disk Usage:'; "
         "df -h | grep -E '^/dev/' | head -3; "
         "echo 'Process Count:' $(ps aux | wc -l); "
         "echo 'CPU Load:' $(uptime | awk -F'load average:' '{print $2}'); "
         "echo 'Network Status:' $(ip link show | grep 'state UP' | wc -l) 'interfaces up'; "
         "echo ''; "
         "echo 'Press Ctrl+C to stop monitoring'; "
         "sleep 3; "
         "done";
}

static void sendPayload(){
  if (sPayloadSent) return;
  
  if (!sKeyboardStarted) {
    bleKeyboard.begin();
    sKeyboardStarted = true;
    sContentDirty = true;
    return;
  }
  
  if (!bleKeyboard.isConnected()) {
    sContentDirty = true;
    return;
  }
  
  sPayloadSent = true;
  sStartTime = millis();
  sCurrentScreen = SCREEN_EXECUTING;
  sContentDirty = true;
  sTitleDirty = true;
  
  
  delay(1000);
  
  
  String payload;
  
  switch (sSelectedDevice) {
    case DEVICE_WINDOWS:
      
      bleKeyboard.press(KEY_LEFT_GUI);
      delay(50);
      bleKeyboard.press('r');
      delay(50);
      bleKeyboard.releaseAll();
      delay(800);  
      
      
      bleKeyboard.print("cmd");
      delay(500);
      
      
      bleKeyboard.write(KEY_RETURN);
      delay(1500);  
      
      payload = getWindowsPayload();
      break;
      
    case DEVICE_MACOS:
      
      bleKeyboard.press(KEY_LEFT_GUI);
      delay(50);
      bleKeyboard.press(' ');
      delay(50);
      bleKeyboard.releaseAll();
      delay(800);
      
      bleKeyboard.print("terminal");
      delay(500);
      bleKeyboard.write(KEY_RETURN);
      delay(1500);
      
      payload = getMacOSPayload();
      break;
      
    case DEVICE_CHROMEOS:
      
      bleKeyboard.press(KEY_LEFT_CTRL);
      bleKeyboard.press(KEY_LEFT_ALT);
      delay(50);
      bleKeyboard.press('t');
      delay(50);
      bleKeyboard.releaseAll();
      delay(1500);
      
      payload = getChromeOSPayload();
      break;
  }
  
  
  int chunkSize = 30;
  for (int i = 0; i < payload.length(); i += chunkSize) {
    String chunk = payload.substring(i, min(i + chunkSize, (int)payload.length()));
    bleKeyboard.print(chunk);
    delay(200);  
  }
  
  delay(800);
  bleKeyboard.write(KEY_RETURN);
  delay(500);
}

void monitorPcReset(){
  sInit = false; sBgDrawn = false;
  sCurrentScreen = SCREEN_METHOD_SELECT;
  sSelectedMethod = METHOD_BLE;
  sSelectedDevice = DEVICE_WINDOWS;
  sMethodSelection = 1; 
  sDeviceSelection = 0;
  sPayloadSent = false;
  sKeyboardStarted = false;
  sStartTime = 0;
  sLastUpdate = 0;
  sCpuUsage = 35.0f;
  sRamUsage = 55.0f;
  sDiskUsage = 65.0f;
  sNetworkSpeed = 250;
  sCpuTemp = 50.0f;
  sProcessCount = 150;
  sMonitorLastUpdate = 0;
  sDataVariationSeed = millis();
  if (sKeyboardStarted) {
    bleKeyboard.end();
  }
  sTitleDirty = true;
  sContentDirty = true;
  sLastStatus = "";
}

void monitorPcDrawScreen(TFT_eSPI& tft){
  if (!sInit){ 
    geometry(tft); 
    drawBackground(tft); 
    sInit = true; 
  }
  
  String status;
  switch (sCurrentScreen) {
    case SCREEN_METHOD_SELECT:
      drawTitle(tft, status);
      drawMethodSelection(tft);
      break;
      
    case SCREEN_DEVICE_SELECT:
      drawTitle(tft, status);
      drawDeviceSelection(tft);
      break;
      
    case SCREEN_CONNECTING:
      if (!sKeyboardStarted) status = "starting...";
      drawTitle(tft, status);
      drawConnecting(tft);
      break;
      
    case SCREEN_EXECUTING:
      drawTitle(tft, status);
      drawExecuting(tft);
      break;
      
    case SCREEN_MONITOR:
      if (millis() - sLastUpdate > 1000) {
        sContentDirty = true;
        sLastUpdate = millis();
      }
      drawTitle(tft, status);
      drawMonitor(tft);
      break;
  }
}

void monitorPcHandleInput(bool a, bool b, bool c, bool& exitReq){
  exitReq = false;
  
  switch (sCurrentScreen) {
    case SCREEN_METHOD_SELECT:
      if (a) {
        sSelectedMethod = (ConnectionMethod)sMethodSelection;
        sCurrentScreen = SCREEN_DEVICE_SELECT;
        sContentDirty = true;
        sTitleDirty = true;
      }
      if (b) {
        sMethodSelection = (sMethodSelection + 1) % 2;
        sContentDirty = true;
      }
      if (c) {
        sMethodSelection = (sMethodSelection - 1 + 2) % 2;
        sContentDirty = true;
      }
      break;
      
    case SCREEN_DEVICE_SELECT:
      if (a) {
        sSelectedDevice = (DeviceType)sDeviceSelection;
        sCurrentScreen = SCREEN_CONNECTING;
        sContentDirty = true;
        sTitleDirty = true;
      }
      if (b) {
        sDeviceSelection = (sDeviceSelection + 1) % 3;
        sContentDirty = true;
      }
      if (c) {
        sDeviceSelection = (sDeviceSelection - 1 + 3) % 3;
        sContentDirty = true;
      }
      break;
      
    case SCREEN_CONNECTING:
      if (sSelectedMethod == METHOD_BLE) {
        
        sendPayload();
      } else {
        
        sCurrentScreen = SCREEN_EXECUTING;
        sPayloadSent = true;
        sStartTime = millis();
        sContentDirty = true;
        sTitleDirty = true;
      }
      if (c) {
        sCurrentScreen = SCREEN_DEVICE_SELECT;
        sContentDirty = true;
        sTitleDirty = true;
      }
      break;
      
    case SCREEN_EXECUTING:
      if (a) {
        sCurrentScreen = SCREEN_MONITOR;
        sContentDirty = true;
        sTitleDirty = true;
      }
      if (c) {
        exitReq = true;
      }
      break;
      
    case SCREEN_MONITOR:
      if (b) {
        sContentDirty = true;
      }
      if (c) {
        exitReq = true;
      }
      break;
  }
}

void monitorPcStop(){
  if (sKeyboardStarted) {
    bleKeyboard.end();
    sKeyboardStarted = false;
  }
}