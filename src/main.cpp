#include <Arduino_GFX_Library.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <Wire.h>
#include "ZoeLogo.h"
#include "Icons.h"

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

/*
 * Pinout for Waveshare ESP32-C6-Touch-LCD-1.47
 */
#define TFT_BL 23
#define TFT_RST 22
#define TFT_DC 15
#define TFT_CS 14
#define TFT_SCK 1
#define TFT_MOSI 2
#define BTN_PIN 9

// Touch pins
#define TOUCH_SDA 18
#define TOUCH_SCL 19
#define TOUCH_RST 20

// I2C Address for AXS5106
#define TOUCH_ADDR 0x63

// Software Version
#define SW_VERSION "v1.2.1-Stable"

// Color definitions (RGB 565)
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// Menu States
enum State {
  STATE_HOME,
  STATE_MENU,
  STATE_INFO,
  STATE_WIFI_SCAN,
  STATE_BT_SCAN,
  STATE_BRIGHTNESS,
  STATE_BT_LIST,
  STATE_BT_DEVICE_INFO
};

State currentState = STATE_HOME;
int menuIndex = 0;
const int menuCount = 4;
const char *menuItems[] = {"SYS INFO", "WIFI SCAN", "BT SCAN", "BRIGHTNESS"};

// Forward declarations
void drawTopBar();
void disconnectOBD();

// Brightness
int currentBrightness = 128; // 0-255

// Display Objects
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, TFT_RST, 1 /* rotation: 1 = Landscape */, false /* IPS */,
    172 /* width */, 320 /* height */, 34 /* col offset 1 */,
    0 /* row offset 1 */, 34 /* col offset 2 */, 0 /* row offset 2 */
);

// BLE Scan & Connection variables
int scanTime = 5;
NimBLEScan *pBLEScan;
bool isBluetoothConnected = false;
bool bleConnecting = false;

// BLE Client (NimBLE)
NimBLEClient *pClient = nullptr;
NimBLERemoteCharacteristic *pTxChar = nullptr; // Write to OBD
NimBLERemoteCharacteristic *pRxChar = nullptr; // Notify from OBD

// OBD response buffer (static, no heap fragmentation)
static char obdBuffer[256];
static int obdBufIndex = 0;
String lastOBDValue = "";
unsigned long lastOBDPollTime = 0;
unsigned long lastOBDRxTime = 0;  // OBD adat érkezés (car icon zöld / heartbeat)
bool obdHeartbeatLit = false;
const unsigned long OBD_POLL_INTERVAL = 2000; // ms between polls
volatile bool bleDisconnectedFlag = false;

// ZOE OBD adatok
float obdSOC = -1;        // Akkumulátor töltöttség %
int   obdSOH = -1;        // Akkumulátor egészség %
float obdHVBatTemp = -99;  // HV akku hőmérséklet °C
String obd12V = "";       // 12V feszültség
int   obdPollIndex = 0;   // Polling rotáció: 0=SOC, 1=SOH, 2=Temp, 3=12V
bool  obdZoeMode = false;  // true = ZOE UDS mód aktív

// Cached BLE Devices
#define MAX_BLE_DEVICES 30
struct CachedDevice {
  String name;
  String address;
  NimBLEAddress bleAddress;
  int rssi;
};
CachedDevice btDevices[MAX_BLE_DEVICES];
int btTotalDevices = 0;

// BLE List Scrolling & Selection
int btListScrollY = 0;
int btListMaxScrollY = 0;
int btSelectedDeviceIndex = -1;

// Touch variables
int lastX = -1, lastY = -1;
int startX = -1, startY = -1;
bool touching = false;
bool isSwipingBrightness = false;
unsigned long touchStartTime = 0;
unsigned long lastTapTime = 0;

// Set up raw touch storage for debugging
int lastRawX = 0, lastRawY = 0;

