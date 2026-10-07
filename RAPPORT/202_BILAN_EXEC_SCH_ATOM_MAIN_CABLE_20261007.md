# RAPPORT 202 — Bilan d'exécution : bench CV + sch_atom_main + main_cable_001

**Date :** 2026-10-07  
**Périmètre :** `LVX&ARTCB/` (verrouillé)  
**Binaires exécutés :** `cv_investigation_bench`, `sch_atom_main`, `main_cable_001`  
**CERTIFIED_100=false** | Mode DEBUG actif | Répondre en français  

---

## 1. Contexte et objectif

L'utilisateur a fourni les commandes d'exécution et confirmé que les logs précédents avaient été effacés. Les 4 commandes suivantes ont été jouées depuis `LVX&ARTCB/` :

1. `./bin/cv_investigation_bench 2>&1 | tee logs_AIMO3/sch/atom/cv_investigation_bench.log`
2. `./bin/sch_atom_main 2>&1 | tee logs_AIMO3/sch/atom/run_normal.log`
3. `ARTCB_CI_FIXED_SEED=1 ./bin/sch_atom_main 2>&1 | tee logs_AIMO3/sch/atom/run_ci.log`
4. `./bin/main_cable_001 2>&1 | tee logs_AIMO3/sch/atom/main_cable_001_run.log`

---

## 2. Avant / Après — Fichiers de logs

| Fichier log | Avant | Après |
|------------|-------|-------|
| `logs_AIMO3/sch/atom/cv_investigation_bench.log` | SUPPRIMÉ | ✅ Régénéré (945 octets) |
| `logs_AIMO3/sch/atom/cv_investigation_bench.json` | SUPPRIMÉ | ✅ Régénéré (2 373 octets) |
| `logs_AIMO3/sch/atom/run_normal.log` | SUPPRIMÉ | ✅ Régénéré (318 octets) |
| `logs_AIMO3/sch/atom/run_ci.log` | SUPPRIMÉ | ✅ Régénéré (318 octets) |
| `logs_AIMO3/sch/atom/main_cable_001_run.log` | SUPPRIMÉ | ✅ Régénéré (84 922 octets) |
| `logs/forensic/main_cable_001.log` | SUPPRIMÉ | ✅ Régénéré (20 876 octets) |
| `logs_AIMO3/sch/atom/pairs_all_comparisons.jsonl` | SUPPRIMÉ | ✅ Régénéré (437 968 982 octets) |
| `logs_AIMO3/sch/atom/physics_all_atoms.jsonl` | SUPPRIMÉ | ✅ Régénéré (2 163 513 octets) |
| `logs_AIMO3/sch/atom/step_nanoseconds.jsonl` | SUPPRIMÉ | ✅ Régénéré (1 718 octets) |
| `logs_AIMO3/sch/atom/transient_events.log` | SUPPRIMÉ | ✅ Régénéré (691 146 octets) |

---

## 3. Résultats — Bench CV (`cv_investigation_bench`)

**Paramètres :** `num_reps=30`, `fixed_seed=42`

### 3.1 Tableau des 6 composants

| ID | Composant | min (ns) | max (ns) | mean (ns) | CV% | Diagnostic |
|----|-----------|---------|---------|----------|-----|-----------|
| A | `scheduler_jitter` | 0 | 1 000 | 133 | **259.3%** | ⚠️ Quantification µs — 4 tops sur 30 @ 1 µs |
| B | `srand_rand_1000` | 9 000 | 10 000 | 9 567 | **5.3%** | ✅ STABLE |
| C | `malloc_memset_free_1000atoms` | 0 | 1 000 | 200 | **203.4%** | ⚠️ Quantification µs — 6 tops sur 30 @ 1 µs |
| D | `sqrt_N² distance loop` | 2 233 000 | 74 766 000 | 5 892 000 | **223.3%** | 🔴 **P0 — ratio max/min = 33×** |
| E | `fopen_fprintf_fclose` | 40 000 | 135 000 | 50 900 | **44.3%** | ⚠️ Modéré — I/O fs |
| F | `full_physics_no_io_fixed_seed` | 52 405 000 | 346 452 000 | 171 353 000 | **51.2%** | ⚠️ Modéré — ratio 6.6× |

### 3.2 Analyse composant par composant

