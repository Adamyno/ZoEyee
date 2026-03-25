#include "DisplayManager.h"
#include "Config.h"
#include "Globals.h"
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Icons.h"

// Official Waveshare JD9853 Initialisation Sequence
void DisplayManager::initLCD() {
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

void DisplayManager::setBrightness(int val) {
  currentBrightness = constrain(val, 0, 255);
  ledcWrite(TFT_BL, currentBrightness);
}

// ============================================================
// DashParam system: each parameter has display metadata + icon
// ============================================================

struct DashParam {
  const char *label;         // Short name: "SOH", "SOC", etc.
  const char *unit;          // Unit string: "%", "°C", "RPM", "Bar"
  float noDataSentinel;      // Below this = no data
  bool isInt;                // Show as integer?
  int decimals;              // 0 or 1
  uint16_t iconColor;        // Primary icon color (RGB565)
  void (*drawIcon)(Arduino_GFX *g, int cx, int cy, uint16_t color);
};

// --- Icon drawing functions ---

static void drawIconHeart(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Heart: two filled circles (top) + filled triangle (bottom)
  int r = 5;
  g->fillCircle(cx - 5, cy - 4, r, color);
  g->fillCircle(cx + 5, cy - 4, r, color);
  g->fillTriangle(cx - 10, cy - 2, cx + 10, cy - 2, cx, cy + 10, color);
  // Smooth the gap between circles and triangle
  g->fillRect(cx - 5, cy - 4, 10, 4, color);
}
static void drawIconBatterySOC(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Large white battery outline
  g->drawRoundRect(cx - 11, cy - 13, 22, 26, 3, WHITE);
  g->drawRoundRect(cx - 10, cy - 12, 20, 24, 2, WHITE);
  g->fillRect(cx - 4, cy - 16, 8, 4, WHITE);  // battery terminal cap
  // Yellow lightning bolt inside battery
  uint16_t bolt = 0xFFE0;
  g->fillTriangle(cx + 2, cy - 9, cx - 3, cy - 9, cx - 1, cy + 1, bolt);
  g->fillTriangle(cx - 2, cy + 1, cx + 3, cy + 1, cx + 1, cy + 9, bolt);
  g->fillRect(cx - 2, cy - 1, 4, 3, bolt);  // connect the two triangles
}

static void drawIconThermometerBig(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Larger thermometer: bigger bulb, taller stem
  g->fillCircle(cx, cy + 10, 8, color);       // big outer bulb
  g->fillRoundRect(cx - 4, cy - 14, 8, 24, 4, color); // wide stem
  // Inner mercury
  uint16_t inner = 0x8000;  // dark red
  g->fillCircle(cx, cy + 10, 5, inner);
  g->fillRect(cx - 2, cy - 8, 4, 16, inner);
  // Tick marks (right side)
  g->drawLine(cx + 5, cy - 10, cx + 9, cy - 10, 0x7BEF);
  g->drawLine(cx + 5, cy - 5,  cx + 8, cy - 5,  0x7BEF);
  g->drawLine(cx + 5, cy,      cx + 9, cy,      0x7BEF);
  g->drawLine(cx + 5, cy + 5,  cx + 8, cy + 5,  0x7BEF);
}

static void drawIconBatteryThermo(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Large white battery outline (same as SOC)
  g->drawRoundRect(cx - 11, cy - 13, 22, 26, 3, WHITE);
  g->drawRoundRect(cx - 10, cy - 12, 20, 24, 2, WHITE);
  g->fillRect(cx - 4, cy - 16, 8, 4, WHITE);  // terminal cap
  // Red thermometer inside battery
  uint16_t red = 0xF800;
  g->fillCircle(cx, cy + 7, 4, red);    // bulb
  g->fillRect(cx - 1, cy - 7, 3, 14, red);   // stem
  g->fillCircle(cx, cy + 7, 2, 0x8000); // inner bulb
}

static void drawIconSnowflake(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // 6-spoke snowflake with crystal branches
  for (int a = 0; a < 6; a++) {
    float angle = a * 3.14159f / 3.0f;
    int ex = cx + (int)(12 * cos(angle));
    int ey = cy + (int)(12 * sin(angle));
    g->drawLine(cx, cy, ex, ey, color);
    // Crystal branches at 2/3 distance
    float bA1 = angle + 0.55f;
    float bA2 = angle - 0.55f;
    int mx = cx + (int)(8 * cos(angle));
    int my = cy + (int)(8 * sin(angle));
    g->drawLine(mx, my, mx + (int)(4*cos(bA1)), my + (int)(4*sin(bA1)), color);
    g->drawLine(mx, my, mx + (int)(4*cos(bA2)), my + (int)(4*sin(bA2)), color);
  }
  g->fillCircle(cx, cy, 2, color); // center dot
}

static void drawIconGauge(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Pressure gauge: half-circle arc + needle
  // Draw arc (top half of circle, using line segments)
  for (int a = 180; a <= 360; a += 8) {
    float rad = a * 3.14159f / 180.0f;
    int x1 = cx + (int)(11 * cos(rad));
    int y1 = cy + (int)(11 * sin(rad));
    float rad2 = (a + 8) * 3.14159f / 180.0f;
    int x2 = cx + (int)(11 * cos(rad2));
    int y2 = cy + (int)(11 * sin(rad2));
    g->drawLine(x1, y1, x2, y2, color);
  }
  // Thicker arc
  for (int a = 180; a <= 360; a += 8) {
    float rad = a * 3.14159f / 180.0f;
    int x1 = cx + (int)(10 * cos(rad));
    int y1 = cy + (int)(10 * sin(rad));
    float rad2 = (a + 8) * 3.14159f / 180.0f;
    int x2 = cx + (int)(10 * cos(rad2));
    int y2 = cy + (int)(10 * sin(rad2));
    g->drawLine(x1, y1, x2, y2, color);
  }
  // Needle (pointing to ~2 o'clock = 300°)
  float needleRad = 310 * 3.14159f / 180.0f;
  int nx = cx + (int)(8 * cos(needleRad));
  int ny = cy + (int)(8 * sin(needleRad));
  g->drawLine(cx, cy, nx, ny, WHITE);
  g->drawLine(cx+1, cy, nx+1, ny, WHITE);
  // Center pivot
  g->fillCircle(cx, cy, 2, WHITE);
  // Base line
  g->drawLine(cx - 11, cy + 1, cx + 11, cy + 1, color);
}

// --- Dashboard parameter definitions ---
// This array drives the entire home screen layout.
// Future: user can swap entries per slot via long-press picker.

static DashParam dashParams[] = {
  // label   unit   sentinel  isInt  dec  color   drawIcon
  {"SOH",   "%",    -1,  true,  0, 0xF800, drawIconHeart},         // Red heart
  {"",      "%",    -1,  false, 0, 0xFFFF, drawIconBatterySOC},    // White bat + yellow bolt (no label)
  {"Cabin", "\xB0""C", -99, false, 0, 0xFD20, drawIconThermometerBig}, // Orange thermo, vertical label
  {"",      "\xB0""C", -99, false, 0, 0xFFFF, drawIconBatteryThermo}, // White bat + red thermo (no label)
  {"RPM",   "RPM",  -1,  true,  0, 0xB7FF, drawIconSnowflake},    // Light-blue snowflake
  {"BAR",   "Bar",  -1,  false, 1, 0xB7FF, drawIconSnowflake},    // Same snowflake, BAR label
};

// Helper: get current OBD value for slot index
static float getSlotValue(int slotIndex) {
  switch (slotIndex) {
    case 0: return (float)obdSOH;
    case 1: return obdSOC;
    case 2: return obdCabinTemp;
    case 3: return obdHVBatTemp;
    case 4: return obdACRpm;
    case 5: return obdACPressure;
    default: return -999;
  }
}

// Helper: draw value + unit text for a cell
static void drawCellValue(Arduino_GFX *g, int x0, int y0, int cellH,
                          const DashParam &param, float value) {
  int textX = x0 + 48;
  int textY = y0 + cellH / 2 + 18;
  bool hasData = (value > param.noDataSentinel);

  g->setFont(&FreeSans24pt7b);
  g->setTextColor(WHITE);
  g->setTextSize(1);

  if (hasData) {
    char buf[16];
    if (param.isInt) {
      snprintf(buf, sizeof(buf), "%d", (int)value);
    } else if (param.decimals >= 1) {
      snprintf(buf, sizeof(buf), "%.1f", value);
    } else {
      snprintf(buf, sizeof(buf), "%.0f", value);
    }
    g->setCursor(textX, textY);
    g->print(buf);

    // Unit in smaller font
    int16_t bx, by;
    uint16_t bw, bh;
    g->getTextBounds(buf, textX, textY, &bx, &by, &bw, &bh);
    int unitX = textX + bw + 2;
    g->setFont(&FreeSans9pt7b);
    g->setTextColor(0xBDF7); // light grey
    g->setCursor(unitX, textY);
    g->print(param.unit);
  } else {
    g->setCursor(textX, textY);
    g->print("--");
  }
}

void DisplayManager::showHome() {
  gfx->fillScreen(BLACK);

  const int cols = 2;
  const int rows = 3;
  const int cellW = 160;
  const int cellH = 57;  // 172 / 3

  // Draw grid lines (subtle dark grey)
  gfx->drawLine(cellW, 0, cellW, 172, 0x4208);
  for (int r = 1; r < rows; r++) {
    gfx->drawLine(0, r * cellH, 320, r * cellH, 0x4208);
  }

  for (int i = 0; i < 6; i++) {
    int col = i % cols;
    int row = i / cols;
    int x0 = col * cellW;
    int y0 = row * cellH;
    int cx = x0 + 22;
    int cy = y0 + 18;  // icon center (upper part of cell)

    // Draw colored icon
    const DashParam &p = dashParams[i];
    if (p.drawIcon) {
      p.drawIcon(gfx, cx, cy, p.iconColor);
    }

    // Draw label: skip if empty, draw vertically on left edge for Cabin
    if (strlen(p.label) > 0) {
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(p.iconColor);
      gfx->setTextSize(1);

      if (i == 2) {
        // Cabin: draw label vertically on the left edge of the cell
        // Each character drawn top-to-bottom, rotated 90° using setRotation
        // Since GFX doesn't support per-text rotation easily, draw chars
        // top-to-bottom at x=x0+6, stepping ~11px per char
        int charY = y0 + 8;
        for (const char *ch = p.label; *ch != '\0'; ch++) {
          char buf[2] = {*ch, '\0'};
          gfx->setCursor(x0 + 2, charY);
          gfx->print(buf);
          charY += 12;
        }
      } else {
        // Center label below the icon
        int16_t lx, ly;
        uint16_t lw, lh;
        gfx->getTextBounds(p.label, 0, 0, &lx, &ly, &lw, &lh);
        gfx->setCursor(cx - lw / 2, y0 + 50);
        gfx->print(p.label);
      }
    }

    // Draw value + unit
    float val = getSlotValue(i);
    drawCellValue(gfx, x0, y0, cellH, p, val);
  }

  // Draw status LED overlaying top-right corner
  drawStatusLED();
}

void DisplayManager::updateHomeOBD() {
  // Differential update: only redraw cells whose values have changed.
  static float prevValues[6] = {-999, -999, -999, -999, -999, -999};
  static bool firstRun = true;

  if (firstRun || currentState != STATE_HOME) {
    showHome();
    for (int i = 0; i < 6; i++) prevValues[i] = getSlotValue(i);
    firstRun = false;
    return;
  }

  const int cols = 2;
  const int cellW = 160;
  const int cellH = 57;

  for (int i = 0; i < 6; i++) {
    float newVal = getSlotValue(i);
    float diff = newVal - prevValues[i];
    if (diff < 0) diff = -diff;
    if (diff < 0.05f) continue;

    prevValues[i] = newVal;

    int col = i % cols;
    int row = i / cols;
    int x0 = col * cellW;
    int y0 = row * cellH;

    // Clear only the text area (right side of cell, after icon+label)
    gfx->fillRect(x0 + 44, y0 + 2, cellW - 46, cellH - 4, BLACK);

    // Redraw value + unit
    drawCellValue(gfx, x0, y0, cellH, dashParams[i], newVal);
  }

  drawStatusLED();
}

// ── Status LED ──────────────────────────────────────────────
void DisplayManager::drawStatusLED() {
  const int ledX = 308;
  const int ledY = 12;
  const int ledR = 7;

  bool canActive = (lastOBDRxTime > 0 && millis() - lastOBDRxTime < 500);
  bool wifiConn  = (WiFi.status() == WL_CONNECTED);

  uint16_t ledColor;
  if (canActive) {
    ledColor = GREEN;
  } else if (isBluetoothConnected) {
    ledColor = 0x03FF; // bright blue
  } else if (wifiConn) {
    ledColor = YELLOW;
  } else {
    ledColor = 0x7BEF; // grey
  }

  gfx->fillCircle(ledX, ledY, ledR, ledColor);
  gfx->drawCircle(ledX, ledY, ledR, WHITE);
}

// ── Menu, Brightness, Info screens (keep old top-bar nav) ───
void DisplayManager::drawMenu(bool fullRedraw) {
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    drawTopBar();
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(125, 35);
    gfx->println("MENU");
    gfx->drawLine(0, 40, 320, 40, WHITE);
    gfx->setFont(&FreeSans18pt7b);
    gfx->setTextColor(CYAN, BLACK);
    gfx->fillTriangle(10, 100, 25, 90, 25, 110, CYAN);
    gfx->fillTriangle(310, 100, 295, 90, 295, 110, CYAN);
  }
  gfx->fillRect(28, 50, 264, 122, BLACK);

  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(1);
  int textX = 160 - (strlen(menuItems[menuIndex]) * 9);
  if (menuIndex == 0)
    textX = 138;
  else if (menuIndex == 1)
    textX = 120;
  else if (menuIndex == 2)
    textX = 90;
  else if (menuIndex == 3)
    textX = 60;
  gfx->setCursor(textX, 108);
  gfx->print(menuItems[menuIndex]);

  int iconX = 160, iconY = 145;
  if (menuIndex == 0) {
    gfx->drawXBitmap(iconX - 28, iconY - 28, icon_info_menu_bits, 56, 56,
                     WHITE);
  } else if (menuIndex == 1) {
    gfx->drawXBitmap(110, 114, icon_wifi_menu_bits, 100, 56, WHITE);
  } else if (menuIndex == 2) {
    gfx->drawXBitmap(iconX - 18, 117, icon_bt_menu_bits, 37, 56, 0x019B);
  } else if (menuIndex == 3) {
    gfx->fillCircle(iconX, iconY, 8, YELLOW);
    for (int a = 0; a < 8; a++) {
      float angle = a * 3.14159f / 4.0f;
      int x1 = iconX + (int)(11 * cos(angle));
      int y1 = iconY + (int)(11 * sin(angle));
      int x2 = iconX + (int)(16 * cos(angle));
      int y2 = iconY + (int)(16 * sin(angle));
      gfx->drawLine(x1, y1, x2, y2, YELLOW);
    }
  }
}