// Official Waveshare JD9853 Initialisation Sequence
void lcd_reg_init(void) {
  static const uint8_t init_operations[] = {BEGIN_WRITE,
                                            WRITE_COMMAND_8,
                                            0x11,
                                            END_WRITE,
                                            DELAY,
                                            120,
                                            BEGIN_WRITE,
                                            WRITE_C8_D16,
                                            0xDF,
                                            0x98,
                                            0x53,
                                            WRITE_C8_D8,
                                            0xB2,
                                            0x23,
                                            WRITE_COMMAND_8,
                                            0xB7,
                                            WRITE_BYTES,
                                            4,
                                            0x00,
                                            0x47,
                                            0x00,
                                            0x6F,
                                            WRITE_COMMAND_8,
                                            0xBB,
                                            WRITE_BYTES,
                                            6,
                                            0x1C,
                                            0x1A,
                                            0x55,
                                            0x73,
                                            0x63,
                                            0xF0,
                                            WRITE_C8_D16,
                                            0xC0,
                                            0x44,
                                            0xA4,
                                            WRITE_C8_D8,
                                            0xC1,
                                            0x16,
                                            WRITE_COMMAND_8,
                                            0xC3,
                                            WRITE_BYTES,
                                            8,
                                            0x7D,
                                            0x07,
                                            0x14,
                                            0x06,
                                            0xCF,
                                            0x71,
                                            0x72,
                                            0x77,
                                            WRITE_COMMAND_8,
                                            0xC4,
                                            WRITE_BYTES,
                                            12,
                                            0x00,
                                            0x00,
                                            0xA0,
                                            0x79,
                                            0x0B,
                                            0x0A,
                                            0x16,
                                            0x79,
                                            0x0B,
                                            0x0A,
                                            0x16,
                                            0x82,
                                            WRITE_COMMAND_8,
                                            0xC8,
                                            WRITE_BYTES,
                                            32,
                                            0x3F,
                                            0x32,
                                            0x29,
                                            0x29,
                                            0x27,
                                            0x2B,
                                            0x27,
                                            0x28,
                                            0x28,
                                            0x26,
                                            0x25,
                                            0x17,
                                            0x12,
                                            0x0D,
                                            0x04,
                                            0x00,
                                            0x3F,
                                            0x32,
                                            0x29,
                                            0x29,
                                            0x27,
                                            0x2B,
                                            0x27,
                                            0x28,
                                            0x28,
                                            0x26,
                                            0x25,
                                            0x17,
                                            0x12,
                                            0x0D,
                                            0x04,
                                            0x00,
                                            WRITE_COMMAND_8,
                                            0xD0,
                                            WRITE_BYTES,
                                            5,
                                            0x04,
                                            0x06,
                                            0x6B,
                                            0x0F,
                                            0x00,
                                            WRITE_C8_D16,
                                            0xD7,
                                            0x00,
                                            0x30,
                                            WRITE_C8_D8,
                                            0xE6,
                                            0x14,
                                            WRITE_C8_D8,
                                            0xDE,
                                            0x01,
                                            WRITE_COMMAND_8,
                                            0xB7,
                                            WRITE_BYTES,
                                            5,
                                            0x03,
                                            0x13,
                                            0xEF,
                                            0x35,
                                            0x35,
                                            WRITE_COMMAND_8,
                                            0xC1,
                                            WRITE_BYTES,
                                            3,
                                            0x14,
                                            0x15,
                                            0xC0,
                                            WRITE_C8_D16,
                                            0xC2,
                                            0x06,
                                            0x3A,
                                            WRITE_C8_D16,
                                            0xC4,
                                            0x72,
                                            0x12,
                                            WRITE_C8_D8,
                                            0xBE,
                                            0x00,
                                            WRITE_C8_D8,
                                            0xDE,
                                            0x02,
                                            WRITE_COMMAND_8,
                                            0xE5,
                                            WRITE_BYTES,
                                            3,
                                            0x00,
                                            0x02,
                                            0x00,
                                            WRITE_COMMAND_8,
                                            0xE5,
                                            WRITE_BYTES,
                                            3,
                                            0x01,
                                            0x02,
                                            0x00,
                                            WRITE_C8_D8,
                                            0xDE,
                                            0x00,
                                            WRITE_C8_D8,
                                            0x35,
                                            0x00,
                                            WRITE_C8_D8,
                                            0x3A,
                                            0x05,
                                            WRITE_COMMAND_8,
                                            0x2A,
                                            WRITE_BYTES,
                                            4,
                                            0x00,
                                            0x22,
                                            0x00,
                                            0xCD,
                                            WRITE_COMMAND_8,
                                            0x2B,
                                            WRITE_BYTES,
                                            4,
                                            0x00,
                                            0x00,
                                            0x01,
                                            0x3F,
                                            WRITE_C8_D8,
                                            0xDE,
                                            0x02,
                                            WRITE_COMMAND_8,
                                            0xE5,
                                            WRITE_BYTES,
                                            3,
                                            0x00,
                                            0x02,
                                            0x00,
                                            WRITE_C8_D8,
                                            0xDE,
                                            0x00,
                                            WRITE_C8_D8,
                                            0x36,
                                            0x00,
                                            WRITE_COMMAND_8,
                                            0x21,
                                            END_WRITE,
                                            DELAY,
                                            10,
                                            BEGIN_WRITE,
                                            WRITE_COMMAND_8,
                                            0x29,
                                            END_WRITE};
  bus->batchOperation(init_operations, sizeof(init_operations));
}

void touch_init() {
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);
  digitalWrite(TOUCH_RST, HIGH);
  delay(100);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
}

bool touch_read(int &x, int &y) {
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

void setBrightness(int val) {
  currentBrightness = constrain(val, 0, 255);
  ledcWrite(TFT_BL, currentBrightness);
}

// UI Kirajzoló függvények
void showHome() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  if (isBluetoothConnected) {
    // === ZOE Dashboard ===
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(CYAN); gfx->setTextSize(1);
    gfx->setCursor(105, 35); gfx->print("ZOE Dashboard");
    gfx->drawLine(0, 40, 320, 40, WHITE);

    // SOC (nagy, kiemelt)
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(WHITE); gfx->setTextSize(1);
    gfx->setCursor(10, 65); gfx->print("SOC:");
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(obdSOC >= 20 ? GREEN : RED);
    if (obdSOC >= 0) { gfx->setCursor(70, 70); gfx->printf("%.0f%%", obdSOC); }
    else { gfx->setCursor(70, 70); gfx->print("--"); }

    // SOH
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(WHITE); gfx->setTextSize(1);
    gfx->setCursor(190, 65); gfx->print("SOH:");
    gfx->setFont(&FreeSans12pt7b); gfx->setTextColor(CYAN);
    if (obdSOH >= 0) { gfx->setCursor(240, 65); gfx->printf("%d%%", obdSOH); }
    else { gfx->setCursor(240, 65); gfx->print("--"); }

    // Akku hőmérséklet
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(WHITE); gfx->setTextSize(1);
    gfx->setCursor(10, 100); gfx->print("Bat Temp:");
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(obdHVBatTemp > 35 ? RED : (obdHVBatTemp < 5 ? BLUE : GREEN));
    if (obdHVBatTemp > -99) { gfx->setCursor(110, 100); gfx->printf("%.0fC", obdHVBatTemp); }
    else { gfx->setCursor(110, 100); gfx->print("--"); }

    // 12V
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(WHITE); gfx->setTextSize(1);
    gfx->setCursor(10, 130); gfx->print("12V:");
    gfx->setFont(&FreeSans12pt7b); gfx->setTextColor(YELLOW);
    if (obd12V.length() > 0) { gfx->setCursor(60, 130); gfx->print(obd12V.c_str()); }
    else { gfx->setCursor(60, 130); gfx->print("--"); }

    // Státusz sor (Sötétebb narancs: 0xFB40)
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(0xFB40); gfx->setTextSize(1);
    gfx->setCursor(10, 165); gfx->print("Double tap for menu");
  } else {
    gfx->setFont(&FreeSans18pt7b); gfx->setTextColor(RED); gfx->setTextSize(1);
    gfx->setCursor(35, 90); gfx->print("No Connection");
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(WHITE); gfx->setTextSize(1);
    gfx->setCursor(50, 130); gfx->print("Connect via BT SCAN");
    // Státusz sor (Sötétebb narancs: 0xFB40)
    gfx->setFont(&FreeSans9pt7b); gfx->setTextColor(0xFB40); gfx->setTextSize(1);
    gfx->setCursor(10, 165); gfx->print("Double tap for menu");
  }
}

