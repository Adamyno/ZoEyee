#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <cstring>

// Mocking Arduino String-like behavior with std::string
// In Arduino, String::substring returns a NEW String object (heap allocation)
// String::c_str() returns a pointer to the internal buffer.

long original_parseUDSBits(const std::string &fullHex, int startByte, int endByte) {
    uint64_t val = 0;
    for (int i = startByte; i <= endByte; i++) {
        std::string byteStr = fullHex.substr(i * 2, 2);
        val = (val << 8) | strtol(byteStr.c_str(), NULL, 16);
    }
    return val;
}

long optimized_parseUDSBits(const std::string &fullHex, int startByte, int endByte) {
    uint64_t val = 0;
    char buf[3] = {0};
    const char* s = fullHex.c_str();
    for (int i = startByte; i <= endByte; i++) {
        buf[0] = s[i * 2];
        buf[1] = s[i * 2 + 1];
        val = (val << 8) | strtol(buf, NULL, 16);
    }
    return val;
}

int main() {
    std::string testHex = "614300000000000000000000000000000000000000000000000000000000";
    int startByte = 2;
    int endByte = 15;
    int iterations = 1000000;

    // Verify correctness
    long res1 = original_parseUDSBits(testHex, startByte, endByte);
    long res2 = optimized_parseUDSBits(testHex, startByte, endByte);
    if (res1 != res2) {
        std::cerr << "Validation failed: " << res1 << " != " << res2 << std::endl;
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        original_parseUDSBits(testHex, startByte, endByte);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Original: " << elapsed.count() << "s" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        optimized_parseUDSBits(testHex, startByte, endByte);
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "Optimized: " << elapsed.count() << "s" << std::endl;

    return 0;
}
