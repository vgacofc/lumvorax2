// GPU Cache Warmup - Optimisation C255v8n
// Objectif: Préchauffer caches L1/L2 GPU avant exécution

#include <stdint.h>
#include <string.h>

// Préchauffage cache avec pattern d'accès SHA-256
void gpu_cache_warmup_sha256(void* buffer, size_t size) {
    if (!buffer || size == 0) return;
    
    // Pattern 1: Accès séquentiel (cache L1)
    volatile uint32_t* ptr = (uint32_t*)buffer;
    size_t count = size / sizeof(uint32_t);
    uint32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += ptr[i];
    }
    
    // Pattern 2: Accès strided (cache L2)
    for (size_t i = 0; i < count; i += 16) {
        sum += ptr[i];
    }
    
    // Pattern 3: Écriture pour marquer dirty
    for (size_t i = 0; i < count; i += 64) {
        ptr[i] = sum;
    }
    
    // Forcer flush vers GPU
    __sync_synchronize();
}

// Préchauffage adresses batch buffers
void gpu_cache_warmup_batch_buffers(void** buffers, size_t count, size_t buffer_size) {
    if (!buffers || count == 0) return;
    
    for (size_t i = 0; i < count; i++) {
        if (buffers[i]) {
            gpu_cache_warmup_sha256(buffers[i], buffer_size);
        }
    }
}

// Préchauffage contextes GPU
void gpu_cache_warmup_contexts(void** contexts, size_t count) {
    if (!contexts || count == 0) return;
    
    // Toucher chaque contexte pour charger en cache
    for (size_t i = 0; i < count; i++) {
        if (contexts[i]) {
            volatile uint64_t* ctx = (uint64_t*)contexts[i];
            uint64_t dummy = ctx[0];
            (void)dummy;
        }
    }
}