void updateHomeOBD() {
  if (!isBluetoothConnected) { showHome(); return; }
  // Frissítjük a teljes dashboardot (kicsi kijelzőn ez gyors)
  showHome();
}

// UDS hex válasz dekódolása: "622002 1A2B" → 0x1A2B
int parseUDSHex(const String& resp, const char* expectedPrefix, int byteCount) {
  String r = resp; r.trim(); r.replace(" ", "");
  String prefix = String(expectedPrefix); prefix.replace(" ", "");
  int idx = r.indexOf(prefix);
  if (idx < 0) return -1;
  String hexPart = r.substring(idx + prefix.length(), idx + prefix.length() + byteCount * 2);
  return (int)strtol(hexPart.c_str(), NULL, 16);
}

void drawMenu(bool fullRedraw = true) {
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    drawTopBar();
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(106, 35);
    gfx->println("MAIN MENU");
    gfx->drawLine(0, 40, 320, 40, WHITE);
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(CYAN, BLACK);
    gfx->fillTriangle(10, 100, 25, 90, 25, 110, CYAN);
    gfx->fillTriangle(310, 100, 295, 90, 295, 110, CYAN);
  }
  // Expand clearing rectangle to ensure long text like "BRIGHTNESS" is fully erased
  // Previous width was 240, starting at x=40. Let's make it start at 30 and width 260.
  // Also lower the Y starting point slightly to clear the taller text completely.
  gfx->fillRect(28, 50, 264, 60, BLACK);
  
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(1);
  int textX = 160 - (strlen(menuItems[menuIndex]) * 9); 
  if (menuIndex == 0) textX = 85;
  else if (menuIndex == 1) textX = 70;
  else if (menuIndex == 2) textX = 90;
  else if (menuIndex == 3) textX = 60;
  // Move text down from 95 to 110 to align exactly between the triangles (Y: 90 to 110, center 100)
  // For FreeSans18pt, the baseline is the Y coordinate.
  gfx->setCursor(textX, 108);
  gfx->print(menuItems[menuIndex]);
}

void showBrightness(bool fullRedraw = true) {
  int sliderY = 40;
  int sliderX = 140;
  int sliderW = 40;
  int sliderH = 100;
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    drawTopBar();
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(84, 35);
    gfx->println("BRIGHTNESS");
    gfx->drawLine(0, 40, 320, 40, WHITE);
    gfx->drawRect(sliderX, sliderY, sliderW, sliderH, WHITE);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(10, 165);
    gfx->println("Swipe Right -> Back");
  }
  int fillH = map(currentBrightness, 0, 255, 0, sliderH - 4);
  if (fillH > 0)
    gfx->fillRect(sliderX + 2, sliderY + sliderH - 2 - fillH, sliderW - 4,
                  fillH, CYAN);
  if (sliderH - 4 - fillH > 0)
    gfx->fillRect(sliderX + 2, sliderY + 2, sliderW - 4, (sliderH - 4) - fillH,
                  BLACK);
  gfx->fillRect(sliderX + sliderW + 5, 41, 110, 120, BLACK);
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(sliderX + sliderW + 15, sliderY + (sliderH / 2) + 15);
  gfx->printf("%d%%", (currentBrightness * 100) / 255);
}

void showInfo() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(112, 35);
  gfx->println("SYS INFO");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(10, 65);
  gfx->printf("SW Version: %s\n", SW_VERSION);
  gfx->setCursor(10, 95);
  gfx->printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
  gfx->setCursor(10, 125);
  gfx->printf("Flash: %u KB\n", ESP.getFlashChipSize() / 1024);
  gfx->setCursor(10, 155);
  gfx->printf("Heap: %u KB\n", ESP.getFreeHeap() / 1024);
}

void drawTopBar() {
  gfx->fillRect(0, 0, 320, 20, BLACK);
  if (currentState == STATE_MENU) {
    gfx->fillTriangle(15, 2, 5, 11, 25, 11, WHITE);
    gfx->fillRect(10, 11, 10, 8, WHITE);
    gfx->fillRect(13, 15, 4, 4, BLACK);
  } else if (currentState != STATE_HOME) {
    gfx->fillTriangle(5, 10, 15, 4, 15, 16, WHITE);
    gfx->fillRect(15, 8, 6, 4, WHITE);
  }
  // WiFi Icon
  bool isWifiConnected = (WiFi.status() == WL_CONNECTED);
  bool isWifiAP = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
  uint16_t wifiColor = 0x7BEF; // Halvány szürke placeholder
  if (isWifiConnected) wifiColor = WHITE;
  else if (isWifiAP) wifiColor = GREEN;
  
  if (isWifiAP) {
    gfx->drawXBitmap(270, 7, icon_ap_bits, icon_ap_width, icon_ap_height, wifiColor);
  } else {
    gfx->drawXBitmap(266, 2, icon_wifi_bits, icon_wifi_width, icon_wifi_height, wifiColor);
  }

  // Bluetooth Icon
  uint16_t btColor = isBluetoothConnected ? 0x03FF : 0x7BEF; // Világosabb kék ha csatlakozva, amúgy szürke
  gfx->drawXBitmap(294, 4, icon_bt_bits, icon_bt_width, icon_bt_height, btColor);

  // OBD (Car) Icon
  if (isBluetoothConnected) {
    if (lastOBDRxTime > 0 && millis() - lastOBDRxTime < 10000) {
      gfx->drawXBitmap(242, 2, icon_car_bits, icon_car_width, icon_car_height, GREEN);
    } else {
      gfx->drawXBitmap(242, 2, icon_car_bits, icon_car_width, icon_car_height, 0x7BEF);
    }
  }
}

