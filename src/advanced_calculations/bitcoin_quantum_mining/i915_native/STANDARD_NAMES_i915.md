# STANDARD_NAMES_i915.md — Conventions de Nommage i915 Native

**Version** : 1.0  
**Date** : 2026-05-11  
**Module** : i915_native (Bitcoin Mining via i915 DRM natif)  
**Objectif** : 100% indépendant OpenCL/Level Zero, traçabilité bit-level totale

---

## 1. PRÉFIXES OBLIGATOIRES

### Fonctions Publiques
- `btc_gen9_*` : Fonctions spécifiques Gen9 (UHD 620)
- `btc_i915_*` : Fonctions génériques i915 DRM
- `btc_drm_*` : Fonctions bas niveau DRM

### Structures
- `btc_gen9_context` : Contexte GPU Gen9
- `btc_i915_buffer` : Buffer GEM i915
- `btc_drm_batch` : Batch buffer DRM

### Macros
- `BTC_GEN9_*` : Constantes Gen9
- `BTC_I915_*` : Constantes i915
- `BTC_DRM_*` : Constantes DRM

---

## 2. FICHIERS SOURCES

### Structure Répertoires
```
i915_native/
├── src/                    # Code source C
│   ├── btc_gen9_native_runner.c
│   ├── btc_gen9_native_compiler.h
│   ├── btc_i915_drm_wrapper.c      (à créer)
│   ├── btc_bit_level_tracker.c     (à créer)
│   └── btc_memory_tracker_gpu.c    (à créer)
├── include/                # Headers publics
│   ├── btc_gen9_native_runner.h
│   ├── btc_i915_drm_wrapper.h      (à créer)
│   └── btc_bit_level_tracker.h     (à créer)
├── tests/                  # Tests unitaires
│   ├── test_gen9_native.c
│   ├── test_btc_mining_native.c
│   └── test_bit_level_tracker.c    (à créer)
├── kernels/                # Binaires ISA Gen9
│   └── btc_sha256_mining_gen9.bin
├── logs/forensic/          # Logs forensiques
│   ├── btc_mining_native_sha256.log
│   └── *.lum                       (format bit-level)
├── CHAT/                   # Rapports conversation
│   └── RAPPORT_C198_*.md
├── bin/                    # Exécutables compilés
└── build/                  # Objets compilation
```

### Conventions Noms Fichiers
- **Source C** : `btc_<module>_<fonction>.c`
- **Header** : `btc_<module>_<fonction>.h`
- **Test** : `test_<module>_<fonction>.c`
- **Kernel** : `btc_<algo>_<arch>.bin`
- **Log** : `btc_<module>_<timestamp>.log`
- **Rapport** : `RAPPORT_C<num>_<phase>_<sujet>.md`

---

## 3. CONVENTIONS VARIABLES

### Variables Locales
```c
uint32_t ctx_id;           // ID contexte GPU
uint32_t gem_handle;       // Handle buffer GEM
uint64_t gpu_addr;         // Adresse GPU virtuelle
size_t buffer_size;        // Taille buffer
int drm_fd;                // File descriptor DRM
```

### Variables Globales (INTERDITES sauf constantes)
```c
static const uint32_t BTC_GEN9_MAX_CONTEXTS = 3;
static const size_t BTC_GEN9_BATCH_SIZE = 220;
```

### Pointeurs
```c
btc_gen9_context *ctx;     // Contexte GPU
uint8_t *batch_buffer;     // Buffer batch
void *mapped_ptr;          // Pointeur mmap
```

---

## 4. CONVENTIONS FONCTIONS

### Signatures Standard
```c
// Initialisation : retourne 0 si succès, -1 si échec
int btc_gen9_init(btc_gen9_context **ctx, int drm_fd);

// Exécution : retourne 0 si succès, errno si échec
int btc_gen9_execute(btc_gen9_context *ctx, uint32_t nonce_start);

// Nettoyage : toujours void (cleanup garanti)
void btc_gen9_cleanup(btc_gen9_context *ctx);

// Getters : retourne valeur directement
uint64_t btc_gen9_get_hashrate(btc_gen9_context *ctx);
```

### Gestion Erreurs
```c
// Toujours vérifier errno après ioctl()
if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
    int err = errno;
    LOG_ERROR(ctx, "EXECBUFFER2 failed: errno=%d (%s)", err, strerror(err));
    return -err;
}
```

---

## 5. CONVENTIONS LOGS

### Format Timestamp
```
[timestamp_sec.nanosec] LEVEL: message
[1455.360607099] INFO: EXEC_START: ctx_id=1 dispatch=1
```

### Niveaux Log
- `DEBUG` : Détails techniques (adresses, tailles)
- `INFO` : Événements normaux (init, exec, cleanup)
- `WARN` : Situations anormales non bloquantes
- `ERROR` : Échecs critiques (errno, crashes)
- `FORENSIC` : Traçabilité bit-level

### Macros Log
```c
#define LOG_DEBUG(ctx, fmt, ...)   btc_log(ctx, "DEBUG", fmt, ##__VA_ARGS__)
#define LOG_INFO(ctx, fmt, ...)    btc_log(ctx, "INFO", fmt, ##__VA_ARGS__)
#define LOG_WARN(ctx, fmt, ...)    btc_log(ctx, "WARN", fmt, ##__VA_ARGS__)
#define LOG_ERROR(ctx, fmt, ...)   btc_log(ctx, "ERROR", fmt, ##__VA_ARGS__)
#define LOG_FORENSIC(ctx, fmt, ...) btc_log(ctx, "FORENSIC", fmt, ##__VA_ARGS__)
```

