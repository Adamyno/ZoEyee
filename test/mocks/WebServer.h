#pragma once
#include "Arduino.h"
#include <functional>
#include <map>

class WebServer {
public:
    WebServer(int port) {}
    void on(const char* path, int method, std::function<void()> handler) {}
    void send(int code, const char* type, const String& content) {}
    void begin() {}
    void handleClient() {}
    bool hasArg(const char* name) { return false; }
    String arg(const char* name) { return ""; }
};
