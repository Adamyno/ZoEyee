#include "Config.h"
#include "Globals.h"

State currentState = STATE_HOME;

// Default dashboard configuration (all slots empty until configured)
DashSlot dashPages[MAX_PAGES][6] = {
  {{-1}, {-1}, {-1}, {-1}, {-1}, {-1}}, // Page 0
  {{-1}, {-1}, {-1}, {-1}, {-1}, {-1}}, // Page 1
  {{-1}, {-1}, {-1}, {-1}, {-1}, {-1}}, // Page 2
  {{-1}, {-1}, {-1}, {-1}, {-1}, {-1}}, // Page 3
  {{-1}, {-1}, {-1}, {-1}, {-1}, {-1}}, // Page 4
  {{-1}, {-1}, {-1}, {-1}, {-1}, {-1}}, // Page 5
  {{-1}, {-1}, {-1}, {-1}, {-1}, {-1}}, // Page 6
};
int currentPage = 0;
int numPages = 1;
unsigned long pageSwipeTime = 0;
int pickerScrollIndex = 0;
int pickerSlotIndex = 0;
int pickerPage = 0;
bool pickerJustOpened = false;
int currentSlotIndex = 0;
int menuIndex = 0;
const int menuCount = 5;
const char *menuItems[] = {"INFO", "WIFI", "BT SCAN", "BRIGHTNESS", "SETTINGS"};

int currentBrightness = 128; // 0-255

// Display Objects
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, TFT_RST, 1 /* rotation: 1 = Landscape */, false /* IPS */,
    172 /* width */, 320 /* height */, 34 /* col offset 1 */,
    0 /* row offset 1 */, 34 /* col offset 2 */, 0 /* row offset 2 */
);

int scanTime = 5;
NimBLEScan *pBLEScan = nullptr;
bool isBluetoothConnected = false;
bool bleConnecting = false;
TaskHandle_t btReconnectTaskHandle = nullptr;
volatile bool btReconnectDone = false;
volatile bool btReconnectResult = false;

NimBLEClient *pClient = nullptr;
NimBLERemoteCharacteristic *pTxChar = nullptr;
NimBLERemoteCharacteristic *pRxChar = nullptr;

String lastOBDValue = "";
char obdBuffer[256];
int obdBufIndex = 0;
unsigned long lastOBDPollTime = 0;
unsigned long lastOBDRxTime = 0;
unsigned long lastOBDSentTime = 0;
bool obdResponsePending = false;
volatile bool bleDisconnectedFlag = false;
const unsigned long OBD_NEXT_REQUEST_DELAY = 150;
const unsigned long OBD_RESPONSE_TIMEOUT = 2000;

float obdSOC = -1;
int obdSOH = -1;
float obdHVBatTemp = -99;
float obdCabinTemp = -99;
float obdACRpm = -1;
float obdACPressure = -1;
float obdExtTemp = -99;
String obd12V = "";
float obd12VFloat = -1;
float obdCellVoltageMax = -1;
float obdCellVoltageMin = -1;
int   obdFanSpeed = -99;
int   obdClimateLoopMode = -99;
float obdMaxChargePower = -1;
float obdHVBatVoltage = -1;
float obdHVBatCurrent = -999;
float obdDCPower = -999;
float obdAvailEnergy = -1;
float obdACPhase = -1;
float obdInsulationRes = -1;

String btTargetMAC = "";
String btTargetName = "";
uint8_t btTargetType = 0;
int obdPollIndex = 0;
unsigned long pollCycleStartTime = 0;
bool obdZoeMode = false;
int obdCurrentECU = 0;  // 0=EVC, 1=HVAC

// HVAC state machine
HvacPollState hvacState = HVAC_IDLE;
unsigned long hvacCmdSentTime = 0;
const unsigned long HVAC_AT_TIMEOUT = 1000;    // 1s for AT commands
const unsigned long HVAC_ISOTP_TIMEOUT = 5000; // 5s for IsoTP data queries

// LBC state machine
LbcPollState lbcState = LBC_IDLE;
unsigned long lbcCmdSentTime = 0;

CachedDevice btDevices[MAX_BLE_DEVICES];
int btTotalDevices = 0;

int btSelectedDeviceIndex = -1;

int lastX = -1, lastY = -1;
int startX = -1, startY = -1;
bool touching = false;
bool isSwipingBrightness = false;
unsigned long touchStartTime = 0;
unsigned long lastTapTime = 0;
int lastRawX = 0, lastRawY = 0;

CachedWifi wifiNetworks[MAX_WIFI_NETWORKS];
int wifiCount = 0;
int wifiSelectedIndex = 0;
bool wifiAPActive = false;
bool wifiAutoSave = false;
bool wifiScanning = false;
String wifiPassword = "";
String wifiTargetSSID = "";
unsigned long wifiTransitionTime = 0;
State wifiNextState = STATE_WIFI_MENU;

const char *kbRowsLower[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
const char *kbRowsUpper[] = {"QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
const char *kbRowsNum[] = {"1234567890", "-/:;()&@\"", ".,?!'"};
bool kbShift = false;
bool kbNumbers = false;

Preferences preferences;
