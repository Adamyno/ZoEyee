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
  currentBrightness = constrain(val, 1, 255);
  ledcWrite(TFT_BL, currentBrightness);
}

// ============================================================
// DashParam system: each parameter has display metadata + icon
// ============================================================

// ============================================================
// Color gradient system for value display
// ============================================================

struct ColorStop {
  float value;
  uint16_t color;
};

// RGB565 linear interpolation between two colors
static uint16_t lerpColor565(uint16_t c1, uint16_t c2, float t) {
  if (t <= 0.0f) return c1;
  if (t >= 1.0f) return c2;
  int r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
  int r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
  int r = r1 + (int)((r2 - r1) * t);
  int g = g1 + (int)((g2 - g1) * t);
  int b = b1 + (int)((b2 - b1) * t);
  return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

// Map a value to a color using gradient stops
static uint16_t mapValueToColor(float value, const ColorStop *stops, int count) {
  if (value <= stops[0].value) return stops[0].color;
  if (value >= stops[count - 1].value) return stops[count - 1].color;
  for (int i = 0; i < count - 1; i++) {
    if (value <= stops[i + 1].value) {
      float t = (value - stops[i].value) / (stops[i + 1].value - stops[i].value);
      return lerpColor565(stops[i].color, stops[i + 1].color, t);
    }
  }
  return stops[count - 1].color;
}

// --- Color functions for each parameter type ---

static uint16_t colorWhite(float v) { return WHITE; }

// Battery temp: <22 light blue, 22-32 green, 33-36 orange, 37+ red
static uint16_t colorBatTemp(float v) {
  static const ColorStop stops[] = {
    {18.0f, 0xAEFF},   // light blue
    {22.0f, 0xAEFF},   // light blue
    {24.0f, 0x07E0},   // green
    {32.0f, 0x07E0},   // green
    {34.5f, 0xFD20},   // orange
    {36.0f, 0xFD20},   // orange
    {37.0f, 0xF800},   // red
  };
  return mapValueToColor(v, stops, 7);
}

// AC RPM: 0-900 d-green, 900-1400 l-green, 1500-3500 blue, 3500-4500 orange, 4500-6500 red, 6500+ purple
static uint16_t colorACRpm(float v) {
  static const ColorStop stops[] = {
    {0.0f,    0x03A0},   // dark green
    {900.0f,  0x03A0},   // dark green
    {1200.0f, 0x07E0},   // light green
    {1400.0f, 0x07E0},   // light green
    {1600.0f, 0xAEFF},   // blue start
    {3400.0f, 0xAEFF},   // blue end
    {3600.0f, 0xFD20},   // orange start
    {4400.0f, 0xFD20},   // orange end
    {4600.0f, 0xF800},   // red start
    {6200.0f, 0xF800},   // red end
    {6600.0f, 0xD01F},   // purple start
  };
  return mapValueToColor(v, stops, 11);
}

// AC Pressure: <7 red, 7-10 orange, 10-15 green, 15-18 light blue, 18+ purple
static uint16_t colorACPressure(float v) {
  static const ColorStop stops[] = {
    {4.0f,  0xF800},   // red
    {7.0f,  0xF800},   // red
    {8.5f,  0xFD20},   // orange
    {10.0f, 0xFD20},   // orange
    {12.0f, 0x07E0},   // green
    {15.0f, 0x07E0},   // green
    {16.5f, 0xAEFF},   // light blue
    {18.0f, 0xAEFF},   // light blue
    {20.0f, 0xD01F},   // purple
  };
  return mapValueToColor(v, stops, 9);
}

// ============================================================
// DashParam system: each parameter has display metadata + icon
// ============================================================

struct DashParam {
  const char *label;         // Short name: "SOH", "SOC", etc.
  const char *fullName;      // Full name for picker: "Battery SOH", etc.
  const char *unit;          // Unit string: "%", "°C", "", etc.
  float noDataSentinel;      // Below this = no data
  bool isInt;                // Show as integer?
  int decimals;              // 0 or 1
  uint16_t iconColor;        // Primary icon color (RGB565)
  void (*drawIcon)(Arduino_GFX *g, int cx, int cy, uint16_t color);
  uint16_t (*getValueColor)(float value);  // Dynamic text color
  // OBD metadata
  int ecuId;                 // 0=EVC, 1=HVAC
};

// --- Icon drawing functions ---

static void drawIconHeart(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Heart: two filled circles (top) + filled triangle (bottom)
  // Shifted up by 4px and widened to ~28px as requested
  int r = 7;
  int dy = -4; // Shift up
  g->fillCircle(cx - 7, cy - 6 + dy, r, color);
  g->fillCircle(cx + 7, cy - 6 + dy, r, color);
  g->fillTriangle(cx - 14, cy - 3 + dy, cx + 14, cy - 3 + dy, cx, cy + 14 + dy, color);
  // Smooth the gap
  g->fillRect(cx - 7, cy - 6 + dy, 14, 4, color);
}
static void drawIconBatterySOC(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Big battery outline filling ~28x44 area (proportional battery shape)
  // Battery body: 28w x 40h, centered at (cx, cy+4)
  int bx = cx - 14, by = cy - 16, bw = 28, bh = 40;
  g->drawRoundRect(bx, by, bw, bh, 4, WHITE);
  g->drawRoundRect(bx + 1, by + 1, bw - 2, bh - 2, 3, WHITE);
  // Battery terminal cap (top center)
  g->fillRoundRect(cx - 5, by - 4, 10, 5, 2, WHITE);
  // 3 light blue charge bars inside battery body
  //uint16_t barColor = 0x653F;  // Light blue
  uint16_t barColor = 0x03B7;  // Cyan
  int barX = bx + 4, barW = bw - 8, barH = 8, gap = 3;
  int barStartY = by + bh - 5 - barH;  // bottom bar
  g->fillRect(barX, barStartY, barW, barH, barColor);
  g->fillRect(barX, barStartY - barH - gap, barW, barH, barColor);
  g->fillRect(barX, barStartY - 2 * (barH + gap), barW, barH, barColor);
  // Yellow lightning bolt ⚡ drawn as a polygon (zigzag shape)
  uint16_t bolt = 0xFFE0; // Yellow
  // Upper part
  g->fillTriangle(cx + 4, cy - 12, cx - 2, cy - 12, cx - 6, cy, bolt);
  g->fillTriangle(cx + 4, cy - 12, cx - 6, cy, cx + 2, cy, bolt);
  // Lower part
  g->fillTriangle(cx - 2, cy + 1, cx + 6, cy + 1, cx - 4, cy + 16, bolt);
  // Fill the connection area between upper and lower
  g->fillRect(cx - 4, cy - 1, 8, 4, bolt);
}

static void drawIconThermometerBig(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Tall thermometer filling ~16x48 area
  // Outer bulb at bottom (radius 9)
  g->fillCircle(cx, cy + 17, 9, color);
  // Tall stem from top to bulb
  g->fillRoundRect(cx - 5, cy - 22, 10, 39, 5, color);
  // Inner mercury (bright red)
  uint16_t inner = 0xF800; // Bright red
  g->fillCircle(cx, cy + 17, 6, inner);
  g->fillRect(cx - 3, cy - 14, 6, 29, inner);
  // Tick marks (right side of stem)
  uint16_t tick = 0x7BEF;
  g->drawLine(cx + 6, cy - 18, cx + 10, cy - 18, tick);
  g->drawLine(cx + 6, cy - 12, cx + 9,  cy - 12, tick);
  g->drawLine(cx + 6, cy - 6,  cx + 10, cy - 6,  tick);
  g->drawLine(cx + 6, cy,      cx + 9,  cy,      tick);
  g->drawLine(cx + 6, cy + 6,  cx + 10, cy + 6,  tick);
}

static void drawIconBatteryThermo(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Big battery outline filling ~28x44 (same size as SOC battery)
  int bx = cx - 14, by = cy - 16, bw = 28, bh = 40;
  g->drawRoundRect(bx, by, bw, bh, 4, WHITE);
  g->drawRoundRect(bx + 1, by + 1, bw - 2, bh - 2, 3, WHITE);
  // Battery terminal cap
  g->fillRoundRect(cx - 5, by - 4, 10, 5, 2, WHITE);

  // 3 cyan charge bars inside battery body (matching SOC style)
  uint16_t barColor = 0x03B7;  // Cyan
  int barX = bx + 4, barW = bw - 8, barH = 8, gap = 3;
  int barStartY = by + bh - 5 - barH;  // bottom bar
  g->fillRect(barX, barStartY, barW, barH, barColor);
  g->fillRect(barX, barStartY - barH - gap, barW, barH, barColor);
  g->fillRect(barX, barStartY - 2 * (barH + gap), barW, barH, barColor);

  // Detailed thermometer inside battery (white + bright red)
  uint16_t thOuter = WHITE;   // White thermometer body
  uint16_t thInner = 0xF800;  // Bright red mercury
  // Outer bulb (bottom)
  g->fillCircle(cx, cy + 14, 6, thOuter);
  // Stem
  g->fillRoundRect(cx - 3, cy - 10, 6, 24, 3, thOuter);
  // Inner mercury (bright red)
  g->fillCircle(cx, cy + 14, 3, thInner);
  g->fillRect(cx - 1, cy - 5, 3, 17, thInner);
  // Tick marks inside battery (White)
  g->drawLine(cx + 4, cy - 6, cx + 8, cy - 6, WHITE);
  g->drawLine(cx + 4, cy - 1, cx + 7, cy - 1, WHITE);
  g->drawLine(cx + 4, cy + 4, cx + 8, cy + 4, WHITE);
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

static void drawIcon12VBattery(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // 12V car battery: rectangle body + 2 terminal posts on top
  int bw = 26, bh = 32;
  int bx = cx - bw/2, by = cy - bh/2 + 4;
  // Battery body
  g->drawRoundRect(bx, by, bw, bh, 3, color);
  g->drawRoundRect(bx+1, by+1, bw-2, bh-2, 2, color);
  // Terminal posts (2 small rectangles on top)
  g->fillRect(cx - 8, by - 4, 5, 5, color);
  g->fillRect(cx + 3, by - 4, 5, 5, color);
  // "12" text inside
  g->setFont(&FreeSans9pt7b);
  g->setTextColor(color);
  g->setTextSize(1);
  g->setCursor(cx - 8, cy + 12);
  g->print("12");
}

static void drawIconOutdoorThermo(Arduino_GFX *g, int cx, int cy, uint16_t color) {
  // Outdoor thermometer similar to cabin but with cyan/blue color
  // Outer bulb at bottom
  g->fillCircle(cx, cy + 17, 9, color);
  // Tall stem from top to bulb
  g->fillRoundRect(cx - 5, cy - 22, 10, 39, 5, color);
  // Inner mercury (cyan)
  uint16_t inner = 0x07FF; // CYAN
  g->fillCircle(cx, cy + 17, 6, inner);
  g->fillRect(cx - 3, cy - 14, 6, 29, inner);
  // Tick marks
  uint16_t tick = 0x7BEF;
  g->drawLine(cx + 6, cy - 18, cx + 10, cy - 18, tick);
  g->drawLine(cx + 6, cy - 12, cx + 9,  cy - 12, tick);
  g->drawLine(cx + 6, cy - 6,  cx + 10, cy - 6,  tick);
  g->drawLine(cx + 6, cy,      cx + 9,  cy,      tick);
  g->drawLine(cx + 6, cy + 6,  cx + 10, cy + 6,  tick);
}

static DashParam dashParams[] = {
  // label  fullName          unit   sentinel isInt dec color   drawIcon              getValueColor     ecuId
  {"SOH",  "Battery SOH",    "%",    -1,  true,  0, 0xF800, drawIconHeart,          colorWhite,       0},
  {"",     "Battery SOC",    "%",    -1,  false, 0, 0xFFFF, drawIconBatterySOC,     colorWhite,       0},
  {"IN",   "Cabin Temp",     "\xB0""C", -99, false, 0, WHITE, drawIconThermometerBig, colorWhite,       1},
  {"",     "Battery Temp",   "\xB0""C", -99, false, 0, 0xFFFF, drawIconBatteryThermo, colorBatTemp,    0},
  {"RPM",  "AC Compressor",  "",     -1,  true,  0, 0xB7FF, drawIconSnowflake,      colorACRpm,       1},
  {"BAR",  "AC Pressure",    "",     -1,  false, 1, 0xB7FF, drawIconSnowflake,      colorACPressure,  1},
  {"V",    "12V Battery",    "",     -1,  false, 1, 0xFD20, drawIcon12VBattery,     colorWhite,       2},
  {"OUT",  "Ext. Temp",      "\xB0""C", -99, false, 0, 0x07FF, drawIconOutdoorThermo,  colorWhite,       1},
};
static const int DASH_PARAM_COUNT = sizeof(dashParams) / sizeof(dashParams[0]);

// Helper: get current OBD value for a parameter index
static float getParamValue(int paramIndex) {
  switch (paramIndex) {
    case 0: return (float)obdSOH;
    case 1: return obdSOC;
    case 2: return obdCabinTemp;
    case 3: return obdHVBatTemp;
    case 4: return obdACRpm;
    case 5: return obdACPressure;
    case 6: return obd12VFloat;
    case 7: return obdExtTemp;
    default: return -999;
  }
}

// Helper: draw value + unit text for a cell
static void drawCellValue(Arduino_GFX *g, int x0, int y0, int cellH,
                          const DashParam &param, float value) {
  int textX = x0 + 48;
  int textY = y0 + cellH / 2 + 18;
  bool hasData = (value > param.noDataSentinel);

  // Get dynamic text color for this value
  uint16_t valColor = WHITE;
  if (param.getValueColor && hasData) {
    valColor = param.getValueColor(value);
  }

  g->setFont(&FreeSans24pt7b);
  g->setTextColor(valColor);
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

    // Unit in smaller font (only if unit string is non-empty)
    if (strlen(param.unit) > 0) {
      int16_t bx, by;
      uint16_t bw, bh;
      g->getTextBounds(buf, textX, textY, &bx, &by, &bw, &bh);
      int unitX = textX + bw + 2;
      g->setFont(&FreeSans9pt7b);
      g->setTextColor(0xBDF7); // light grey
      g->setCursor(unitX, textY);
      g->print(param.unit);
    }
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
    int paramIdx = dashPages[currentPage][i].paramIndex;
    if (paramIdx < 0 || paramIdx >= DASH_PARAM_COUNT) continue; // Empty cell

    int col = i % cols;
    int row = i / cols;
    int x0 = col * cellW;
    int y0 = row * cellH;
    int cx = x0 + 22;
    int cy = y0 + 26;

    const DashParam &p = dashParams[paramIdx];
    if (p.drawIcon) {
      p.drawIcon(gfx, cx, cy, p.iconColor);
    }

    // Draw label
    if (strlen(p.label) > 0) {
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(p.iconColor);
      gfx->setTextSize(1);

      if (paramIdx == 2) {
        // "IN" label rotated 90°
        gfx->setRotation(0);
        int px = 172 - 1 - (y0 + 30);
        int py = x0 + 2;
        gfx->setFont(&FreeSans9pt7b);
        gfx->setTextColor(p.iconColor);
        gfx->setTextSize(1);
        gfx->setCursor(px, py + 14);
        gfx->print(p.label);
        gfx->setRotation(1);
      } else {
        int16_t lx, ly;
        uint16_t lw, lh;
        gfx->getTextBounds(p.label, 0, 0, &lx, &ly, &lw, &lh);
        gfx->setCursor(cx - lw / 2, y0 + 50);
        gfx->print(p.label);
      }
    }

    // Draw value + unit
    float val = getParamValue(paramIdx);
    drawCellValue(gfx, x0, y0, cellH, p, val);
  }

  // Draw status LED
  drawStatusLED();

  // Draw page indicator if recently swiped
  if (pageSwipeTime > 0) {
    drawPageIndicator();
  }
}

void DisplayManager::updateHomeOBD() {
  // Differential update: only redraw cells whose values have changed.
  static float prevValues[6] = {-999, -999, -999, -999, -999, -999};
  static int prevPage = -1;
  static bool firstRun = true;

  if (firstRun || currentState != STATE_HOME || prevPage != currentPage) {
    showHome();
    for (int i = 0; i < 6; i++) {
      int paramIdx = dashPages[currentPage][i].paramIndex;
      prevValues[i] = (paramIdx >= 0 && paramIdx < DASH_PARAM_COUNT) ? getParamValue(paramIdx) : -999;
    }
    prevPage = currentPage;
    firstRun = false;
    return;
  }

  const int cols = 2;
  const int cellW = 160;
  const int cellH = 57;

  for (int i = 0; i < 6; i++) {
    int paramIdx = dashPages[currentPage][i].paramIndex;
    if (paramIdx < 0 || paramIdx >= DASH_PARAM_COUNT) continue; // Empty cell

    float newVal = getParamValue(paramIdx);
    float diff = newVal - prevValues[i];
    if (diff < 0) diff = -diff;
    if (diff < 0.05f) continue;

    prevValues[i] = newVal;

    int col = i % cols;
    int row = i / cols;
    int x0 = col * cellW;
    int y0 = row * cellH;

    // Clear only the text area
    gfx->fillRect(x0 + 44, y0 + 2, cellW - 46, cellH - 4, BLACK);

    // Redraw value + unit
    drawCellValue(gfx, x0, y0, cellH, dashParams[paramIdx], newVal);
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

  int iconX = 160, iconY = 111;
  if (menuIndex == 0) {
    gfx->drawXBitmap(iconX - 28, iconY - 28, icon_info_menu_bits, 56, 56,
                     WHITE);
  } else if (menuIndex == 1) {
    gfx->drawXBitmap(iconX - 50, iconY - 28, icon_wifi_menu_bits, 100, 56, WHITE);
  } else if (menuIndex == 2) {
    gfx->drawXBitmap(iconX - 18, iconY - 28, icon_bt_menu_bits, 37, 56, 0x019B);
  } else if (menuIndex == 3) {
    gfx->fillCircle(iconX, iconY, 16, YELLOW);
    for (int a = 0; a < 8; a++) {
      float angle = a * 3.14159f / 4.0f;
      int x1 = iconX + (int)(20 * cos(angle));
      int y1 = iconY + (int)(20 * sin(angle));
      int x2 = iconX + (int)(28 * cos(angle));
      int y2 = iconY + (int)(28 * sin(angle));
      gfx->drawLine(x1, y1, x2, y2, YELLOW);
      gfx->drawLine(x1 + 1, y1, x2 + 1, y2, YELLOW);
      gfx->drawLine(x1, y1 + 1, x2, y2 + 1, YELLOW);
    }
  } else if (menuIndex == 4) {
    // Grey gear icon
    uint16_t gearColor = 0x7BEF;
    gfx->fillCircle(iconX, iconY, 14, gearColor);
    gfx->fillCircle(iconX, iconY, 7, BLACK);
    for (int a = 0; a < 8; a++) {
      float angle = a * 3.14159f / 4.0f;
      int tx = iconX + (int)(18 * cos(angle));
      int ty = iconY + (int)(18 * sin(angle));
      gfx->fillCircle(tx, ty, 5, gearColor);
    }
    gfx->fillCircle(iconX, iconY, 7, BLACK);
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

void DisplayManager::showSettings(bool fullRedraw) {
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    drawTopBar();
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(98, 35);
    gfx->println("SETTINGS");
    gfx->drawLine(0, 40, 320, 40, WHITE);

    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(0x7BEF, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(70, 65);
    gfx->print("Number of pages");

    // Left/right arrows
    gfx->fillTriangle(60, 110, 80, 95, 80, 125, CYAN);
    gfx->fillTriangle(260, 110, 240, 95, 240, 125, CYAN);
  }

  // Clear number area
  gfx->fillRect(90, 80, 140, 60, BLACK);
  gfx->setFont(&FreeSans24pt7b);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(150, 125);
  gfx->printf("%d", numPages);

  // Page info
  gfx->fillRect(60, 145, 200, 25, BLACK);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(0x7BEF, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(90, 162);
  gfx->printf("Swipe L/R to adjust");
}

void DisplayManager::drawPageIndicator() {
  if (numPages <= 1) return;

  const int dotR = 4;
  const int dotSpacing = 14;
  int totalW = numPages * dotSpacing - (dotSpacing - dotR * 2);
  int startX = (320 - totalW) / 2;
  int y = 164;

  // Clear indicator area
  gfx->fillRect(startX - 6, y - dotR - 2, totalW + 12, dotR * 2 + 4, BLACK);

  for (int i = 0; i < numPages; i++) {
    int cx = startX + i * dotSpacing + dotR;
    if (i == currentPage) {
      gfx->fillCircle(cx, y, dotR, WHITE);
    } else {
      gfx->drawCircle(cx, y, dotR, 0x7BEF);
    }
  }
}

void DisplayManager::showSlotPicker(bool fullRedraw) {
  int totalItems = 1 + DASH_PARAM_COUNT;
  int itemH = 57;
  int startY = 0;
  int currentParamIdx = dashPages[pickerPage][pickerSlotIndex].paramIndex;

  if (fullRedraw) {
    gfx->fillScreen(BLACK);
  }

  // Clear items area
  gfx->fillRect(0, startY, 314, 172, BLACK);

  for (int vis = 0; vis < 3; vis++) {
    int idx = (pickerScrollIndex + vis) % totalItems;

    int y0 = startY + vis * itemH;

    bool isSelected = false;
    if (idx == 0 && currentParamIdx < 0) isSelected = true;
    if (idx > 0 && (idx - 1) == currentParamIdx) isSelected = true;

    if (isSelected) {
      gfx->fillRect(0, y0, 314, itemH - 1, 0x0333);
    }

    if (idx == 0) {
      int icx = 28, icy = y0 + itemH / 2;
      gfx->drawLine(icx - 8, icy - 8, icx + 8, icy + 8, 0x7BEF);
      gfx->drawLine(icx - 8, icy + 8, icx + 8, icy - 8, 0x7BEF);
      gfx->setFont(&FreeSans12pt7b);
      gfx->setTextColor(0x7BEF, isSelected ? 0x0333 : BLACK);
      gfx->setTextSize(1);
      gfx->setCursor(55, y0 + itemH / 2 + 7);
      gfx->print("EMPTY");
    } else {
      int paramIdx = idx - 1;
      const DashParam &p = dashParams[paramIdx];
      int iconCX = 28;
      int iconCY = y0 + itemH / 2;
      if (p.drawIcon) {
        p.drawIcon(gfx, iconCX, iconCY, p.iconColor);
      }
      gfx->setFont(&FreeSans12pt7b);
      gfx->setTextColor(WHITE, isSelected ? 0x0333 : BLACK);
      gfx->setTextSize(1);
      gfx->setCursor(55, y0 + itemH / 2 + 7);
      gfx->print(p.fullName);
    }

    if (vis < 2) {
      gfx->drawLine(0, y0 + itemH - 1, 314, y0 + itemH - 1, 0x2104);
    }
  }

  // Scroll indicator (circular)
  if (totalItems > 3) {
    int scrollBarH = 160;
    int scrollBarY = 6;
    gfx->fillRect(316, scrollBarY, 4, scrollBarH, 0x2104);
    int handleH = (3 * scrollBarH) / totalItems;
    if (handleH < 12) handleH = 12;
    int handleY = scrollBarY + (pickerScrollIndex * (scrollBarH - handleH)) / totalItems;
    gfx->fillRect(316, handleY, 4, handleH, 0x7BEF);
  }
}
