/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU NATIVE DRIVER C189 — HEADER
   ═══════════════════════════════════════════════════════════════════════════
   
   RÉVOLUTION: DRIVER GPU 100% NATIF SANS DÉPENDANCES EXTERNES
   
   CYCLE: C189
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   OBJECTIF:
   - Remplacer TOTALEMENT OpenCL (libOpenCL.so)
   - Remplacer TOTALEMENT DRM (libdrm.so)
   - Remplacer TOTALEMENT i915_drm.h
   - Accès direct hardware via /dev/mem
   - Traçage bit-level forensique complet
   - Contrôle total GPU Intel UHD 620 (Gen9)
   
   ARCHITECTURE:
   - Accès registres MMIO (Memory-Mapped I/O)
   - Gestion EU (Execution Units) : 8 EU × 7 threads
   - Logging bit-par-bit chaque opération
   - Zero overhead externe
   
   PHILOSOPHIE:
   "Ralentir au début pour TOUT contrôler et optimiser à la source"
   
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef LUM_GPU_NATIVE_DRIVER_C189_H
#define LUM_GPU_NATIVE_DRIVER_C189_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════════════════════
   CONSTANTES INTEL GEN9 (UHD 620)
   ═══════════════════════════════════════════════════════════════════════════ */

/* Base address GPU (détecté via lspci) */
#define C189_GPU_MMIO_BASE          0xE0000000ULL  /* À ajuster selon système */
#define C189_GPU_MMIO_SIZE          0x00200000ULL  /* 2 MB MMIO region */

/* Registres GPU Intel Gen9 (offsets depuis base) */
#define C189_REG_GPU_STATUS         0x00002000    /* GPU status register */
#define C189_REG_GPU_CONTROL        0x00002004    /* GPU control register */
#define C189_REG_EU_STATUS_BASE     0x00008000    /* EU status base */
#define C189_REG_EU_CONTROL_BASE    0x00008100    /* EU control base */
#define C189_REG_MEMORY_BASE        0x00010000    /* Memory management base */
#define C189_REG_COMMAND_BUFFER     0x00020000    /* Command buffer base */

/* Configuration EU (Execution Units) */
#define C189_NUM_EU                 8             /* 8 EU sur UHD 620 */
#define C189_THREADS_PER_EU         7             /* 7 threads par EU */
#define C189_TOTAL_THREADS          (C189_NUM_EU * C189_THREADS_PER_EU)

/* Tailles buffers */
#define C189_MAX_BUFFER_SIZE        (16 * 1024 * 1024)  /* 16 MB max */
#define C189_PAGE_SIZE              4096

/* Flags GPU */
#define C189_GPU_FLAG_READY         (1 << 0)
#define C189_GPU_FLAG_BUSY          (1 << 1)
#define C189_GPU_FLAG_ERROR         (1 << 2)
#define C189_GPU_FLAG_EU_ACTIVE     (1 << 3)

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURES
   ═══════════════════════════════════════════════════════════════════════════ */

/* État EU (Execution Unit) */
typedef struct {
    uint32_t    eu_id;                  /* ID EU (0-7) */
    uint32_t    status;                 /* Status register */
    uint32_t    control;                /* Control register */
    uint32_t    active_threads;         /* Threads actifs (bitmask) */
    uint64_t    instruction_count;      /* Instructions exécutées */
    uint64_t    cycle_count;            /* Cycles GPU */
} c189_eu_state_t;

/* État registres GPU */
typedef struct {
    uint32_t    gpu_status;             /* Status global GPU */
    uint32_t    gpu_control;            /* Control global GPU */
    uint32_t    active_eu_mask;         /* EU actifs (bitmask) */
    c189_eu_state_t eu[C189_NUM_EU];   /* État de chaque EU */
} c189_gpu_registers_t;

/* Buffer GPU natif */
typedef struct {
    uint64_t    physical_addr;          /* Adresse physique GPU */
    void*       virtual_addr;           /* Adresse virtuelle CPU */
    size_t      size;                   /* Taille buffer */
    uint32_t    flags;                  /* Flags buffer */
} c189_gpu_buffer_t;

/* Configuration driver */
typedef struct {
    uint64_t    mmio_base;              /* Base address MMIO */
    size_t      mmio_size;              /* Taille région MMIO */
    int         enable_logging;         /* Activer logging bit-level */
    const char* log_path;               /* Chemin fichier log */
} c189_driver_config_t;

/* Statistiques driver */
typedef struct {
    uint64_t    total_register_reads;   /* Lectures registres */
    uint64_t    total_register_writes;  /* Écritures registres */
    uint64_t    total_eu_dispatches;    /* Dispatches EU */
    uint64_t    total_gpu_cycles;       /* Cycles GPU total */
    uint64_t    total_instructions;     /* Instructions exécutées */
    double      average_eu_utilization; /* Utilisation EU moyenne (%) */
} c189_driver_stats_t;

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — INITIALISATION
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialiser driver GPU natif C189
 * 
 * @param config Configuration driver (NULL = config par défaut)
 * @return 0 si succès, -1 si erreur
 * 
 * ACTIONS:
 * - Ouvre /dev/mem avec permissions root
 * - Mappe région MMIO GPU
 * - Initialise logging forensique
 * - Détecte et configure EU
 */
