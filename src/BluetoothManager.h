#pragma once
#include <Arduino.h>
#include "Globals.h"

class BluetoothManager {
public:
    static void runBLEScan();
    static void showList(bool fullRedraw = true);
    static void showDeviceInfo();
    static void showStatus();
    static bool connect(int deviceIndex);
    static bool connectByMAC(String mac);
    static void startReconnectTask(String mac); // Non-blocking reconnect via FreeRTOS task
    static void disconnect();
};
