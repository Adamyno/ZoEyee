#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <Wire.h>

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>

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
#define SW_VERSION "v1.1.0"

// Color definitions (RGB 565)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Menu States
enum State {
  STATE_HOME,
  STATE_MENU,
  STATE_INFO,
  STATE_WIFI_SCAN,
  STATE_BT_SCAN,
  STATE_TOUCH_TEST,
  STATE_BRIGHTNESS,
  STATE_BT_LIST,
  STATE_BT_DEVICE_INFO
};

State currentState = STATE_HOME;
int menuIndex = 0;
const int menuCount = 5;
const char* menuItems[] = {"SYS INFO", "WIFI SCAN", "BT SCAN", "TOUCH TEST", "BRIGHTNESS"};

// Forward declarations
void drawTopBar();

// Brightness
int currentBrightness = 128; // 0-255

// Display Objects
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, TFT_RST, 1 /* rotation: 1 = Landscape */, false /* IPS */,
  172 /* width */, 320 /* height */,
  34 /* col offset 1 */, 0 /* row offset 1 */,
  34 /* col offset 2 */, 0 /* row offset 2 */
);

// BLE Scan & Connection variables
int scanTime = 5; 
NimBLEScan* pBLEScan;
bool isBluetoothConnected = false;
bool bleConnecting = false;

// BLE Client (NimBLE)
NimBLEClient* pClient = nullptr;
NimBLERemoteCharacteristic* pTxChar = nullptr; // Write to OBD
NimBLERemoteCharacteristic* pRxChar = nullptr; // Notify from OBD

// OBD response buffer (static, no heap fragmentation)
static char obdBuffer[256];
static int obdBufIndex = 0;
String lastOBDValue = "";
unsigned long lastOBDPollTime = 0;
const unsigned long OBD_POLL_INTERVAL = 1500; // ms between ATRV polls
volatile bool bleDisconnectedFlag = false; // Set by callback, checked by loop()

// Cached BLE Devices
#define MAX_BLE_DEVICES 30
struct CachedDevice {
  String name;
  String address;
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
unsigned long touchStartTime = 0;

// Set up raw touch storage for debugging
int lastRawX = 0, lastRawY = 0;

// Official Waveshare JD9853 Initialisation Sequence
void lcd_reg_init(void) {
  static const uint8_t init_operations[] = {
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x11,  // Sleep Out
    END_WRITE,
    DELAY, 120,
    BEGIN_WRITE,
    WRITE_C8_D16, 0xDF, 0x98, 0x53,
    WRITE_C8_D8, 0xB2, 0x23, 
    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 4, 0x00, 0x47, 0x00, 0x6F,
    WRITE_COMMAND_8, 0xBB,
    WRITE_BYTES, 6, 0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,
    WRITE_C8_D16, 0xC0, 0x44, 0xA4,
    WRITE_C8_D8, 0xC1, 0x16, 
    WRITE_COMMAND_8, 0xC3,
    WRITE_BYTES, 8, 0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,
    WRITE_COMMAND_8, 0xC4,
    WRITE_BYTES, 12, 0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,
    WRITE_COMMAND_8, 0xC8,
    WRITE_BYTES, 32, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
    WRITE_COMMAND_8, 0xD0,
    WRITE_BYTES, 5, 0x04, 0x06, 0x6B, 0x0F, 0x00,
    WRITE_C8_D16, 0xD7, 0x00, 0x30,
    WRITE_C8_D8, 0xE6, 0x14, 
    WRITE_C8_D8, 0xDE, 0x01, 
    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 5, 0x03, 0x13, 0xEF, 0x35, 0x35,
    WRITE_COMMAND_8, 0xC1,
    WRITE_BYTES, 3, 0x14, 0x15, 0xC0,
    WRITE_C8_D16, 0xC2, 0x06, 0x3A,
    WRITE_C8_D16, 0xC4, 0x72, 0x12,
    WRITE_C8_D8, 0xBE, 0x00, 
    WRITE_C8_D8, 0xDE, 0x02, 
    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3, 0x00, 0x02, 0x00,
    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3, 0x01, 0x02, 0x00,
    WRITE_C8_D8, 0xDE, 0x00, 
    WRITE_C8_D8, 0x35, 0x00, 
    WRITE_C8_D8, 0x3A, 0x05, 
    WRITE_COMMAND_8, 0x2A,
    WRITE_BYTES, 4, 0x00, 0x22, 0x00, 0xCD,
    WRITE_COMMAND_8, 0x2B,
    WRITE_BYTES, 4, 0x00, 0x00, 0x01, 0x3F,
    WRITE_C8_D8, 0xDE, 0x02, 
    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3, 0x00, 0x02, 0x00,
    WRITE_C8_D8, 0xDE, 0x00, 
    WRITE_C8_D8, 0x36, 0x00, // MADCTL: Normal portrait
    WRITE_COMMAND_8, 0x21, // Display Inversion On
    END_WRITE,
    DELAY, 10,
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x29,  // Display On
    END_WRITE
  };
  bus->batchOperation(init_operations, sizeof(init_operations));
}

// AXS5106 initialization
void touch_init() {
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);
  digitalWrite(TOUCH_RST, HIGH);
  delay(100);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
}

