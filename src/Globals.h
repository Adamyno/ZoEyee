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
  STATE_BT_STATUS,
  STATE_SLOT_PICKER,   // Parameter selection overlay
  STATE_SETTINGS,      // Settings screen
  STATE_VEHICLE_PICKER,
  STATE_LANGUAGE_PICKER,
  STATE_SINGLE_MODULE
};

// HVAC Polling State Machine (non-blocking)
enum HvacPollState {
  HVAC_IDLE = 0,
  // Switch to HVAC ECU
  HVAC_SWITCH_SH,       // ATSH744
  HVAC_SWITCH_CRA,      // ATCRA764
  HVAC_SWITCH_FCSH,     // ATFCSH744
  // Set raw mode for multi-frame
  HVAC_SET_ATS0,        // ATS0
  HVAC_SET_ATCAF0,      // ATCAF0
  HVAC_SET_ATAL,        // ATAL
  HVAC_SET_FCSD,        // ATFCSD300000
  HVAC_SET_FCSM,        // ATFCSM1
  HVAC_SET_STFF,        // ATSTFF
  // Open diagnostic session
  HVAC_SESSION,         // 10C0
  // Data queries
  HVAC_QUERY_2121,      // Cabin temp
  HVAC_QUERY_2143,      // External temp + AC pressure
  HVAC_QUERY_2144,      // AC compressor RPM
  HVAC_QUERY_2167,      // Climate Loop Mode
  // Restore normal mode
  HVAC_RESTORE_ATS1,    // ATS1
  HVAC_RESTORE_ATCAF1,  // ATCAF1
  HVAC_RESTORE_ATST32,  // ATST32
  // Switch back to EVC
  HVAC_BACK_SH,         // ATSH7E4
  HVAC_BACK_CRA,        // ATCRA7EC
  HVAC_BACK_FCSH,       // ATFCSH7E4
  HVAC_DONE
};

// LBC Polling State Machine (non-blocking, for 2103 cell voltage query)
enum LbcPollState {
  LBC_IDLE = 0,
  // Switch to LBC ECU
  LBC_SWITCH_SH,        // ATSH79B
  LBC_SWITCH_CRA,       // ATCRA7BB
  LBC_SWITCH_FCSH,      // ATFCSH79B
  // Open extended diagnostic session
  LBC_SESSION,          // 10C0 (as ISO-TP: 0210C0)
  // Set raw mode for multi-frame
  LBC_SET_ATS0,         // ATS0
  LBC_SET_ATCAF0,       // ATCAF0
  LBC_SET_ATAL,         // ATAL
  // Data queries
  LBC_QUERY_2101,       // 022101 (Max charge, input/output power)
  LBC_QUERY_2103,       // 022103 (Max & Min cell voltage)
  // Restore normal mode
  LBC_RESTORE_ATS1,     // ATS1
  LBC_RESTORE_ATCAF1,   // ATCAF1
  LBC_RESTORE_ATST32,   // ATST32
  LBC_DONE
};

// Dashboard Slot configuration
struct DashSlot {
  int paramIndex;     // Index into DisplayManager's parameter register
};

#define MAX_DASH_PARAMS 19
extern DashSlot dashPages[MAX_PAGES][6];
extern int currentPage;
extern int numPages;
extern unsigned long pageSwipeTime;
extern int pickerScrollIndex;
extern int pickerSlotIndex;
extern int pickerPage;
extern bool pickerJustOpened;
extern int currentSlotIndex; // For tracking which slot is being edited
extern int settingsScrollIndex;

// Auto-scroll
extern bool autoScrollEnabled;
extern int autoScrollInterval;       // 1-30 seconds
extern unsigned long lastAutoScrollTime;

// Vehicle type
extern int vehicleType;
extern const char* vehicleTypes[];
extern const int vehicleTypeCount;
extern int vehiclePickerScroll;

// Language
extern int currentLanguage;          // 0=EN, 1=HU
extern const char* languageNames[];
extern const int languageCount;
extern int languagePickerScroll;

// Single module view
extern int singleModuleParamIdx;

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
extern TaskHandle_t btReconnectTaskHandle; // FreeRTOS task for non-blocking reconnect
extern volatile bool btReconnectDone;      // true when task finishes
extern volatile bool btReconnectResult;    // true if connect succeeded

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
extern float obdExtTemp;
extern String obd12V;
extern float obd12VFloat;
extern float obdCellVoltageMax;
extern float obdCellVoltageMin;
extern int   obdFanSpeed;
extern int   obdClimateLoopMode;
extern float obdMaxChargePower;
extern float obdHVBatVoltage;
extern float obdHVBatCurrent;
extern float obdDCPower;
extern float obdAvailEnergy;
extern float obdACPhase;
extern float obdInsulationRes;

extern String btTargetMAC;
extern String btTargetName;
extern uint8_t btTargetType;
extern int obdPollIndex;
extern unsigned long pollCycleStartTime;
extern bool obdZoeMode;
extern int obdCurrentECU;  // 0=EVC, 1=HVAC

// HVAC state machine
extern HvacPollState hvacState;
extern unsigned long hvacCmdSentTime;
extern const unsigned long HVAC_AT_TIMEOUT;
extern const unsigned long HVAC_ISOTP_TIMEOUT;

// LBC state machine
extern LbcPollState lbcState;
extern unsigned long lbcCmdSentTime;

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
extern bool wifiScanning; // true while async WiFi scan is in progress
extern String wifiPassword;
extern String wifiTargetSSID;
extern unsigned long wifiTransitionTime;
extern State wifiNextState;

// Keyboard layout
extern const char *kbRowsLower[];
extern const char *kbRowsUpper[];
extern const char *kbRowsNum[];
extern bool kbShift;
extern bool kbNumbers;

#include <Preferences.h>
extern Preferences preferences;
