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

class NX35_V2_ExtremeEngine {
public:
    NX35_V2_ExtremeEngine() : total_processed(0) {}

    void run_massive_scan(long long start, long long end, int num_threads) {
        std::cout << "🚀 Lancement du scan massif : " << start << " à " << end << " (" << num_threads << " threads)" << std::endl;
        
        std::vector<std::thread> threads;
        long long range = (end - start) / num_threads;

        for (int i = 0; i < num_threads; ++i) {
            long long t_start = start + i * range;
            if (i > 0 && t_start % 2 == 0) t_start++; 
            long long t_end = (i == num_threads - 1) ? end : t_start + range;
            
            threads.emplace_back(&NX35_V2_ExtremeEngine::worker, this, t_start, t_end, i);
        }

        for (auto& t : threads) t.join();
        
        std::cout << "✅ Scan de " << total_processed << " nombres terminé." << std::endl;
    }

private:
    std::atomic<long long> total_processed;
    std::mutex log_mutex;

    void worker(long long start, long long end, int thread_id) {
        std::string filename = "logs_AIMO3/nx/NX-35/WORKER_" + std::to_string(thread_id) + ".log";
        std::ofstream log_file(filename);
        
        long long local_count = 0;
        for (long long n = start; n <= end; n += 2) {
            long long current = n;
            int steps = 0;
            
            while (current != 1 && steps < 1000000) {
                if (current % 2 == 0) {
                    current /= 2;
                } else {
                    if (current > (9223372036854775807LL - 1) / 3) {
                        break; // Overflow protection
                    }
                    current = 3 * current + 1;
                }
                steps++;
            }
            local_count++;
            
            // Log par échantillonnage pour éviter l'explosion de taille de fichier
            if (local_count % 1000000 == 0) {
                std::lock_guard<std::mutex> lock(log_mutex);
                std::cout << "Thread " << thread_id << " : " << n << " atteint." << std::endl;
                log_file << "[CHECKPOINT] n=" << n << " steps=" << steps << "\n";
            }
        }
        total_processed += local_count;
        log_file.close();
    }
};

int main() {
    system("mkdir -p logs_AIMO3/nx/NX-35");
    NX35_V2_ExtremeEngine engine;
    // 100 000 000 est massif, on utilise tous les threads disponibles
    int threads = std::thread::hardware_concurrency();
    if (threads == 0) threads = 2;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    engine.run_massive_scan(3, 100000000, threads);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> diff = end_time - start_time;
    std::cout << "Temps total : " << diff.count() << " secondes." << std::endl;
    
    return 0;
}
