#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>
#include "../../src/ObdManager.h"
#include "../../src/WebConsole.h"
#include "../../test/mocks/NimBLEDevice.h"
#include "../../src/Globals.h"

// Reset global state for testing
void resetState() {
    WebConsole::clearLogBuffer();
    // We shouldn't access pTxChar's members if we are not sure what it points to.
    // In our tests, we will re-assign pTxChar anyway.
}

void test_sendCommand_normal() {
    resetState();

    isBluetoothConnected = true;
    NimBLERemoteCharacteristic mockTxChar;
    pTxChar = &mockTxChar;

    const char* cmd = "ATZ";
    ObdManager::sendCommand(cmd);

    // Verify it appended '\r'
    std::string expectedCmd = "ATZ\r";
    assert(pTxChar->lastWrittenValue.size() == expectedCmd.length());
    assert(std::memcmp(pTxChar->lastWrittenValue.data(), expectedCmd.data(), expectedCmd.length()) == 0);

    // Verify WebConsole logging
    String logBuf = WebConsole::getLogBuffer();
    assert(logBuf == "TX: ATZ\n");

    std::cout << "test_sendCommand_normal passed" << std::endl;
}

void test_sendCommand_null() {
    resetState();

    isBluetoothConnected = true;
    NimBLERemoteCharacteristic mockTxChar;
    pTxChar = &mockTxChar;

    std::cout << "Starting test_sendCommand_null..." << std::endl;
    ObdManager::sendCommand(nullptr);

    // Should not crash, and should not write anything
    assert(pTxChar->lastWrittenValue.empty());

    // WebConsole should be empty
    String logBuf = WebConsole::getLogBuffer();
    assert(logBuf == "");

    std::cout << "test_sendCommand_null passed" << std::endl;
}

void test_sendCommand_too_long() {
    resetState();

    isBluetoothConnected = true;
    NimBLERemoteCharacteristic mockTxChar;
    pTxChar = &mockTxChar;

    // Create a 63-character string. ObdManager has 64 char buffer.
    // strlen + 2 > 64 will trigger the length check.
    std::string longCmd(63, 'A');
    ObdManager::sendCommand(longCmd.c_str());

    // Should be dropped
    assert(pTxChar->lastWrittenValue.empty());

    // WebConsole should be empty (or error logged, but currently it's empty according to implementation)
    String logBuf = WebConsole::getLogBuffer();
    assert(logBuf == "");

    std::cout << "test_sendCommand_too_long passed" << std::endl;
}

void test_sendCommand_no_bluetooth() {
    resetState();

    isBluetoothConnected = false;
    NimBLERemoteCharacteristic mockTxChar;
    pTxChar = &mockTxChar;

    ObdManager::sendCommand("ATZ");

    // Should not write
    assert(pTxChar->lastWrittenValue.empty());

    // No WebConsole log
    String logBuf = WebConsole::getLogBuffer();
    assert(logBuf == "");

    std::cout << "test_sendCommand_no_bluetooth passed" << std::endl;
}

void test_sendCommand_null_pTxChar() {
    resetState();

    isBluetoothConnected = true;
    pTxChar = nullptr;

    ObdManager::sendCommand("ATZ");

    // No crash, and WebConsole shouldn't log
    String logBuf = WebConsole::getLogBuffer();
    assert(logBuf == "");

    std::cout << "test_sendCommand_null_pTxChar passed" << std::endl;
}

int main() {
    test_sendCommand_normal();
    test_sendCommand_null();
    test_sendCommand_too_long();
    test_sendCommand_no_bluetooth();
    test_sendCommand_null_pTxChar();

    std::cout << "All ObdManager::sendCommand tests passed!" << std::endl;
    return 0;
}
