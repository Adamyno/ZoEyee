#pragma once

#include <Arduino.h>

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
#define SW_VERSION "1.3.6"

// Color definitions (RGB 565)
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// System limits
#define MAX_BLE_DEVICES 30
#define MAX_WIFI_NETWORKS 20

// WiFi AP credentials
#define WIFI_AP_SSID "ZoEyee-Config"
#define WIFI_AP_PASS "ZoEyee@123"
