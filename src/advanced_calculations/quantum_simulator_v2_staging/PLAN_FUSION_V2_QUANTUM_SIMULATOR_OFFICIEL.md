# Plan de réalisation — Fusion V2 vers le simulateur officiel `src/advanced_calculations/quantum_simulator.c`

## 1) État factuel immédiat (sans invention)

## 1.1 Fichiers officiels copiés en zone isolée (staging)
Copie réalisée sans toucher aux originaux:
- `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.c`
- `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.h`
- `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.o`

Objectif: préparer la V2 de fusion sans mélanger avec la version officielle tant que validation complète non terminée.

## 1.2 Résultats/logs trouvés pour `vkernel_nqubit_v2`
- Trouvés et versionnés:
  - `src/quantum/vkernel_nqubit_v2/results/20260302_194455/comparison_summary_v2.json`
  - `src/quantum/vkernel_nqubit_v2/results/20260302_194455/comparison_summary_v2.md`
  - `src/quantum/vkernel_nqubit_v2/results/20260302_194455/nqubit_forensic_ns_v2.jsonl`

## 1.3 Résultats/logs trouvés pour le simulateur officiel `src/advanced_calculations/quantum_simulator.c`
- **Aucun artefact de run dédié, explicitement lié à `quantum_simulator.c`, n'a été trouvé dans `src/advanced_calculations/`**.
- Le fichier de test `src/tests/test_quantum_simulator_complete.c` affiche des messages console, mais ne persiste pas de rapport/log forensic structuré.

Conclusion stricte: aujourd'hui, il existe des résultats consolidés pour V2 (`src/quantum/...`), mais pas de dossier de résultats versionné équivalent prouvant des exécutions complètes du simulateur officiel `advanced_calculations/quantum_simulator.c`.

---

## 2) Comparaison technique — `quantum_simulator.c` officiel vs `vkernel_nqubit_v2`

## 2.1 Simulateur officiel (`src/advanced_calculations/quantum_simulator.c`)
- Modèle "qubit/LUM" avec structures riches (`quantum_lum_t`, `quantum_result_t`, `quantum_config_t`).
- Portes quantiques de base (Hadamard, Pauli X/Z, phase).
- Fonctions de création/destruction, mesure, stress test.
- Intégration mémoire/forensic via `memory_tracker`/`forensic_logger` (niveau code), mais sans pipeline de run standardisé versionné dans `src/advanced_calculations/results/...`.

## 2.2 Simulateur V2 (`src/quantum/vkernel_nqubit_v2/v_kernel_quantum_nx_v2.c`)
- Benchmark dynamique multi-scénarios/multi-steps.
- Logs forensic JSONL nanoseconde dédiés à la campagne (`scenario_margin`, `summary`).
- KPI comparatifs directs NX vs baseline (`avg_score`, `win_rate`, `nqubits_per_sec`).

## 2.3 Écart principal à combler dans la fusion
Le module officiel possède une base de simulation plus "moteur", mais manque d'un pipeline de benchmark forensic reproductible au niveau de granularité V2 (KPI comparatifs, traces run-id, rapports auto).

---

## 3) Interprétation exacte de "NX gagne ~65.3%"

## 3.1 Calcul concret
- `nqubit_win_rate = 0.6527777778`
- Sur 360 scénarios: `0.6527777778 * 360 = 235` scénarios gagnés par NX.
- Donc `360 - 235 = 125` scénarios perdus par NX, soit **34.7222%**.

## 3.2 Que représentent ces 34.7% ?
Ils représentent les scénarios où le score baseline local (`baseline_qubit_avg_score` par scénario) dépasse le score NX.

## 3.3 "Zones de fragilité": ce qui est connu vs inconnu
- **Connu** (depuis log actuel):
  - au moins un cas échantillonné perdant (`scenario=256`, `margin<0`).
- **Inconnu avec certitude** (car le log actuel est échantillonné tous les 64 scénarios):
  - la liste complète des 125 scénarios perdants,
  - leur typologie exacte.

Donc, sans nouveau run enrichi, on ne peut pas donner "toutes les zones" avec exactitude ligne par ligne.

---

## 4) Plan d'intégration/fusion V2 -> officiel (avant/après, sans régression)

## 4.1 Objectifs non négociables
1. Ne pas toucher à la version officielle tant que la staging V2 n'est pas validée.
2. Zéro warning, zéro stub, zéro placeholder, zéro hardcoding caché.
3. Ajouter des logs forensic (pas en retirer).
4. Tests unitaires + intégration + stress élevé (objectif 99% CPU/RAM) avec preuves.
5. Mode RNG configurable, incluant **désactivation PRNG logiciel** pour usage hardware-only quand disponible.

