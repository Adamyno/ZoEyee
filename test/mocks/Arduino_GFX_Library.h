#pragma once
#include <stdint.h>

#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define YELLOW 0xFFE0

class Arduino_DataBus {};

extern const int FreeSans12pt7b;
extern const int FreeSans9pt7b;

class Arduino_GFX {
public:
    void fillRect(int x, int y, int w, int h, uint16_t color) {}
    void setTextColor(uint16_t c, uint16_t bg) {}
    void setTextSize(uint8_t s) {}
    void setCursor(int x, int y) {}
    void print(const char* str) {}
    void setFont(const void* f) {}
    void fillCircle(int x, int y, int r, uint16_t color) {}
};
