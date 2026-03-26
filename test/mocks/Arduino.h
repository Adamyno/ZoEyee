#pragma once
#include <string>
#include <iostream>
#include <cstdint>
#include <cstring>

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

    String substring(int start, int end) const {
        if (start < 0) start = 0;
        if (start >= (int)std::string::length()) return String("");
        if (end > (int)std::string::length()) end = std::string::length();
        if (end <= start) return String("");
        return String(std::string::substr(start, end - start));
    }

    String substring(int start) const {
        if (start < 0) start = 0;
        if (start >= (int)std::string::length()) return String("");
        return String(std::string::substr(start));
    }

    const char* c_str() const {
        return std::string::c_str();
    }

    void trim() {
        size_t first = find_first_not_of(' ');
        if (std::string::npos == first) {
            std::string::clear();
            return;
        }
        size_t last = find_last_not_of(' ');
        std::string::assign(substr(first, (last - first + 1)));
    }

    void replace(const char* find, const char* replace_with) {
        size_t pos = 0;
        std::string f(find);
        std::string r(replace_with);
        while ((pos = std::string::find(f, pos)) != std::string::npos) {
            std::string::replace(pos, f.length(), r);
            pos += r.length();
        }
    }

    int indexOf(const char* s) const {
        size_t pos = std::string::find(s);
        if (pos == std::string::npos) return -1;
        return (int)pos;
    }

    int indexOf(const String& s) const {
        size_t pos = std::string::find(s);
        if (pos == std::string::npos) return -1;
        return (int)pos;
    }

    int indexOf(char c) const {
        size_t pos = std::string::find(c);
        if (pos == std::string::npos) return -1;
        return (int)pos;
    }

    char charAt(int i) const {
        if (i < 0 || i >= (int)length()) return 0;
        return std::string::at(i);
    }

    void toUpperCase() {
        for (char& c : *this) {
            c = toupper(c);
        }
    }

    bool startsWith(const char* s) const {
        return std::string::find(s) == 0;
    }

    bool endsWith(const char* s) const {
        std::string suffix(s);
        if (std::string::length() >= suffix.length()) {
            return (0 == std::string::compare(std::string::length() - suffix.length(), suffix.length(), suffix));
        } else {
            return false;
        }
    }
};

class SerialMock {
public:
    void println(const char* s) { std::cout << s << std::endl; }
    void println(const String& s) { std::cout << s.c_str() << std::endl; }

    template<typename... Args>
    void printf(const char* format, Args... args) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), format, args...);
        std::cout << buffer;
    }
};

extern SerialMock Serial;

// Mock millis and delay
unsigned long millis();
void delay(unsigned long ms);

#define HTTP_GET 0
