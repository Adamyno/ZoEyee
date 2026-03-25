#include "../../src/Globals.h"
#include "../../test/mocks/NimBLEDevice.h"

SerialMock Serial;

// Define dummy variables needed by Globals.h
NimBLERemoteCharacteristic* pTxChar = nullptr;
bool isBluetoothConnected = false;

// Other variables to satisfy linker, even if not directly used in sendCommand tests
NimBLERemoteCharacteristic* pRxChar = nullptr;
NimBLEScan* pBLEScan = nullptr;
NimBLEClient* pClient = nullptr;

String lastOBDValue = "";
char obdBuffer[256];
int obdBufIndex = 0;
unsigned long lastOBDPollTime = 0;
unsigned long lastOBDRxTime = 0;
unsigned long lastOBDSentTime = 0;
bool obdHeartbeatLit = false;
bool obdResponsePending = false;
volatile bool bleDisconnectedFlag = false;

const unsigned long OBD_NEXT_REQUEST_DELAY = 100;
const unsigned long OBD_RESPONSE_TIMEOUT = 2000;

float obdSOC = 0.0f;
int obdSOH = 0;
float obdHVBatTemp = 0.0f;
float obdCabinTemp = 0.0f;
float obdACRpm = 0.0f;
float obdACPressure = 0.0f;
String obd12V = "";

String btTargetMAC = "";
String btTargetName = "";
uint8_t btTargetType = 0;
int obdPollIndex = 0;
bool obdZoeMode = false;
int obdCurrentECU = 0;

State currentState = STATE_HOME;

// We need to provide dummy implementations for WebConsole to link if we don't compile it,
// but we will compile WebConsole.cpp, so we need to provide WebConsole dependencies if any,
// or just compile it in.

// DisplayManager dependencies in ObdManager?
// Yes, DisplayManager::updateHomeOBD() is called in ObdManager.cpp, but not in sendCommand.
// We'll need a dummy DisplayManager namespace to satisfy the linker if we compile ObdManager.cpp.

namespace DisplayManager {
    void updateHomeOBD() {}
}

// Arduino gfx dependencies (used in ObdManager)
Arduino_GFX* gfx = nullptr;
Arduino_DataBus* bus = nullptr;

const int FreeSans12pt7b = 0;
const int FreeSans9pt7b = 0;

// Other things from Globals.h
CachedDevice btDevices[10];
int btTotalDevices = 0;
int btSelectedDeviceIndex = 0;
int scanTime = 5;
bool bleConnecting = false;

DashSlot dashPages[MAX_PAGES][6];
int currentPage = 0;
int currentSlotIndex = 0;

int menuIndex = 0;
const int menuCount = 0;
const char* menuItems[1] = {nullptr};

int currentBrightness = 128;
int lastX=0, lastY=0, startX=0, startY=0;
bool touching = false;
bool isSwipingBrightness = false;
unsigned long touchStartTime = 0;
unsigned long lastTapTime = 0;
int lastRawX = 0, lastRawY = 0;

CachedWifi wifiNetworks[10];
int wifiCount = 0;
int wifiSelectedIndex = 0;
bool wifiAPActive = false;
bool wifiAutoSave = false;
String wifiPassword = "";
String wifiTargetSSID = "";
unsigned long wifiTransitionTime = 0;
State wifiNextState = STATE_HOME;

const char *kbRowsLower[1] = {nullptr};
const char *kbRowsUpper[1] = {nullptr};
const char *kbRowsNum[1] = {nullptr};
bool kbShift = false;
bool kbNumbers = false;

Preferences preferences;

void showWifiStatus() {}

unsigned long mock_millis_val = 0;
unsigned long millis() {
    return mock_millis_val++;
}

void delay(unsigned long ms) {
    mock_millis_val += ms;
}
