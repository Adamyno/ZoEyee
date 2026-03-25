#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Arduino_GFX_Library.h>
#include "Config.h"

// Menu States
enum State {
  STATE_HOME,
  STATE_MENU,
  STATE_INFO,
  STATE_WIFI_MENU,
  STATE_WIFI_CLIENT_MENU,
  STATE_WIFI_LIST,
  STATE_WIFI_KEYBOARD,
  STATE_WIFI_CONNECTING,
  STATE_WIFI_STATUS,
  STATE_BT_SCAN,
  STATE_BRIGHTNESS,
  STATE_BT_LIST,
  STATE_BT_DEVICE_INFO,
  STATE_BT_STATUS
};

extern State currentState;
extern int menuIndex;
extern const int menuCount;
extern const char *menuItems[];

// Forward declaration for function still in main.cpp
void showWifiStatus();

extern int currentBrightness; // 0-255

// Display Objects
extern Arduino_DataBus *bus;
extern Arduino_GFX *gfx;

// BLE Scan & Connection variables
extern int scanTime;
extern NimBLEScan *pBLEScan;
extern bool isBluetoothConnected;
extern bool bleConnecting;

extern NimBLEClient *pClient;
extern NimBLERemoteCharacteristic *pTxChar; // Write to OBD
extern NimBLERemoteCharacteristic *pRxChar; // Notify from OBD

// OBD response buffer (static, no heap fragmentation)
extern String lastOBDValue;
extern char obdBuffer[256];
extern int obdBufIndex;
extern unsigned long lastOBDPollTime;
extern unsigned long lastOBDRxTime;
extern unsigned long lastOBDSentTime;
extern bool obdHeartbeatLit;
extern bool obdResponsePending;
extern volatile bool bleDisconnectedFlag;
extern const unsigned long OBD_NEXT_REQUEST_DELAY;
extern const unsigned long OBD_RESPONSE_TIMEOUT;

// ZOE OBD adatok
extern float obdSOC;
extern int obdSOH;
extern float obdHVBatTemp;
extern float obdCabinTemp;
extern float obdACRpm;
extern float obdACPressure;
extern String obd12V;

extern String btTargetMAC;
extern String btTargetName;
extern uint8_t btTargetType;
extern int obdPollIndex;
extern bool obdZoeMode;
extern int obdCurrentECU;  // 0=EVC, 1=HVAC

// Cached BLE Devices
struct CachedDevice {
  String name;
  String address;
  NimBLEAddress bleAddress;
  int rssi;
};
extern CachedDevice btDevices[];
extern int btTotalDevices;

// BLE List Scrolling & Selection
extern int btListScrollY;
extern int btListMaxScrollY;
extern int btSelectedDeviceIndex;

// Touch variables
extern int lastX, lastY;
extern int startX, startY;
extern bool touching;
extern bool isSwipingBrightness;
extern unsigned long touchStartTime;
extern unsigned long lastTapTime;
extern int lastRawX, lastRawY;

// WiFi Management variables
struct CachedWifi {
  String ssid;
  int rssi;
  bool encrypted;
};
extern CachedWifi wifiNetworks[];
extern int wifiCount;
extern int wifiSelectedIndex;
extern bool wifiAPActive;
extern bool wifiAutoSave;
extern String wifiPassword;
extern String wifiTargetSSID;

// Keyboard layout
extern const char *kbRowsLower[];
extern const char *kbRowsUpper[];
extern const char *kbRowsNum[];
extern bool kbShift;
extern bool kbNumbers;

#include <Preferences.h>
extern Preferences preferences;
