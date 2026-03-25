#pragma once
#include "Arduino.h"

class NimBLERemoteCharacteristic;

class ObdManager {
public:
    static bool manualMode;
    static void sendManualCommand(const char* cmd);
    static void onBLENotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify) {}
    static void sendCommand(const char *cmd) {}
    static bool initOBD() { return true; }
    static void processPolling() {}
    static void readHvacBlocking() {}
};