---

## 6. CONVENTIONS BIT-LEVEL TRACKING

### Format Fichier .lum
```
# Header (32 bytes)
MAGIC: 0x4C554D00 (LUM\0)
VERSION: 1
TIMESTAMP: 1455360607099
MODULE: i915_native
ARCH: Gen9

# Événements (variable)
[offset] EVENT_TYPE | data_hex | metadata
[0x0000] GEM_CREATE | handle=0x00000001 size=0x00100000
[0x0020] GPU_MMAP   | addr=0x7f8a4c000000 size=0x00100000
[0x0040] BATCH_WRITE| offset=0x0000 data=0x79000002...
[0x0060] EXECBUFFER2| ctx_id=1 batch_offset=0x0000
[0x0080] GPU_WAIT   | timeout=1000000000ns result=0
```

### Événements Trackés
- `GEM_CREATE` : Création buffer GEM
- `GEM_CLOSE` : Destruction buffer GEM
- `GPU_MMAP` : Mapping mémoire GPU
- `GPU_MUNMAP` : Unmapping mémoire GPU
- `BATCH_WRITE` : Écriture batch buffer
- `EXECBUFFER2` : Soumission GPU
- `GPU_WAIT` : Attente completion
- `CTX_CREATE` : Création contexte
- `CTX_DESTROY` : Destruction contexte

---

## 7. CONVENTIONS MEMORY TRACKING

### Structure Tracking
```c
typedef struct {
    void *ptr;              // Adresse RAM/GPU
    size_t size;            // Taille allocation
    uint64_t timestamp;     // Timestamp allocation
    const char *location;   // Fichier:ligne
    uint32_t gem_handle;    // Handle GEM (si GPU)
    bool is_gpu;            // true=GPU, false=RAM
} btc_memory_entry;
```

### Macros Allocation
```c
#define BTC_MALLOC(size) \
    btc_memory_track_alloc(malloc(size), size, __FILE__, __LINE__, false)

#define BTC_GEM_CREATE(drm_fd, size) \
    btc_memory_track_gem_create(drm_fd, size, __FILE__, __LINE__)
```

---

## 8. CONVENTIONS TESTS

### Noms Tests
```c
void test_btc_gen9_init_success(void);
void test_btc_gen9_execute_single_dispatch(void);
void test_btc_gen9_execute_errno5_bug(void);
void test_btc_bit_level_tracker_create_lum(void);
```

### Assertions
```c
#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        fprintf(stderr, "FAIL: %s:%d: %s != %s\n", \
                __FILE__, __LINE__, #a, #b); \
        exit(1); \
    }

#define ASSERT_SUCCESS(ret) ASSERT_EQ(ret, 0)
#define ASSERT_ERRNO(ret, expected_errno) ASSERT_EQ(-ret, expected_errno)
```

---

## 9. CONVENTIONS MAKEFILES

### Targets Standard
```makefile
all: bin/test_btc_mining_native

clean:
	rm -rf build/ bin/

test: bin/test_btc_mining_native
	./bin/test_btc_mining_native

forensic: test
	cat logs/forensic/btc_mining_native_*.log
```

### Variables
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11
LDFLAGS = -ldrm -lpthread -lm
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin
```

---

## 10. CONVENTIONS RAPPORTS

### Structure Rapport
```markdown
# RAPPORT_C<num>_PHASE<phase>_<SUJET>.md

## 1. CONTEXTE
## 2. OBJECTIFS
## 3. MÉTHODOLOGIE
## 4. RÉSULTATS
## 5. ANALYSE FORENSIQUE
## 6. DÉCOUVERTES CRITIQUES
## 7. RECOMMANDATIONS
## 8. PROCHAINES ÉTAPES
```

### Métriques Standard
- **Temps init** : ms
- **Temps dispatch** : sec (3 décimales)
- **Hashrate** : MH/s (2 décimales)
- **Dispatches max** : nombre avant échec
- **Errno** : code + description

---

## 11. DIFFÉRENCES AVEC level_zero_native

| Aspect | level_zero_native | i915_native |
|--------|-------------------|-------------|
| **API** | Level Zero (ze_*) | i915 DRM (ioctl) |
| **Dépendances** | libze_loader.so | libdrm.so uniquement |
| **Contextes** | ze_context_handle_t | uint32_t ctx_id |
| **Buffers** | ze_device_mem_alloc | GEM objects |
| **Exécution** | zeCommandListAppendLaunchKernel | DRM_IOCTL_I915_GEM_EXECBUFFER2 |
| **Préfixes** | btc_ze_* | btc_gen9_*, btc_i915_* |
| **Répertoire** | level_zero_native/ | i915_native/ |

---

## 12. RÈGLES STRICTES

###  ❌ INTERDICTIONS ABSOLUES
1. **Aucune dépendance OpenCL** : Pas de `#include <CL/cl.h>`
2. **Aucune dépendance Level Zero** : Pas de `#include <level_zero/ze_api.h>`
3. **Pas de variables globales** : Sauf constantes `static const`
4. **Pas de malloc() direct** : Utiliser macros tracking
5. **Pas de printf() direct** : Utiliser macros LOG_*

### ✅ OBLIGATIONS ABSOLUES
1. **Vérifier errno** : Après chaque ioctl()
2. **Cleanup garanti** : Même en cas d'échec
3. **Logs forensiques** : Chaque allocation/libération
4. **Format .lum** : Traçabilité bit-level
5. **Tests unitaires** : Chaque fonction publique

---

**FIN STANDARD_NAMES_i915.md**