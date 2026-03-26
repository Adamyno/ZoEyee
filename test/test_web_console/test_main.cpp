#include <iostream>
#include <cassert>
#include <vector>
#include "../../src/WebConsole.h"

// Define the static members as they are in WebConsole.cpp but for our test environment
// Since we are compiling WebConsole.cpp with our test, we should be careful about duplicate definitions
// However, if we compile them together, we need to handle the dependencies.

void test_basic_push() {
    WebConsole::clearLogBuffer();
    WebConsole::pushLog("Test Line 1");
    assert(WebConsole::getLogBuffer() == "Test Line 1\n");
    std::cout << "test_basic_push passed" << std::endl;
}

void test_multiple_push() {
    WebConsole::clearLogBuffer();
    WebConsole::pushLog("Line 1");
    WebConsole::pushLog("Line 2");
    assert(WebConsole::getLogBuffer() == "Line 1\nLine 2\n");
    std::cout << "test_multiple_push passed" << std::endl;
}

void test_buffer_limit() {
    WebConsole::clearLogBuffer();

    // Create a string that is almost 4096 characters
    std::string largeContent(4000, 'A');
    WebConsole::pushLog(largeContent.c_str()); // This will add \n, total 4001

    assert(WebConsole::getLogBuffer().length() == 4001);

    // Adding another 100 chars should trigger the limit (4101 > 4096)
    // The logic is:
    // logBuffer += line + "\n";
    // if(logBuffer.length() > 4096) {
    //    logBuffer = logBuffer.substring(logBuffer.length() - 2048);
    // }

    std::string moreContent(100, 'B');
    WebConsole::pushLog(moreContent.c_str());

    // Total length was 4001. After adding "B"*100 + "\n", it becomes 4102.
    // 4102 > 4096, so it takes substring(4102 - 2048) = substring(2054).
    // Resulting length should be 2048.

    assert(WebConsole::getLogBuffer().length() == 2048);

    // Verify it contains the end of the content
    String buffer = WebConsole::getLogBuffer();
    assert(buffer.substring(buffer.length() - 5) == "BBBB\n");

    std::cout << "test_buffer_limit passed" << std::endl;
}

void test_exact_limit() {
    WebConsole::clearLogBuffer();
    // 4096 chars exactly (including the \n added by pushLog)
    std::string content(4095, 'C');
    WebConsole::pushLog(content.c_str());
    assert(WebConsole::getLogBuffer().length() == 4096);

    // Should NOT have truncated yet
    std::string content2(1, 'D');
    WebConsole::pushLog(content2.c_str());
    // Now length is 4096 + 1 ('D') + 1 ('\n') = 4098.
    // 4098 > 4096, so it truncates to 2048.
    assert(WebConsole::getLogBuffer().length() == 2048);

    std::cout << "test_exact_limit passed" << std::endl;
}

void test_empty_push() {
    WebConsole::clearLogBuffer();
    WebConsole::pushLog("");
    assert(WebConsole::getLogBuffer() == "\n");
    std::cout << "test_empty_push passed" << std::endl;
}

void test_huge_push() {
    WebConsole::clearLogBuffer();
    // What if a single push is > 4096 chars?
    std::string hugeContent(5000, 'E');
    WebConsole::pushLog(hugeContent.c_str());

    // The length will be 5001. 5001 > 4096.
    // Result length should be 2048.
    assert(WebConsole::getLogBuffer().length() == 2048);

    // Verify it contains the end of the content
    String buffer = WebConsole::getLogBuffer();
    assert(buffer.substring(buffer.length() - 5) == "EEEE\n");

    std::cout << "test_huge_push passed" << std::endl;
}

int main() {
    test_basic_push();
    test_multiple_push();
    test_buffer_limit();
    test_exact_limit();
    test_empty_push();
    test_huge_push();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
