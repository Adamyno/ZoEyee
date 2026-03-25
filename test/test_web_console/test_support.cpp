#include "../../test/mocks/Arduino.h"
#include "../../test/mocks/WebServer.h"
#include "../../test/mocks/WiFi.h"
#include "../../test/mocks/ObdManager.h"

SerialMock Serial;
bool ObdManager::manualMode = false;
void ObdManager::sendManualCommand(const char* cmd) {}
