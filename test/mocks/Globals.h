#pragma once
#include "Arduino.h"

// State enum as defined in Globals.h
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

class NimBLERemoteCharacteristic; // Forward declaration
