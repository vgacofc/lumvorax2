# Rapport 190 — Audit P0 FU002 : provenance bit→LUM→transformation vérifiée (S182)

**Date :** 2026-10-04T01:05:52Z  
**Session :** S182  
**SHA Git HEAD :** 8aaabe5  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

---

## 1. Contexte et objectif

Le registre OPEN du rapport 189 (P0 prioritaire) demandait de **vérifier la chaîne réelle de provenance bit-level** :

> *Processus : FU002 trace `bit → BIT_ID → LUM_ID → transformation → résultat`.  
> Ce qui existe : `forensic_unif_002.c/.h`, tests TSan, validations NS.  
> Ce qui manque : preuve démontrée de la provenance bit-level complète sur artefact réel (log JSONL).*

Ce rapport comble ce manque : audit ligne par ligne du code source FU002 + analyse exhaustive du log `forensic_unif_002_session.jsonl` (11 sessions, 3721 lignes).

---

## 2. Architecture FU002 (source lue sur HEAD 8aaabe5)

### 2.1 Identifiants

| Identifiant | Format | Calcul |
|-------------|--------|--------|
| `BIT_ID`    | `uint64_t` | bits [63:32] = `run_id` · bits [31:0] = `g_bit_seq` monotone |
| `LUM_ID`    | `uint64_t` | bits [63:32] = `run_id` · bits [31:16] = `group_index` · bits [15:0] = `bit_in_group` |
| `run_id`    | `uint32_t` | `(ts.tv_sec ^ ts.tv_nsec) ^ PID` — non cryptographique, unique par session |

**Fichier :** `src/debug/forensic_unif_002.c` lignes 202–225

### 2.2 Chaîne de traçabilité déclarée (header, lignes 14–19)

```
bit d'entrée
    → forensic_unif002_new_bit_id(bval)       [ÉTAPE 3, main.c L204]
    → BIT_ID (run_id | seq)
    → forensic_unif002_log_bit_input()         [main.c L205]
    → forensic_unif002_lum_id_from_bit()       [main.c L230]
    → LUM_ID (run_id | group_index | bit_in_group)
    → forensic_unif002_log_lum_created()       [main.c L232]
    → forensic_unif002_log_transformation_ex() [main.c L274, L330, L384, L426]
    → chaque module SIMD/PARALLEL/ZERO_COPY/PARETO avec bit_before/bit_after
    → SESSION_END (stats intégrité)
```

### 2.3 Corrections P0–P3 vérifiées dans le source

| Fix    | AVANT (ligne source)                          | APRÈS (ligne source) | Fichier |
|--------|-----------------------------------------------|----------------------|---------|
| P0 SERIALIZE | Deux mutex séparés seq++/write → entrelacement | Un mutex unique couvre `seq++ → stats → fprintf → fflush` (L263–288) | `forensic_unif_002.c` |
| P1 CONTINUITY | `last_seq_seen=0` → faux DUPLICATE sur seq=0 | Flag `has_last_seq_seen=false` (L144) | `forensic_unif_002.c` |
| P2 TIMESTAMPS | 1 timestamp CLOCK_REALTIME | Dual : `ts_rt` + `ts_mono` (L73–85, L96–113) | `forensic_unif_002.c` |
| P3 BIT-VALUE | Pas de before/after | `bit_value_before` + `bit_value_after` dans `fu002_event_t` + `log_transformation_ex()` (L315–360) | `forensic_unif_002.c` |
| S182-P0 | main.c ne propageait pas `bit_vals[]` | Tableau `bit_vals[]` conserve valeur réelle, passé à `log_transformation_ex()` (L198–208, L274, L330, L384, L426) | `src/main.c` |

---

## 3. Audit des logs JSONL (artefact réel)

### 3.1 Fichier audité

**Chemin :** `logs/forensic/forensic_unif_002_session.jsonl`  
**Taille :** 3721 lignes | 11 sessions complètes

### 3.2 Historique des sessions