// AXS5106 data read
bool touch_read(int &x, int &y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x01); // Touch points reg
  if (Wire.endTransmission() != 0) return false;
  
  if (Wire.requestFrom(TOUCH_ADDR, 11) != 11) return false;
  
  uint8_t data[11];
  for(int i=0; i<11; i++) data[i] = Wire.read();
  
  int points = data[1] & 0x0F;
  if (points == 0) return false;
  
  int rawX = (((int)(data[2] & 0x0F)) << 8) | data[3];
  int rawY = (((int)(data[4] & 0x0F)) << 8) | data[5];
  
  lastRawX = rawX;
  lastRawY = rawY;

  // New mapping for Landscape (Rotation 1 - 320x172)
  x = rawY;
  y = rawX; // Removed "172 -" to fix inversion
  
  return true;
}

void setBrightness(int val) {
  currentBrightness = constrain(val, 0, 255);
  ledcWrite(TFT_BL, currentBrightness);
}

void showHome() {
  gfx->fillScreen(BLACK);
  gfx->fillRect(0, 0, 320, 20, BLACK);
  
  // Show BT icon if connected
  if (isBluetoothConnected) {
    gfx->fillRoundRect(294, 2, 18, 16, 6, BLUE);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(298, 15);
    gfx->print("B");
  }
  
  if (isBluetoothConnected) {
    // Label
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(CYAN);
    gfx->setTextSize(1);
    gfx->setCursor(110, 55);
    gfx->print("OBD Voltage");
    
    // Voltage value
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(GREEN);
    gfx->setTextSize(1);
    if (lastOBDValue.length() > 0) {
      // Center the value
      int estW = lastOBDValue.length() * 16;
      int cx = (320 - estW) / 2;
      gfx->setCursor(cx, 105);
      gfx->print(lastOBDValue.c_str());
    } else {
      gfx->setCursor(90, 105);
      gfx->print("Waiting...");
    }
  } else {
    // No connection
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(RED);
    gfx->setTextSize(1);
    gfx->setCursor(35, 90);
    gfx->print("No Connection");
    
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(50, 130);
    gfx->print("Connect via BT SCAN");
  }
}

// Partial redraw of just the voltage area on Home screen
void updateHomeOBD() {
  // Clear voltage area
  gfx->fillRect(0, 60, 320, 55, BLACK);
  
  if (isBluetoothConnected) {
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(GREEN);
    gfx->setTextSize(1);
    if (lastOBDValue.length() > 0) {
      int estW = lastOBDValue.length() * 16;
      int cx = (320 - estW) / 2;
      gfx->setCursor(cx, 105);
      gfx->print(lastOBDValue.c_str());
    } else {
      gfx->setCursor(90, 105);
      gfx->print("Waiting...");
    }
  } else {
    // Connection was lost — full redraw
    showHome();
  }
}

