#pragma once
#include <Arduino.h>
#include "Globals.h"
#include <WiFi.h>

class WifiManager {
public:
    static constexpr int KB_KEY_W = 28;
    static constexpr int KB_KEY_H = 28;
    static constexpr int KB_START_Y = 28;
    static constexpr int KB_BTN_SHIFT_X = 5;
    static constexpr int KB_BTN_SHIFT_W = 55;
    static constexpr int KB_BTN_SPACE_X = 65;
    static constexpr int KB_BTN_SPACE_W = 120;
    static constexpr int KB_BTN_DEL_X = 190;
    static constexpr int KB_BTN_DEL_W = 55;
    static constexpr int KB_BTN_OK_X = 250;
    static constexpr int KB_BTN_OK_W = 65;

    static constexpr int KB_BAR_X = 0;
    static constexpr int KB_BAR_Y = 0;
    static constexpr int KB_BAR_W = 320;
    static constexpr int KB_BAR_H = 24;
    static constexpr uint16_t KB_BAR_COLOR = 0x18E3;
    static constexpr int KB_PW_TEXT_X = 5;
    static constexpr int KB_PW_TEXT_Y = 17;
    static constexpr int KB_PW_MAX_LEN = 28;

    static void showMenu();
    static void showClientMenu();
    static void showList(bool fullRedraw = true);
    static void showKeyboard();
    static void connect();
    static void showStatus();
};
