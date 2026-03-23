#pragma once
#include <Arduino.h>
#include "Globals.h"
#include <WiFi.h>

class WifiManager {
public:
    static void showMenu();
    static void showClientMenu();
    static void showList(bool fullRedraw = true);
    static void showKeyboard();
    static void connect();
    static void showStatus();
};
