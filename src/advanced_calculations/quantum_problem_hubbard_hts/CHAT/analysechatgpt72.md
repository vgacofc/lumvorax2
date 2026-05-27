# Rapport analysechatgpt72.md
## Inspection totale — Logs bruts AVANT/APRÈS + Anomalies cachées + État d'avancement
**Date UTC :** 2026-03-30T14:30:00Z  
**Runs inspectés :** `research_20260330T140836Z_3256` (AVANT) → `research_20260330T142036Z_399` (EN COURS)  
**Run archivé référence :** `research_20260324T194802Z_3229`  
**Référence analyse précédente :** `analysechatgpt71.4.md`

---

## 1. ÉTAT D'AVANCEMENT EN TEMPS RÉEL

### Workflow : Quantum Research Cycle C37
- **Statut :** RUNNING (démarré 14:20:36Z, ~10 min en cours)
- **Phase actuelle :** `fullscale` — module 2/15 en simulation MC
- **Module terminé :** `hubbard_hts_core` ✅
- **Module en cours :** `qcd_lattice_fullscale` — step 5000/14000 (35%)
- **ETA estimée module 2 :** +~8 min (basé sur vitesse ~47s/500 steps)
- **ETA fin fullscale (15 modules) :** ~2h30 total

### Progression granulaire LumVorax (checkpoint nanoseconde)
| Step | Energy (eV) | Pairing | Sign | Timestamp |
|------|-------------|---------|------|-----------|
| 0 | 1.9871614649 | 0.7843 | 0.0306 | 14:20:36Z |
| 500 | 1.9919976059 | 0.7704 | 0.0918 | 14:20:37Z |
| 1000 | 1.9921253121 | 0.7676 | — | 14:20:37Z |
| 3000 | 2.9761843685 | 0.6143 | 0.0278 | 14:25:41Z |
| 3500 | 2.9761873032 | 0.6172 | 0.0139 | 14:26:28Z |
| 4000 | 2.9761930323 | 0.6158 | 0.0139 | 14:27:18Z |
| 4500 | 2.9762099519 | 0.6154 | **-0.0278** | 14:28:05Z |
| 5000 | 2.9762247381 | 0.6115 | **-0.0417** | 14:28:52Z |

---

## 2. CORRECTIONS DES RAPPORTS PRÉCÉDENTS — ÉTAT RÉEL

### Correction C1 — Macros FORENSIC_LOG manquantes (rapport 71.3)
**Revendiqué dans 71.3 :** "0 occurrences restantes ✅"  
**Résultat grep actuel :**
```
355: * FORENSIC_LOG_TID : enregistre...     → COMMENTAIRE (doc)
356: * FORENSIC_LOG_CONV : trace...         → COMMENTAIRE (doc)
439: * via FORENSIC_LOG_OP_FULL.            → COMMENTAIRE (doc)
503: * C72 : enrichi avec FORENSIC_LOG_OP_FULL → COMMENTAIRE (doc)
```
**→ CONFIRMÉ ✅ : 0 appel actif restant. Les 4 occurrences grep sont des commentaires de documentation, pas du code compilé. La compilation advanced_parallel réussit sans erreur.**

### Correction C2 — Guard auto-start C37 (rapport 71.3)
**Revendiqué dans 71.3 :** "Fichier `.c37_autorun_disabled` créé ✅"  
**Résultat actuel :**
```
GUARD ABSENT — fichier manquant au démarrage du run 14:20:36Z
→ C37 s'est relancé automatiquement ⚠️
```
**→ RÉGRESSION ❌ : Le fichier guard a disparu entre le run précédent et maintenant.**  
**Action corrective immédiate :** Guard recréé à 14:30:00Z ✅  
```
-rw-r--r-- runner runner 0 Mar 30 14:30 .c37_autorun_disabled
```

