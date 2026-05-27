#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <iomanip>

int main() {
    auto now_time = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now_time);
    
    std::string filename = "logs_AIMO3/nx/NX-32/FORENSIC_STREAM_" + std::to_string(timestamp) + ".log";
    std::ofstream log(filename, std::ios::binary);
    
    if (!log.is_open()) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier de log." << std::endl;
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<500000; ++i) {
        auto now = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        uint64_t state = 0x5555555555555555 ^ ns;
        log << ns << " | " << state << " | 0x" << std::hex << (state ^ 0xAAAAAAAAAAAAAAAAL) << "\n";
    }
    log.close();
    
    // Mettre à jour le pointeur Merkle sans écraser le fichier de log NX-31
    std::ofstream chain("logs_AIMO3/nx/NX-31/MERKLE_CHAIN_POINTER.txt", std::ios::app);
    chain << "SESSION_NX32_BIT_BY_BIT: " << timestamp << " | FILE: " << filename << "\n";
    chain.close();

    std::cout << "Log bit-à-bit généré : " << filename << std::endl;
    return 0;
}
