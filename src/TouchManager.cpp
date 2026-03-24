#include "TouchManager.h"
#include "Config.h"
#include "Globals.h"
#include "DisplayManager.h"
#include "WifiManager.h"
#include "BluetoothManager.h"
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <WiFi.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

void TouchManager::init() {
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);
  digitalWrite(TOUCH_RST, HIGH);
  delay(100);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
}

bool TouchManager::read(int &x, int &y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x01);
  if (Wire.endTransmission() != 0)
    return false;
  if (Wire.requestFrom(TOUCH_ADDR, 11) != 11)
    return false;

  uint8_t data[11];
  for (int i = 0; i < 11; i++)
    data[i] = Wire.read();
  if ((data[1] & 0x0F) == 0)
    return false;

  lastRawX = (((int)(data[2] & 0x0F)) << 8) | data[3];
  lastRawY = (((int)(data[4] & 0x0F)) << 8) | data[5];
  x = lastRawY;
  y = lastRawX;
  return true;
}

void TouchManager::processGestures() {
  int tx, ty;
  if (read(tx, ty)) {
    if (!touching) {
      startX = tx;
      startY = ty;
      touching = true;
      isSwipingBrightness = false;
      touchStartTime = millis();
    }
    int deltaX = tx - startX;
    int deltaY = ty - startY;

    if (abs(deltaY) > 10 && abs(deltaX) < 40) {
      isSwipingBrightness = true;
      int newBright = currentBrightness - deltaY;
      newBright = constrain(newBright, 0, 255);
      if (newBright != currentBrightness) {
        DisplayManager::setBrightness(newBright);
        if (currentState == STATE_BRIGHTNESS)
          DisplayManager::showBrightness(false);
      }
      startY = ty;
    }
    lastX = tx;
    lastY = ty;
  } else {
    if (touching) {
      if (isSwipingBrightness) {
        touching = false;
        return;
      }
      int deltaX = lastX - startX;
      int deltaY = lastY - startY;
      unsigned long tapDuration = millis() - touchStartTime;

      if (currentState == STATE_HOME) {
        if (abs(deltaX) < 30 && abs(deltaY) < 30 && tapDuration < 500) {
          if (millis() - lastTapTime < 500) {
            currentState = STATE_MENU;
            DisplayManager::drawMenu();
            lastTapTime = 0;
          } else {
            lastTapTime = millis();
          }
        }
        touching = false;
        return;
      }

      if (abs(deltaY) < 50 && tapDuration < 500) {
        if (deltaX > 80) {
          if (currentState == STATE_MENU) {
            menuIndex = (menuIndex - 1 + menuCount) % menuCount;
            DisplayManager::drawMenu(false);
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
              btSelectedDeviceIndex =
                  (btSelectedDeviceIndex - 1 + btTotalDevices) % btTotalDevices;
              BluetoothManager::showList(false);
            } else {
              currentState = STATE_MENU;
              DisplayManager::drawMenu();
            }
          } else if (currentState == STATE_WIFI_LIST) {
            if (wifiCount > 0) {
              wifiSelectedIndex =
                  (wifiSelectedIndex - 1 + wifiCount) % wifiCount;
              WifiManager::showList(false);
            }
          } else if (currentState == STATE_BT_DEVICE_INFO) {
            currentState = STATE_BT_LIST;
            BluetoothManager::showList();
          } else {
            currentState = STATE_MENU;
            DisplayManager::drawMenu();
          }
        } else if (deltaX < -80) {
          if (currentState == STATE_MENU) {
            menuIndex = (menuIndex + 1) % menuCount;
            DisplayManager::drawMenu(false);
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
              btSelectedDeviceIndex =
                  (btSelectedDeviceIndex + 1) % btTotalDevices;
              BluetoothManager::showList(false);
            }
          } else if (currentState == STATE_WIFI_LIST) {
            if (wifiCount > 0) {
              wifiSelectedIndex = (wifiSelectedIndex + 1) % wifiCount;
              WifiManager::showList(false);
            }
          }
        } else if (abs(deltaX) < 15 && abs(deltaY) < 15) {
          if (startY < 40 && startX < 80) {
            if (currentState == STATE_MENU) {
              currentState = STATE_HOME;
              DisplayManager::showHome();
            } else if (currentState == STATE_BT_DEVICE_INFO) {
              currentState = STATE_BT_LIST;
              BluetoothManager::showList();
            } else if (currentState == STATE_WIFI_CLIENT_MENU) {
              currentState = STATE_WIFI_MENU;
              WifiManager::showMenu();
            } else if (currentState == STATE_WIFI_LIST) {
              currentState = STATE_WIFI_CLIENT_MENU;
              WifiManager::showClientMenu();
            } else if (currentState == STATE_WIFI_MENU ||
                       currentState == STATE_WIFI_STATUS) {
              currentState = STATE_MENU;
              DisplayManager::drawMenu();
            } else if (currentState == STATE_WIFI_KEYBOARD) {
              // Keyboard: ne lépjen ki a bal felső sarokra koppintva!
              // A BACK gombot kell használni (DEL-en ha üres a jelszó)
            } else if (currentState != STATE_HOME) {
              currentState = STATE_MENU;
              DisplayManager::drawMenu();
            }
          } else if (currentState == STATE_MENU) {
            if (startY < 110) {
              if (menuIndex == 0) {
                currentState = STATE_INFO;
                DisplayManager::showInfo();
              } else if (menuIndex == 1) {
                currentState = STATE_WIFI_MENU;
                WifiManager::showMenu();
              } else if (menuIndex == 2) {
                if (isBluetoothConnected) {
                  currentState = STATE_BT_STATUS;
                  BluetoothManager::showStatus();
                } else {
                  currentState = STATE_BT_SCAN;
                  BluetoothManager::runBLEScan();
                }
              } else if (menuIndex == 3) {
                currentState = STATE_BRIGHTNESS;
                DisplayManager::showBrightness();
              }
            }
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
              if (startX >= 20 && startX <= 150 && startY >= 110 &&
                  startY <= 145) {
                if (!isBluetoothConnected && !bleConnecting)
                  BluetoothManager::connect(btSelectedDeviceIndex);
                else if (isBluetoothConnected)
                  BluetoothManager::disconnect();
                BluetoothManager::showList(true);
              } else if (startX >= 170 && startX <= 300 && startY >= 110 &&
                         startY <= 145) {
                currentState = STATE_BT_DEVICE_INFO;
                BluetoothManager::showDeviceInfo();
              }
            }
          } else if (currentState == STATE_BT_STATUS) {
            if (startY >= 146 && startY <= 174) {
              preferences.remove("bt_mac");
              preferences.remove("bt_name");
              preferences.remove("bt_type");
              btTargetMAC = "";
              btTargetName = "";
              BluetoothManager::disconnect();
              currentState = STATE_MENU;
              DisplayManager::drawMenu();
            }
          } else if (currentState == STATE_WIFI_MENU) {
            // AP Mode button
            if (startY >= 50 && startY <= 78) {
              wifiAPActive = !wifiAPActive;
              if (wifiAPActive) {
                WiFi.mode(WIFI_AP);
                WiFi.softAP("ZoEyee-Config");
              } else {
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_OFF);
              }
              WifiManager::showMenu();
            }
            // Client Menu button
            else if (startY >= 88 && startY <= 116) {
              currentState = STATE_WIFI_CLIENT_MENU;
              WifiManager::showClientMenu();
            }
            // Status button
            else if (startY >= 126 && startY <= 154) {
              currentState = STATE_WIFI_STATUS;
              WifiManager::showStatus();
            }
          } else if (currentState == STATE_WIFI_CLIENT_MENU) {
            // Scan Networks button
            if (startY >= 50 && startY <= 78) {
              gfx->fillScreen(BLACK);
              DisplayManager::drawTopBar();
              gfx->setFont(&FreeSans12pt7b);
              gfx->setTextColor(YELLOW);
              gfx->setTextSize(1);
              gfx->setCursor(80, 90);
              gfx->print("Scanning...");
              if (wifiAPActive) {
                WiFi.softAPdisconnect(true);
                wifiAPActive = false;
              }
              WiFi.mode(WIFI_STA);
              int n = WiFi.scanNetworks();
              wifiCount = min(n, MAX_WIFI_NETWORKS);
              for (int i = 0; i < wifiCount; i++) {
                wifiNetworks[i].ssid = WiFi.SSID(i);
                wifiNetworks[i].rssi = WiFi.RSSI(i);
                wifiNetworks[i].encrypted =
                    (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
              }
              wifiSelectedIndex = 0;
              currentState = STATE_WIFI_LIST;
              WifiManager::showList();
            }
            // Save & Auto-Conn checkbox
            else if (startX >= 30 && startX <= 200 && startY >= 88 &&
                     startY <= 116) {
              wifiAutoSave = !wifiAutoSave;
              preferences.putBool("auto", wifiAutoSave);

              // If checking it ON and connected, save current
              if (wifiAutoSave && WiFi.status() == WL_CONNECTED) {
                preferences.putString("ssid", WiFi.SSID());
                preferences.putString("pw",
                                      wifiPassword); // stores last used pass
              }
              WifiManager::showClientMenu();
            }
            // Delete button (X: 230 to 290)
            else if (startX >= 230 && startX <= 290 && startY >= 88 &&
                     startY <= 116) {
              if (preferences.getString("ssid", "").length() > 0) {
                preferences.remove("ssid");
                preferences.remove("pw");
                preferences.putBool("auto", false);
                wifiAutoSave = false;
                WifiManager::showClientMenu();
              }
            }
            // Disconnect button
            else if (startY >= 126 && startY <= 154) {
              if (WiFi.status() == WL_CONNECTED) {
                WiFi.disconnect();
                WifiManager::showClientMenu();
              }
            }
          } else if (currentState == STATE_WIFI_LIST) {
            // Connect button (moved down to Y=130)
            if (startX >= 90 && startX <= 230 && startY >= 130 &&
                startY <= 160 && wifiCount > 0) {
              wifiTargetSSID = wifiNetworks[wifiSelectedIndex].ssid;
              if (wifiNetworks[wifiSelectedIndex].encrypted) {
                wifiPassword = "";
                kbShift = false;
                kbNumbers = false;
                currentState = STATE_WIFI_KEYBOARD;
                WifiManager::showKeyboard();
              } else {
                wifiPassword = "";
                WifiManager::connect();
              }
            }
          } else if (currentState == STATE_WIFI_KEYBOARD) {
            // Keyboard touch detection
            const char **rows =
                kbNumbers ? kbRowsNum : (kbShift ? kbRowsUpper : kbRowsLower);
            int keyW = 28, keyH = 28;
            int startYkb = 28;
            bool handled = false;

            for (int r = 0; r < 3 && !handled; r++) {
              int rowLen = strlen(rows[r]);
              int rowStartX = (320 - rowLen * keyW) / 2;
              for (int c = 0; c < rowLen; c++) {
                int kx = rowStartX + c * keyW;
                int ky = startYkb + r * (keyH + 4);
                if (startX >= kx && startX <= kx + keyW && startY >= ky &&
                    startY <= ky + keyH) {
                  wifiPassword += rows[r][c];
                  if (kbShift)
                    kbShift = false; // Auto-off shift after one char
                  WifiManager::showKeyboard();
                  handled = true;
                  break;
                }
              }
            }

            if (!handled) {
              int btnY = startYkb + 3 * (keyH + 4);
              if (startY >= btnY && startY <= btnY + keyH) {
                if (startX >= 5 && startX <= 60) {
                  // Shift / 123 toggle: lowercase→Shift, Shift→123,
                  // 123→lowercase
                  if (kbNumbers) {
                    kbNumbers = false;
                    kbShift = false;
                  } else if (kbShift) {
                    kbShift = false;
                    kbNumbers = true;
                  } else {
                    kbShift = true;
                  }
                  WifiManager::showKeyboard();
                } else if (startX >= 65 && startX <= 185) {
                  // Space
                  wifiPassword += ' ';
                  WifiManager::showKeyboard();
                } else if (startX >= 190 && startX <= 245) {
                  // Backspace or BACK
                  if (wifiPassword.length() > 0) {
                    wifiPassword.remove(wifiPassword.length() - 1);
                    WifiManager::showKeyboard();
                  } else {
                    // BACK: return to WiFi list
                    currentState = STATE_WIFI_LIST;
                    WifiManager::showList();
                  }
                } else if (startX >= 250 && startX <= 315) {
                  // OK → connect
                  WifiManager::connect();
                }
              }
            }
          } else {
            currentState = STATE_MENU;
            DisplayManager::drawMenu();
          }
        }
      }
      touching = false;
    }
  }
}