| Session | `run_id` | `dup` | `loss` | `ok` | Statut |
|---------|----------|-------|--------|------|--------|
| 1 | 1872825470 | 1 | 0 | false | ❌ P1 BUG pré-correction |
| 2 | 1109752017 | 1 | 0 | false | ❌ P1 BUG pré-correction |
| 3 | 1147087631 | 1 | 0 | false | ❌ P1 BUG pré-correction |
| 4 | 1829709222 | 1 | 0 | true  | ⚠️ dup=1 mais ok=true (transition) |
| 5 | 1255637001 | 0 | 0 | true  | ✅ Post P1 fix |
| 6–11 | … | 0 | 0 | true  | ✅ **6 sessions consécutives OK** |

**Observation :** Les 3 premières sessions exhibent `"op":"DUPLICATE:seq=0"` au seq=4 — exactement le bug P1 identifié (check_continuity(0) avec `last_seq_seen` initialisé à 0 voyait 0==0 → DUPLICATE). La session 5 et toutes les suivantes n'ont plus ce bug → preuve que la correction P1 est active dans le binaire commité.

### 3.3 Audit provenance bit-level (session 11 — la plus récente, `run_id=1204452248`)

6 audits automatisés exécutés sur Python avec lecture JSONL réelle :

| Audit | Description | Résultat | Attendu |
|-------|-------------|----------|---------|
| A1 | LUM_CREATED sans parent BIT_INPUT | **0** | 0 ✅ |
| A2 | BIT_INPUT sans LUM_CREATED correspondant | **0** | 0 ✅ |
| A3 | LUM_TRANSFORMED avec parent_id orphelin | **0** | 0 ✅ |
| A4 | LUM_ID encoding incorrect (run_id bits[63:32]) | **0** | 0 ✅ |
| A5 | Events hors séquence monotone | **0** | 0 ✅ |
| A6 | `bit_before` ≠ `bit_val` original dans BIT_INPUT | **0** | 0 ✅ |

**Résultat session 11 :** `bits=64 lums=64 tf=257 loss=0 dup=0 ok=1` ✅

### 3.4 Distribution des événements (session 11)

| Type événement | Code | Nombre |
|----------------|------|--------|
| SESSION_START  | 16   | 1 |
| BIT_INPUT      | 1    | **64** (= PAYLOAD_BYTES×8 = 8×8) |
| LUM_CREATED    | 2    | **64** (1-1 avec BIT_INPUT) |
| LUM_TRANSFORMED| 3    | **257** (64 SIMD + 64 PARALLEL + 64 ZERO_COPY + 64 PARETO + 1 résumé) |
| LUM_RESULT     | 5    | 1 |
| SESSION_END    | 17   | 1 |

### 3.5 Exemple de chaîne complète vérifiée (bit 0)

```
seq=3  BIT_INPUT  bit_id=0x47ca7b9800000000 val=1 src=PAYLOAD
seq=67 LUM_CREATED lum_id=0x47ca7b9800000000 parent_id=0x47ca7b9800000000  ← même bit_id
seq=X  LUM_TRANSFORMED (SIMD)  parent_id=0x47ca7b9800000000 bit_before=1 bit_after=1
seq=X  LUM_TRANSFORMED (PARALLEL) parent_id=0x47ca7b9800000000 bit_before=1 bit_after=1
seq=X  LUM_TRANSFORMED (ZERO_COPY) parent_id=0x47ca7b9800000000 bit_before=1 bit_after=1
seq=X  LUM_TRANSFORMED (PARETO) parent_id=0x47ca7b9800000000 bit_before=1 bit_after=1
```

La chaîne `bit → BIT_ID → LUM_ID → 4 transformations → résultat` est **traçable sans rupture** dans l'artefact JSONL réel.

---

## 4. Limites honnêtes confirmées

