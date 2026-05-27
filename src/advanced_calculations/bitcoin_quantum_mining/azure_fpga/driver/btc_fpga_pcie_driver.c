/**
 * @file btc_fpga_pcie_driver.c
 * @brief Driver PCIe pour FPGA Xilinx U250 Bitcoin Mining avec Traçabilité LumVorax
 * 
 * PROJET C - PHASE 15Y-F28 - PRIORITÉ ABSOLUE
 * 
 * Architecture:
 * - Communication PCIe avec FPGA
 * - DMA bidirectionnel (host ↔ FPGA)
 * - Traçabilité bit-level complète
 * - Memory tracker RAM+FPGA
 * - Format .lum forensic unifié
 * 
 * Compatibilité:
 * - Xilinx U250 (Alveo)
 * - XRT (Xilinx Runtime)
 * - Linux kernel 5.15+
 * 
 * @author LumVorax Team
 * @date 2026-05-11
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>

// Headers LumVorax (chemins relatifs depuis azure_fpga/)
#include "../../unified/include/btc_forensic_logger.h"
#include "../../i915_native/include/btc_memory_tracker_gpu.h"

/* ============================================================================
 * SECTION 1: STRUCTURES ET CONSTANTES
 * ============================================================================ */

#define FPGA_VENDOR_ID      0x10EE  // Xilinx
#define FPGA_DEVICE_ID      0x5000  // U250
#define FPGA_BAR0_SIZE      (64 * 1024 * 1024)  // 64 MB
#define FPGA_DMA_BUFFER_SIZE (16 * 1024 * 1024) // 16 MB

// Registres FPGA (offsets dans BAR0)
#define REG_CONTROL         0x0000  // Contrôle (start, reset)
#define REG_STATUS          0x0004  // Status (busy, done)
#define REG_BLOCK_HEADER    0x0010  // Header Bitcoin (256 bits)
#define REG_NONCE_START     0x0030  // Nonce début
#define REG_NONCE_COUNT     0x0034  // Nombre nonces
#define REG_TARGET_ZEROS    0x0038  // Zéros requis
#define REG_NONCE_FOUND     0x0040  // Nonce trouvé
#define REG_HASH_FOUND      0x0044  // Hash trouvé (256 bits)
#define REG_HASHES_COMPUTED 0x0064  // Nombre hashes
#define REG_FORENSIC_TS     0x0070  // Timestamp forensic
#define REG_FORENSIC_EVENT  0x0078  // Événement forensic
#define REG_MEM_ALLOC       0x0080  // Allocations mémoire
#define REG_MEM_FREE        0x0088  // Libérations mémoire
#define REG_MEM_CURRENT     0x0090  // Mémoire actuelle
#define REG_MEM_PEAK        0x0098  // Pic mémoire

// Bits contrôle
#define CTRL_START          (1 << 0)
#define CTRL_RESET          (1 << 1)

// Bits status
#define STATUS_BUSY         (1 << 0)
#define STATUS_DONE         (1 << 1)
#define STATUS_VALID_FOUND  (1 << 2)

/**
 * @brief Contexte driver FPGA
 */
typedef struct {
    // PCIe
    int pcie_fd;                    // File descriptor PCIe
    void* bar0_mapped;              // BAR0 mappé en mémoire
    size_t bar0_size;               // Taille BAR0
    
    // DMA
    void* dma_buffer;               // Buffer DMA
    size_t dma_size;                // Taille buffer DMA
    uint64_t dma_phys_addr;         // Adresse physique DMA
    
    // Configuration
    uint8_t block_header[32];       // Header Bitcoin
    uint32_t nonce_start;           // Nonce début
    uint32_t nonce_count;           // Nombre nonces
    uint8_t target_zeros;           // Zéros requis
    
    // Résultats
    uint32_t nonce_found;           // Nonce trouvé
    uint8_t hash_found[32];         // Hash trouvé
    bool valid_found;               // Hash valide trouvé
    uint64_t hashes_computed;       // Nombre hashes
    
    // Traçabilité LumVorax
    btc_mem_tracker_t* mem_tracker; // Memory tracker
    bool forensic_enabled;          // Forensic activé
    FILE* forensic_log;             // Fichier log forensic
    
    // Thread monitoring
    pthread_t monitor_thread;       // Thread monitoring FPGA
    bool monitor_running;           // Thread actif
    pthread_mutex_t mutex;          // Mutex protection
} btc_fpga_ctx_t;

/* ============================================================================
 * SECTION 2: ACCÈS REGISTRES FPGA
 * ============================================================================ */

