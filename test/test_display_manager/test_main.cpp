#include <Arduino.h>
#include <unity.h>
#include "DisplayManager.h"
#include "Globals.h"
#include "Config.h"

// --- Mocking ledcWrite ---
// To verify ledcWrite is called correctly, we override it.
// In ESP32 Arduino, ledcWrite is a strong function.
// A common technique in testing is to redefine it via a macro before including the source
// but since DisplayManager.cpp includes Arduino.h, we can't easily intercept it there
// unless we use a build flag.
// Assuming we are compiling natively where ledcWrite is NOT defined, we can define it:

#ifndef ARDUINO
// If compiling natively
int last_ledc_pin = -1;
int last_ledc_val = -1;

void ledcWrite(uint8_t pin, uint32_t val) {
    last_ledc_pin = pin;
    last_ledc_val = val;
}
#else
// If compiling for ESP32, ledcWrite is already defined in the core.
// We can't easily mock it without linker flags (-Wl,--wrap=ledcWrite),
// but we can still test that currentBrightness is set correctly.
#endif

void setUp(void) {
    // Reset state before each test
    currentBrightness = 0;
#ifndef ARDUINO
    last_ledc_pin = -1;
    last_ledc_val = -1;
#endif
}

void tearDown(void) {
    // clean up after each test
}

void test_setBrightness_negative(void) {
    DisplayManager::setBrightness(-50);
    TEST_ASSERT_EQUAL(0, currentBrightness);
#ifndef ARDUINO
    TEST_ASSERT_EQUAL(TFT_BL, last_ledc_pin);
    TEST_ASSERT_EQUAL(0, last_ledc_val);
#endif
}

void test_setBrightness_zero(void) {
    DisplayManager::setBrightness(0);
    TEST_ASSERT_EQUAL(0, currentBrightness);
#ifndef ARDUINO
    TEST_ASSERT_EQUAL(TFT_BL, last_ledc_pin);
    TEST_ASSERT_EQUAL(0, last_ledc_val);
#endif
}

void test_setBrightness_in_range(void) {
    DisplayManager::setBrightness(128);
    TEST_ASSERT_EQUAL(128, currentBrightness);
#ifndef ARDUINO
    TEST_ASSERT_EQUAL(TFT_BL, last_ledc_pin);
    TEST_ASSERT_EQUAL(128, last_ledc_val);
#endif
}

void test_setBrightness_max(void) {
    DisplayManager::setBrightness(255);
    TEST_ASSERT_EQUAL(255, currentBrightness);
#ifndef ARDUINO
    TEST_ASSERT_EQUAL(TFT_BL, last_ledc_pin);
    TEST_ASSERT_EQUAL(255, last_ledc_val);
#endif
}

void test_setBrightness_above_max(void) {
    DisplayManager::setBrightness(300);
    TEST_ASSERT_EQUAL(255, currentBrightness);
#ifndef ARDUINO
    TEST_ASSERT_EQUAL(TFT_BL, last_ledc_pin);
    TEST_ASSERT_EQUAL(255, last_ledc_val);
#endif
}

// PlatformIO testing entry point
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_setBrightness_negative);
    RUN_TEST(test_setBrightness_zero);
    RUN_TEST(test_setBrightness_in_range);
    RUN_TEST(test_setBrightness_max);
    RUN_TEST(test_setBrightness_above_max);

    UNITY_END();
}

void loop() {
    delay(100);
}

// Native testing entry point
#ifndef ARDUINO
int main(int argc, char **argv) {
    setUp();
    UNITY_BEGIN();

    RUN_TEST(test_setBrightness_negative);
    RUN_TEST(test_setBrightness_zero);
    RUN_TEST(test_setBrightness_in_range);
    RUN_TEST(test_setBrightness_max);
    RUN_TEST(test_setBrightness_above_max);

    UNITY_END();
    return 0;
}
#endif