#pragma once

// ============================================================
// ZoEyee Localization System
// ============================================================
// Language indices (must match languageNames[] in Globals.cpp)
#define LANG_EN 0
#define LANG_HU 1
#define LANG_COUNT 2

// Helper macro: L(key) returns the localized string for currentLanguage
extern int currentLanguage;
#define L(key) (lang_##key[currentLanguage])

// Macro for dashboard parameter names (indexed by param number)
#define L_PARAM(idx) (lang_paramNames[idx][currentLanguage])

// ── Declarations ───────────────────────────────────────────
// All arrays are defined in Lang.cpp

// Menu & Navigation
extern const char* const lang_MENU[];
extern const char* const lang_INFO[];
extern const char* const lang_WIFI[];
extern const char* const lang_BT_SCAN[];
extern const char* const lang_BRIGHTNESS[];
extern const char* const lang_SETTINGS[];

// Settings
extern const char* const lang_PAGES[];
extern const char* const lang_AUTO_SCROLL[];
extern const char* const lang_CAR_TYPE[];
extern const char* const lang_LANGUAGE[];
extern const char* const lang_ON[];
extern const char* const lang_OFF[];
extern const char* const lang_PAGES_DESC[];
extern const char* const lang_TAP_ARROWS[];
extern const char* const lang_TAP_TOGGLE[];
extern const char* const lang_INTERVAL[];

// Brightness
extern const char* const lang_BRIGHT_1[];
extern const char* const lang_BRIGHT_2[];
extern const char* const lang_BRIGHT_3[];
extern const char* const lang_BRIGHT_4[];

// Info Screen
extern const char* const lang_FMT_SW_VER[];
extern const char* const lang_FMT_FLASH[];
extern const char* const lang_FMT_RAM[];
extern const char* const lang_FMT_UPTIME[];

// Bluetooth
extern const char* const lang_BT_DEVICES[];
extern const char* const lang_CONNECT[];
extern const char* const lang_DETAILS[];
extern const char* const lang_NO_DEVICES[];
extern const char* const lang_CONNECTED[];
extern const char* const lang_DEVICE_DETAILS[];
extern const char* const lang_SWIPE_BACK[];
extern const char* const lang_BT_STATUS[];
extern const char* const lang_DISCONNECT[];
extern const char* const lang_BLE_SCAN[];
extern const char* const lang_SCANNING[];
extern const char* const lang_CONNECTING[];
extern const char* const lang_NO_OBD[];
extern const char* const lang_VERIFY_OBD[];
extern const char* const lang_FMT_NAME[];
extern const char* const lang_FMT_MAC[];
extern const char* const lang_FMT_MAC2[];
extern const char* const lang_FMT_RSSI[];
extern const char* const lang_FMT_DEVICE[];
extern const char* const lang_STATUS_CONNECTED[];
extern const char* const lang_STATUS_RECONNECTING[];

// WiFi
extern const char* const lang_AP_MODE_ON[];
extern const char* const lang_AP_MODE_OFF[];
extern const char* const lang_CLIENT_MENU[];
extern const char* const lang_STATUS[];
extern const char* const lang_SCAN_NETWORKS[];
extern const char* const lang_SAVE_AUTO[];
extern const char* const lang_DEL[];
extern const char* const lang_WIFI_LIST[];
extern const char* const lang_NO_NETWORKS[];
extern const char* const lang_SPACE_KEY[];
extern const char* const lang_BACK[];
extern const char* const lang_CONNECTED_OK[];
extern const char* const lang_CONN_FAILED[];
extern const char* const lang_WIFI_STATUS[];
extern const char* const lang_MODE_AP[];
extern const char* const lang_MODE_CLIENT[];
extern const char* const lang_NOT_CONNECTED[];

// Dashboard
extern const char* const lang_EMPTY[];
extern const char* const lang_paramNames[][LANG_COUNT];
