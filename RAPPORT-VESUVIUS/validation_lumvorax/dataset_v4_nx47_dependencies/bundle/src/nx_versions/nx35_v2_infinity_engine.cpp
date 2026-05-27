#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>

class NX35_V2_InfinityEngine {
public:
    NX35_V2_InfinityEngine() : total_processed(0) {}

    void run_infinite_scan(long long start, int num_threads) {
        std::cout << "🔥 MODE INFINI ACTIVÉ : Poussée hardware à 99%" << std::endl;
        std::cout << "🚀 Threads actifs : " << num_threads << std::endl;
        
        std::vector<std::thread> threads;
        running = true;

        for (int i = 0; i < num_threads; ++i) {
            long long t_start = start + (i * 2000000); 
            threads.emplace_back(&NX35_V2_InfinityEngine::worker, this, t_start, i);
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            
            if (elapsed.count() > 15) { // 15s pour cette exécution
                running = false;
                break;
            }
        }

        for (auto& t : threads) t.join();
        std::cout << "🏁 Session de poussée terminée. Total traité : " << total_processed << std::endl;
    }

private:
    std::atomic<long long> total_processed;
    std::atomic<bool> running;
    std::mutex log_mutex;

    void worker(long long start, int thread_id) {
        std::string filename = "logs_AIMO3/nx/NX-35/INFINITY_WORKER_" + std::to_string(thread_id) + ".log";
        std::ofstream log_file(filename);
        
        long long n = start;
        if (n % 2 == 0) n++;

        while (running) {
            long long current = n;
            int steps = 0;
            
            while (current != 1 && steps < 1000000) {
                if (current % 2 == 0) {
                    current /= 2;
                } else {
                    if (current > (9223372036854775807LL - 1) / 3) break;
                    current = 3 * current + 1;
                }
                steps++;
            }
            
            total_processed++;
            if (total_processed % 5000000 == 0) {
                std::lock_guard<std::mutex> lock(log_mutex);
                std::cout << "[LOG] Global Total " << total_processed << " | Thread " << thread_id << " n=" << n << std::endl;
            }
            
            n += 2; 
        }
        log_file << "Dernier n traité par thread " << thread_id << " : " << n << std::endl;
        log_file.close();
    }
};

int main() {
    system("mkdir -p logs_AIMO3/nx/NX-35");
    NX35_V2_InfinityEngine engine;
    int threads = std::thread::hardware_concurrency();
    if (threads == 0) threads = 4;
    
    engine.run_infinite_scan(100000001, threads);
    return 0;
}