void drawMenu(bool fullRedraw = true) {
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    
    drawTopBar();
    
    // Header
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    // Center: "MAIN MENU"
    gfx->setCursor(106, 35); // Shifted Y from 20 to 35
    gfx->println("MAIN MENU");
    gfx->drawLine(0, 40, 320, 40, WHITE);

    // Arrows
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setCursor(10, 95); // Shifted Y
    gfx->print("<");
    gfx->setCursor(290, 95); // Shifted Y
    gfx->print(">");
  }

  // Big Text (clear only this area)
  gfx->fillRect(40, 50, 240, 60, BLACK); // Increased height for larger font bounds
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(1);
  int textWidth = strlen(menuItems[menuIndex]) * 18; // approx max width
  int textX = (320 - textWidth) / 2;
  // Fallback centering for "SYS INFO" vs "BRIGHTNESS"
  if (menuIndex == 0) textX = 85; 
  else if (menuIndex == 1) textX = 70;
  else if (menuIndex == 2) textX = 90;
  else if (menuIndex == 3) textX = 60;
  else if (menuIndex == 4) textX = 50;
  
  gfx->setCursor(textX, 95); // Baseline Y=85->95 for vertically centered look
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
    // Center: "BRIGHTNESS"
    gfx->setCursor(84, 35); // Shifted Y
    gfx->println("BRIGHTNESS");
    gfx->drawLine(0, 40, 320, 40, WHITE);

    gfx->drawRect(sliderX, sliderY, sliderW, sliderH, WHITE);

    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(10, 165); // Shifted Y 150->165
    gfx->println("Swipe Right -> Back");
  }

  // Draw ONLY the changing interior of the slider (Bottom to Top)
  int fillH = map(currentBrightness, 0, 255, 0, sliderH - 4);
  // Bottom part (active)
  if (fillH > 0) {
    gfx->fillRect(sliderX + 2, sliderY + sliderH - 2 - fillH, sliderW - 4, fillH, CYAN);
  }
  // Top part (background)
  if (sliderH - 4 - fillH > 0) {
    gfx->fillRect(sliderX + 2, sliderY + 2, sliderW - 4, (sliderH - 4) - fillH, BLACK);
  }
  
  // Clear and update the percentage text (shifted to the right of slider)
  // Clear area: Start at Y=41 to protect the header line at Y=40
  gfx->fillRect(sliderX + sliderW + 5, 41, 110, 120, BLACK); 
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(sliderX + sliderW + 15, sliderY + (sliderH / 2) + 15);
  gfx->printf("%d%%", (currentBrightness * 100) / 255);
}

void drawTopBar(); // Forward declaration

void showInfo() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  // Center: "SYS INFO"
  gfx->setCursor(112, 35); // Shifted Y
  gfx->println("SYS INFO");
  gfx->drawLine(0, 40, 320, 40, WHITE);

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(10, 65); // Shifted Y from 50
  gfx->printf("SW Version: %s\n", SW_VERSION);
  gfx->setCursor(10, 95); // Shifted Y from 80
  gfx->printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
  gfx->setCursor(10, 125); // Shifted Y from 110
  gfx->printf("Flash: %u KB\n", ESP.getFlashChipSize() / 1024);
  gfx->setCursor(10, 155); // Shifted Y from 140
  gfx->printf("Heap: %u KB\n", ESP.getFreeHeap() / 1024);
}

void drawTopBar() {
  // Clear top bar area
  gfx->fillRect(0, 0, 320, 20, BLACK);
  
  if (currentState == STATE_MENU) {
    // Draw House icon (Home)
    // 1. Slightly larger Triangle roof (overhangs on both sides)
    gfx->fillTriangle(15, 2, 5, 11, 25, 11, WHITE);
    // 2. Square body
    gfx->fillRect(10, 11, 10, 8, WHITE);
    // 3. Tiny door
    gfx->fillRect(13, 15, 4, 4, BLACK);
  }
  else if (currentState != STATE_HOME) {
    // Simple thick left arrow
    // 1. Triangle head
    gfx->fillTriangle(5, 10, 15, 4, 15, 16, WHITE);
    // 2. Thick tail
    gfx->fillRect(15, 8, 6, 4, WHITE);
  }

  if (isBluetoothConnected) {
    // Blue oval
    gfx->fillRoundRect(294, 2, 18, 16, 6, BLUE);
    // White "B" inside
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(298, 15);
    gfx->print("B");
  }
}

