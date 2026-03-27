#include "Icons.h"
#include "ZoeLogo.h"
#include <Arduino_GFX_Library.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <math.h>

#include "Config.h"
#include "Globals.h"
#include "DisplayManager.h"
#include "TouchManager.h"
#include "WifiManager.h"
#include "BluetoothManager.h"
#include "ObdManager.h"
#include "WebConsole.h"

// UI and Touch logics moved to DisplayManager.cpp and TouchManager.cpp

// =================================================================================
// WiFi Management UI Functions moved to WifiManager.cpp
// =================================================================================
// Bluetooth and OBD logics moved to BluetoothManager.cpp and ObdManager.cpp

#ifndef PIO_UNIT_TESTING
void setup(void) {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  ledcAttach(TFT_BL, 5000, 8);
  DisplayManager::setBrightness(currentBrightness);
  if (!gfx->begin())
    Serial.println("gfx->begin() failed!");
  DisplayManager::initLCD();
  gfx->setRotation(1);
  TouchManager::init();

  preferences.begin("wifi", false);
  wifiAutoSave = preferences.getBool("auto", false);
  wifiTargetSSID = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("pw", "");
  btTargetMAC = preferences.getString("bt_mac", "");
  btTargetName = preferences.getString("bt_name", "");
  btTargetType = preferences.getUChar("bt_type", 0);
  numPages = preferences.getUChar("pages", 1);
  if (numPages < 1) numPages = 1;
  if (numPages > MAX_PAGES) numPages = MAX_PAGES;
  // Load slot configurations
  for (int p = 0; p < MAX_PAGES; p++) {
    for (int s = 0; s < 6; s++) {
      char key[8];
      snprintf(key, sizeof(key), "p%ds%d", p, s);
      dashPages[p][s].paramIndex = preferences.getChar(key, -1);
    }
  }
  
  WiFi.mode(WIFI_STA); // Initialize LwIP networking stack unconditionally to avoid WebServer crash
  
  if (wifiAutoSave && wifiTargetSSID.length() > 0) {
    WiFi.setAutoReconnect(true);
    if (wifiPassword.length() > 0)
      WiFi.begin(wifiTargetSSID.c_str(), wifiPassword.c_str());
    else
      WiFi.begin(wifiTargetSSID.c_str());
  }

  // Draw Boot Logo
  gfx->fillScreen(BLACK);
  // Pozicionálás: 320x172 kijelzőn közepelve (zoe256: 256x106)
  gfx->drawXBitmap(32, 15, zoe256_bits, zoe256_width, zoe256_height, WHITE);

  // ZoEyee felirat
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(CYAN);
  gfx->setTextSize(1);
  gfx->setCursor(115, 140);
  gfx->print("ZoEyee");

  // Verziószám (automatikusan középre igazítva)
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(0x7BEF); // Világosszürke
  int16_t vx1, vy1;
  uint16_t vw, vh;
  gfx->getTextBounds(SW_VERSION, 0, 0, &vx1, &vy1, &vw, &vh);
  gfx->setCursor((320 - vw) / 2, 162);
  gfx->print(SW_VERSION);

  delay(5000);

  DisplayManager::showHome();

  NimBLEDevice::init("ZoEyee-Scanner");
  NimBLEDevice::setOwnAddrType(
      BLE_OWN_ADDR_PUBLIC); // Fixált gyári MAC – Konnwei ne utasítsa el!
  
  WebConsole::begin(); // Start the OBD diagnostic web server
  
  Serial.printf("[SYS] Setup Kész. Free heap: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
  WebConsole::handleClient(); // Process incoming Web Console HTTP requests
  TouchManager::processGestures();

  // WiFi Connection State Transition Timer
  if (currentState == STATE_WIFI_CONNECTING && wifiTransitionTime > 0 && millis() > wifiTransitionTime) {
    wifiTransitionTime = 0;
    currentState = wifiNextState;
    if (currentState == STATE_WIFI_STATUS) {
      WifiManager::showStatus();
    } else if (currentState == STATE_WIFI_MENU) {
      WifiManager::showMenu();
    }
  }

  // Async WiFi scan completion handler
  if (wifiScanning) {
    int16_t scanResult = WiFi.scanComplete();
    if (scanResult >= 0) {
      // Scan finished
      wifiScanning = false;
      wifiCount = min((int)scanResult, MAX_WIFI_NETWORKS);
      for (int i = 0; i < wifiCount; i++) {
        wifiNetworks[i].ssid = WiFi.SSID(i);
        wifiNetworks[i].rssi = WiFi.RSSI(i);
        wifiNetworks[i].encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      }
      WiFi.scanDelete();
      wifiSelectedIndex = 0;
      currentState = STATE_WIFI_LIST;
      WifiManager::showList();
    } else if (scanResult == WIFI_SCAN_FAILED) {
      // Scan failed
      wifiScanning = false;
      wifiCount = 0;
      currentState = STATE_WIFI_LIST;
      WifiManager::showList();
    }
    // else: WIFI_SCAN_RUNNING — still scanning, do nothing
  }

  // Auto-scroll update for BT List
  if (currentState == STATE_BT_LIST && btTotalDevices > 0) {
    if (btDevices[btSelectedDeviceIndex].name.length() > 18) {
      static unsigned long lastMarqueeUpdate = 0;
      if (millis() - lastMarqueeUpdate > 400) {
        lastMarqueeUpdate = millis();
        BluetoothManager::showList(false); // Update only the dynamic region
      }
    }
  }

  static bool lastBtnState = HIGH;
  bool currentBtnState = digitalRead(BTN_PIN);
  if (lastBtnState == HIGH && currentBtnState == LOW) {
    if (currentState == STATE_MENU) {
      menuIndex = (menuIndex + 1) % menuCount;
      DisplayManager::drawMenu(false);
    } else {
      currentState = STATE_MENU;
      DisplayManager::drawMenu();
    }
    delay(200);
  }
  lastBtnState = currentBtnState;

  if (bleDisconnectedFlag) {
    bleDisconnectedFlag = false;
    Serial.println("[BLE] Disconnect detected, refreshing UI");
    if (currentState == STATE_HOME)
      DisplayManager::showHome();
    else if (currentState == STATE_BT_LIST)
      BluetoothManager::showList(true);
    else
      DisplayManager::drawTopBar();
  }

  // Periodic top bar refresh (WiFi signal bars update)
  static unsigned long lastTopBarRefresh = 0;
  static int lastWifiRssi = 0;
  static bool lastWifiConnected = false;
  static bool lastWifiAP = false;
  static bool lastBtConnected = false;
  static bool lastCanActive = false;

  if (millis() - lastTopBarRefresh > 1000) {
    lastTopBarRefresh = millis();
    bool isWifiConn = (WiFi.status() == WL_CONNECTED);
    bool isWifiAP =
        (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    int currentRssi = isWifiConn ? WiFi.RSSI() : 0;

    // Evaluate if redraw is needed based on visual changes
    int oldBars = (lastWifiRssi > -50)   ? 4
                  : (lastWifiRssi > -65) ? 3
                  : (lastWifiRssi > -75) ? 2
                                         : 1;
    int newBars = (currentRssi > -50)   ? 4
                  : (currentRssi > -65) ? 3
                  : (currentRssi > -75) ? 2
                                        : 1;
    bool canActive = (isBluetoothConnected && lastOBDRxTime > 0 &&
                      millis() - lastOBDRxTime < 10000);

    bool needsRefresh = false;
    if (isWifiConn != lastWifiConnected || isWifiAP != lastWifiAP)
      needsRefresh = true;
    if (isWifiConn && oldBars != newBars)
      needsRefresh = true;
    if (isBluetoothConnected != lastBtConnected)
      needsRefresh = true;
    if (canActive != lastCanActive)
      needsRefresh = true;

    if (needsRefresh) {
      lastWifiConnected = isWifiConn;
      lastWifiAP = isWifiAP;
      lastWifiRssi = currentRssi;
      lastBtConnected = isBluetoothConnected;
      lastCanActive = canActive;

      if (currentState == STATE_HOME || currentState == STATE_MENU) {
        DisplayManager::drawTopBar(true); // Soft refresh
      }
    }
  }

  if (currentState == STATE_HOME && isBluetoothConnected && !bleConnecting) {
    ObdManager::processPolling();
  }

  // --- Auto-Reconnect Logic ---
  static unsigned long lastWifiReconnectTime = 0;
  if (wifiAutoSave && wifiTargetSSID.length() > 0 && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiReconnectTime > 10000) {
        lastWifiReconnectTime = millis();
        // UI feedback handled implicitly by drawTopBar red circle
        WiFi.disconnect();
        if (wifiPassword.length() > 0) WiFi.begin(wifiTargetSSID.c_str(), wifiPassword.c_str());
        else WiFi.begin(wifiTargetSSID.c_str());
    }
  }

  static unsigned long lastBtReconnectTime = 0;
  if (currentState == STATE_HOME && !isBluetoothConnected && !bleConnecting && btTargetMAC.length() > 0
      && btReconnectTaskHandle == nullptr) {
    if (millis() - lastBtReconnectTime > 20000) {
        lastBtReconnectTime = millis();
        DisplayManager::drawTopBar(true); 
        BluetoothManager::startReconnectTask(btTargetMAC); // Non-blocking!
    }
  }

  // Handle BT reconnect task completion
  if (btReconnectDone) {
    btReconnectDone = false;
    if (btReconnectResult) {
      Serial.println("[BLE] Auto-reconnect SUCCESS");
      if (currentState == STATE_HOME) {
        DisplayManager::showHome();
      }
    } else {
      Serial.println("[BLE] Auto-reconnect FAILED");
    }
    DisplayManager::drawTopBar(true);
  }
  // ----------------------------

  // Page indicator timeout
  if (currentState == STATE_HOME && pageSwipeTime > 0 && millis() - pageSwipeTime > 2000) {
    pageSwipeTime = 0;
    // Clear indicator area
    gfx->fillRect(0, 156, 320, 16, BLACK);
  }

  delay(20);
}
#endif