void runWifiScan() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(106, 35);
  gfx->println("WIFI SCAN");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(10, 65);
  gfx->println("Scanning...");

  int n = WiFi.scanNetworks();
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(106, 35);
  gfx->println("WIFI LIST");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  gfx->setFont(&FreeSans9pt7b);
  if (n == 0) {
    gfx->setTextColor(RED);
    gfx->setCursor(10, 65);
    gfx->println("No networks");
  } else {
    gfx->setTextColor(WHITE);
    for (int i = 0; i < n && i < 6; ++i) {
      gfx->setCursor(5, 65 + (i * 18));
      gfx->printf("%s (%d)\n", WiFi.SSID(i).substring(0, 30).c_str(),
                  WiFi.RSSI(i));
    }
  }
}

void showBTList(bool fullRedraw = true);

// =================================================================================
// 1. Szkennelés stabilitása (A Wi-Fi figyelmeztetés elfedésével)
// =================================================================================
void runBLEScan() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(112, 35);
  gfx->println("BLE SCAN");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(75, 95);
  gfx->println("Scanning...");

  Serial.println("[BLE] Felkészülés a szkennelésre...");

  // Csak akkor hívjuk meg a leválasztást, ha a Wi-Fi fut, hogy ne dobjon piros
  // hibaüzenetet
  if (WiFi.getMode() != WIFI_OFF) {
    WiFi.scanDelete();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(300);
  }

  NimBLEDevice::deinit(true);
  delay(300);
  NimBLEDevice::init("ZoEyee-Scanner");
  NimBLEDevice::setOwnAddrType(
      BLE_OWN_ADDR_PUBLIC); // Fixált gyári MAC – Konnwei ne utasítsa el!

  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setDuplicateFilter(false);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(60);

  pBLEScan->clearResults();

  Serial.printf("[BLE] Szkennelés indítása (scanTime=%d s)... \n", scanTime);
  Serial.flush();
  unsigned long startMillis = millis();

  if (pBLEScan->start(scanTime * 1000, false, true)) {
    Serial.println("[BLE] Rádió aktív, szkennelés folyamatban...");
    delay(200);
    while (pBLEScan->isScanning()) {
      delay(100);
      if (millis() - startMillis > (scanTime * 1000 + 4000))
        break;
    }
    Serial.printf("[BLE] Szkennelés vége %lu ms után.\n",
                  millis() - startMillis);
    Serial.flush();
  } else {
    Serial.println("[BLE] HIBA: pBLEScan->start() sikertelen!");
  }

  NimBLEScanResults foundDevices = pBLEScan->getResults();
  btTotalDevices = foundDevices.getCount();

  Serial.printf("[BLE] Szkennelés befejeződött. Talált eszközök: %d\n",
                btTotalDevices);
  Serial.flush();

  if (btTotalDevices > MAX_BLE_DEVICES)
    btTotalDevices = MAX_BLE_DEVICES;

  for (int i = 0; i < btTotalDevices; i++) {
    const NimBLEAdvertisedDevice *device = foundDevices.getDevice(i);
    btDevices[i].name = device->getName().length() > 0
                            ? device->getName().c_str()
                            : "Unknown Device";
    btDevices[i].address = device->getAddress().toString().c_str();
    btDevices[i].bleAddress = device->getAddress();
    btDevices[i].rssi = device->getRSSI();
    Serial.printf("[BLE] Látom: %s [%s] RSSI: %d\n", btDevices[i].name.c_str(),
                  btDevices[i].address.c_str(), btDevices[i].rssi);
  }
  pBLEScan->clearResults();

  btSelectedDeviceIndex = 0;
  currentState = STATE_BT_LIST;
  showBTList();
}

void showBTList(bool fullRedraw) {
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    drawTopBar();
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(100, 35);
    gfx->println("BT DEVICES");
    gfx->drawLine(0, 40, 320, 40, WHITE);
    gfx->fillTriangle(10, 70, 20, 62, 20, 78, CYAN);
    gfx->fillTriangle(310, 70, 300, 62, 300, 78, CYAN);
    gfx->fillRoundRect(20, 106, 130, 43, 8, GREEN);
    gfx->fillRoundRect(22, 108, 126, 39, 6, BLACK);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(GREEN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(42, 134);
    gfx->print("CONNECT");
    gfx->fillRoundRect(170, 106, 130, 43, 8, CYAN);
    gfx->fillRoundRect(172, 108, 126, 39, 6, BLACK);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(195, 134);
    gfx->print("DETAILS");
  }
  gfx->fillRect(30, 45, 260, 45, BLACK);
  if (btTotalDevices == 0) {
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75);
    gfx->println("No devices found.");
  } else {
    CachedDevice &dev = btDevices[btSelectedDeviceIndex];
    gfx->setTextColor(WHITE, BLACK);
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextSize(1);

    // Szöveg görgetése ha túl hosszú
    int charWidth = 12;
    String nameToDraw = dev.name;
    int maxChars = 18;
    if (nameToDraw.length() > maxChars) {
      int offset = (millis() / 400) % (nameToDraw.length() - maxChars + 6);
      if (offset > nameToDraw.length() - maxChars) offset = nameToDraw.length() - maxChars; // Pause at end
      nameToDraw = nameToDraw.substring(offset, offset + maxChars);
    }
    int textWidth = nameToDraw.length() * charWidth;
    int textX = (320 - textWidth) / 2;
    if (textX < 30) textX = 30;
    gfx->setCursor(textX, 75);
    gfx->print(nameToDraw.c_str());

    if (isBluetoothConnected && btSelectedDeviceIndex == 0) {
      gfx->fillRoundRect(22, 108, 126, 39, 6, GREEN);
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(BLACK, GREEN);
      gfx->setTextSize(1);
      gfx->setCursor(35, 134);
      gfx->print("CONNECTED");
    } else {
      gfx->fillRoundRect(22, 108, 126, 39, 6, BLACK);
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(GREEN, BLACK);
      gfx->setTextSize(1);
      gfx->setCursor(42, 134);
      gfx->print("CONNECT");
    }

    int scrollbarY = 165;
    int scrollbarX = 20;
    int scrollbarW = 280;
    int scrollbarH = 6;
    gfx->fillRect(scrollbarX, scrollbarY, scrollbarW, scrollbarH, 0x2104);
    if (btTotalDevices > 1) {
      int handleW = scrollbarW / btTotalDevices;
      if (handleW < 10)
        handleW = 10;
      int slideRange = scrollbarW - handleW;
      float scrollPct =
          (float)btSelectedDeviceIndex / (float)(btTotalDevices - 1);
      int handleX = scrollbarX + (int)(scrollPct * slideRange);
      gfx->fillRect(handleX, scrollbarY, handleW, scrollbarH, CYAN);
    } else {
      gfx->fillRect(scrollbarX, scrollbarY, scrollbarW, scrollbarH, CYAN);
    }
  }
}