void runWifiScan() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  // Center: "WIFI SCAN"
  gfx->setCursor(106, 35); // Shifted Y from 20
  gfx->println("WIFI SCAN");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(10, 65); // Shifted Y from 50
  gfx->println("Scanning...");

  int n = WiFi.scanNetworks();
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  // Center: "WIFI LIST"
  gfx->setCursor(106, 35); // Shifted Y
  gfx->println("WIFI LIST");
  gfx->drawLine(0, 40, 320, 40, WHITE);

  gfx->setFont(&FreeSans9pt7b);
  if (n == 0) {
    gfx->setTextColor(RED);
    gfx->setCursor(10, 65);
    gfx->println("No networks");
  } else {
    gfx->setTextColor(WHITE);
    for (int i = 0; i < n && i < 6; ++i) { // Fit fewer visible lines with bigger font
      gfx->setCursor(5, 65 + (i * 18));
      gfx->printf("%s (%d)\n", WiFi.SSID(i).substring(0, 30).c_str(), WiFi.RSSI(i));
    }
  }
}

void showBTList(bool fullRedraw = true); // Forward declaration

void runBLEScan() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  // Center: "BLE SCAN"
  gfx->setCursor(112, 35); // Shifted Y
  gfx->println("BLE SCAN");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  // Center: "Scanning..."
  gfx->setCursor(75, 95); // Shifted Y
  gfx->println("Scanning...");

  // WiFi lekapcsolása (disconnect), de a módot nem állítjuk OFF-ra,
  // mert az ESP32-C6-on az lekapcsolná a közös RF PHY rádiót is!
  WiFi.disconnect(true);
  delay(100); 

  // Beragadt eredmények ürítése
  pBLEScan->clearResults();

  Serial.println("[BLE] Szkennelés indítási parancs kiküldve...");

  // NimBLE 2.x: start() is non-blocking, returns bool if started successfully
  if (pBLEScan->start(scanTime)) {
    Serial.println("[BLE] Rádió aktív, szkennelés folyamatban...");
    while (pBLEScan->isScanning()) {
      delay(100);
    }
    Serial.println("[BLE] Szkennelési idő letelt.");
  } else {
    Serial.println("[BLE] HIBA: Az RF hardver megtagadta a szkennelés indítását!");
  }

  NimBLEScanResults foundDevices = pBLEScan->getResults();
  btTotalDevices = foundDevices.getCount();
  
  Serial.printf("[BLE] Feldolgozott eszközök száma: %d\n", btTotalDevices);
  
  // Cap at max devices
  if (btTotalDevices > MAX_BLE_DEVICES) btTotalDevices = MAX_BLE_DEVICES;

  for (int i = 0; i < btTotalDevices; i++) {
    const NimBLEAdvertisedDevice* device = foundDevices.getDevice(i);
    btDevices[i].name = device->getName().length() > 0 ? device->getName().c_str() : "Unknown Device";
    btDevices[i].address = device->getAddress().toString().c_str();
    btDevices[i].rssi = device->getRSSI();
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
    // Center: "BT DEVICES"
    gfx->setCursor(100, 35); // Shifted Y
    gfx->println("BT DEVICES");
    gfx->drawLine(0, 40, 320, 40, WHITE);

    // Arrows
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setCursor(10, 95); // Shifted Y from 60
    gfx->print("<");
    gfx->setCursor(290, 95); // Shifted Y from 60
    gfx->print(">");

    // Draw Static Buttons (CONNECT / DETAILS)
    gfx->drawRect(20, 110, 130, 35, GREEN);
    gfx->fillRect(22, 112, 126, 31, BLACK); // Connect btn background
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(GREEN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(42, 133); // Baseline Y approx
    gfx->print("CONNECT");

    gfx->drawRect(170, 110, 130, 35, CYAN);
    gfx->fillRect(172, 112, 126, 31, BLACK); // Details btn background
    gfx->setTextColor(CYAN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(195, 133); // Baseline Y approx
    gfx->print("DETAILS");
  }

  // Clear name area
  gfx->fillRect(30, 50, 260, 40, BLACK); // Increased clear height

  if (btTotalDevices == 0) {
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75); // Baseline Y
    gfx->println("No devices found.");
  } else {
    // Current Device Name
    CachedDevice& dev = btDevices[btSelectedDeviceIndex];
    gfx->setTextColor(WHITE, BLACK);
    
    // Determine font size based on string length to try and make it fit
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setTextSize(1);
    
    // Approximating character width for FreeSans12pt7b
    int charWidth = 12;
    int textWidth = dev.name.length() * charWidth; 
    int textX = (320 - textWidth) / 2;
    if (textX < 30) textX = 30; // Min bounds
    
    gfx->setCursor(textX, 75); // Baseline Y
    gfx->print(dev.name.c_str());

    // Update Connect button state for Current Device
    if (isBluetoothConnected && btSelectedDeviceIndex == 0 /* Hypothetical matching */) {
       gfx->fillRect(22, 112, 126, 31, GREEN);
       gfx->setFont(&FreeSans9pt7b);
       gfx->setTextColor(BLACK, GREEN);
       gfx->setTextSize(1);
       gfx->setCursor(35, 133); // Baseline Y
       gfx->print("CONNECTED");
    } else {
       gfx->fillRect(22, 112, 126, 31, BLACK); 
       gfx->setFont(&FreeSans9pt7b);
       gfx->setTextColor(GREEN, BLACK);
       gfx->setTextSize(1);
       gfx->setCursor(42, 133); // Baseline Y
       gfx->print("CONNECT");
    }

    // Horizontal Scrollbar at very bottom
    int scrollbarY = 165;
    int scrollbarX = 20;
    int scrollbarW = 280;
    int scrollbarH = 6;
    
    // Clear whole bar first
    gfx->fillRect(scrollbarX, scrollbarY, scrollbarW, scrollbarH, 0x2104);
    
    if (btTotalDevices > 1) {
      int handleW = scrollbarW / btTotalDevices;
      if (handleW < 10) handleW = 10;
      
      int slideRange = scrollbarW - handleW;
      float scrollPct = (float)btSelectedDeviceIndex / (float)(btTotalDevices - 1);
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
  // Center: "DEVICE DETAILS"
  gfx->setCursor(76, 35); // Shifted Y from 20
  gfx->println("DEVICE DETAILS");
  gfx->drawLine(0, 40, 320, 40, WHITE);

  if (btSelectedDeviceIndex >= 0 && btSelectedDeviceIndex < btTotalDevices) {
    CachedDevice& dev = btDevices[btSelectedDeviceIndex];
    
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(10, 65); // Shifted Y from 50
    gfx->printf("Name: %s", dev.name.c_str());
    gfx->setCursor(10, 95); // Shifted Y from 80
    gfx->printf("MAC : %s", dev.address.c_str());
    gfx->setCursor(10, 125); // Shifted Y from 110
    gfx->printf("RSSI: %d dBm", dev.rssi);

    gfx->setTextColor(CYAN, BLACK);
    gfx->setCursor(10, 165); // Shifted Y from 150
    gfx->println("Swipe Right -> Back");
  }
}

void startTouchTest() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  // Center: "TOUCH TEST"
  gfx->setCursor(100, 35); // Shifted Y from 20
  gfx->println("TOUCH TEST");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(CYAN);
  gfx->setTextSize(1);
  gfx->setCursor(10, 165); // Shifted Y from 150
  gfx->println("Swipe Right -> Back");
}

// ============================================================
// BLE Client Connection Infrastructure
// ============================================================

class MyClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* client) {
    Serial.println("[BLE/NimBLE] Connected to server");
  }
  void onDisconnect(NimBLEClient* client, int reason) {
    Serial.printf("[BLE/NimBLE] Disconnected (reason=%d)\n", reason);
    isBluetoothConnected = false;
    pTxChar = nullptr;
    pRxChar = nullptr;
    bleDisconnectedFlag = true; // Signal loop() to refresh UI
  }
};