| Limite | Source | Statut |
|--------|--------|--------|
| `run_id` non cryptographique (`ts^PID`) | `forensic_unif_002.c` L136 | ⚠️ Documenté dans header L43 |
| Résolution horloge ≠ nanoseconde (macOS ~1µs) | Rapport 188 + header L44 | ⚠️ Documenté |
| `lum_id` ≠ `bit_id` structurellement (group_index/bit_in_group) | header L66–72 | ℹ️ Normal — encodage délibéré différent |
| `PAYLOAD_BYTES=8` → 64 bits : couverture limitée | `main.c` L66 | ⚠️ Non représentatif d'un flux long |
| TSan PASS ≠ absence de TOCTOU multi-producteurs | Rapport 188 §6 | ⚠️ Documenté |

### Limite nouvelle identifiée (S182) :

**check_continuity() appel avec `b` (index boucle), pas avec l'event_seq FU002.**

```c
// main.c L207 — ACTUEL :
forensic_unif002_check_continuity((uint64_t)b);  // b = 0..63, indice de bit

// Ce n'est PAS l'event_seq interne FU002 (qui serait ~3, 4, 5... après les SESSION_START).
// L'argument attendu par check_continuity() est la "séquence attendue" du BIT.
// Ici c'est correct pour les bits (on vérifie la continuité de 0→63),
// mais la sémantique est différente de l'event_seq global du log.
```

**Conséquence :** `check_continuity()` vérifie la séquentialité des bits d'entrée (0→63), **pas** la séquentialité des événements FU002. Ce n'est pas un bug (la boucle est séquentielle, b monotone, aucune perte détectée) mais c'est une distinction sémantique à documenter : la continuité vérifiée est celle du **flux de bits**, pas du **flux d'événements**.

---

## 5. Conclusion P0 FU002

| Critère P0 | État |
|------------|------|
| Chaîne `bit → BIT_ID` démontrée dans artefact JSONL | ✅ VÉRIFIÉ |
| Chaîne `BIT_ID → LUM_ID` (lum_id_from_bit) démontrée | ✅ VÉRIFIÉ |
| Chaîne `LUM_ID → transformation_ex` avec bit_before/bit_after | ✅ VÉRIFIÉ |
| Chaîne `transformation → résultat` (SIMD→PARALLEL→ZERO_COPY→PARETO) | ✅ VÉRIFIÉ |
| Audit d'intégrité 6/6 PASS sur session récente | ✅ |
| Séquence monotone event_seq (pas de saut) | ✅ |
| Bug P1 DUPLICATE:seq=0 corrigé (sessions 5→11 clean) | ✅ |
| Provenance réelle vs déclarée : **cohérente** | ✅ |

**P0 FU002 : CLOSED — provenance bit-level démontrée sur artefact JSONL réel (6 audits PASS, session 11 propre).**

**Prochaine priorité (registre OPEN) :** P1 Richardson-PROTOCOL-003 — solution manufacturée/exacte pour convergence vérifiable.

---

## 6. Registre OPEN (mis à jour)

| Priorité | Chantier | État |
|----------|----------|------|
| ~~P0~~ | ~~FU002 provenance bit-level~~ | ✅ **CLOSED — rapport 190** |
| P1 | Richardson-PROTOCOL-003 : solution exacte/manufacturée | OPEN |
| P2 | Ordre temporel Chorin complet (advection+diffusion+Poisson+projection+BCs) | OPEN |
| P3 | T04 renforcé : fenêtre finale complète min/max/moy/écart-type/pente | OPEN |
| P4 | Lyapunov robuste : λ=-1.426 signe robuste, robustesse quantitative | OPEN |
| P5 | BUILD-THREAD-001 : couverture concurrente globale | OPEN |
| P6 | BUILD-PROOF-001 : CI C reproductible | OPEN |
| P7 | BUILD-PORT-002 : cible portabilité réelle | OPEN |

---

*Rapport produit par analyse statique du code source (HEAD 8aaabe5) + audit dynamique du JSONL (3721 lignes, 11 sessions). Aucune donnée inventée. CERTIFIED_100=false.*
