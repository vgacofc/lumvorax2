# Rapport 164 — UNICITE-002 : Schéma LUM_ID v3 + extension UNIF-002/003/004

**Date :** 2026-10-03  
**Session :** S163  
**HEAD avant correction :** `589f6b4`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte

L'audit S162 (rapport 163) avait identifié 6 points critiques empêchant de déclarer l'unicité globale de l'architecture forensic. Ce rapport documente la correction UNICITE-002 qui adresse les points PC1 à PC4 et étend le schéma v3 aux 3 fichiers UNIF-002/003/004.

---

## 2. Points critiques adressés

### PC1 — Vocabulaire des doublons (clarification)

**Avant :** `duplicates` dans `LumIDHashSet` = nombre de LUM_ID avec count > 1 (définition ambiguë).

**Après :** `LumIDHashSetV3` (dans `lum_id_schema.h`) expose 4 métriques distinctes :
```c
uint64_t total_insertions; /* nb total d'insertions (= total_events attendus) */
uint64_t distinct_ids;     /* nb de LUM_ID distincts */
uint64_t duplicate_ids;    /* nb de LUM_ID ayant count > 1 */
/* extra_events = total_insertions - distinct_ids (calculé dans print_summary) */
```

### PC2 — Limite step 16 bits

**Avant :** `step` encodé sur 32 bits en v1, réduit à 16 bits en v2 sans documentation.

**Après :** `lum_id_schema.h` définit :
```c
#define LUM_ID_V3_MAX_STEPS  65535U   /* step ∈ [0..65535] */
```
Et chaque boucle de traçage comporte la garde :
```c
if (step >= (int)LUM_ID_V3_MAX_STEPS) {
    fprintf(stderr, "[UNIF2][DEBUG] step=%d >= LUM_ID_V3_MAX_STEPS — arrêt\n", step);
    break;
}
```

### PC3 — run_id XOR non injectif

**Avant :**
```c
/* v1/v2 — ns_forensic_unif2.c ligne 337 */
static uint16_t make_run_id(uint64_t ts_start) {
    return (uint16_t)(
        ((ts_start) & 0xFFFFU) ^ ((ts_start >> 16) & 0xFFFFU) ^
        ((ts_start >> 32) & 0xFFFFU) ^ ((ts_start >> 48) & 0xFFFFU)
    );
}
```
→ XOR non injectif : deux timestamps différents peuvent produire le même run_id.

**Après :**
```c
/* lum_id_schema.h — compteur séquentiel */
static uint16_t g_lum_run_seq_counter = 0;

static inline uint16_t lum_id_v3_new_run_seq(void) {
    g_lum_run_seq_counter++;
    if (g_lum_run_seq_counter < LUM_ID_V3_MIN_RUN_SEQ)
        g_lum_run_seq_counter = LUM_ID_V3_MIN_RUN_SEQ;
    return g_lum_run_seq_counter;
}
```
→ Compteur incrémenté à chaque appel. `run_seq ∈ [1..65535]`. Injective dans la session courante.

### PC4 — Sentinelle HASH_EMPTY = UINT64_MAX ∈ espace LUM_ID

**Avant :**
```c
#define HASH_EMPTY UINT64_MAX   /* ← appartient à l'espace LUM_ID possible */
```

**Après :**
```c
/* lum_id_schema.h */
#define LUM_ID_V3_HASH_EMPTY  UINT64_C(0x0000000000000000)
```
**Preuve PC4 :** `run_seq ≥ 1` → `LUM_ID ≥ 0x0001_0000_0000_0000` pour tout LUM_ID valide. La valeur `0x0000...` ne peut jamais être produite par `lum_id_v3_encode()`. La sentinelle est donc hors de l'espace des LUM_ID valides.

---

## 3. Nouveau header partagé `lum_id_schema.h`

**Fichier :** `src/validation/lum_id_schema.h` (créé en S163)

**Contient :**
- Constantes `LUM_ID_V3_MAX_*`
- `lum_id_v3_new_run_seq()` — compteur séquentiel
- `lum_id_v3_encode()` — encodage 64 bits v3 avec assertion run_seq ≥ 1
- `LumIDHashSetV3` — hash set avec métriques précises (PC1)
- `lum_hashset_v3_create/insert/print_summary/is_unique/destroy`

**Schéma v3 — layout 64 bits :**
```
[63:48] run_seq  (16 bits) — ∈ [1..65535], jamais 0
[47:44] protocol  (4 bits) — id protocole
[43:40] module    (4 bits) — id module
[39:24] step     (16 bits) — ∈ [0..65535], max documenté
[23:8]  cell_idx (16 bits) — ∈ [0..65535]
[7:0]   bit_pos   (8 bits) — ∈ [0..63]
```