void showBTDeviceInfo() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(76, 35);
  gfx->println("DEVICE DETAILS");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  if (btSelectedDeviceIndex >= 0 && btSelectedDeviceIndex < btTotalDevices) {
    CachedDevice &dev = btDevices[btSelectedDeviceIndex];
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(10, 65);
    gfx->printf("Name: %s", dev.name.c_str());
    gfx->setCursor(10, 95);
    gfx->printf("MAC : %s", dev.address.c_str());
    gfx->setCursor(10, 125);
    gfx->printf("RSSI: %d dBm", dev.rssi);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setCursor(10, 165);
    gfx->println("Swipe Right -> Back");
  }
}


class MyClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *client) {
    Serial.println("[BLE/NimBLE] Connected to server");
  }
  void onDisconnect(NimBLEClient *client, int reason) {
    Serial.printf("[BLE/NimBLE] Disconnected (reason=%d)\n", reason);
    isBluetoothConnected = false;
    pTxChar = nullptr;
    pRxChar = nullptr;
    bleDisconnectedFlag = true;
  }
};

void onBLENotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData,
                 size_t length, bool isNotify) {
  Serial.print("[OBD] RX RAW: ");
  for (size_t i = 0; i < length; i++)
    Serial.printf("%02X ", pData[i]);
  Serial.println();

  for (size_t i = 0; i < length; i++) {
    char c = (char)pData[i];
    if (c == '>') {
      // Válasz vége: '\r' mentén szétválasztjuk a sorokat, AT echo-t kihagyjuk,
      // az első értékes sort vesszük válasznak.
      obdBuffer[obdBufIndex] = '\0';

      String fullResponse = "";
      char *ptr = obdBuffer;
      while (*ptr) {
        // Sor kiemelése
        char *lineStart = ptr;
        while (*ptr && *ptr != '\r')
          ptr++;
        if (*ptr == '\r') {
          *ptr = '\0';
          ptr++;
        }

        // Trim
        char *s = lineStart;
        while (*s == ' ')
          s++;
        int l = strlen(s);
        while (l > 0 && (s[l - 1] == ' ' || s[l - 1] == '\n'))
          s[--l] = '\0';

        if (strlen(s) == 0)
          continue; // Üres sor → skip
        String line = String(s);
        line.trim();
        if (line.length() == 0)
          continue;

        String lineUpper = line;
        lineUpper.toUpperCase();
        
        // Echo sor: pontosan egy AT parancs (pl. "ATZ" vagy "ATE0") → skip
        if (lineUpper.startsWith("AT") && lineUpper.indexOf(' ') < 0 &&
            lineUpper.length() <= 6)
          continue;

        // ISO-TP Multi-frame hosszak és prefixek szűrése
        // Ha csak 3 karakter és nincs benne szóköz, az valószínűleg a teljes hossz (pl. "00D")
        if (lineUpper.length() <= 3 && lineUpper.indexOf(' ') < 0) {
          continue;
        }

        // Ha a sor "0:", "1:", "2:" stb.-vel kezdődik (ELM327 multi-frame formátum)
        if (lineUpper.length() >= 2 && lineUpper.charAt(1) == ':') {
          lineUpper = lineUpper.substring(2);
          lineUpper.trim();
        } else if (lineUpper.length() >= 3 && lineUpper.charAt(2) == ':') {
          lineUpper = lineUpper.substring(3);
          lineUpper.trim();
        }

        if (fullResponse.length() > 0) fullResponse += " ";
        fullResponse += lineUpper;
      }

      obdBufIndex = 0;
      obdBuffer[0] = '\0';
      if (fullResponse.length() == 0)
        continue; // Semmi hasznos → skip

      lastOBDValue = fullResponse;
      Serial.printf("[OBD] Response complete: '%s'\n", lastOBDValue.c_str());
    } else if (c != '\n') {
      // '\r'-t is eltároljuk, fontos elválasztóként!
      if (obdBufIndex < (int)sizeof(obdBuffer) - 1)
        obdBuffer[obdBufIndex++] = c;
    }
  }
}

void sendOBDCommand(const char *cmd) {
  if (pTxChar != nullptr && isBluetoothConnected) {
    char fullCmd[64];
    snprintf(fullCmd, sizeof(fullCmd), "%s\r", cmd);
    pTxChar->writeValue((uint8_t *)fullCmd, strlen(fullCmd));
    Serial.printf("[OBD] Sent: %s\n", cmd);
  }
}

