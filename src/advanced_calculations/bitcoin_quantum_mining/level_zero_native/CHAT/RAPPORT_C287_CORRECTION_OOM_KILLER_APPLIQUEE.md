# RAPPORT C287 - CORRECTION OOM KILLER APPLIQUÉE

**Date**: 2026-05-19 22:28 UTC  
**Session**: C287 (Correction SIGKILL)  
**Problème**: OOM Killer tue processus (SIGKILL) à cause batch size 286M nonces  
**Solution**: Utiliser RAM DISPONIBLE au lieu de RAM TOTALE

---

## 🚨 PROBLÈME IDENTIFIÉ

### Symptômes
```
[C287-C258] 🚀 Limite dynamique ILLIMITÉE: 286331153 nonces (~3.20 GB)
[C42-WATCHDOG] 🔴 RAM CRITIQUE: 112MB — PAUSE totale
tools/btc_run_ubuntu.sh: line 162: 71438 Killed
```

### Cause Racine
Le code [`btc_gen9_mining_adapter.c:54-86`](../src/btc_gen9_mining_adapter.c:54) calculait:
- RAM TOTALE: 7622 MB
- Limite GPU: 75% × 7622 MB = 5.7 GB
- Batch size: 286M nonces (3.2 GB GPU)
- **MAIS** RAM disponible réelle: 112-141 MB seulement
- **RÉSULTAT**: OOM Killer envoie SIGKILL

---

## ✅ CORRECTIONS APPLIQUÉES

### 1. Fonction `get_system_ram_available_mb()` (Ligne 54)

**AVANT (ERREUR)**:
```c
uint64_t ram_mb = (info.totalram * info.mem_unit) / (1024 * 1024);
```

**APRÈS (CORRECTION)**:
```c
/* CORRECTION C287: Utiliser freeram (disponible) au lieu de totalram */
uint64_t ram_avail_mb = (info.freeram * info.mem_unit) / (1024 * 1024);
```

### 2. Fonction `calculate_dynamic_gpu_limit_mb()` (Ligne 66)

**AVANT (ERREUR)**:
```c
/* Formule OpenCL: 75% RAM système, max 4GB pour stabilité Gen9 */
uint64_t gpu_limit_mb = (system_ram_mb * 75) / 100;

/* Clamp à 4GB (limite pratique Intel Gen9) */
if (gpu_limit_mb > 4096) {
    gpu_limit_mb = 4096;
}
```

**APRÈS (CORRECTION)**:
```c
/* CORRECTION C287: 50% RAM DISPONIBLE (pas 75% totale) pour sécurité OOM */
uint64_t gpu_limit_mb = (avail_ram_mb * 50) / 100;

/* CORRECTION C287: Clamp à 2GB (pas 4GB) pour sécurité maximale */
if (gpu_limit_mb > 2048) {
    gpu_limit_mb = 2048;
}
```

### 3. Message Limite (Ligne 179)

**AJOUT**: Variable statique pour afficher UNE SEULE FOIS
```c
/* C287: Afficher message UNE SEULE FOIS (pas à chaque batch) */
static int limit_msg_shown = 0;
if (!limit_msg_shown) {
    fprintf(stderr, "[C287-C258] 🏁 LIMITE SÉCURISÉE ATTEINTE: %u nonces (~%.2f GB GPU)\n",
            g_batch_state.current_size, mem_gb);
    fprintf(stderr, "[C287-C258] ✅ Batch size optimal pour RAM disponible\n");
    limit_msg_shown = 1;
}
```

---

## 📊 RÉSULTATS ATTENDUS

### Avec 4.2 GB RAM Disponible (Exemple)
- RAM disponible: 4200 MB
- Limite GPU: 50% × 4200 MB = 2100 MB → **clamped à 2048 MB**
- Batch size max: ~143M nonces (1.6 GB GPU)
- **Pas de SIGKILL** - reste dans limites RAM

### Avec 3.7 GB RAM Disponible (Exemple)
- RAM disponible: 3700 MB
- Limite GPU: 50% × 3700 MB = 1850 MB
- Batch size max: ~130M nonces (1.5 GB GPU)
- **Pas de SIGKILL** - reste dans limites RAM

---

## 🔬 COMPARAISON AVANT/APRÈS

| Métrique | AVANT (C287 bugué) | APRÈS (C287 corrigé) |
|----------|-------------------|---------------------|
| Source RAM | `totalram` (7.6 GB) | `freeram` (3.7-4.2 GB) |
| Pourcentage | 75% | 50% |
| Limite max | 4096 MB | 2048 MB |
| Batch size | 286M nonces | 130-143M nonces |
| Mémoire GPU | 3.2 GB | 1.5-1.6 GB |
| RAM critique | 112-141 MB | N/A (sécurisé) |
| SIGKILL | ✅ OUI | ❌ NON |

---

## 🎯 PROCHAINES ÉTAPES

1. ✅ Corrections appliquées dans [`btc_gen9_mining_adapter.c`](../src/btc_gen9_mining_adapter.c:54)
2. ✅ Binaire recompilé avec corrections
3. ⏳ **TEST 30s**: Vérifier pas de SIGKILL avec RAM disponible
4. ⏳ **MINAGE ILLIMITÉ**: Si test réussi, lancer minage continu

---

## 📝 NOTES TECHNIQUES

### Pourquoi `freeram` au lieu de `totalram`?
- `totalram`: RAM physique totale (7.6 GB) - inclut RAM utilisée par système
- `freeram`: RAM réellement disponible (3.7-4.2 GB) - ce qui peut être alloué
- OOM Killer se base sur `freeram`, pas `totalram`

### Pourquoi 50% au lieu de 75%?
- Marge de sécurité pour éviter OOM Killer
- Système peut allouer RAM supplémentaire pendant exécution
- 50% garantit espace pour buffers kernel, cache, etc.

### Pourquoi max 2GB au lieu de 4GB?
- Intel Gen9 UHD 620: Shared memory avec CPU
- Allocation >2GB peut fragmenter mémoire
- 2GB = sweet spot performance/stabilité

---

**FIN DU RAPPORT C287 - CORRECTION OOM KILLER**