### Correction C3 — Bug AC-09 fullscale (ed_validation_2x2 U=8)
**Revendiqué dans 71.2 :** "correction à appliquer dans fullscale runner"  
**Résultat actuel :** Bug toujours actif dans `hubbard_hts_core` :
```
model=1.992202  vs  ref=1.985600  →  abs_e=0.006602  error_bar=0.005  → within=0 ❌
```
**→ NON CORRIGÉ ❌ : la correction AC-09 n'a été appliquée qu'au runner advanced_parallel, pas au fullscale runner.**

### Correction C4 — FK Supabase (quantum_run_files → quantum_csv_rows)
**Revendiqué dans 71.3 :** "fix FK Supabase dans streamer"  
**Résultat actuel :** Test Supabase : 25/26 ✅ — FK non visible dans les logs → **statut inconnu, à vérifier au prochain upload.**

### Correction C5 — Doppler Token 401
**Revendiqué dans 71.4 :** "nouveau token vgac à intégrer"  
**Résultat actuel :**
```
[⚠ WARN] Doppler API (DOPPLER_TOKEN) — HTTP 401 sur /secrets ET /me
```
**→ NON CORRIGÉ ❌ : token Doppler toujours invalide.**

---

## 3. LOGS BRUTS — COMPARAISON AVANT/APRÈS

### research_execution.log
| Seq | Type | Champ | AVANT (run 140836) | APRÈS (run 142036) | Delta |
|-----|------|--------|-------------------|--------------------|-------|
| 001 | START | run_id | research_20260330T140836Z_3256 | research_20260330T142036Z_399 | — |
| 003 | BASELINE | classic_run | NOT_FOUND | NOT_FOUND | identique ⚠️ |
| 004 | BASE_RESULT | energy (hubbard_hts_core) | 1.992202 | 1.992202 | **Δ=0.000000** |
| 004 | BASE_RESULT | pairing | 0.751526 | 0.751526 | **Δ=0.000000** |
| 004 | BASE_RESULT | sign | 0.306122 | 0.306122 | **Δ=0.000000** |
| 004 | BASE_RESULT | elapsed_ns | 13,178,032,200 | 13,107,716,532 | **Δ=-70ms** |
| 004 | BASE_RESULT | cpu_peak | 100.00 | 100.00 | identique |
| 004 | BASE_RESULT | mem_peak | 87.96% | 84.74% | **Δ=-3.22%** |
| 005 | BENCH_QMC_RT | abs_e energy | 0.006602 | 0.006602 | **Δ=0** ← bug AC-09 |
| 005 | BENCH_QMC_RT | within energy | 0 | 0 | **HORS BARRES** ❌ |
| 006 | BENCH_QMC_RT | within pairing | 1 | 1 | dans les barres ✅ |

**Observation clé :** Les résultats sont **strictement déterministes** (seed LCG fixe). Aucun progrès physique entre les deux runs → correctifs AC-09 non actifs dans le fullscale runner.

### benchmark_comparison_qmc_dmrg.csv (AVANT vs APRÈS)
```
AVANT (run 140836):
hubbard_hts_core,energy_eV,95,8,1.9856,1.9922,0.0066,0.00332,0.005,0  ← within=0 ❌

APRÈS (run 142036):
hubbard_hts_core,energy_eV,95,8,1.9856,1.9922,0.0066,0.00332,0.005,0  ← within=0 ❌

→ IDENTIQUE. Bug AC-09 fullscale non corrigé.
```

### LumVorax CSV — AVANT vs APRÈS (tailles)
| Run | Lignes CSV | Modules traités | Note |
|-----|-----------|-----------------|------|
| run 140836 | non mesuré | 2+ | — |
| run 142036 | 377 lignes | 2 modules | en cours |
| run 3229 (24/03) | — | 15 modules | run complet |

---

## 4. NOUVELLES ANOMALIES DÉCOUVERTES

