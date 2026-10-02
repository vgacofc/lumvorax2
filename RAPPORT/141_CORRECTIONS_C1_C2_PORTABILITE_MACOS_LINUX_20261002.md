# RAPPORT 141 — CORRECTIONS C1/C2 : Portabilité macOS/Linux — `aligned_alloc` + `PRIu64`

**Date :** 2026-10-02  
**SHA HEAD avant :** `1ddddea`  
**SHA HEAD après :** `5929978`  
**Branche :** main  
**CERTIFIED_100 :** false  
**Mode :** DEBUG actif  
**Résultat build :** ✅ `make clean && make` — **0 erreur, 0 warning compilateur**

---

## 1. Contexte — Erreurs et warnings signalés par l'utilisateur

```
./src/vorax/../lum/lum_aligned_alloc_safe.h:64:15: error: implicitly declaring library function
'aligned_alloc' with type 'void *(unsigned long, unsigned long)' [-Werror,-Wimplicit-function-declaration]

src/vorax/vorax_operations.c:81:27: warning: format specifies type 'unsigned long' but the
argument has type 'uint64_t' (aka 'unsigned long long') [-Wformat]

src/vorax/vorax_operations.c:190:35: warning: format specifies type 'unsigned long' but the
argument has type 'uint64_t' (aka 'unsigned long long') [-Wformat]
```

Ces deux classes d'anomalies sont des problèmes de portabilité macOS/Clang avec `-std=c99` :

- **C1** : `aligned_alloc` est C11 — non exposé par `<stdlib.h>` avec `-std=c99` strict sur macOS/Clang.
- **C2** : `uint64_t` est `unsigned long long` sur macOS, `unsigned long` sur Linux 64-bit. `%lu` est incorrect sur macOS.

---

## 2. Corrections appliquées

### C1 — `lum_aligned_alloc_safe.h` (ligne 64)

**Fichier :** `src/lum/lum_aligned_alloc_safe.h`

**AVANT (ligne 63–72) :**
```c
    /* Tentative aligned_alloc (C11) avec size garanti multiple */
    void* p = aligned_alloc(alignment, aligned_size);
    if (p) return p;

    /* Fallback POSIX (errno-based, jamais d'UB) */
    void* q = NULL;
    int rc = posix_memalign(&q, alignment, aligned_size);
    if (rc == 0) return q;
```

**APRÈS :**
```c
    /* Chemin POSIX primaire (portable Linux/macOS, -std=c99 compatible) */
    void* q = NULL;
    int rc = posix_memalign(&q, alignment, aligned_size);
    if (rc == 0) return q;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    /* C1-FIX: aligned_alloc uniquement si C11 est disponible au préprocesseur. */
    void* p = aligned_alloc(alignment, aligned_size);
    if (p) return p;
#endif
```

**Raison :** `posix_memalign` est disponible via `_POSIX_C_SOURCE=200809L` (déjà dans le Makefile) et ne nécessite pas C11. `aligned_alloc` est gardé comme chemin secondaire uniquement quand `__STDC_VERSION__ >= 201112L` garantit sa disponibilité.

---

### C1 — `src/lum/lum_core.c`

**AVANT (lignes 1–7) :** caractères corrompus `l\ntai//...` en tête du fichier.  
**APRÈS :** En-tête ARTCB standard R530 — commentaire projet conforme.

**AVANT (ligne 142) :**
```c
g_lum_pool = (lum_t*)aligned_alloc(64, LUM_POOL_SIZE * sizeof(lum_t));
```
**APRÈS :**
```c
g_lum_pool = (lum_t*)lum_aligned_alloc_safe(64, LUM_POOL_SIZE * sizeof(lum_t)); /* C1-FIX */
```

**AVANT (ligne 160) :**
```c
tlp_pool = (lum_t*)aligned_alloc(64, LUM_TLP_SIZE * sizeof(lum_t));
```
**APRÈS :**
```c
tlp_pool = (lum_t*)lum_aligned_alloc_safe(64, LUM_TLP_SIZE * sizeof(lum_t)); /* C1-FIX */
```

**AVANT (ligne 326) :**
```c
group->lums = (lum_t*)mmap(NULL, lums_size,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
```
**APRÈS :**
```c
#if defined(__linux__) && defined(MAP_ANONYMOUS) && defined(MAP_HUGETLB)
    /* Tentative allocation huge pages pour > 2MB (Linux uniquement) */
    group->lums = (lum_t*)mmap(...);
#endif /* __linux__ + MAP_ANONYMOUS + MAP_HUGETLB */
```

