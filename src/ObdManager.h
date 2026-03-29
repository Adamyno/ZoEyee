#pragma once
#include <Arduino.h>
#include "Globals.h"

class ObdManager {
public:
    static bool manualMode;
    static void onBLENotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify);
    static void sendCommand(const char *cmd);
    static void sendManualCommand(const char *cmd);
    static bool initOBD();
    static void processPolling();
    static void processHvacStep();  // Non-blocking HVAC state machine
    static void processLbcStep();   // Non-blocking LBC state machine
    static void resetPollIndex();   // Reset poll index (on page change)
    static void buildPollList();    // Build poll list from current page
};
