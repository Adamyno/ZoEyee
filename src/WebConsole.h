#pragma once
#include <Arduino.h>
#include <WebServer.h>

class WebConsole {
public:
    static void begin();
    static void handleClient();
    static void pushLog(const String& line);

#ifdef UNIT_TEST
    // Testing helpers
    static const String& getLogBuffer() { return logBuffer; }
    static void clearLogBuffer() { logBuffer = ""; }
#endif

private:
    static WebServer server;
    static String logBuffer;
};