void DisplayManager::showBrightness(bool fullRedraw) {
  int sliderY = 50;
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
    gfx->setTextColor(0x7BEF, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(5, 75);
    gfx->print("Swipe up/down");
    gfx->setCursor(5, 95);
    gfx->print("anywhere to");
    gfx->setCursor(5, 115);
    gfx->print("adjust");
    gfx->setCursor(5, 135);
    gfx->print("brightness.");
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

void DisplayManager::showInfo() {
  gfx->fillScreen(BLACK);
  drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(128, 35);
  gfx->println("INFO");
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

void DisplayManager::drawTopBar(bool softRefresh) {
  if (currentState == STATE_HOME) {
    // Home screen: only update LED, no nav bar
    drawStatusLED();
    return;
  }

  // Non-HOME screens: navigation bar + LED
  if (!softRefresh) {
    gfx->fillRect(0, 0, 320, 20, BLACK);
    if (currentState == STATE_MENU) {
      gfx->fillTriangle(15, 2, 5, 11, 25, 11, WHITE);
      gfx->fillRect(10, 11, 10, 8, WHITE);
      gfx->fillRect(13, 15, 4, 4, BLACK);
    } else {
      gfx->fillTriangle(5, 10, 15, 4, 15, 16, WHITE);
      gfx->fillRect(15, 8, 6, 4, WHITE);
    }
  } else {
    gfx->fillRect(290, 0, 30, 24, BLACK);
  }
  drawStatusLED();
}