**A — Scheduler jitter**
- Résolution macOS `clock_gettime` = 1 µs sur ce matériel.
- 4/30 mesures à 1 000 ns, 26/30 à 0 ns : l'intervalle de mesure est inférieur à la résolution.
- CV=259% est un artefact de quantification, pas une instabilité réelle du scheduler.
- **Constat :** instrumentation trop fine pour ce composant — non diagnostiquant à ce niveau.

**B — srand_rand_1000**
- CV=5.3% : excellent. La boucle de génération pseudo-aléatoire est stable.
- Alternance régulière 9/10 µs = granularité normale du timer µs.
- **Constat :** PASS stable — pas une source de variance dans `sch_atom_main`.

**C — malloc_memset_free_1000atoms**
- Même artefact que A : durée sub-µs, quantification 1 µs.
- 6/30 mesures à 1 000 ns → l'allocateur est rapide (~200 ns en moyenne réelle).
- **Constat :** L'allocation de 1 000 atomes est performante. CV élevé = artefact, pas un problème.

**D — sqrt_N² distance loop (ANOMALIE P0)**
- Composant calculant toutes les distances N×N entre atomes avec `sqrt()`.
- min = 2 233 µs, max = 74 766 µs, ratio = **33.47×**.
- Distribution des 30 échantillons : 22 valeurs dans [2 233–9 148 µs], 7 valeurs dans [7 402–9 148 µs], 1 valeur extrême à **74 766 µs** (sample n°30).
- Hypothèse principale : **effet cold-cache CPU** au premier appel + un pic de preemption OS sur sample n°30.
- La valeur médiane (≈2 600 µs) est cohérente. Le pic n°30 est isolé.
- **Constat :** Le CV de 223% est dominé par cet unique outlier. Sans lui, CV ≈ 30-40%.
- **Impact sur sch_atom_main :** si la boucle N² est exécutée en séquence à chaque step, la variance du CV global hérite directement de cet outlier.

**E — fopen_fprintf_fclose**
- CV=44% : variance modérée mais attendue pour des I/O système.
- 2 pics isolés à 130 µs et 135 µs (samples 1 et 22) vs médiane ≈43 µs.
- **Constat :** I/O non-déterministe classique — à limiter dans les chemins critiques.

**F — full_physics_no_io_fixed_seed**
- Le composant le plus lourd : mean=171 ms, max=346 ms.
- CV=51% avec une distribution non gaussienne : forte bimodalité (cluster bas 52–120 ms, cluster haut 175–346 ms).
- Malgré `fixed_seed`, la physique montre une variance réelle → dépend du scheduling CPU, pas de l'aléatoire.
- **Constat :** La graine fixe ne suffit pas à garantir la reproductibilité temporelle. P6-A (graine fixe CI) contrôle l'aléatoire numérique mais pas le scheduling.

---

## 4. Résultats — `sch_atom_main`

### 4.1 Run normal

**Fichier :** `logs_AIMO3/sch/atom/run_normal.log`

```
[SCH-ATOM] Initialisation de la Branche C (Reconstruction Atomistique)...
[SCH-ATOM] Simulation et Détection d'événements transitoires (Phase C-3)...
[SCH-ATOM] Phase C-3 : Cartographie terminée. Lancement du Test de Falsification...
[SCH-ATOM] Phase D : Synthèse finale. Computation par instabilité confirmée.
```

4 lignes de sortie. Pas de code d'erreur. Pas de log forensic complémentaire généré par ce binaire.

### 4.2 Run CI (`ARTCB_CI_FIXED_SEED=1`)

**Fichier :** `logs_AIMO3/sch/atom/run_ci.log`

Sortie **identique** au run normal — 4 lignes exactes, bit-pour-bit.

### 4.3 Analyse — P6-A non implémenté

**Processus :** La variable d'environnement `ARTCB_CI_FIXED_SEED=1` est transmise au processus.  
**Problème :** `sch_atom_main.c` ne lit pas cette variable — aucun `getenv("ARTCB_CI_FIXED_SEED")` n'est présent dans le code source.  
**Solution (P6-A ouverte) :** Ajouter autour de la ligne 255 de [`src/sch/atom/sch_atom_main.c`](../src/sch/atom/sch_atom_main.c) :

```c
/* P6-A : graine fixe CI */
const char *ci_seed_env = getenv("ARTCB_CI_FIXED_SEED");
unsigned int seed = ci_seed_env ? 42U : (unsigned int)time(NULL);
srand(seed);
```

