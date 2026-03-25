#pragma once
#include <cstdint>
#include <vector>

class NimBLERemoteCharacteristic {
public:
    std::vector<uint8_t> lastWrittenValue;

    void writeValue(const uint8_t* data, size_t length) {
        lastWrittenValue.assign(data, data + length);
    }
};

class NimBLEScan {};
class NimBLEClient {};
class NimBLEAddress {};
class NimBLEDevice {};
