#include "src/btc_gen9_native_runner.c"

int main() {
    btc_gen9_config_t config = {
        .batch_size = 32768,  // Solution #1: 32K nonces
        .work_group_size = 256,
        .kernel_path = "kernels/btc_sha256_mining_gen9.bin",
        .log_path = "logs/c278_tests/solution1_batch32k.log",
        .enable_profiling = true,
        .preempt_timeout_ms = 0,  // Pas de modification timeout
        .target_hashrate = 0,
        .auto_batch_size = false
    };
    
    btc_gen9_context_t* ctx;
    if (btc_gen9_init(&ctx, &config) < 0) {
        fprintf(stderr, "❌ Init failed\n");
        return 1;
    }
    
    printf("✅ Init OK - Testing 100 dispatches...\n");
    
    for (int i = 0; i < 100; i++) {
        if (btc_gen9_dispatch(ctx) < 0) {
            fprintf(stderr, "❌ Dispatch %d failed\n", i+1);
            btc_gen9_cleanup(ctx);
            return 1;
        }
        if ((i+1) % 10 == 0) {
            printf("  ✓ %d/100 dispatches OK\n", i+1);
        }
    }
    
    printf("✅ 100 dispatches réussis!\n");
    printf("📊 Hashrate: %.2f MH/s\n", 
           (ctx->total_hashes / 1000000.0) / ctx->total_time_sec);
    
    btc_gen9_cleanup(ctx);
    return 0;
}