### ANOMALIE A1 — Sign fermionique oscillant et divergeant (qcd_lattice_fullscale)
**Sévérité : HAUTE**  
Dans le module `qcd_lattice_fullscale`, le signe fermionique évolue de façon alarmante :
```
step    0  : sign = +0.030612   (3.0%)
step  500  : sign = +0.091836   (9.2%)  ← montée normale
step 3000  : sign = +0.027778   (2.8%)  ← chute brutale
step 3500  : sign = +0.013889   (1.4%)  ← décroissance continue
step 4000  : sign = +0.013889   (1.4%)
step 4500  : sign = -0.027778   (-2.8%) ← INVERSION DE SIGNE ⚠️
step 5000  : sign = -0.041667   (-4.2%) ← amplification négative
```
**Diagnostic :** Le signe change de polarité entre step 4000 et 4500 — signe d'un **"sign problem" QMC sévère**. Pour un réseau QCD à basse température, ceci invalide les moyennes Monte Carlo (bruit exponentiel). Le résultat final sera statistiquement biaisé.

### ANOMALIE A2 — Énergie step=0 identique entre modules
**Sévérité : MOYENNE**  
```
hubbard_hts_core    ckpt_step=0 → energy=1.9871614649
qcd_lattice_fullscale ckpt_step=0 → energy=1.9871614649 (IDENTIQUE)
```
**Diagnostic :** L'énergie initiale (step 0) est strictement identique pour deux modules physiquement différents. Ceci indique que l'état RNG/MC n'est pas réinitialisé correctement entre modules, ou que le checkpoint step 0 capture l'état du module précédent. Le module `qcd_lattice_fullscale` commence avec les observables de `hubbard_hts_core` au step 0 — **bug potentiel de réinitialisation de l'accumulateur**.

### ANOMALIE A3 — normalized_observables_trace.csv vide
**Sévérité : BASSE (normale en mid-run)**  
```
problem,step,energy_norm,pairing_norm,sign_ratio,cpu_percent,mem_percent,elapsed_ns
(aucune donnée)
```
Le fichier n'est populé qu'en fin de phase fullscale. Normale pour un run en cours.

### ANOMALIE A4 — BASELINE latest_classic_run=NOT_FOUND
**Sévérité : MOYENNE**  
```
000003 | BASELINE latest_classic_run=NOT_FOUND
```
Les 3 runs du jour (`140836`, `135032`, `142036`) ne sont pas reconnus comme "classiques". Ceci signifie que le système de baseline compare contre **rien** — les regressions ne peuvent pas être détectées automatiquement.

### ANOMALIE A5 — numerical_stability_suite.csv et new_tests_results.csv vides
**Sévérité : BASSE (normale en mid-run)**  
Ces fichiers ne sont remplis qu'après la phase fullscale. Normal.

### ANOMALIE A6 — Guard C37 disparu entre deux runs
**Sévérité : HAUTE**  
Le fichier `.c37_autorun_disabled` créé par le rapport 71.3 a disparu. Le run 142036 s'est lancé automatiquement à 14:20 sans action de l'utilisateur. Cause probable : le guard est créé dans l'environnement mais pas versionné (git ne trace pas les fichiers vides dans les répertoires C). À chaque reset/restore Replit, ce fichier disparaît.  
**Solution recommandée :** Intégrer le guard directement dans `run_research_cycle.sh` via variable d'environnement `C37_AUTORUN_ENABLED=1` plutôt que par fichier.

---

## 5. ÉTAT DES FICHIERS HW/INIT (LumVorax brut — module 1)

```
INIT version=3.0_cycle17_NL03_NV01_NV02_AC01_NANO_ANOMALY
HW_SAMPLE init:cpu_delta_pct=0.0000
HW_SAMPLE init:mem_used_pct=84.6831%   ← 84.7% mémoire utilisée dès le départ
HW_SAMPLE init:mem_total_kb=65,849,792 KB (63 GB)
HW_SAMPLE init:mem_avail_kb=10,086,148 KB (9.6 GB disponible)
HW_SAMPLE init:vm_rss_kb=2,240 KB
HW_SAMPLE init:vm_peak_kb=8,868 KB

simulate_fs:sites=196 (14×14)
simulate_fs:steps=14000
simulate_fs:temp_K=95.0
simulate_fs:U_eV=8.0, t_eV=1.0, mu_eV=0.2
simulate_fs:dt_raw=0.01, dt_scale=0.02174  (dt/h_scale=0.01/9.2)
simulate_fs:h_scale_eV=9.2
```

