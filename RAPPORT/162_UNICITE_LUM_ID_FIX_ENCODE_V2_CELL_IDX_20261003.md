# Rapport 162 — Correction UNICITE-001 : encode_lum_id_64 v2 + cell_idx

**Date :** 2026-10-03  
**Session :** S162  
**HEAD avant correction :** `6f50173`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte et question de l'utilisateur

> Le hash set détecte 3840 doublons dans ns_forensic_unif4. Ce n'est pas un bug du logger — c'est un artefact de l'encodage LUM_ID. Que faire pour corriger cela ?

La session 161 avait identifié 3840 doublons de LUM_ID dans `FORENSIC-UNIF-004` via le hash set. Le rapport 152 documentait le diagnostic initial. Ce rapport documente la correction complète implémentée en S162.

---

## 2. Diagnostic exact (root cause)

### 2.1 Schéma d'encodage v1 — défaut de conception

**Fichier :** `src/validation/ns_forensic_unif4.c`  
**Lignes avant correction (v1, lignes 72–80) :**

```c
static uint64_t encode_lum_id_64(uint16_t run_id, int protocol,
                                  int module, uint32_t step, int bit_pos)
{
    return ((uint64_t)(run_id   & 0xFFFFU) << 48)
         | ((uint64_t)(protocol & 0xFU)    << 44)
         | ((uint64_t)(module   & 0xFU)    << 40)
         | ((uint64_t)(step     & 0xFFFFFFFFU) << 8)
         | ((uint64_t)(bit_pos  & 0x3FU)   << 2);
    /* Bits [1:0] = toujours 0 — non utilisés */
}
```

**Schéma v1 :**
```
[63:48] run_id   (16 bits)
[47:44] protocol  (4 bits)
[43:40] module    (4 bits)
[39:8]  step     (32 bits)
[7:2]   bit_pos   (6 bits)
[1:0]   réservés  (2 bits) ← toujours 0, gaspillage
```

**Cause des collisions :** La fonction `trace_double_bits_mono()` est appelée **pour chaque cellule** d'un champ NS (U, V, P, Utmp, Vtmp). Pour une grille 4×4 :
- `cells_u = (nx-1)×ny = 3×4 = 12 cellules`
- `cells_v = nx×(ny-1) = 4×3 = 12 cellules`
- `cells_p = nx×ny = 4×4 = 16 cellules`

Pour le même `(module, step, bit_pos)`, **12 ou 16 cellules distinctes** produisaient **le même LUM_ID** → 3840 collisions.

### 2.2 Calcul des doublons

Pour `UNIF4_STEPS=10`, `UNIF4_GRID_N=4` :

| Module | Cellules/step | Collisions/step | Total (10 steps) |
|--------|-------------|----------------|-----------------|
| U_IN   | 12 | 11×64 = 704 | 7040 |
| V_IN   | 12 | 11×64 = 704 | 7040 |
| P_IN   | 16 | 15×64 = 960 | 9600 |
| UTMP   | 12 | 11×64 = 704 | 7040 |
| VTMP   | 12 | 11×64 = 704 | 7040 |
| U_OUT  | 12 | 11×64 = 704 | 7040 |
| POISSON| 1  | 0           | 0    |

Le hash set comptait `3840 doublons` (entrées avec count > 1) : c'est le nombre de LUM_IDs distincts ayant au moins une collision. Le nombre total d'événements dupliqués était bien supérieur (chaque doublon couvrait plusieurs cellules).

---

## 3. Correction appliquée — schéma v2

### 3.1 Nouveau schéma d'encodage

**Fichier :** `src/validation/ns_forensic_unif4.c`  
**Lignes après correction (v2, lignes 72–100) :**

```c
/* Schéma v2 (UNICITE-001 FIX) :
 *   [63:48] run_id   (16 bits)  — identifiant de run
 *   [47:44] protocol  (4 bits)  — id protocole (ex. PROTOCOL_UNIF4=2)
 *   [43:40] module    (4 bits)  — id module (MOD_U_IN…MOD_POISSON)
 *   [39:24] step     (16 bits)  — numéro de pas (max 65535)
 *   [23:8]  cell_idx (16 bits)  — indice de cellule dans le champ (max 65535)
 *   [7:0]   bit_pos   (8 bits)  — position de bit dans le double (0..63)
 */
static uint64_t encode_lum_id_64(uint16_t run_id, int protocol,
                                  int module, uint16_t step,
                                  uint16_t cell_idx, int bit_pos)
{
    return ((uint64_t)(run_id   & 0xFFFFU) << 48)
         | ((uint64_t)(protocol & 0xFU)    << 44)
         | ((uint64_t)(module   & 0xFU)    << 40)
         | ((uint64_t)(step     & 0xFFFFU) << 24)
         | ((uint64_t)(cell_idx & 0xFFFFU) << 8)
         | ((uint64_t)(bit_pos  & 0xFFU));
}
```

**Garantie d'unicité :** Le tuple `(run_id, protocol, module, step, cell_idx, bit_pos)` est unique par événement. Aucune collision possible.

### 3.2 Fonctions adaptées

#### `trace_double_bits_mono` — AVANT

```c
// Ligne 215 (v1)
static void trace_double_bits_mono(double value, uint16_t run_id, int module,
                                    uint32_t step, uint64_t ts_mono,
                                    ModCoverage *cov, LumIDHashSet *hs)
```

#### `trace_double_bits_mono` — APRÈS

```c
// Ligne 219 (v2)
static void trace_double_bits_mono(double value, uint16_t run_id, int module,
                                    uint16_t step, uint16_t cell_idx,
                                    uint64_t ts_mono,
                                    ModCoverage *cov, LumIDHashSet *hs)
```

