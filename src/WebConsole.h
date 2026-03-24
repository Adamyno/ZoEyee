#pragma once
#include <Arduino.h>
#include <WebServer.h>

class WebConsole {
public:
    static void begin();
    static void handleClient();
    static void pushLog(const String& line);

private:
    static WebServer server;
    static String logBuffer;
};
