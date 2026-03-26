#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>

int main() {
    std::ofstream log("logs_AIMO3/nx/NX-32/FORENSIC_NX32_BIT_BY_BIT_NANOSECOND.log", std::ios::binary);
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<500000; ++i) {
        auto now = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        uint64_t state = 0x5555555555555555 ^ ns;
        log << ns << " | " << state << " | 0x" << std::hex << (state ^ 0xAAAAAAAAAAAAAAAAL) << "\n";
    }
    log.close();
    return 0;
}