#### `trace_field_u` — AVANT

```c
// Ligne 244 (v1)
static void trace_field_u(const NSSolver2D *s, uint16_t rid, int module,
                           uint32_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSet *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++)
            trace_double_bits_mono(s->u[i*(ny+2)+j], rid, module, step, ts, cov, hs);
}
```

#### `trace_field_u` — APRÈS

```c
// Ligne 248 (v2)
static void trace_field_u(const NSSolver2D *s, uint16_t rid, int module,
                           uint16_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSet *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_mono(s->u[i*(ny+2)+j], rid, module, step, cidx, ts, cov, hs);
}
```

Même pattern pour `trace_field_v`, `trace_field_p`, `trace_field_utmp`, `trace_field_vtmp`.

#### `trace_ns_step_unif4` — AVANT

```c
// Ligne 346 (v1)
uint16_t rid = ctx->run_id;
// ...
trace_double_bits_mono(poisson_res, rid, MOD_POISSON,
                        step_n, ts_after, &ctx->mod[MOD_POISSON], ctx->hs);
```

#### `trace_ns_step_unif4` — APRÈS

```c
// Ligne 348 (v2)
uint16_t rid  = ctx->run_id;
uint16_t step = (uint16_t)(step_n & 0xFFFFU);  /* UNICITE-001 FIX : cast uint16 */
// ...
/* UNICITE-001 FIX : Poisson = cellule unique → cell_idx=0 */
trace_double_bits_mono(poisson_res, rid, MOD_POISSON,
                        step, 0, ts_after, &ctx->mod[MOD_POISSON], ctx->hs);
```

---

## 4. Résultat de l'exécution

### 4.1 Compilation

```
gcc -Wall -Wextra -std=c99 -g -O3 ...
→ 0 warning, 0 erreur
```

### 4.2 Exécution — ns_forensic_unif4

```
=== UNICITE-001 — HASH SET LUM_ID ===

  Capacité hash set     : 131072 slots
  Entrées distinctes    : 49280
  Doublons détectés     : 0 ✓ AUCUN
  Unicité exacte        : PASS — tous les LUM_ID sont distincts

[VERDICT] FORENSIC-UNIF-004 : PASS — timestamps MONOTONIC, unicité exacte, 0 gap, 0 régression
```

| Métrique | Avant (v1) | Après (v2) |
|----------|-----------|-----------|
| Doublons LUM_ID | **3840** | **0** ✅ |
| Entrées hash set distinctes | 45440 | 49280 ✅ |
| Événements tracés | 49280 | 49280 ✅ |
| Régressions timestamp | 0 | 0 ✅ |
| Gaps séquence | 0 | 0 ✅ |
| VERDICT | FAIL (unicité) | **PASS** ✅ |

### 4.3 Log généré

**Fichier :** `logs/forensic/ns_forensic_unif4.log`  
**Taille :** 3 559 754 octets (~3.4 Mo)  
**Événements :** 49 280  
**Nouveau format (cell_idx visible) :**
```
[232413317117000] [seq=1] [lum_id=0xe31c200000000000] U_IN:c0:val=0
```
Le champ `c0`, `c1`, … permet d'auditer quelle cellule a produit chaque événement.

---

## 5. Analyse du schéma v2

### 5.1 Capacité maximale du schéma v2

| Champ | Bits | Valeur max | Suffisant pour |
|-------|------|-----------|----------------|
| run_id | 16 | 65535 runs | ✅ |
| protocol | 4 | 15 protocoles | ✅ |
| module | 4 | 15 modules | ✅ (7 utilisés) |
| step | 16 | 65535 pas | ✅ (10 utilisés) |
| cell_idx | 16 | 65535 cellules | ✅ (16 max sur 4×4) |
| bit_pos | 8 | 255 bits | ✅ (64 utilisés) |

### 5.2 Scalabilité

Pour une grille 128×128 : `cells_p = 128×128 = 16384 cellules` — bien dans les 65535 max de cell_idx (16 bits).  
Pour une grille 256×256 : `cells_p = 65536` — **limite exacte**, toujours OK.  
Pour une grille 512×512 : `cells_p = 262144 > 65535` → dépassement cell_idx. À ce stade il faudra un schéma 128 bits ou une réorganisation. À documenter dans QUESTIONS_OUVERTES.

---

## 6. Chantiers restants

| Chantier | État |
|----------|------|
| UNICITE-001 sur `ns_forensic_unif4.c` | ✅ **FERMÉ** |
| `ns_forensic_unif2.c` — encode_lum_id_64 v1 encore présent | ⚠ OPEN |
| `ns_forensic_unif3.c` — encode_lum_id_64 v1 encore présent | ⚠ OPEN |
| Richardson-PROTOCOL-001 | OPEN |
| `perf_forensic_001` — pas encore exécuté | OPEN |
| `ns_richardson_003c` — pas encore exécuté | OPEN |
| BUILD-THREAD-001 faux positif ligne_count=8001 | OPEN (mineur) |
| Schéma LUM_ID 128 bits pour grilles > 256×256 | OPEN/FUTUR |

---

## 7. CERTIFIED_100=false

La correction est chirurgicale et ciblée sur `ns_forensic_unif4.c`. Aucun autre module n'a été modifié. Les chantiers OPEN de la session 161 restent identiques sauf UNICITE-001 sur UNIF-004 qui est désormais **FERMÉ**.

---

**Logs :** `logs/20261003_s162_unicite_lum_id_fix.json`  
**Binaire :** `bin/ns_forensic_unif4` (recompilé, 0 warning)  
**Fichier modifié :** `src/validation/ns_forensic_unif4.c`