// Notify callback — receives data from OBD adapter (static buffer, no heap fragmentation)
void onBLENotify(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  for (size_t i = 0; i < length; i++) {
    char c = (char)pData[i];
    if (c == '>') {
      // '>' is the ELM327 prompt, response is complete
      obdBuffer[obdBufIndex] = '\0';
      // Trim whitespace
      char* start = obdBuffer;
      while (*start == ' ' || *start == '\r' || *start == '\n') start++;
      int len = strlen(start);
      while (len > 0 && (start[len-1] == ' ' || start[len-1] == '\r' || start[len-1] == '\n')) {
        start[--len] = '\0';
      }
      lastOBDValue = String(start);
      obdBufIndex = 0;
      Serial.printf("[OBD] Response complete: %s\n", lastOBDValue.c_str());
    } else if (c != '\r') { // Skip \r characters
      if (obdBufIndex < (int)sizeof(obdBuffer) - 1) {
        obdBuffer[obdBufIndex++] = c;
      }
    }
  }
}

// Send a command string to the OBD adapter
void sendOBDCommand(const char* cmd) {
  if (pTxChar != nullptr && isBluetoothConnected) {
    char fullCmd[64];
    snprintf(fullCmd, sizeof(fullCmd), "%s\r", cmd);
    pTxChar->writeValue((uint8_t*)fullCmd, strlen(fullCmd));
    Serial.printf("[OBD] Sent: %s\n", cmd);
  }
}

