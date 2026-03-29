#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

class DisplayManager {
public:
    static void initLCD();
    static void setBrightness(int val);
    static void showHome();
    static void updateHomeOBD();
    static void drawMenu(bool fullRedraw = true);
    static void showBrightness(bool fullRedraw = true);
    static void showInfo();
    static void showSettings(bool fullRedraw = true);
    static void showSlotPicker(bool fullRedraw = true);
    static void drawPageIndicator();
    static void restoreBottomRow();
    static void drawTopBar(bool softRefresh = false);
    static void drawStatusLED();
};

#endif