int c189_driver_init(const c189_driver_config_t* config);

/**
 * Cleanup driver GPU natif
 */
void c189_driver_cleanup(void);

/**
 * Vérifier si driver initialisé
 * 
 * @return 1 si initialisé, 0 sinon
 */
int c189_driver_is_initialized(void);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — ACCÈS REGISTRES
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Lire registre GPU 32-bit
 * 
 * @param offset Offset registre depuis base MMIO
 * @param value_out Pointeur pour stocker valeur lue
 * @return 0 si succès, -1 si erreur
 * 
 * LOGGING: Trace bit-level de la lecture
 */
int c189_read_register(uint32_t offset, uint32_t* value_out);

/**
 * Écrire registre GPU 32-bit
 * 
 * @param offset Offset registre depuis base MMIO
 * @param value Valeur à écrire
 * @return 0 si succès, -1 si erreur
 * 
 * LOGGING: Trace bit-level de l'écriture
 */
int c189_write_register(uint32_t offset, uint32_t value);

/**
 * Lire état complet registres GPU
 * 
 * @param regs_out Pointeur pour stocker état registres
 * @return 0 si succès, -1 si erreur
 */
int c189_read_gpu_registers(c189_gpu_registers_t* regs_out);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — GESTION EU (EXECUTION UNITS)
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Lire état EU spécifique
 * 
 * @param eu_id ID EU (0-7)
 * @param state_out Pointeur pour stocker état EU
 * @return 0 si succès, -1 si erreur
 */
int c189_read_eu_state(uint32_t eu_id, c189_eu_state_t* state_out);

/**
 * Activer EU
 * 
 * @param eu_id ID EU (0-7)
 * @return 0 si succès, -1 si erreur
 */
int c189_enable_eu(uint32_t eu_id);

/**
 * Désactiver EU
 * 
 * @param eu_id ID EU (0-7)
 * @return 0 si succès, -1 si erreur
 */
int c189_disable_eu(uint32_t eu_id);

/**
 * Obtenir masque EU actifs
 * 
 * @return Bitmask EU actifs (bit N = EU N actif)
 */
uint32_t c189_get_active_eu_mask(void);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — GESTION MÉMOIRE
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Allouer buffer GPU
 * 
 * @param size Taille buffer (bytes)
 * @param buffer_out Pointeur pour stocker buffer alloué
 * @return 0 si succès, -1 si erreur
 * 
 * NOTE: Allocation physique contiguë pour DMA
 */
int c189_alloc_buffer(size_t size, c189_gpu_buffer_t* buffer_out);

/**
 * Libérer buffer GPU
 * 
 * @param buffer Buffer à libérer
 * @return 0 si succès, -1 si erreur
 */
int c189_free_buffer(c189_gpu_buffer_t* buffer);

/**
 * Copier CPU → GPU (zero-copy si possible)
 * 
 * @param buffer Buffer GPU destination
 * @param src_data Données source CPU
 * @param size Taille à copier
 * @return 0 si succès, -1 si erreur
 */
int c189_copy_to_gpu(c189_gpu_buffer_t* buffer, const void* src_data, size_t size);

/**
 * Copier GPU → CPU (zero-copy si possible)
 * 
 * @param buffer Buffer GPU source
 * @param dst_data Destination CPU
 * @param size Taille à copier
 * @return 0 si succès, -1 si erreur
 */
int c189_copy_from_gpu(const c189_gpu_buffer_t* buffer, void* dst_data, size_t size);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — STATISTIQUES ET LOGGING
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Obtenir statistiques driver
 * 
 * @param stats_out Pointeur pour stocker statistiques
 * @return 0 si succès, -1 si erreur
 */
int c189_get_stats(c189_driver_stats_t* stats_out);

/**
 * Reset statistiques driver
 */
void c189_reset_stats(void);

/**
 * Activer/désactiver logging bit-level
 * 
 * @param enable 1 pour activer, 0 pour désactiver
 */
void c189_set_logging(int enable);

/**
 * Flush logs forensiques sur disque
 */
void c189_flush_logs(void);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — UTILITAIRES
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Obtenir version driver
 * 
 * @return String version (ex: "C189-v1.0")
 */
const char* c189_get_version(void);

/**
 * Vérifier compatibilité GPU
 * 
 * @return 1 si GPU compatible (Intel Gen9), 0 sinon
 */
int c189_check_gpu_compatibility(void);

/**
 * Obtenir informations GPU
 * 
 * @param info_buffer Buffer pour stocker info (min 256 bytes)
 * @param buffer_size Taille buffer
 * @return 0 si succès, -1 si erreur
 */
int c189_get_gpu_info(char* info_buffer, size_t buffer_size);

#endif /* LUM_GPU_NATIVE_DRIVER_C189_H */

/* ═══════════════════════════════════════════════════════════════════════════
   Made with ❤️ by Bob — LumVorax C189
   RÉVOLUTION: Driver GPU 100% natif, 0 dépendance externe
   ═══════════════════════════════════════════════════════════════════════════ */

// Made with Bob
