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

void DisplayManager::showHome() {
  gfx->fillScreen(BLACK);

  // === Fullscreen 3x2 Grid Dashboard ===
  const int gridW = 320;
  const int gridH = 172;
  const int cols = 2;
  const int rows = 3;
  const int cellW = gridW / cols;   // 160
  const int cellH = gridH / rows;   // 57

  // Draw grid lines (subtle dark grey)
  gfx->drawLine(cellW, 0, cellW, gridH, 0x4208);
  for (int r = 1; r < rows; r++) {
    gfx->drawLine(0, r * cellH, gridW, r * cellH, 0x4208);
  }

  struct CellData {
    int iconType;
    float value;
    float noDataSentinel;
    const char *unit;
    bool isInt;
  };

  CellData cells[6] = {
    {0, (float)obdSOH,   -1,  "%",  true },  // SOH
    {1, obdSOC,          -1,  "%",  false},  // SOC
    {2, obdCabinTemp,    -99, "\xB0" "C",  false},  // Cabin
    {3, obdHVBatTemp,    -99, "\xB0" "C",  false},  // Battery
    {4, obdACRpm,       -1,  "",    true },  // AC RPM
    {5, obdACPressure,   -1,  "",    false},  // AC Pressure
  };

  for (int i = 0; i < 6; i++) {
    int col = i % cols;
    int row = i / cols;
    int x0 = col * cellW;
    int y0 = row * cellH;
    int cx = x0 + 24;
    int cy = y0 + cellH / 2;

    // --- Draw icon ---
    uint16_t ic = 0x7BEF;
    switch (cells[i].iconType) {
      case 0: // Battery health (SOH)
        gfx->drawRect(cx-8, cy-12, 16, 24, ic);
        gfx->fillRect(cx-3, cy-14, 6, 3, ic);
        gfx->fillCircle(cx-3, cy-2, 2, ic);
        gfx->fillCircle(cx+3, cy-2, 2, ic);
        gfx->fillTriangle(cx-5, cy-1, cx+5, cy-1, cx, cy+5, ic);
        break;
      case 1: // Battery charge (SOC)
        gfx->drawRect(cx-8, cy-12, 16, 24, ic);
        gfx->fillRect(cx-3, cy-14, 6, 3, ic);
        gfx->drawLine(cx+2, cy-8, cx-3, cy, ic);
        gfx->drawLine(cx-3, cy, cx+2, cy, ic);
        gfx->drawLine(cx+2, cy, cx-3, cy+8, ic);
        break;
      case 2: // Thermometer (cabin)
        gfx->drawCircle(cx, cy+7, 5, ic);
        gfx->fillRect(cx-2, cy-12, 4, 16, ic);
        gfx->fillCircle(cx, cy+7, 3, ic);
        gfx->drawRect(cx-3, cy-13, 6, 2, ic);
        break;
      case 3: // Battery+temp
        gfx->drawRect(cx-10, cy-10, 12, 20, ic);
        gfx->fillRect(cx-7, cy-12, 6, 3, ic);
        gfx->drawCircle(cx+8, cy+5, 3, ic);
        gfx->fillRect(cx+7, cy-8, 2, 12, ic);
        break;
      case 4: // Snowflake (AC RPM)
      case 5: // Snowflake (AC Pressure)
        // 6-spoke snowflake
        for (int a = 0; a < 6; a++) {
          float angle = a * 3.14159f / 3.0f;
          int ex = cx + (int)(11 * cos(angle));
          int ey = cy + (int)(11 * sin(angle));
          gfx->drawLine(cx, cy, ex, ey, ic);
          // small branches
          float bAngle1 = angle + 0.5f;
          float bAngle2 = angle - 0.5f;
          int mx = cx + (int)(7 * cos(angle));
          int my = cy + (int)(7 * sin(angle));
          gfx->drawLine(mx, my, mx + (int)(4*cos(bAngle1)), my + (int)(4*sin(bAngle1)), ic);
          gfx->drawLine(mx, my, mx + (int)(4*cos(bAngle2)), my + (int)(4*sin(bAngle2)), ic);
        }
        break;
    }

    // --- Draw value ---
    gfx->setFont(&FreeSans24pt7b);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);

    int textX = x0 + 48;
    int textY = y0 + cellH / 2 + 18;
    bool hasData = (cells[i].value > cells[i].noDataSentinel);

    if (hasData) {
      char buf[16];
      if (cells[i].isInt) {
        snprintf(buf, sizeof(buf), "%d", (int)cells[i].value);
      } else {
        if (cells[i].iconType == 5) {
          snprintf(buf, sizeof(buf), "%.1f", cells[i].value);
        } else {
          snprintf(buf, sizeof(buf), "%.0f", cells[i].value);
        }
      }
      gfx->setCursor(textX, textY);
      gfx->print(buf);

      // Draw unit in smaller font after value
      int16_t x1, y1;
      uint16_t w, h;
      gfx->getTextBounds(buf, textX, textY, &x1, &y1, &w, &h);
      int unitX = textX + w + 2;
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(0xBDF7); // light grey
      if (cells[i].iconType == 4) {
        // AC RPM
        gfx->setCursor(unitX, textY);
        gfx->print("RPM");
      } else if (cells[i].iconType == 5) {
        // AC Pressure
        gfx->setCursor(unitX, textY);
        gfx->print("Bar");
      } else if (strlen(cells[i].unit) > 0) {
        gfx->setCursor(unitX, textY);
        gfx->print(cells[i].unit);
      }
    } else {
      gfx->setCursor(textX, textY);
      gfx->print("--");
    }
  }

  // Draw status LED overlaying top-right corner
  drawStatusLED();
}

void DisplayManager::updateHomeOBD() {
  showHome();
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

