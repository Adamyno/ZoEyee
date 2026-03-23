#pragma once
#include <Arduino.h>
#include "Globals.h"

class BluetoothManager {
public:
    static void runBLEScan();
    static void showList(bool fullRedraw = true);
    static void showDeviceInfo();
    static bool connect(int deviceIndex);
    static void disconnect();
};