/**
 * @brief Lire registre 32 bits
 */
static inline uint32_t fpga_read32(btc_fpga_ctx_t* ctx, uint32_t offset) {
    volatile uint32_t* reg = (volatile uint32_t*)((uint8_t*)ctx->bar0_mapped + offset);
    uint32_t value = *reg;
    
    // Log forensic
    if (ctx->forensic_enabled) {
        btc_forensic_log(BTC_FORENSIC_EVENT_CUSTOM, BTC_FORENSIC_LEVEL_DEBUG,
                        "FPGA_READ32", offset, value);
    }
    
    return value;
}

/**
 * @brief Écrire registre 32 bits
 */
static inline void fpga_write32(btc_fpga_ctx_t* ctx, uint32_t offset, uint32_t value) {
    volatile uint32_t* reg = (volatile uint32_t*)((uint8_t*)ctx->bar0_mapped + offset);
    *reg = value;
    
    // Log forensic
    if (ctx->forensic_enabled) {
        btc_forensic_log(BTC_FORENSIC_EVENT_CUSTOM, BTC_FORENSIC_LEVEL_DEBUG,
                        "FPGA_WRITE32", offset, value);
    }
}

/**
 * @brief Lire registre 64 bits
 */
static inline uint64_t fpga_read64(btc_fpga_ctx_t* ctx, uint32_t offset) {
    uint32_t low = fpga_read32(ctx, offset);
    uint32_t high = fpga_read32(ctx, offset + 4);
    return ((uint64_t)high << 32) | low;
}

/**
 * @brief Écrire registre 64 bits
 */
static inline void fpga_write64(btc_fpga_ctx_t* ctx, uint32_t offset, uint64_t value) {
    fpga_write32(ctx, offset, (uint32_t)(value & 0xFFFFFFFF));
    fpga_write32(ctx, offset + 4, (uint32_t)(value >> 32));
}

/* ============================================================================
 * SECTION 3: INITIALISATION ET CLEANUP
 * ============================================================================ */

/**
 * @brief Initialiser contexte FPGA
 */
btc_fpga_ctx_t* btc_fpga_init(const char* pcie_device, bool enable_forensic) {
    btc_fpga_ctx_t* ctx = calloc(1, sizeof(btc_fpga_ctx_t));
    if (!ctx) {
        fprintf(stderr, "ERROR: Failed to allocate FPGA context\n");
        return NULL;
    }
    
    // Initialiser forensic
    ctx->forensic_enabled = enable_forensic;
    if (enable_forensic) {
        btc_forensic_init("logs/forensic/fpga_pcie.lum", 
                         BTC_FORENSIC_BUFFER_DEFAULT, true, 3);
        btc_forensic_log(BTC_FORENSIC_EVENT_INIT, BTC_FORENSIC_LEVEL_INFO,
                        "FPGA_INIT_START", 0, 0);
    }
    
    // Initialiser memory tracker
    ctx->mem_tracker = btc_mem_tracker_create(10000);
    if (!ctx->mem_tracker) {
        fprintf(stderr, "ERROR: Failed to create memory tracker\n");
        free(ctx);
        return NULL;
    }
    
    // Ouvrir device PCIe
    ctx->pcie_fd = open(pcie_device, O_RDWR | O_SYNC);
    if (ctx->pcie_fd < 0) {
        fprintf(stderr, "ERROR: Failed to open PCIe device %s\n", pcie_device);
        btc_mem_tracker_destroy(ctx->mem_tracker);
        free(ctx);
        return NULL;
    }
    
    // Mapper BAR0
    ctx->bar0_size = FPGA_BAR0_SIZE;
    ctx->bar0_mapped = mmap(NULL, ctx->bar0_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, ctx->pcie_fd, 0);
    if (ctx->bar0_mapped == MAP_FAILED) {
        fprintf(stderr, "ERROR: Failed to mmap BAR0\n");
        close(ctx->pcie_fd);
        btc_mem_tracker_destroy(ctx->mem_tracker);
        free(ctx);
        return NULL;
    }
    
    // Tracker allocation BAR0
    btc_mem_track_mmap_gpu(ctx->mem_tracker, ctx->bar0_mapped, 0, 
                          ctx->bar0_size, __FILE__, __LINE__);
    
    // Allouer buffer DMA
    ctx->dma_size = FPGA_DMA_BUFFER_SIZE;
    ctx->dma_buffer = mmap(NULL, ctx->dma_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ctx->dma_buffer == MAP_FAILED) {
        fprintf(stderr, "ERROR: Failed to allocate DMA buffer\n");
        munmap(ctx->bar0_mapped, ctx->bar0_size);
        close(ctx->pcie_fd);
        btc_mem_tracker_destroy(ctx->mem_tracker);
        free(ctx);
        return NULL;
    }
    
    // Tracker allocation DMA
    btc_mem_track_alloc_gpu(ctx->mem_tracker, 1, ctx->dma_size, 
                           __FILE__, __LINE__);
    
    // Initialiser mutex
    pthread_mutex_init(&ctx->mutex, NULL);
    
    // Reset FPGA
    fpga_write32(ctx, REG_CONTROL, CTRL_RESET);
    usleep(1000);  // 1ms
    fpga_write32(ctx, REG_CONTROL, 0);
    
    if (enable_forensic) {
        btc_forensic_log(BTC_FORENSIC_EVENT_INIT, BTC_FORENSIC_LEVEL_INFO,
                        "FPGA_INIT_COMPLETE", 0, 0);
    }
    
    printf("✓ FPGA initialized: BAR0=%p DMA=%p\n", 
           ctx->bar0_mapped, ctx->dma_buffer);
    
    return ctx;
}

