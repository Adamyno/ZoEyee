#include "WifiManager.h"
#include "DisplayManager.h"
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

void WifiManager::showMenu() {
  gfx->fillScreen(BLACK);
  DisplayManager::drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(134, 35);
  gfx->print("WIFI");
  gfx->drawLine(0, 40, 320, 40, WHITE);

  // AP Mode button
  uint16_t apColor = wifiAPActive ? GREEN : WHITE;
  gfx->drawRoundRect(30, 50, 260, 28, 6, apColor);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(apColor);
  gfx->setCursor(100, 70);
  gfx->print(wifiAPActive ? "AP MODE: ON" : "AP MODE: OFF");

  // Client Menu button
  gfx->drawRoundRect(30, 88, 260, 28, 6, WHITE);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setCursor(85, 108);
  gfx->print("CLIENT MENU");

  // Status button
  gfx->drawRoundRect(30, 126, 260, 28, 6, CYAN);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(CYAN);
  gfx->setCursor(120, 146);
  gfx->print("STATUS");
}

void WifiManager::showClientMenu() {
  gfx->fillScreen(BLACK);
  DisplayManager::drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(85, 35);
  gfx->print("CLIENT MENU");
  gfx->drawLine(0, 40, 320, 40, WHITE);

  // Scan button
  gfx->drawRoundRect(30, 50, 260, 28, 6, CYAN);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(CYAN);
  gfx->setCursor(75, 70);
  gfx->print("SCAN NETWORKS");

  // Save Config checkbox
  gfx->drawRect(30, 92, 20, 20, WHITE);
  if (wifiAutoSave) {
    gfx->drawLine(30, 92, 50, 112, GREEN);
    gfx->drawLine(50, 92, 30, 112, GREEN);
  }
  gfx->setTextColor(WHITE);
  gfx->setCursor(60, 108);
  gfx->print("Save & Auto-Conn");

  // Delete button (only if saved target SSID exists)
  if (preferences.getString("ssid", "").length() > 0) {
    gfx->fillRoundRect(230, 88, 60, 28, 6, RED);
    gfx->setTextColor(WHITE);
    gfx->setCursor(240, 108);
    gfx->print("DEL");
  }

  // Disconnect button
  bool connected = (WiFi.status() == WL_CONNECTED);
  uint16_t disColor = connected ? RED : 0x3186; // Grey if not connected
  gfx->drawRoundRect(30, 126, 260, 28, 6, disColor);
  gfx->setTextColor(disColor);
  gfx->setCursor(100, 146);
  gfx->print("DISCONNECT");
}

void WifiManager::showList(bool fullRedraw) {
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    DisplayManager::drawTopBar();
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW);
    gfx->setTextSize(1);
    gfx->setCursor(95, 35);
    gfx->print("WIFI LIST");
    gfx->drawLine(0, 40, 320, 40, WHITE);
  }

  if (wifiCount == 0) {
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(RED);
    gfx->setCursor(80, 100);
    gfx->print("No networks found");
    return;
  }

  // Clear dynamic area
  gfx->fillRect(0, 45, 320, 130, BLACK);

  // Navigation arrows
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(CYAN);
  gfx->setCursor(5, 80);
  gfx->print("\x11"); // Left triangle
  gfx->setCursor(295, 80);
  gfx->print("\x10"); // Right triangle

  // Network name
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  String dispName = wifiNetworks[wifiSelectedIndex].ssid;
  if (dispName.length() > 22)
    dispName = dispName.substring(0, 20) + "..";
  int16_t tx = 160 - (dispName.length() * 5);
  if (tx < 35)
    tx = 35;
  gfx->setCursor(tx, 70);
  gfx->print(dispName.c_str());

  // Signal strength bars
  int rssi = wifiNetworks[wifiSelectedIndex].rssi;
  int bars = (rssi > -50) ? 4 : (rssi > -65) ? 3 : (rssi > -75) ? 2 : 1;
  int barX = 110;
  int barBaseY = 98;
  for (int b = 0; b < 4; b++) {
    int barH = 4 + b * 4;
    uint16_t barColor = (b < bars) ? CYAN : 0x3186;
    gfx->fillRect(barX + b * 8, barBaseY - barH, 5, barH, barColor);
  }

  // Lock icon (if encrypted)
  if (wifiNetworks[wifiSelectedIndex].encrypted) {
    int lockX = barX + 42;
    int lockY = barBaseY - 14;
    gfx->fillRect(lockX, lockY + 5, 10, 7, 0xFBE0);        // Narancs test
    gfx->drawRoundRect(lockX + 1, lockY, 8, 7, 3, 0xFBE0); // Félkör szár
  }

  // Index
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(130, 118);
  gfx->printf("%d / %d", wifiSelectedIndex + 1, wifiCount);

  // Connect button
  gfx->fillRoundRect(90, 130, 140, 30, 8, GREEN);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(BLACK);
  gfx->setCursor(120, 151);
  gfx->print("CONNECT");
}