// Connect to a BLE OBD adapter by device index
bool connectToOBD(int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= btTotalDevices) return false;
  if (bleConnecting) return false;
  
  bleConnecting = true;
  
  // Show "Connecting..." on screen
  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(85, 75);
  gfx->print("Connecting...");
  
  String address = btDevices[deviceIndex].address;
  Serial.printf("[BLE] Connecting to: %s (%s)\n", btDevices[deviceIndex].name.c_str(), address.c_str());
  
  // Create client
  if (pClient != nullptr) {
    pClient->disconnect();
    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;
  }
  
  pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallbacks());
  
  // Connect
  if (!pClient->connect(NimBLEAddress(std::string(address.c_str()), 0))) {
    Serial.println("[BLE] Connection failed!");
    bleConnecting = false;
    return false;
  }
  
  Serial.println("[BLE] Connected! Discovering services...");
  
  // Discover services and find TX (Write) / RX (Notify) characteristics
  // Skip standard BLE services (GAP 0x1800, GATT 0x1801) - they are NOT OBD
  pTxChar = nullptr;
  pRxChar = nullptr;
  
  const std::vector<NimBLERemoteService*>& services = pClient->getServices(true);
  for (auto pSvc : services) {
    String svcUUID = String(pSvc->getUUID().toString().c_str());
    Serial.printf("[BLE] Service: %s\n", svcUUID.c_str());
    
    // Skip standard services (GAP, GATT, Device Info)
    if (svcUUID.indexOf("1800") >= 0 || svcUUID.indexOf("1801") >= 0 || svcUUID.indexOf("180a") >= 0) {
      Serial.println("[BLE]   (Skipping standard service)");
      continue;
    }
    
    NimBLERemoteCharacteristic* svcTx = nullptr;
    NimBLERemoteCharacteristic* svcRx = nullptr;
    
    const std::vector<NimBLERemoteCharacteristic*>& chars = pSvc->getCharacteristics(true);
    for (auto pChr : chars) {
      bool writable = pChr->canWrite() || pChr->canWriteNoResponse();
      Serial.printf("[BLE]   Char: %s (canWrite=%d, canWriteNR=%d, canNotify=%d)\n", 
        pChr->getUUID().toString().c_str(), pChr->canWrite(), pChr->canWriteNoResponse(), pChr->canNotify());
      
      if (writable && svcTx == nullptr) {
        svcTx = pChr;
      }
      if (pChr->canNotify() && svcRx == nullptr) {
        svcRx = pChr;
      }
    }
    
    // Prefer a service that has BOTH TX and RX
    if (svcTx != nullptr && svcRx != nullptr && (pTxChar == nullptr || pRxChar == nullptr)) {
      pTxChar = svcTx;
      pRxChar = svcRx;
      Serial.printf("[BLE]   -> Selected TX: %s\n", pTxChar->getUUID().toString().c_str());
      Serial.printf("[BLE]   -> Selected RX: %s\n", pRxChar->getUUID().toString().c_str());
    }
  }
  
  if (pTxChar == nullptr || pRxChar == nullptr) {
    Serial.println("[BLE] Could not find TX/RX characteristics!");
    pClient->disconnect();
    bleConnecting = false;
    return false;
  }
  
  // Subscribe to notifications (NimBLE API)
  pRxChar->subscribe(true, onBLENotify);
  
  isBluetoothConnected = true;
  bleConnecting = false;
  
  // === ELM327 Identity Validation ===
  // Send ATZ (reset) and wait for response containing "ELM"
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
  sendOBDCommand("ATZ");
  
  // Wait up to 3 seconds for ELM response
  unsigned long verifyStart = millis();
  bool isELM = false;
  while (millis() - verifyStart < 3000) {
    delay(50);
    if (lastOBDValue.length() > 0) {
      Serial.printf("[OBD] Verify response: '%s'\n", lastOBDValue.c_str());
      if (lastOBDValue.indexOf("ELM") >= 0) {
        isELM = true;
        break;
      }
    }
  }
  
  if (!isELM) {
    Serial.println("[BLE] Not an ELM327/OBD adapter! Disconnecting.");
    
    // Show error on screen
    gfx->fillRect(30, 50, 260, 60, BLACK);
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75);
    gfx->print("Not an OBD device");
    
    delay(2000); // Show error for 2 seconds
    
    pClient->disconnect();
    isBluetoothConnected = false;
    pTxChar = nullptr;
    pRxChar = nullptr;
    return false;
  }
  
  Serial.printf("[BLE] ELM327 detected: %s\n", lastOBDValue.c_str());
  
  // Disable echo
  sendOBDCommand("ATE0");
  delay(500);
  
  Serial.println("[BLE] OBD adapter initialized!");
  return true;
}