/**
 * @brief Détruire contexte FPGA
 */
void btc_fpga_cleanup(btc_fpga_ctx_t* ctx) {
    if (!ctx) return;
    
    if (ctx->forensic_enabled) {
        btc_forensic_log(BTC_FORENSIC_EVENT_CLEANUP, BTC_FORENSIC_LEVEL_INFO,
                        "FPGA_CLEANUP_START", 0, 0);
    }
    
    // Arrêter thread monitoring
    if (ctx->monitor_running) {
        ctx->monitor_running = false;
        pthread_join(ctx->monitor_thread, NULL);
    }
    
    // Tracker libérations
    if (ctx->mem_tracker) {
        btc_mem_track_munmap_gpu(ctx->mem_tracker, ctx->bar0_mapped, 
                                __FILE__, __LINE__);
        btc_mem_track_free_gpu(ctx->mem_tracker, 1, __FILE__, __LINE__);
        
        // Afficher stats
        btc_mem_print_stats(ctx->mem_tracker);
        btc_mem_detect_leaks(ctx->mem_tracker);
        btc_mem_print_leaks(ctx->mem_tracker, 10);
        
        // Export .lum
        btc_mem_export_lum(ctx->mem_tracker, "logs/forensic/fpga_memory.lum");
        
        btc_mem_tracker_destroy(ctx->mem_tracker);
    }
    
    // Libérer ressources
    if (ctx->dma_buffer != MAP_FAILED) {
        munmap(ctx->dma_buffer, ctx->dma_size);
    }
    
    if (ctx->bar0_mapped != MAP_FAILED) {
        munmap(ctx->bar0_mapped, ctx->bar0_size);
    }
    
    if (ctx->pcie_fd >= 0) {
        close(ctx->pcie_fd);
    }
    
    pthread_mutex_destroy(&ctx->mutex);
    
    if (ctx->forensic_enabled) {
        btc_forensic_log(BTC_FORENSIC_EVENT_CLEANUP, BTC_FORENSIC_LEVEL_INFO,
                        "FPGA_CLEANUP_COMPLETE", 0, 0);
        btc_forensic_shutdown();
    }
    
    free(ctx);
    
    printf("✓ FPGA cleanup complete\n");
}

/* ============================================================================
 * SECTION 4: MINING OPERATIONS
 * ============================================================================ */

/**
 * @brief Démarrer mining sur FPGA
 */
int btc_fpga_start_mining(btc_fpga_ctx_t* ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_lock(&ctx->mutex);
    
    if (ctx->forensic_enabled) {
        btc_forensic_log(BTC_FORENSIC_EVENT_BATCH_START, BTC_FORENSIC_LEVEL_INFO,
                        "FPGA_MINING_START", ctx->nonce_start, ctx->nonce_count);
    }
    
    // Écrire configuration
    for (int i = 0; i < 32; i += 4) {
        uint32_t word = *(uint32_t*)(&ctx->block_header[i]);
        fpga_write32(ctx, REG_BLOCK_HEADER + i, word);
    }
    
    fpga_write32(ctx, REG_NONCE_START, ctx->nonce_start);
    fpga_write32(ctx, REG_NONCE_COUNT, ctx->nonce_count);
    fpga_write32(ctx, REG_TARGET_ZEROS, ctx->target_zeros);
    
    // Démarrer mining
    fpga_write32(ctx, REG_CONTROL, CTRL_START);
    
    pthread_mutex_unlock(&ctx->mutex);
    
    printf("✓ FPGA mining started: nonce=%u count=%u zeros=%u\n",
           ctx->nonce_start, ctx->nonce_count, ctx->target_zeros);
    
    return 0;
}

