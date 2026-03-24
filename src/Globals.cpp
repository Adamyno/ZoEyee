#include "Config.h"
#include "Globals.h"

State currentState = STATE_HOME;
int menuIndex = 0;
const int menuCount = 4;
const char *menuItems[] = {"INFO", "WIFI", "BT SCAN", "BRIGHTNESS"};

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

NimBLEClient *pClient = nullptr;
NimBLERemoteCharacteristic *pTxChar = nullptr;
NimBLERemoteCharacteristic *pRxChar = nullptr;

String lastOBDValue = "";
char obdBuffer[256];
int obdBufIndex = 0;
unsigned long lastOBDPollTime = 0;
unsigned long lastOBDRxTime = 0;
unsigned long lastOBDSentTime = 0;
bool obdHeartbeatLit = false;
bool obdResponsePending = false;
volatile bool bleDisconnectedFlag = false;
const unsigned long OBD_NEXT_REQUEST_DELAY = 150;
const unsigned long OBD_RESPONSE_TIMEOUT = 1500;

float obdSOC = -1;
int obdSOH = -1;
float obdHVBatTemp = -99;
float obdCabinTemp = -99;
float obdACRpm = -1;
float obdACPressure = -1;
String obd12V = "";
int obdPollIndex = 0;
bool obdZoeMode = false;
int obdCurrentECU = 0;  // 0=EVC, 1=HVAC

CachedDevice btDevices[MAX_BLE_DEVICES];
int btTotalDevices = 0;

int btListScrollY = 0;
int btListMaxScrollY = 0;
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
String wifiPassword = "";
String wifiTargetSSID = "";

const char *kbRowsLower[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
const char *kbRowsUpper[] = {"QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
const char *kbRowsNum[] = {"1234567890", "-/:;()&@\"", ".,?!'"};
bool kbShift = false;
bool kbNumbers = false;

Preferences preferences;
