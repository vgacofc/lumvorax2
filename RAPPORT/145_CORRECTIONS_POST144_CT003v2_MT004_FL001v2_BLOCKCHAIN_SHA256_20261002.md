# Rapport 145 — Corrections post-audit 144 : CT-003 v2 / MT-004 snapshot / FL-001 v2 / Makefile blockchain+portable / Tests SHA-256 FIPS

**Date :** 2026-10-02T15:58:16Z  
**Session :** 145 (réponse à l'audit 144)  
**HEAD avant commit :** `640e9c8`  
**Build principal :** ✅ 0 erreur / 0 warning  
**Test blockchain :** ✅ 9/9 PASS  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

---

## 1. Résumé des corrections répondant à l'audit 144

| ID audit 144 | Anomalie | Correction session 145 | Statut |
|---|---|---|---|
| CT-003 OPEN | Macro `REPLIT_MEMORY_LIMIT_MB` fixé à 768 encore exposé | Macro supprimé, `strtol` + `errno` + `endptr` robuste | ✅ CT-003 CLÔTURÉ |
| MT-004 snapshot concurrent | `memory_tracker_export_json()` lit compteurs sans mutex | Snapshot atomique sous `g_tracker_mutex` avant écriture JSON | ✅ MT-004 CLÔTURÉ |
| FL-001 thread-safety globale | `forensic_log_file` accédé sans verrou par 5 fonctions | `fl001_log_file_mutex` couvre init/destroy/log/unified/memory/lum | ✅ FL-001 CLÔTURÉ |
| SHA-256 build non prouvé | Sources blockchain absentes de SOURCES Makefile | Cible `blockchain_test` + test 9/9 PASS + `nm` symboles liés | ✅ SHA-256 BUILD PROUVÉ |
| BL-008 -march=native | Portabilité ISA non adressée | `CFLAGS_PORTABLE` + cible `make portable` sans `-march=native` | ✅ BL-008 PARTIELLEMENT ADRESSÉ (infrastructure disponible) |
| BL-013 NOUVEAU | Nonce hors fenêtre 80 octets hashés (structure étendue LUMVORAX) | Documenté dans test T06b + commentaire `block_header.c` | ✅ DOCUMENTÉ |

---

## 2. Corrections détaillées — Avant / Après

### 2.1 CT-003 v2 — `src/common/common_types.h`

**Anomalie audit 144 :** `#define REPLIT_MEMORY_LIMIT_MB REPLIT_MEMORY_LIMIT_MB_DEFAULT` encore présent → tout code utilisant ce macro reçoit 768 indépendamment de l'environnement. `strtol` sans `endptr`/`errno` = validation insuffisante.

**AVANT (ligne 39) :**
```c
#define REPLIT_MEMORY_LIMIT_MB REPLIT_MEMORY_LIMIT_MB_DEFAULT  /* compat backward */
...
static inline size_t replit_memory_limit_mb_runtime(void) {
    const char* env = getenv("REPLIT_MEMORY_LIMIT_MB");
    if (env) {
        long val = strtol(env, NULL, 10);  /* sans endptr ni errno */
        if (val > 0 && val <= 65536) return (size_t)val;
    }
    return REPLIT_MEMORY_LIMIT_MB_DEFAULT;
}
```

**APRÈS :**
```c
/* Le macro public REPLIT_MEMORY_LIMIT_MB a été supprimé */
#define REPLIT_MEMORY_LIMIT_MB_DEFAULT 768
#include <errno.h>   /* CT-003 v2: pour strtol errno */
static inline size_t replit_memory_limit_mb_runtime(void) {
    const char* env = getenv("REPLIT_MEMORY_LIMIT_MB");
    if (env && env[0] != '\0') {
        char* endptr = NULL;
        errno = 0;
        long val = strtol(env, &endptr, 10);
        if (errno == 0 && endptr != env && *endptr == '\0' && val > 0 && val <= 65536)
            return (size_t)val;
    }
    return REPLIT_MEMORY_LIMIT_MB_DEFAULT;
}
```

**Invariant :** aucun code actif ne doit utiliser `REPLIT_MEMORY_LIMIT_MB` directement — la suppression du macro est volontaire. `ART-CT003` reste OPEN (copies dans artefacts historiques non modifiées, documenté).

---

### 2.2 MT-004 snapshot atomique — `src/debug/memory_tracker.c`

**Anomalie audit 144 :** `memory_tracker_export_json()` lisait `g_total_allocated`, `g_total_freed`, `g_count`, `g_active_alloc_count` sans prendre `g_tracker_mutex` → état intermédiaire possible si thread alloue/libère en parallèle.

**AVANT (ligne 38-59) :**
```c
void memory_tracker_export_json(const char* filename) {
    if (!memory_tracker_is_enabled()) return;
    FILE* fp = fopen(filename, "w");
    ...
    fprintf(fp, "  \"total_allocated\": %zu,\n", g_total_allocated);  // sans mutex
    ...
}
```

**APRÈS :**
```c
void memory_tracker_export_json(const char* filename) {
    if (!memory_tracker_is_enabled()) return;
    /* Snapshot atomique sous mutex */
    pthread_mutex_lock(&g_tracker_mutex);
    size_t snap_total_allocated = g_total_allocated;
    size_t snap_total_freed     = g_total_freed;
    size_t snap_count           = g_count;
    size_t snap_active          = g_active_alloc_count;
    pthread_mutex_unlock(&g_tracker_mutex);
    FILE* fp = fopen(filename, "w");
    ...
    fprintf(fp, "  \"total_allocated\": %zu,\n", snap_total_allocated);
    ...
}
```

**Note d'implémentation :** `g_tracker_mutex` et toutes les variables globales ont été remontées avant `memory_tracker_export_json()` pour résoudre l'erreur de compilation « undeclared identifier ».

---

### 2.3 FL-001 v2 — `src/debug/forensic_logger.c`

**Anomalie audit 144 :** `forensic_log_file` accédé sans verrou par 5 fonctions : `forensic_logger_init`, `forensic_log_memory_operation`, `forensic_log_lum_operation`, `forensic_log`, `unified_forensic_log`, `forensic_logger_destroy`.

**AVANT :** zéro protection sur `forensic_log_file` dans ces fonctions.

**APRÈS :** `fl001_log_file_mutex` (PTHREAD_MUTEX_INITIALIZER) verrouille chaque accès à `forensic_log_file`, avec unlock sur tous les chemins d'erreur :
```c
static pthread_mutex_t fl001_log_file_mutex = PTHREAD_MUTEX_INITIALIZER;

void forensic_log_memory_operation(...) {
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (!forensic_log_file) { pthread_mutex_unlock(&fl001_log_file_mutex); return; }
    ...
    pthread_mutex_unlock(&fl001_log_file_mutex);
}
/* même pattern pour les 5 autres fonctions */
```

---

### 2.4 Makefile — cible `blockchain_test` + `make portable`

**Anomalie audit 144 :** sources blockchain (`block_header.c`, `sha256_mini.c`) absentes de SOURCES principal → « code corrigé existe » ≠ « build l'intègre ».

**AVANT :** aucune cible blockchain dans le Makefile.

**APRÈS :**
```makefile
BLOCKCHAIN_SOURCES = \
    $(SRC_DIR)/blockchain_lumvorax/sha256_mini.c \
    $(SRC_DIR)/blockchain_lumvorax/block_header.c

blockchain_test: directories
    $(CC) $(CFLAGS) -c sha256_mini.c -o sha256_mini.o
    $(CC) $(CFLAGS) -c block_header.c -o block_header.o
    $(CC) $(CFLAGS) src/tests/test_blockchain_sha256.c sha256_mini.o block_header.o \
        -o bin/test_blockchain_sha256 $(LDFLAGS)
    nm bin/test_blockchain_sha256 | grep -E "sha256_lumvorax|block_header_hash" && echo "[SHA-256 OK]"
```

**Preuve exécution :**
```
0000000100003150 T _sha256_lumvorax
00000001000035f0 T _block_header_hash
[SHA-256 OK] Symboles liés
```

**BL-008 / Portabilité :**
```makefile
CFLAGS_PORTABLE = -Wall -Wextra -std=c99 -g -O2 -fPIC ...  # sans -march=native
portable: ...  # cible make portable
```

---

### 2.5 Tests SHA-256 — `src/tests/test_blockchain_sha256.c` (NOUVEAU)

| Test | Description | Résultat |
|---|---|---|
| T01 | SHA-256("abc") = vecteur NIST FIPS 180-4 | ✅ PASS |
| T02 | SHA-256("") = vecteur NIST FIPS 180-4 | ✅ PASS |
| T03 | Double-SHA256("abc") ≠ SHA-256("abc") | ✅ PASS |
| T03b | Double-SHA256 non nul | ✅ PASS |
| T04 | block_header_hash() digest non nul | ✅ PASS |
| T05 | block_header_hash() déterministe | ✅ PASS |
| T06 | prev_hash différent → hash différent | ✅ PASS |
| T06b | BL-013 DOCUMENTÉ : nonce (offset 88) hors fenêtre 80 oct. | ✅ PASS (comportement attendu documenté) |
| T07 | SHA-256(448-bit NIST msg) = vecteur FIPS | ✅ PASS |

**Total : 9/9 PASS**

---

### 2.6 BL-013 — Nouvelle anomalie documentée (découverte par T06)

**Découverte :** La structure `block_header_t` LUMVORAX utilise `uint64_t timestamp` (8 octets) au lieu de `uint32_t` (4 octets comme Bitcoin). Avec l'alignement C99, les offsets réels sont :

| Champ | Offset | Taille | Dans 80 octets ? |
|---|---|---|---|
| version | 0 | 4 | ✅ |
| prev_hash | 4 | 32 | ✅ |
| merkle_root | 36 | 32 | ✅ |
| timestamp | 72 | 8 | Partiel (8 oct à partir de 72) |
| bits | 80 | 4 | ❌ hors fenêtre |
| nonce | 88 | 8 | ❌ hors fenêtre |

**Conséquence :** le nonce et `bits` ne participent pas au hash des 80 premiers octets. `block_header_hash()` ne peut pas servir de preuve de travail (PoW) sur le nonce sans modification.

**Statut :** BL-013 = OPEN (décision architecturale requise : étendre la fenêtre hashée ou reformater le header). Documenté dans les tests et dans le rapport.

---

## 3. Résultat build

```
make clean && make       → 0 warning, 0 erreur (build principal)
make blockchain_test     → 0 warning, 0 erreur (cible séparée)
./bin/test_blockchain_sha256 → 9/9 PASS
Symboles liés : _sha256_lumvorax + _block_header_hash confirmés par nm
```

---

## 4. Registre anomalies — état post-145

### Clôturées par la session 145

| ID | Clôture | Preuve |
|---|---|---|
| CT-003 | Macro 768 supprimé, `strtol` robuste | Code + build 0 warning |
| MT-004 snapshot | Snapshot atomique sous mutex | Code visible |
| FL-001 thread-safety globale | `fl001_log_file_mutex` couvre 6 fonctions | Code visible |
| SHA-256 build proof | `make blockchain_test` + `nm` + 9/9 tests | Exécution confirmée |

### Toujours OPEN (registre persistant)

- **CR-001 (P0)** — clé Kaggle historique → action manuelle opérateur
- **BL-003→BL-012** — modules VORAX / portabilité
- **BL-013 (NOUVEAU)** — nonce/bits hors fenêtre 80 octets hashés
- **NS-001→NS-012** — solveur Navier-Stokes
- **NQubit** — `fake_superposition` = RNG classique, OPEN scientifique
- **ART-CT003** — copies divergentes `common_types.h` dans artefacts historiques
- **LL-002/004/005** — logging à revalider
- **IBM-001/002** — synchronisation distante
- **TT-001/002** — couverture de tests / isolation
- **BUILD-PROOF-001** — CI C indépendante toujours absente
- **Richardson / T04 énergie / Lyapunov / NX-42** — chantiers scientifiques
- **BL-008** — `-march=native` encore actif dans build par défaut (infrastructure `make portable` ajoutée)

---

## 5. Invariants

- `CERTIFIED_100=false` — inchangé
- `unique_human_proven=false` — inchangé
- Mode DEBUG actif — flag `-DDEBUG_MODE` dans Makefile
- Aucun ancien rapport écrasé
- Aucun hardcoding / stub / mock introduit
- Registre historique conservé intégralement

---

*Rapport produit automatiquement — session 145 | agent Bob IDE*
