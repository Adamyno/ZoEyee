#pragma once
#include <string>
#include <iostream>
#include <cstdint>

class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s ? s : "") {}
    String(const std::string& s) : std::string(s) {}

    String& operator+=(const String& other) {
        std::string::operator+=(other);
        return *this;
    }

    String operator+(const char* other) const {
        return String(static_cast<std::string>(*this) + other);
    }

    String operator+(const String& other) const {
        return String(static_cast<std::string>(*this) + static_cast<std::string>(other));
    }

    int length() const {
        return (int)std::string::length();
    }

    String substring(int start) const {
        if (start < 0) start = 0;
        if (start >= (int)std::string::length()) return String("");
        return String(std::string::substr(start));
    }

    const char* c_str() const {
        return std::string::c_str();
    }
};

class SerialMock {
public:
    void println(const char* s) { std::cout << s << std::endl; }
    void println(const String& s) { std::cout << s.c_str() << std::endl; }
};

extern SerialMock Serial;

#define HTTP_GET 0
