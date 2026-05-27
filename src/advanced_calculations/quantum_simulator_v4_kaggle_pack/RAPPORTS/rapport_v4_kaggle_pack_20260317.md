# RAPPORT ANALYSE — quantum_simulator_v4_kaggle_pack
## Date : 2026-03-17  |  Derniers runs : 20260303_035713, 040251, 040704
## Sources brutes analysées
- `replit_kaggle_run_20260303_040704/replit_v3_results/` — JSON résultats modes
- `replit_kaggle_run_20260303_040704/replit_v3_resultssummary.md`
- Logs officiels smoke test (stdout/stderr bruts)
- CSV résultats V3 des 3 runs Kaggle

---

## 1. CONTEXTE

Le simulateur V4 Kaggle Pack exécute des campagnes de validation intensive en mode Kaggle (30 runs par mode, 360 scénarios, 1400 steps), avec vérification de manifeste forensique signé.

---

## 2. DERNIER RUN — 20260303_040704

### 2.1 Configuration campagne
| Paramètre      | Valeur    |
|----------------|-----------|
| run_id         | 20260303_040704 |
| runs_per_mode  | 30        |
| scenarios      | 360       |
| steps          | 1 400     |

### 2.2 Smoke test officiel — stdout brut

```
[TEST] Démarrage test unitaire Quantum Simulator...
[MEMORY_TRACKER] Initialized - tracking enabled
[MEMORY_TRACKER] ALLOC: 0x28dad2b0 (56 bytes) at quantum_simulator.c:262 in quantum_config_create_default()
[OK] Configuration par défaut créée
[OK] LUM Quantique créée (2 états)
[DEBUG] Amplitude[0] = 1.000000 + 0.000000i
[OK] Porte Hadamard appliquée avec succès
[DEBUG] Post-Hadamard Amplitude[0] = 0.707107 + 0.000000i
[MEMORY_TRACKER] ALLOC: 0x28dad3e0 (328 bytes) at quantum_simulator.c:195 in quantum_measure()
[MEMORY_TRACKER] ALLOC: 0x28dad530 (16 bytes) at quantum_simulator.c:203 in quantum_measure()
[OK] Mesure quantique effectuée (Collapse)
[MEMORY_TRACKER] FREE: 0x28dad530 (16 bytes) at quantum_simulator.c:288 in quantum_result_destroy()
[MEMORY_TRACKER] FREE: 0x28dad3e0 (328 bytes) at quantum_simulator.c:290 in quantum_result_destroy()
[MEMORY_TRACKER] FREE: 0x28dad2b0 (56 bytes) at quantum_simulator.c:279 in quantum_config_destroy()
[SUCCESS] Test unitaire Quantum Simulator terminé avec succès
stderr: (vide)
```

> Smoke test RC=0. Pas de fuite mémoire. Hadamard Post-Amplitude = 0.707107 (correct : 1/√2). Collapse effectué.

### 2.3 Résultats par mode (données brutes JSON)

#### Mode : hardware_preferred (30 runs)
| Métrique              | Valeur                |
|-----------------------|-----------------------|
| count                 | 30                    |
| win_rate_mean         | **0.6604** (66.04 %)  |
| win_rate_std          | 0.02458               |
| throughput_mean       | 7 590 451 qubits/s    |
| latency_p95_mean_ns   | 232 914 ns            |
| latency_p99_mean_ns   | 401 692 ns            |

#### Mode : deterministic_seeded (seed=1, 30 runs)
| Métrique              | Valeur                |
|-----------------------|-----------------------|
| count                 | 30                    |
| win_rate_mean         | **0.6563** (65.63 %)  |
| win_rate_std          | 0.02402               |
| throughput_mean       | 6 317 728 qubits/s    |
| latency_p95_mean_ns   | 262 896 ns            |
| latency_p99_mean_ns   | 843 326 ns            |

> Le mode deterministic_seeded présente une latence p99 significativement plus haute (843 ms vs 402 ms) en raison de l'initialisation du seed et de la reproductibilité forcée.

### 2.4 Performance quantum globale (V3 baseline dans contexte Kaggle)
| Métrique              | Valeur            |
|-----------------------|-------------------|
| nqubits_simulated     | 504 000           |
| nqubits_per_sec       | 3 574 185.217     |
| nqubit_avg_score (NX) | **0.962724**      |
| baseline_avg_score    | 0.940151          |
| nqubit_win_rate       | 0.686 (68.61 %)   |
| nqubit_wins           | 247               |
| baseline_wins         | 113               |

### 2.5 Fusion Gate — Critères de validation
| Critère                | Seuil  | Résultat  | Statut     |
|------------------------|--------|-----------|------------|
| min_win_rate_mean      | ≥ 0.60 | 0.6604    | **PASS**   |
| max_win_rate_std       | ≤ 0.05 | 0.02458   | **PASS**   |
| integrity_required     | true   | true      | **PASS**   |
| integrity_ok           | —      | true      | **PASS**   |
| **Fusion Gate global** | —      | —         | **✅ PASS** |

### 2.6 Manifeste forensique
| Étape       | RC  | Fichier                         |
|-------------|-----|---------------------------------|
| Build       | 0   | manifest_forensic_v3.json       |
| Sign        | 0   | manifest_forensic_v3.json.sig   |
| Verify      | 0   | —                               |

Intégrité forensique confirmée — signature valide.

---

## 3. HISTORIQUE DES RUNS KAGGLE

| Run ID              | Dossiers générés                                    |
|---------------------|-----------------------------------------------------|
| 20260303_035713     | kaggle_submission/, notebook/, replit_v3_results/   |
| 20260303_040251     | kaggle_submission/, notebook/, replit_v3_results/   |
| 20260303_040704     | kaggle_submission/, notebook/, replit_v3_results/   |

---

## 4. NOTES SUR LES BIAIS OBSERVÉS (depuis JSON brut)

> **baseline_bias_hypothesis** : la baseline utilise `sigma*0.7` et un drift linéaire de 0.03 ; cela peut simplifier la dynamique baseline et biaiser la distribution des marges.
>
> **fragility_explanation** : les pertes varient selon le seed/contexte car la réalisation du bruit change la trajectoire autour de la cible dans le régime stochastique.

---

## 5. STRUCTURE DES LOGS V4 KAGGLE

```
quantum_simulator_v4_kaggle_pack/
├── RAPPORTS/
│   └── rapport_v4_kaggle_pack_20260317.md    ← CE FICHIER
├── LOGS/                                      ← Dossier créé le 2026-03-17
├── notebook/
│   └── lumvorax_quantum_v4_kaggle.ipynb
├── python_modules/
├── tools/
└── replit_kaggle_run_YYYYMMDD_HHMMSS/
    ├── kaggle_submission/
    ├── notebook/
    └── replit_v3_results/
```

---

## 6. CONCLUSION

Le simulateur V4 Kaggle Pack passe le Fusion Gate avec **66.04 % de win rate** en mode hardware_preferred (30 runs, 360 scénarios, 1400 steps). Le débit moyen de **7.59 millions de qubits/s** est confirmé. L'intégrité forensique est validée (manifeste signé RC=0). Le smoke test est propre, sans fuite mémoire.

**Recommandation** : pour C35, investiguer l'écart de stabilité entre hardware_preferred (std=0.0246) et deterministic_seeded (std=0.0240) ainsi que la latence p99 x2 en mode seeded.