// =================================================================================
// 2. MEGOLDÁS: Lassú, biztonságos kapcsolati paraméterek a klónokhoz!
// =================================================================================
bool connectToOBD(int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= btTotalDevices)
    return false;
  if (bleConnecting)
    return false;

  bleConnecting = true;
  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(85, 75);
  gfx->print("Connecting...");

  NimBLEAddress targetAddr = btDevices[deviceIndex].bleAddress;
  Serial.printf("[BLE] Kapcsolódás: %s [%s]\n",
                btDevices[deviceIndex].name.c_str(),
                targetAddr.toString().c_str());

  if (pClient != nullptr) {
    if (pClient->isConnected())
      pClient->disconnect();
    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;
    delay(200); // Kis szünet, hogy a NimBLE stack és a memória is fellélegezzen
  }

  pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallbacks());

  // KLÓN-KOMPATIBILIS PARAMÉTEREK (Konnwei/Vgate)
  // Interval: 40ms - 100ms (32 - 80), Latency: 0, Timeout: 5000ms (500)
  pClient->setConnectionParams(32, 80, 0, 500);
  pClient->setConnectTimeout(5000);

  if (!pClient->connect(targetAddr)) {
    Serial.println("[BLE] Kapcsolódás sikertelen!");
    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;
    bleConnecting = false;
    return false;
  }

  Serial.println("[BLE] Fizikai kapcsolat sikeres! Várakozás a GATT "
                 "felépülésre (1.5 mp)...");
  delay(1500);

  auto pServices = pClient->getServices(true);
  if (pServices.empty()) {
    Serial.println("[BLE] HIBA: Nem sikerültek a szolgáltatások lekérdezései!");
    pClient->disconnect();
    bleConnecting = false;
    return false;
  }

  pTxChar = nullptr;
  pRxChar = nullptr;
  NimBLERemoteService *pObdSvc = nullptr;

  // 1. Keresés prioritás szerint: FFF0 vagy FFE0
  for (auto pSvc : pServices) {
    String svcUUID = pSvc->getUUID().toString().c_str();
    svcUUID.toLowerCase();
    Serial.printf("[BLE] Látott szolgáltatás: %s\n", svcUUID.c_str());
    if (svcUUID.indexOf("fff0") >= 0 || svcUUID.indexOf("ffe0") >= 0) {
      pObdSvc = pSvc;
      break;
    }
  }

  // 2. Ha nincs FFF0, nézzük az AE30-at
  if (pObdSvc == nullptr) {
    for (auto pSvc : pServices) {
      String svcUUID = pSvc->getUUID().toString().c_str();
      svcUUID.toLowerCase();
      if (svcUUID.indexOf("ae30") >= 0) {
        pObdSvc = pSvc;
        break;
      }
    }
  }

  // Ha találtunk dedikált OBD szolgáltatást
  if (pObdSvc != nullptr) {
    Serial.printf("[BLE]   >>> OBD/KONNWEI SZOLGÁLTATÁS MEGTALÁLVA (%s)! <<<\n",
                  pObdSvc->getUUID().toString().c_str());
    auto pChars = pObdSvc->getCharacteristics(true);
    if (!pChars.empty()) {
      // Kifejezetten olyan sémát keresünk, ami biztosan jó
      for (auto pChr : pChars) {
        String charUUID = pChr->getUUID().toString().c_str();
        charUUID.toLowerCase();
        bool writable = pChr->canWrite() || pChr->canWriteNoResponse();
        bool notifiable = pChr->canNotify() || pChr->canIndicate();
        Serial.printf("[BLE]   Karakterisztika: %s (Write=%d, Notify=%d)\n",
                      charUUID.c_str(), writable, notifiable);

        // Megpróbáljuk a legadekvátabbat párosítani
        if (notifiable && pRxChar == nullptr)
          pRxChar = pChr;
        else if (writable && pTxChar == nullptr)
          pTxChar = pChr;
      }
    }
  }

  // Fallback: Ha még mindig nincs TX/RX, próbáljuk meg bármivel ami TX/RX képes
  // a GAP/GATT-on kívül
  if (pTxChar == nullptr || pRxChar == nullptr) {
    pTxChar = nullptr;
    pRxChar = nullptr; // Reset
    for (auto pSvc : pServices) {
      String svcUUID = pSvc->getUUID().toString().c_str();
      svcUUID.toLowerCase();
      if (svcUUID.indexOf("1800") >= 0 || svcUUID.indexOf("1801") >= 0)
        continue;

      auto pChars = pSvc->getCharacteristics(true);
      if (!pChars.empty()) {
        for (auto pChr : pChars) {
          if ((pChr->canWrite() || pChr->canWriteNoResponse()) &&
              pTxChar == nullptr)
            pTxChar = pChr;
          else if (pChr->canNotify() && pRxChar == nullptr)
            pRxChar = pChr;
        }
      }
      if (pTxChar && pRxChar) {
        Serial.printf("[BLE] Fallback SPP megtalálva a %s szolgáltatásban!\n",
                      pSvc->getUUID().toString().c_str());
        break;
      }
    }
  }

  if (pTxChar == nullptr || pRxChar == nullptr) {
    Serial.println("[BLE] HIBA: Nem találom a TX/RX csatornákat!");
    gfx->fillRect(30, 50, 260, 60, BLACK);
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75);
    gfx->print("No OBD service found!");
    delay(2000);
    disconnectOBD();
    return false;
  }

  // Subscribe és explicite CCCD bekapcsolása a lusta klónokhoz
  if (pRxChar->canNotify()) {
    pRxChar->subscribe(true, onBLENotify);
    delay(200);
    NimBLERemoteDescriptor *p2902 =
        pRxChar->getDescriptor(NimBLEUUID((uint16_t)0x2902));
    if (p2902 != nullptr) {
      uint8_t notifyOn[] = {0x01, 0x00};
      p2902->writeValue(notifyOn, 2, true);
      Serial.println(
          "[BLE]   >>> CCCD (2902) Descriptor manuálisan engedélyezve! <<<");
    } else {
      Serial.println("[BLE]   >>> FIGYELEM: Nincs 2902 CCCD Descriptor! <<<");
    }
  }

  pRxChar->subscribe(true, onBLENotify);
  isBluetoothConnected = true;
  bleConnecting = false;

  obdBufIndex = 0;
  obdBuffer[0] = '\0';
  lastOBDValue = "";

  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(70, 75);
  gfx->print("Verifying OBD...");

  delay(500);

  // Többlépcsős ATZ hitelesítés
  bool isELM = false;
  const char *atzCommands[] = {"ATZ", "ATZ\n"};

  for (int cmdIdx = 0; cmdIdx < 2 && !isELM; cmdIdx++) {
    sendOBDCommand(atzCommands[cmdIdx]);
    unsigned long verifyStart = millis();
    while (millis() - verifyStart < 2500) {
      delay(50);
      if (lastOBDValue.length() > 0) {
        Serial.printf("[OBD] ATZ Válasz: '%s'\n", lastOBDValue.c_str());
        String resp = lastOBDValue;
        resp.toUpperCase();
        if (resp.indexOf("ELM") >= 0 || resp.indexOf("KONNWEI") >= 0 ||
            resp.indexOf("OBD") >= 0 || resp.indexOf("KW") >= 0 ||
            resp.indexOf("V1.5") >= 0 || resp.indexOf("OK") >= 0) {
          isELM = true;
          break;
        }
        lastOBDValue = "";
      }
    }
    if (!isELM)
      Serial.printf(
          "[BLE] Nincs válasz a(z) %s parancsra, újrapróbálkozás...\n",
          atzCommands[cmdIdx]);
  }

  if (!isELM) {
    Serial.println("[BLE] Nem válaszolt mint ELM327. Lecsatlakozás.");
    gfx->fillRect(30, 50, 260, 60, BLACK);
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75);
    gfx->print("Not an OBD device");
    delay(2000);
    disconnectOBD();
    return false;
  }

  Serial.printf("[BLE] ELM327 hitelesítve: %s\n", lastOBDValue.c_str());
  sendOBDCommand("ATE0");
  delay(500);

  // === Renault ZOE CAN protokoll beállítás ===
  Serial.println("[OBD] ZOE CAN protokoll beállítás...");
  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(50, 75);
  gfx->print("Setting up CAN...");

  // Protokoll: ISO 15765-4 CAN (11 bit, 500kbps)
  sendOBDCommand("ATSP6"); delay(500); lastOBDValue = "";
  // EVC ECU fejléc (Electric Vehicle Controller)
  sendOBDCommand("ATSH7E4"); delay(300); lastOBDValue = "";
  // Válasz szűrő: csak a 7EC-es (EVC válasz) kereteket figyeljük  
  sendOBDCommand("ATCRA7EC"); delay(300); lastOBDValue = "";
  // Diagnosztikai session megnyitása (Extended)
  sendOBDCommand("10C0"); delay(500); lastOBDValue = "";

  obdZoeMode = true;
  obdPollIndex = 0;
  obdSOC = -1; obdSOH = -1; obdHVBatTemp = -99; obd12V = "";
  Serial.println("[BLE] OBD adapter felállt, ZOE mód aktív!");
  return true;
}

