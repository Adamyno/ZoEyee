#pragma once
#include <Arduino.h>
#include "Globals.h"

class ObdManager {
public:
    static void onBLENotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify);
    static void sendCommand(const char *cmd);
    static bool initOBD();
    static void processPolling();
};
