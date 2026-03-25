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

    static void showMenu();
    static void showClientMenu();
    static void showList(bool fullRedraw = true);
    static void showKeyboard();
    static void connect();
    static void showStatus();
};
