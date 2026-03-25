#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>

class TouchManager {
public:
    static void init();
    static bool read(int &x, int &y);
    static void processGestures();
private:
    static void handleHomeTouch(int deltaX, int deltaY, unsigned long tapDuration);
    static void handleMenuTouch();
    static void handleBtListTouch();
    static void handleBtStatusTouch();
    static void handleWifiMenuTouch();
    static void handleWifiClientMenuTouch();
    static void handleWifiListTouch();
    static void handleWifiKeyboardTouch();
};

#endif