/**
 * @brief Attendre fin mining
 */
int btc_fpga_wait_done(btc_fpga_ctx_t* ctx, uint32_t timeout_ms) {
    if (!ctx) return -1;
    
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        uint32_t status = fpga_read32(ctx, REG_STATUS);
        
        if (status & STATUS_DONE) {
            // Lire résultats
            pthread_mutex_lock(&ctx->mutex);
            
            ctx->nonce_found = fpga_read32(ctx, REG_NONCE_FOUND);
            ctx->valid_found = (status & STATUS_VALID_FOUND) != 0;
            ctx->hashes_computed = fpga_read64(ctx, REG_HASHES_COMPUTED);
            
            for (int i = 0; i < 32; i += 4) {
                uint32_t word = fpga_read32(ctx, REG_HASH_FOUND + i);
                *(uint32_t*)(&ctx->hash_found[i]) = word;
            }
            
            pthread_mutex_unlock(&ctx->mutex);
            
            if (ctx->forensic_enabled) {
                btc_forensic_log(BTC_FORENSIC_EVENT_BATCH_END, BTC_FORENSIC_LEVEL_INFO,
                                "FPGA_MINING_DONE", ctx->hashes_computed, ctx->valid_found);
                
                if (ctx->valid_found) {
                    btc_forensic_log(BTC_FORENSIC_EVENT_HASH_FOUND, BTC_FORENSIC_LEVEL_INFO,
                                    "FPGA_HASH_FOUND", ctx->nonce_found, 0);
                }
            }
            
            printf("✓ FPGA mining done: hashes=%lu valid=%d nonce=%u\n",
                   ctx->hashes_computed, ctx->valid_found, ctx->nonce_found);
            
            return 0;
        }
        
        usleep(1000);  // 1ms
        elapsed++;
    }
    
    fprintf(stderr, "ERROR: FPGA mining timeout after %u ms\n", timeout_ms);
    return -1;
}

/* ============================================================================
 * SECTION 5: MONITORING FORENSIC
 * ============================================================================ */

/**
 * @brief Thread monitoring événements forensiques FPGA
 */
static void* fpga_monitor_thread(void* arg) {
    btc_fpga_ctx_t* ctx = (btc_fpga_ctx_t*)arg;
    
    while (ctx->monitor_running) {
        // Lire événements forensiques FPGA
        uint64_t timestamp = fpga_read64(ctx, REG_FORENSIC_TS);
        uint32_t event_data = fpga_read32(ctx, REG_FORENSIC_EVENT);
        
        uint32_t event_id = event_data & 0xFFFFFF;
        uint8_t event_type = (event_data >> 24) & 0xFF;
        
        if (event_id > 0 && ctx->forensic_enabled) {
            // Logger événement
            btc_forensic_log((btc_forensic_event_type_t)event_type,
                           BTC_FORENSIC_LEVEL_INFO,
                           "FPGA_EVENT", event_id, timestamp);
        }
        
        // Lire stats mémoire FPGA
        uint64_t mem_alloc = fpga_read64(ctx, REG_MEM_ALLOC);
        uint64_t mem_free = fpga_read64(ctx, REG_MEM_FREE);
        uint64_t mem_current = fpga_read64(ctx, REG_MEM_CURRENT);
        uint64_t mem_peak = fpga_read64(ctx, REG_MEM_PEAK);
        
        // TODO: Synchroniser avec memory tracker local
        
        usleep(10000);  // 10ms
    }
    
    return NULL;
}

/**
 * @brief Démarrer monitoring forensique
 */
int btc_fpga_start_monitoring(btc_fpga_ctx_t* ctx) {
    if (!ctx || ctx->monitor_running) return -1;
    
    ctx->monitor_running = true;
    
    if (pthread_create(&ctx->monitor_thread, NULL, fpga_monitor_thread, ctx) != 0) {
        fprintf(stderr, "ERROR: Failed to create monitor thread\n");
        ctx->monitor_running = false;
        return -1;
    }
    
    printf("✓ FPGA monitoring started\n");
    return 0;
}

// Made with Bob - LumVorax FPGA PCIe Driver