void disconnectOBD() {
  Serial.println("[BLE] Leválasztás kezdeményezése...");
  if (pClient != nullptr && pClient->isConnected()) {
    if (pRxChar != nullptr) {
      pRxChar->unsubscribe();
      delay(100); // Adjunk időt a leiratkozásnak
    }
    pClient->disconnect();
    delay(500); // Kötelező szünet a kínai adaptereknek a bontás feldolgozásához
  }
  isBluetoothConnected = false;
  pTxChar = nullptr;
  pRxChar = nullptr;
  lastOBDValue = "";
  obdBufIndex = 0;
  obdBuffer[0] = '\0';
  Serial.println("[BLE] Sikeresen leválasztva az OBD-ről.");
}

void setup(void) {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  ledcAttach(TFT_BL, 5000, 8);
  setBrightness(currentBrightness);
  if (!gfx->begin())
    Serial.println("gfx->begin() failed!");
  lcd_reg_init();
  gfx->setRotation(1);
  touch_init();

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

  // Verziószám
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(0x7BEF); // Világosszürke
  gfx->setCursor(105, 162);
  gfx->print(SW_VERSION);

  delay(5000);

  showHome();

  NimBLEDevice::init("ZoEyee-Scanner");
  NimBLEDevice::setOwnAddrType(
      BLE_OWN_ADDR_PUBLIC); // Fixált gyári MAC – Konnwei ne utasítsa el!
  Serial.printf("[SYS] Setup Kész. Free heap: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
  int tx, ty;
  if (touch_read(tx, ty)) {
    if (!touching) {
      startX = tx;
      startY = ty;
      touching = true;
      isSwipingBrightness = false;
      touchStartTime = millis();
    }
    int deltaX = tx - startX;
    int deltaY = ty - startY;

    // Global brightness control via vertical swipe
    if (abs(deltaY) > 10 && abs(deltaX) < 40) {
      isSwipingBrightness = true;
      // UP swipe -> negative deltaY -> increase brightness
      int newBright = currentBrightness - deltaY; 
      newBright = constrain(newBright, 0, 255);
      if (newBright != currentBrightness) {
        setBrightness(newBright);
        if (currentState == STATE_BRIGHTNESS) showBrightness(false);
      }
      startY = ty; // Reset y coordinate for continuous smooth adjustment
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
            // Double tap!
            currentState = STATE_MENU;
            drawMenu();
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
            drawMenu(false);
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
              btSelectedDeviceIndex =
                  (btSelectedDeviceIndex - 1 + btTotalDevices) % btTotalDevices;
              showBTList(false);
            } else {
              currentState = STATE_MENU;
              drawMenu();
            }
          } else if (currentState == STATE_BT_DEVICE_INFO) {
            currentState = STATE_BT_LIST;
            showBTList();
          } else {
            currentState = STATE_MENU;
            drawMenu();
          }
        } else if (deltaX < -80) {
          if (currentState == STATE_MENU) {
            menuIndex = (menuIndex + 1) % menuCount;
            drawMenu(false);
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
              btSelectedDeviceIndex =
                  (btSelectedDeviceIndex + 1) % btTotalDevices;
              showBTList(false);
            }
          }
        } else if (abs(deltaX) < 15 && abs(deltaY) < 15) {
          if (startY < 40 && startX < 80) {
            if (currentState == STATE_MENU) {
              currentState = STATE_HOME;
              showHome();
            } else if (currentState == STATE_BT_DEVICE_INFO) {
              currentState = STATE_BT_LIST;
              showBTList();
            } else if (currentState != STATE_HOME) {
              currentState = STATE_MENU;
              drawMenu();
            }
          } else if (currentState == STATE_MENU) {
            if (startY < 110) {
              if (menuIndex == 0) {
                currentState = STATE_INFO;
                showInfo();
              } else if (menuIndex == 1) {
                currentState = STATE_WIFI_SCAN;
                runWifiScan();
              } else if (menuIndex == 2) {
                currentState = STATE_BT_SCAN;
                runBLEScan();
              } else if (menuIndex == 3) {
                currentState = STATE_BRIGHTNESS;
                showBrightness();
              }
            }
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
              if (startX >= 20 && startX <= 150 && startY >= 110 &&
                  startY <= 145) {
                if (!isBluetoothConnected && !bleConnecting)
                  connectToOBD(btSelectedDeviceIndex);
                else if (isBluetoothConnected)
                  disconnectOBD();
                showBTList(true);
              } else if (startX >= 170 && startX <= 300 && startY >= 110 &&
                         startY <= 145) {
                currentState = STATE_BT_DEVICE_INFO;
                showBTDeviceInfo();
              }
            }
          } else {
            currentState = STATE_MENU;
            drawMenu();
          }
        }
      }
      touching = false;
    }
  }

  // Auto-scroll update for BT List
  if (currentState == STATE_BT_LIST && btTotalDevices > 0) {
    if (btDevices[btSelectedDeviceIndex].name.length() > 18) {
      static unsigned long lastMarqueeUpdate = 0;
      if (millis() - lastMarqueeUpdate > 400) {
        lastMarqueeUpdate = millis();
        showBTList(false); // Update only the dynamic region
      }
    }
  }

  static bool lastBtnState = HIGH;
  bool currentBtnState = digitalRead(BTN_PIN);
  if (lastBtnState == HIGH && currentBtnState == LOW) {
    if (currentState == STATE_MENU) {
      menuIndex = (menuIndex + 1) % menuCount;
      drawMenu(false);
    } else {
      currentState = STATE_MENU;
      drawMenu();
    }
    delay(200);
  }
  lastBtnState = currentBtnState;

  if (bleDisconnectedFlag) {
    bleDisconnectedFlag = false;
    Serial.println("[BLE] Disconnect detected, refreshing UI");
    if (currentState == STATE_HOME)
      showHome();
    else if (currentState == STATE_BT_LIST)
      showBTList(true);
    else
      drawTopBar();
  }

  if (currentState == STATE_HOME && isBluetoothConnected) {
    if (millis() - lastOBDPollTime >= OBD_POLL_INTERVAL) {
      lastOBDPollTime = millis();

      if (obdZoeMode) {
        // ZOE UDS polling rotáció
        switch (obdPollIndex) {
          case 0: sendOBDCommand("222002"); break; // SOC
          case 1: sendOBDCommand("223206"); break; // SOH
          case 2:
            // Akku hőmérséklet: az EVC-ből a 42e frame HV Bat Temp mezőjéből
            // CanZE: 7ec,24,31,1,40,0,°C,223028 → de inkább: 42e broadcast
            // Egyszerűbb: EVC-ből 2233d8 (utolsó 10 hőmérséklet log) vagy
            // Standard OBD: 0105 (hűtővíz hőmérséklet)
            // Próbáljuk az ATSH7E4 + 2233d8-at (utolsó hőmérséklet napló)
            sendOBDCommand("2233d8"); break;
          case 3:
            // 12V: visszaváltunk AT parancsra
            sendOBDCommand("ATRV"); break;
        }
        obdPollIndex = (obdPollIndex + 1) % 4;
      } else {
        sendOBDCommand("ATRV");
      }
    }

    // Válasz feldolgozás
    if (lastOBDValue.length() > 0) {
      // Heartbeat bekapcsolása & időbélyeg frissítése
      lastOBDRxTime = millis();
      gfx->fillCircle(234, 10, 3, GREEN);
      obdHeartbeatLit = true;

      String resp = lastOBDValue;
      Serial.printf("[ZOE] Feldolgozás: '%s'\n", resp.c_str());

      if (resp.indexOf("622002") >= 0 || resp.indexOf("62 20 02") >= 0) {
        // SOC: raw × 0.02 %
        int raw = parseUDSHex(resp, "622002", 2);
        if (raw >= 0) { obdSOC = raw * 0.02; Serial.printf("[ZOE] SOC = %.1f%%\n", obdSOC); }
      } else if (resp.indexOf("623206") >= 0 || resp.indexOf("62 32 06") >= 0) {
        // SOH: raw × 1 %
        int raw = parseUDSHex(resp, "623206", 1);
        if (raw >= 0) { obdSOH = raw; Serial.printf("[ZOE] SOH = %d%%\n", obdSOH); }
      } else if (resp.indexOf("6233d8") >= 0 || resp.indexOf("62 33 d8") >= 0) {
        // Hőmérséklet: első bájt, raw - 40 = °C
        int raw = parseUDSHex(resp, "6233d8", 1);
        if (raw >= 0) { obdHVBatTemp = raw - 40; Serial.printf("[ZOE] Bat Temp = %.0f°C\n", obdHVBatTemp); }
      } else if (resp.endsWith("V")) {
        // ATRV válasz (pl. "13.2V")
        obd12V = resp;
        Serial.printf("[ZOE] 12V = %s\n", obd12V.c_str());
      } else if (resp.indexOf("NO DATA") >= 0 || resp.indexOf("ERROR") >= 0) {
        Serial.printf("[ZOE] ECU nem válaszolt: %s\n", resp.c_str());
      }
      lastOBDValue = "";
      updateHomeOBD();
    }
  }

  // Heartbeat villogás takarítása
  if (obdHeartbeatLit && millis() - lastOBDRxTime > 150) {
    obdHeartbeatLit = false;
    gfx->fillCircle(234, 10, 3, BLACK); // Eltüntetjük a zöld pöttyöt
  }

  delay(20);
}