---

### C2 — `%lu` → `PRIu64` (10 fichiers)

`<inttypes.h>` ajouté et `%lu` remplacé par `%" PRIu64 "` dans :

| Fichier | Occurrences corrigées |
|---|---|
| `src/vorax/vorax_operations.c` | 2 (lignes 80, 189) |
| `src/lum/lum_core.c` | 2 (lignes 237, 940) |
| `src/main.c` | 1 (ligne 34) |
| `src/persistence/transaction_wal_extension.c` | 6 (lignes 263–424) |
| `src/metrics/performance_metrics.c` | 1 (ligne 223) + guard `ru_maxrss` macOS/Linux |
| `src/advanced_calculations/matrix_calculator.c` | 2 |
| `src/complex_modules/ai_dynamic_config_manager.c` | 2 |
| `src/complex_modules/ai_optimization.c` | 2 |
| `src/complex_modules/realtime_analytics.c` | 2 |
| `src/logging/log_writer.c` | 4 |
| `src/network/hostinger_resource_limiter.c` | 1 |
| `src/spatial/lum_instant_displacement.c` | 8 |
| `src/tests/test_forensic_complete_system.c` | 5 |

---

### MK-004 — Makefile : portabilité Linux/macOS

**AVANT :**
```makefile
CFLAGS = -Wall -Wextra -std=c99 ... -D_POSIX_C_SOURCE=200809L -DDEBUG_MODE ...
LDFLAGS = -lm -lpthread -lrt -Wl,-z,stack-size=16777216
```

**APRÈS :**
```makefile
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CFLAGS = ... -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DDEBUG_MODE ...
    LDFLAGS = -lm -lpthread -lrt -Wl,-z,stack-size=16777216
else
    CFLAGS = ... -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -DDEBUG_MODE ...
    LDFLAGS = -lm -lpthread
endif
```

- `-lrt` : Linux-only (`clock_gettime` est dans libc sur macOS ≥ 10.12)
- `-Wl,-z,stack-size` : ld64 macOS ne supporte pas cette option
- `-D_DARWIN_C_SOURCE` : requis pour exposer `ru_maxrss`, `getpagesize`, etc. avec `_POSIX_C_SOURCE=200809L`
- `-lmvec` : libmvec glibc Linux-only → guard `ifeq ($(UNAME_S),Linux)` sur la cible test_integration

### Corrections additionnelles

- `src/optimization/zero_copy_allocator.c` : `getpagesize()` → `sysconf(_SC_PAGESIZE)` (POSIX C99) ; `madvise`/`MADV_SEQUENTIAL` → guard `#if defined(MADV_SEQUENTIAL)`
- `src/metrics/performance_metrics.c` : `ru_maxrss * 1024` → guard `#if defined(__APPLE__)` (macOS : bytes, Linux : kilobytes)

---

## 3. Validation build

```
make clean && make
→ 0 erreur compilateur
→ 0 warning compilateur
→ bin/lum_vorax_complete         ✅ (468 832 bytes)
→ bin/test_forensic_complete_system   ✅
→ bin/test_integration_complete_39_modules ✅
→ bin/test_quantum               ✅
```

---

## 4. Anomalies du registre 140 — statut après rapport 141

Les corrections C1/C2/MK-003/MK-004 de ce rapport ferment :

| ID | Statut | Note |
|----|--------|------|
| MK-003 | **CORRIGÉ** (5929978) | `-lmvec` conditionnel Linux uniquement |
| MK-004 (nouveau) | **CORRIGÉ** (5929978) | `-lrt`/`-Wl,-z,stack-size`/`-D_DARWIN_C_SOURCE` |

Les anomalies BL-003→BL-012, FL-001, MT-004, NS-001→NS-012, CT-001/003, LL-002/004/005, IBM-001/002, TT-001/002, CR-001 restent **OPEN** — registre 140 inchangé.

---

## 5. État de certification

**CERTIFIED_100 = false** — Anomalies P0 (CR-001 clé Kaggle) et P1 (BL-003→007, FL-001) encore OPEN.

---

*Rapport 141 généré le 2026-10-02 | Mode DEBUG actif | SHA = 5929978 | ARTCB forensic agent*