## 4.2 Architecture de staging proposée
Dans `src/advanced_calculations/quantum_simulator_v2_staging/`:
- `quantum_simulator_v2.c/.h` (ou patch incrémental sur copie)
- `tools/`:
  - `run_compare_official_vs_v2.py`
  - `collect_hw_entropy.py`
  - `stress_99_resources.py`
  - `build_manifest_v2.py`
  - `verify_manifest_v2.py`
- `tests/`:
  - `test_rng_modes.c`
  - `test_forensic_jsonl_schema.py`
  - `test_scenario_losses_full.py`
  - `test_stress_99_resources.py`
- `results/<run_id>/`:
  - `forensic_ns_full.jsonl`
  - `comparison_summary.json`
  - `manifest_forensic_v2.json`
  - `verify_manifest_report.json`
  - `resource_stress_report.json`

## 4.3 Mode RNG demandé: désactivation PRNG logiciel
Ajouter une configuration explicite:
- `RNG_MODE=hardware_only`: interdit fallback PRNG logiciel; le run échoue proprement si aucune source hardware valide.
- `RNG_MODE=hardware_preferred`: hardware d'abord, fallback logiciel tracé.
- `RNG_MODE=deterministic_seeded`: mode reproductible test.

Chaque run doit journaliser:
- sources détectées (`/dev/hwrng`, `/dev/random`, `/dev/urandom`, autres API système),
- mode choisi,
- preuves de lecture (volume, débit, erreurs).

## 4.4 Campagne avant/après (protocole strict)
### Avant (baseline officiel)
- Exécuter simulateur officiel inchangé.
- Capturer KPI + logs forensic minimum.

### Après (staging V2 fusion)
- Exécuter même matrice de charge.
- Comparer automatiquement:
  - performance (ops/s, nqubits/s, latences p50/p95/p99),
  - stabilité (variance inter-run),
  - intégrité (manifest/hash),
  - coût ressources (CPU/RAM/IO).

Validation Go/No-Go:
- pas de régression critique,
- intégrité `true`,
- tests >= 95% et zéro test critique en échec,
- explication de toute anomalie.

## 4.5 Logs nécessaires pour répondre aux 10 questions d'expert
Pour répondre exactement à toutes les questions demandées:
1. journaliser 100% des scénarios (pas seulement 1/64),
2. exécuter au moins 30 runs multi-seed,
3. exécuter un plan factoriel sur `sigma`, `thermal`, `lyapunov_gain`,
4. exécuter une montée en échelle (`scenarios`, `steps` x10),
5. tester plusieurs baselines,
6. exporter table des scénarios perdants triés,
7. calculer IC95 du gain,
8. corréler gain avec coût CPU/RAM/énergie,
9. intégrer manifest/signature au pipeline V2,
10. publier dashboard KPI business priorisés.

---

## 5) Réponses demandées (version claire)

## 5.1 "Comparaison avec simulateur officiel" 
- Oui, comparaison possible conceptuellement et structurellement.
- Non, preuve run-à-run officielle complète impossible maintenant faute d'artefacts versionnés dédiés au simulateur officiel.

## 5.2 "Retrouve les résultats/logs/rapports officiels"
- Résultat honnête: non trouvés sous une forme consolidée comparable à `vkernel_nqubit_v2/results/...`.

## 5.3 "Que sont les 34% que NX ne réussit pas ?"
- 125 scénarios perdus sur 360 dans cette campagne.
- Leur liste exhaustive n'est pas reconstructible avec le log échantillonné actuel; il faut un run full-trace pour les identifier tous.

## 5.4 "Toutes les zones de fragilité"
- Zone confirmée: scénario 256 (marge négative).
- Zones complètes: non déterminables sans instrumentation full per-scenario.

---

## 6) Plan exécutable en 6 phases
1. **Freeze**: geler baseline officiel + métadonnées environnement.
2. **Instrumentation**: ajouter forensic ns complet et mode RNG hardware-only dans staging.
3. **Validation unitaire**: RNG modes, schema logs, intégrité manifest.
4. **Stress 99%**: campagne CPU/RAM 99% avec garde-fous sécurité.
5. **Comparaison A/B**: officiel vs staging V2 sur même protocole.
6. **Gate final**: fusion vers officiel seulement si zéro régression critique et traçabilité complète.

Ce plan est la base "avant implémentation fusion complète", sans toucher à la version officielle active.

## 7) Mise à jour exécution (itération actuelle)
- `quantum_simulator.o` a été retiré du staging (pas de binaire versionné).
- Fusion V2 ajoutée en fichiers séparés:
  - `quantum_simulator_fusion_v2.h`
  - `quantum_simulator_fusion_v2.c`
- Runner Replit ajouté:
  - `run_on_replit.sh`
- Commande exacte:
  - `bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v2_staging/run_on_replit.sh /workspace/Lumvorax`