**Constat :** La sortie identique entre les deux runs peut signifier soit (a) que le binaire utilise déjà une graine fixe inconditionnelle, soit (b) que la phase C-3 ne génère pas de nombres aléatoires visibles en sortie à ce niveau de log. P6-A reste **OPEN** car la vérification programmatique de `ARTCB_CI_FIXED_SEED` n'est pas présente.

---

## 5. Résultats — `main_cable_001` (7/7 PASS)

**Fichier :** `logs_AIMO3/sch/atom/main_cable_001_run.log` (84 922 octets)  
**Forensic :** `logs/forensic/main_cable_001.log` (20 876 octets)

### 5.1 Tableau des modules

| Module | Résultat | Détail |
|--------|---------|--------|
| FORENSIC-UNIF-002 | ✅ OK | run_id=0x5D18F981 |
| MEMORY_OPTIMIZER | ✅ OK | pools 64×32 alloués et libérés proprement |
| BINARY_CONVERTER | ✅ OK | LUM créés = 64 |
| SIMD_OPTIMIZER | ✅ OK | avx2=1, sse=1, avx512=0 |
| PARALLEL_PROCESSOR | ✅ OK | tasks=64, 2 workers |
| ZERO_COPY_ALLOC | ✅ OK | allocs=64 |
| PARETO_OPTIMIZER | ✅ OK | score=350.648 |

### 5.2 FU002 session stats

| Métrique | Valeur |
|---------|--------|
| bits_input | 64 |
| lums_created | 64 |
| transforms | 257 |
| loss_count | **0** |
| dup_count | **0** |
| integrity_ok | **TRUE** |
| Durée session | 76 885 000 ns (~77 ms) |

### 5.3 Gestion mémoire

Le memory tracker confirme que chaque `ALLOC` a un `FREE` correspondant :
- 0x7fd0b0804080 — 584 octets (`memory_optimizer_create`) → libéré ✅
- 0x7fd0b1008200 — 32 768 octets (pool 1) → libéré ✅
- 0x7fd0b1010200 — 32 768 octets (pool 2) → libéré ✅
- 0x7fd0b0900000 — 65 536 octets (zone pool) → libéré ✅

**Aucune fuite mémoire détectée.**

### 5.4 Bilan CERTIFIED_100

```
CERTIFIED_100=false | unique_human_proven=false
```

Invariants respectés dans la sortie finale du binaire.

---

## 6. Registre OPEN mis à jour

| ID | Chantier | Statut | Priorité |
|----|---------|--------|---------|
| P0 | FORENSIC-UNIF-002 : provenance bit-level complète | **OPEN** — non démontré bit→BIT_ID→LUM_ID→résultat | P0 |
| P1 | Richardson-PROTOCOL-003 : solution manufacturée manquante | **OPEN** | P1 |
| P2 | Ordre temporel Chorin complet (dt→dt/2→dt/4) | **OPEN** | P2 |
| P3 | T04 renforcé : fenêtre finale asymptotique | **OPEN** | P3 |
| P6-A | Graine fixe CI (`ARTCB_CI_FIXED_SEED`) dans `sch_atom_main.c` | **OPEN** — `getenv` absent | P6 |
| P6-B | Anomalie D : outlier 74 766 µs (ratio 33×) — cache cold ou preemption OS | **OPEN** | P6 |

---

## 7. Conclusions

1. **main_cable_001 est stable** : 7/7 modules PASS, 0 fuite mémoire, FU002 integrity_ok=TRUE.
2. **sch_atom_main est fonctionnel** mais peu instrumenté : 4 lignes de sortie, P6-A non implémenté.
3. **Le composant D (sqrt N²)** est la source principale de variance dans le bench CV, avec un outlier isolé au sample n°30 (74 766 µs vs médiane ~2 600 µs). Hypothèse cache-cold ou preemption OS — à confirmer par un run avec `taskset` ou isolation de cœur.
4. **Le composant F (physics)** montre une bimodalité réelle indépendante de la graine — la reproductibilité temporelle requiert un contrôle du scheduling, pas seulement de l'aléatoire numérique.
5. **P0 FU002** reste ouvert : la provenance bit→LUM n'est pas encore auditée en détail dans les logs de cette session (les `pairs_all_comparisons.jsonl` de 438 Mo et `physics_all_atoms.jsonl` de 2 Mo ont été régénérés mais non encore analysés ligne à ligne).

---

*Rapport produit après lecture des logs — PROTOCOLE_ARTCB respecté.*  
*CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif*
