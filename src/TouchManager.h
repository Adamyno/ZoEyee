#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>

class TouchManager {
public:
    static void init();
    static bool read(int &x, int &y);
    static void processGestures();
};

#endif