void WifiManager::showKeyboard() {
  gfx->fillScreen(BLACK);

  // Password display bar
  gfx->fillRect(0, 0, 320, 24, 0x18E3); // Dark gray bar
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(5, 17);
  String displayPw = wifiPassword;
  if (displayPw.length() > 28)
    displayPw = displayPw.substring(displayPw.length() - 28);
  gfx->print(displayPw.c_str());
  gfx->setTextColor(CYAN);
  gfx->print("_");

  // Draw keyboard
  const char **rows =
      kbNumbers ? kbRowsNum : (kbShift ? kbRowsUpper : kbRowsLower);
  int keyW = 28, keyH = 28;
  int startYkb = 28;

  for (int r = 0; r < 3; r++) {
    int rowLen = strlen(rows[r]);
    int rowStartX = (320 - rowLen * keyW) / 2;
    for (int c = 0; c < rowLen; c++) {
      int kx = rowStartX + c * keyW;
      int ky = startYkb + r * (keyH + 4);
      gfx->drawRoundRect(kx, ky, keyW - 2, keyH, 4, 0x7BEF);
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(WHITE);
      gfx->setCursor(kx + 7, ky + 20);
      char ch[2] = {rows[r][c], 0};
      gfx->print(ch);
    }
  }

  // Bottom row: Shift/123, Space, Backspace, OK
  int btnY = startYkb + 3 * (keyH + 4);

  // Shift / 123 button
  gfx->drawRoundRect(5, btnY, 55, keyH, 4, YELLOW);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(12, btnY + 20);
  if (kbNumbers)
    gfx->print("abc");
  else if (kbShift)
    gfx->print("123");
  else
    gfx->fillTriangle(20, btnY + 22, 32, btnY + 6, 44, btnY + 22,
                      YELLOW); // Shift arrow

  // Space bar
  gfx->drawRoundRect(65, btnY, 120, keyH, 4, 0x7BEF);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(100, btnY + 20);
  gfx->print("SPACE");

  // Backspace / Back
  if (wifiPassword.length() == 0) {
    gfx->fillRoundRect(190, btnY, 55, keyH, 4, RED);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(WHITE);
    gfx->setCursor(193, btnY + 20);
    gfx->print("BACK");
  } else {
    gfx->drawRoundRect(190, btnY, 55, keyH, 4, RED);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(RED);
    gfx->setCursor(200, btnY + 20);
    gfx->print("DEL");
  }

  // OK button
  gfx->fillRoundRect(250, btnY, 65, keyH, 4, GREEN);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(BLACK);
  gfx->setCursor(268, btnY + 20);
  gfx->print("OK");
}

void WifiManager::connect() {
  currentState = STATE_WIFI_CONNECTING;
  gfx->fillScreen(BLACK);
  DisplayManager::drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(70, 80);
  gfx->print("Connecting...");
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setCursor(50, 110);
  gfx->print(wifiTargetSSID.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (wifiPassword.length() > 0)
    WiFi.begin(wifiTargetSSID.c_str(), wifiPassword.c_str());
  else
    WiFi.begin(wifiTargetSSID.c_str());

  unsigned long startWait = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWait < 12000) {
    delay(500);
    gfx->setTextColor(CYAN);
    gfx->print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (wifiAutoSave) {
      preferences.putString("ssid", wifiTargetSSID);
      preferences.putString("pw", wifiPassword);
    }
    gfx->fillRect(0, 120, 320, 30, BLACK);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(GREEN);
    gfx->setCursor(90, 140);
    gfx->print("Connected!");
    DisplayManager::drawTopBar(); // Update WiFi icon
    delay(1500);
    currentState = STATE_WIFI_STATUS;
    showStatus();
  } else {
    gfx->fillRect(0, 120, 320, 30, BLACK);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(RED);
    gfx->setCursor(70, 140);
    gfx->print("Connection failed!");
    delay(2000);
    currentState = STATE_WIFI_MENU;
    showMenu();
  }
}

void WifiManager::showStatus() {
  gfx->fillScreen(BLACK);
  DisplayManager::drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(80, 35);
  gfx->print("WIFI STATUS");
  gfx->drawLine(0, 40, 320, 40, WHITE);

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextSize(1);

  if (wifiAPActive) {
    gfx->setTextColor(GREEN);
    gfx->setCursor(10, 65);
    gfx->print("Mode: Access Point");

    gfx->setTextColor(WHITE);
    gfx->setCursor(10, 90);
    gfx->print("SSID: ZoEyee-Config");
    gfx->setCursor(10, 115);
    gfx->printf("IP: %s", WiFi.softAPIP().toString().c_str());
  } else if (WiFi.status() == WL_CONNECTED) {
    gfx->setTextColor(GREEN);
    gfx->setCursor(10, 65);
    gfx->print("Mode: Client (Connected)");

    gfx->setTextColor(WHITE);
    gfx->setCursor(10, 90);
    gfx->printf("SSID: %s", WiFi.SSID().c_str());
    gfx->setCursor(10, 115);
    gfx->printf("IP: %s", WiFi.localIP().toString().c_str());
    gfx->setCursor(10, 140);
    gfx->printf("GW: %s", WiFi.gatewayIP().toString().c_str());
    gfx->setCursor(10, 165);
    gfx->printf("RSSI: %d dBm", WiFi.RSSI());
  } else {
    gfx->setTextColor(RED);
    gfx->setCursor(10, 90);
    gfx->print("Not Connected");
  }
}