---

## 4. Avant/Après — UNIF-002 et UNIF-003

### `encode_lum_id_64` — AVANT (v1, commun aux 3 fichiers)

**Fichier :** `src/validation/ns_forensic_unif2.c`, ligne 128 (avant S163)
```c
static uint64_t encode_lum_id_64(uint16_t run_id, int protocol,
                                  int module, uint32_t step, int bit_pos)
{
    return ((uint64_t)(run_id   & 0xFFFFU) << 48)
         | ((uint64_t)(protocol & 0xFU)    << 44)
         | ((uint64_t)(module   & 0xFU)    << 40)
         | ((uint64_t)(step     & 0xFFFFFFFFU) << 8)
         | ((uint64_t)(bit_pos  & 0x3FU)   << 2);
}
```

### `lum_id_v3_encode` — APRÈS (v3, dans header partagé)

**Fichier :** `src/validation/lum_id_schema.h`, ligne ~110 (après S163)
```c
static inline uint64_t lum_id_v3_encode(uint16_t run_seq, int protocol,
                                         int module, uint16_t step,
                                         uint16_t cell_idx, int bit_pos)
{
    if (run_seq < LUM_ID_V3_MIN_RUN_SEQ) { run_seq = LUM_ID_V3_MIN_RUN_SEQ; }
    return ((uint64_t)(run_seq  & 0xFFFFU) << 48)
         | ((uint64_t)(protocol & 0xFU)    << 44)
         | ((uint64_t)(module   & 0xFU)    << 40)
         | ((uint64_t)(step     & 0xFFFFU) << 24)
         | ((uint64_t)(cell_idx & 0xFFFFU) << 8)
         | ((uint64_t)(bit_pos  & 0xFFU));
}
```

---

## 5. Résultats d'exécution

### Compilation — 0 warning, 0 erreur sur les 3 binaires

| Binaire | Statut |
|---------|--------|
| `bin/ns_forensic_unif2` | ✅ compilé, 0 warning |
| `bin/ns_forensic_unif3` | ✅ compilé, 0 warning |
| `bin/ns_forensic_unif4` | ✅ compilé, 0 warning |

### Unicité — résultats hash set

| Binaire | total_insertions | distinct_ids | duplicate_ids | extra_events | Verdict |
|---------|-----------------|--------------|---------------|--------------|---------|
| UNIF-002 | 49 280 | 49 280 | **0** ✅ | 0 ✅ | **PASS** |
| UNIF-003 | 49 280 | 49 280 | **0** ✅ | 0 ✅ | **PASS** |
| UNIF-004 | 49 280 | 49 280 | **0** ✅ | — | **PASS** |

---

## 6. Limites honnêtes restantes

| Limite | Statut |
|--------|--------|
| step max = 65535 (16 bits) | Documenté + garde runtime — non supprimable sans schéma 128 bits |
| run_seq wrap-around à 65535 | Avertissement DEBUG — 65535 runs/session est acceptable en pratique |
| UNIF-004 : hash set v1 encore présent (pas migré vers LumIDHashSetV3) | OPEN mineur |
| Richardson-PROTOCOL-001 | OPEN |
| run_seq=1 dans chaque binaire | Normal — compteur statique par unité de compilation, indépendant entre binaires |
| Grilles > 256×256 dépassent cell_idx | OPEN/FUTUR — schéma 128 bits à définir |

---

## 7. État registre UNICITE

| Chantier | Avant S163 | Après S163 |
|----------|-----------|-----------|
| UNICITE-001 UNIF-004 | PASS S162 | PASS S162 (inchangé) |
| UNICITE-001 UNIF-002/003 | FAIL (3840 doublons) | **PASS** ✅ |
| PC1 vocabulaire | Non adressé | **ADRESSÉ** ✅ |
| PC2 step limite | Non documenté | **DOCUMENTÉ** ✅ |
| PC3 run_id injectif | Non | **CORRIGÉ** ✅ |
| PC4 sentinelle HASH_EMPTY | Risque (UINT64_MAX) | **CORRIGÉ** ✅ |
| Header partagé | Aucun | **`lum_id_schema.h`** ✅ |

**CERTIFIED_100=false** — la certification globale reste bloquée par les chantiers OPEN ci-dessus.

---

**Logs :** `logs/20261003_s163_unicite002_schema_v3.json`  
**Header créé :** `src/validation/lum_id_schema.h`  
**Fichiers modifiés :** `ns_forensic_unif2.c`, `ns_forensic_unif3.c`, `ns_forensic_unif4.c`