// Disconnect from BLE OBD adapter
void disconnectOBD() {
  if (pClient != nullptr && pClient->isConnected()) {
    pClient->disconnect();
  }
  isBluetoothConnected = false;
  pTxChar = nullptr;
  pRxChar = nullptr;
  lastOBDValue = "";
  obdBufIndex = 0;
  obdBuffer[0] = '\0';
  Serial.println("[BLE] Disconnected from OBD");
}

void setup(void) {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  
  // PWM Backlight
  ledcAttach(TFT_BL, 5000, 8); // 5kHz, 8bit
  setBrightness(currentBrightness);

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  lcd_reg_init();
  
  // A kijelző regisztereit beállító lcd_reg_init() visszateszi portré (0x00) módba az állapotát
  // a GFX könyvtár viszont úgy ismeri, hogy már 1-es (landscape) módban van,
  // ezért kicsikarunk egy frissítést:
  gfx->setRotation(0);
  gfx->setRotation(1); // Set to Landscape
  
  touch_init();

  // Start with Home Screen
  showHome();
  
  // Init BLE (NimBLE - lightweight stack, ~5KB RAM vs ~100KB Bluedroid)
  NimBLEDevice::init("ZoEyee-Scanner");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  // Nem állítunk be setInterval/setWindow-t — a NimBLE alapértékei
  // biztonságos duty cycle-t használnak, ami nem ütközik a WiFi-vel
  
  Serial.printf("[SYS] Free heap: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
  int tx, ty;
  if (touch_read(tx, ty)) {
    if (!touching) {
      startX = tx;
      startY = ty;
      touching = true;
      touchStartTime = millis();
      Serial.printf("Touch Start: mapped(X=%d, Y=%d) raw(X=%d, Y=%d)\n", tx, ty, lastRawX, lastRawY);
    }

    // Long press detection for Home -> Menu (must hold for >800ms without moving)
    int deltaX = tx - startX;
    int deltaY = ty - startY;
    if (currentState == STATE_HOME && (millis() - touchStartTime) > 800 && abs(deltaX) < 30 && abs(deltaY) < 30) {
        currentState = STATE_MENU;
        drawMenu();
        touching = false; // Reset state
        return;
    }
    
    if (currentState == STATE_BRIGHTNESS) {
      if (startX >= 120 && startX <= 200) { // Check if touch started in slider X range
        // Sliding (Vertical adjustment)
        // ty=40 -> 255, ty=140 -> 0
        int newVal = map(ty, 140, 40, 0, 255);
        newVal = constrain(newVal, 0, 255);
        if (abs(newVal - currentBrightness) > 2) {
          setBrightness(newVal);
          showBrightness(false); // Partial redraw
        }
      }
    } else if (currentState == STATE_BT_LIST) {
        // Horizontal list, no sliding detection in else block
    } else if (currentState == STATE_TOUCH_TEST) {
      gfx->drawPixel(tx, ty, WHITE);
    }

    lastX = tx;
    lastY = ty;
  } else {
    if (touching) {
      // On Home screen, only long-press is handled above. Ignore release.
      if (currentState == STATE_HOME) {
        touching = false;
        return;
      }
      int deltaX = lastX - startX;
      int deltaY = lastY - startY;
      unsigned long tapDuration = millis() - touchStartTime;
      
      if (abs(deltaY) < 50 && tapDuration < 500) {
        if (deltaX > 80) { // Swipe Right
          if (currentState == STATE_MENU) {
            menuIndex = (menuIndex - 1 + menuCount) % menuCount;
            drawMenu(false);
          } else if (currentState == STATE_BT_LIST) {
             if (btTotalDevices > 0) {
               btSelectedDeviceIndex = (btSelectedDeviceIndex - 1 + btTotalDevices) % btTotalDevices;
               showBTList(false);
             } else {
               currentState = STATE_MENU;
               drawMenu();
             }
          } else if (currentState == STATE_BT_DEVICE_INFO) {
            // Go back to BT list instead of main menu
            currentState = STATE_BT_LIST;
            showBTList();
          } else {
            currentState = STATE_MENU;
            drawMenu();
          }
        } else if (deltaX < -80) { // Swipe Left
          if (currentState == STATE_MENU) {
            menuIndex = (menuIndex + 1) % menuCount;
            drawMenu(false);
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
              btSelectedDeviceIndex = (btSelectedDeviceIndex + 1) % btTotalDevices;
              showBTList(false);
            }
          }
        } else if (abs(deltaX) < 15 && abs(deltaY) < 15) { // Tap
          // Global Back/Home Button check (Top-Left corner)
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
            if (startY < 110) { // Tap in the main menu area
              if (menuIndex == 0) { currentState = STATE_INFO; showInfo(); }
              else if (menuIndex == 1) { currentState = STATE_WIFI_SCAN; runWifiScan(); }
              else if (menuIndex == 2) { currentState = STATE_BT_SCAN; runBLEScan(); }
              else if (menuIndex == 3) { currentState = STATE_TOUCH_TEST; startTouchTest(); }
              else if (menuIndex == 4) { currentState = STATE_BRIGHTNESS; showBrightness(); }
            }
          } else if (currentState == STATE_BT_LIST) {
            if (btTotalDevices > 0) {
               // Tap on CONNECT (X: 20-150, Y: 110-145)
               if (startX >= 20 && startX <= 150 && startY >= 110 && startY <= 145) {
                 if (!isBluetoothConnected && !bleConnecting) {
                   connectToOBD(btSelectedDeviceIndex);
                 } else if (isBluetoothConnected) {
                   disconnectOBD();
                 }
                 // Force full redraw to update top bar
                 showBTList(true); 
               }
               // Tap on DETAILS (X: 170-300, Y: 110-145) 
               else if (startX >= 170 && startX <= 300 && startY >= 110 && startY <= 145) {
                 currentState = STATE_BT_DEVICE_INFO;
                 showBTDeviceInfo();
               }
            }
          } else if (currentState != STATE_TOUCH_TEST) {
            currentState = STATE_MENU;
            drawMenu();
          }
        }
      }
      touching = false;
    }
  }

  // Backup Btn
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

  // Check for BLE disconnect event (set by callback)
  if (bleDisconnectedFlag) {
    bleDisconnectedFlag = false;
    Serial.println("[BLE] Disconnect detected, refreshing UI");
    if (currentState == STATE_HOME) {
      showHome();
    } else if (currentState == STATE_BT_LIST) {
      showBTList(true);
    } else {
      // At minimum redraw top bar to remove BT icon
      drawTopBar();
    }
  }

  // Periodic OBD voltage polling on Home screen
  if (currentState == STATE_HOME && isBluetoothConnected) {
    if (millis() - lastOBDPollTime >= OBD_POLL_INTERVAL) {
      lastOBDPollTime = millis();
      sendOBDCommand("ATRV");
    }
    // Check if new data arrived and update display
    static String lastDisplayedValue = "";
    if (lastOBDValue != lastDisplayedValue) {
      lastDisplayedValue = lastOBDValue;
      updateHomeOBD();
    }
  }

  delay(20);
}