**Paramètres step 0 site 0 (granularité maximale) :**
```
local_pair_site0_step0 = 0.6738128867
d_site0_step0         = -0.1122071196  ← double occupancy négative ⚠️
n_up_site0            = 0.4438964402
n_dn_site0            = 0.5561035598   (n_up+n_dn=1.0 ✅)
corr_alpha_site0      = 0.0500000000
corr_val_site0        = 0.0002568906
k1_rk2_site0          = -0.0147296102
hopping_lr_site0      = 0.0005764991
U_term_site0          = 1.9748191246
t_hop_site0           = -0.0005764991  (=−hopping_lr ✅)
mu_occ_site0          = -0.0000000000  ← potentiel chimique nul au site 0 ⚠️
local_e_site0         = 1.9742426255
```

**Sous-anomalie A7 — double occupancy négative :**  
`d_site0_step0 = -0.112` — la double occupancy (⟨n↑n↓⟩) ne devrait pas être négative physiquement. Valeur dans [-1, 0] indique un artefact de l'initialisation RNG ou une formule incorrecte pour le premier step.

---

## 6. RÉSUMÉ EXÉCUTIF — TABLEAU DE BORD

| # | Anomalie / Bug | Sévérité | Statut | Rapport origine |
|---|---------------|----------|--------|-----------------|
| AC-09 | energy fullscale hors barres QMC | HAUTE | ❌ Non corrigé | 71.1, 71.2, 71.3 |
| A1 | Sign fermionique oscillant/négatif (qcd) | HAUTE | 🆕 Nouvelle | **72** |
| A2 | Énergie step=0 identique inter-modules | MOYENNE | 🆕 Nouvelle | **72** |
| A4 | BASELINE NOT_FOUND tous les runs | MOYENNE | Persistant | 71.x |
| A6 | Guard C37 disparaît à chaque reset | HAUTE | ✅ Recréé | **72** |
| A7 | double occupancy négative step=0 | MOYENNE | 🆕 Nouvelle | **72** |
| C5 | Doppler token 401 | HAUTE | ❌ Non corrigé | 71.4 |
| C4 | FK Supabase | BASSE | ⚠️ Non vérifiable | 71.3 |

---

## 7. ACTIONS IMMÉDIATES REQUISES

### P1 — Guard C37 permanent (CRITIQUE)
Modifier `run_research_cycle.sh` pour utiliser une variable d'env plutôt qu'un fichier :
```bash
# Au lieu de fichier flag (volatil) :
if [ "${C37_AUTORUN_ENABLED:-0}" != "1" ]; then
    echo "[C37-GUARD] Lancement manuel requis : C37_AUTORUN_ENABLED=1 bash run_research_cycle.sh"
    exit 0
fi
```

### P2 — Corriger bug AC-09 dans fullscale runner
Fichier : `src/hubbard_hts_research_cycle.c` lignes ~970-977  
Condition : détecter `strcmp(probs[i].name, "ed_validation_2x2")` et U≠4, appeler `ed_hubbard_2x2()` au lieu de `simulate_fullscale()`.

### P3 — Investiguer signe négatif qcd_lattice_fullscale
Vérifier la formulation du signe fermionique dans le module QCD — l'inversion entre step 4000 et 4500 est anormale.

### P4 — Investiguer énergie step=0 identique inter-modules
Vérifier la réinitialisation des accumulateurs entre modules dans `simulate_fullscale()`.

### P5 — Régénérer token Doppler
Token `dp.st.dev_lvxdoppler.*` → HTTP 401. Régénérer via dashboard Doppler.

---

*Rapport généré automatiquement par inspection des logs bruts — 2026-03-30T14:30:00Z*  
*Fichiers inspectés : research_execution.log, lumvorax CSV (377 lignes), benchmark_comparison_qmc_dmrg.csv, unit_conversion_fullscale.csv, provenance.log, workflow C37 live logs*
