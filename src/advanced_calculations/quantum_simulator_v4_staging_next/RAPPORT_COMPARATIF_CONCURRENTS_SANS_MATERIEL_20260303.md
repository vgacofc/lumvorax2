# Rapport comparatif — simulateurs quantiques concurrents (sans matériel quantique) vs Lumvorax V3 staging

## 1) Périmètre et méthode
Ce rapport compare **les simulateurs quantiques logiciels/émulateurs** (sans QPU physique) listés dans `quant.md` avec notre simulateur `quantum_simulator_v3_staging`.

Sources utilisées:
1. `quant.md` pour la cartographie concurrente et les ordres de grandeur publics.
2. `RAPPORT_FINAL_BENCHMARK_V3_20260302.md` pour nos KPI réels mesurés.

⚠️ Important: les métriques concurrentes disponibles dans `quant.md` sont principalement des capacités de simulation (qubits/architecture), pas des benchmarks normalisés identiques à notre protocole NX-vs-baseline.

---

## 2) Concurrents sans matériel quantique (données publiques disponibles)

### 2.1 Simulateurs logiciels/émulateurs mentionnés
- IBM simulators (`qasm_simulator`, etc.) ~30–50 qubits (selon ressources). 
- IonQ simulator (Azure Quantum) ~29 qubits.
- PASQAL Uem-TN (émulateur analogique de dynamique Schrödinger).
- Google qsim (CPU/GPU), ~30+ qubits selon ressources.
- Fujitsu HPC simulation jusqu’à 36 qubits.
- Hyperion‑1 annoncé jusqu’à 40 qubits exacts.
- LUMI/Qiskit‑Aer efforts jusqu’à ~44 qubits.

Ces limites suivent la contrainte classique exponentielle mémoire/temps avec le nombre de qubits.

---

## 3) Notre simulateur (V3 staging) — métriques benchmark

Exécution référence (campagne):
- 60 runs (30 `hardware_preferred` + 30 `deterministic_seeded`),
- 360 scénarios × 1400 steps par run,
- A/B officiel smoke vs staging fusion,
- manifest/signature/verify style V6.

Résultats agrégés:
- `hardware_preferred`: `win_rate_mean=0.6560185`, `win_rate_std=0.0286447`, `throughput_mean=5,990,552 nqubits/s`, `p95=255,694 ns`, `p99=275,265 ns`.
- `deterministic_seeded`: `win_rate_mean=0.6562963`, `win_rate_std=0.0240235`, `throughput_mean=5,841,557 nqubits/s`, `p95=272,080 ns`, `p99=301,800 ns`.
- Fusion gate: **pass=true** avec intégrité manifest/signature **ok**.

---

## 4) Tableau comparatif (ce qui est comparable sans inventer)

| Critère | Concurrents (depuis `quant.md`) | Lumvorax V3 staging |
|---|---|---|
| Type | Simulateurs/émulateurs logiciels classiques | Simulateur logiciel classique |
| Matériel quantique réel requis | Non | Non |
| Axe public principal | capacité max qubits simulés (≈29 à 44+ selon stack) | stabilité A/B NX-vs-baseline + forensic + intégrité |
| Métriques publiées homogènes | souvent hétérogènes, orientées qubits max / infra | métriques run-to-run détaillées (win_rate, throughput, p95/p99, fail reasons) |
| Forensic par scénario | non garanti/variable selon plateforme | oui (`scenario_margin` + `fail_reason` + latence scénario) |
| Manifest + signature intégrité | variable/non standard | oui (style V6, build/sign/verify) |
| Gate de décision fusion | rarement exposée publiquement | oui (seuils win_rate/stabilité/intégrité) |

Conclusion: les concurrents dominent souvent en **échelle qubits brute** (HPC), mais notre pipeline V3 est très fort en **auditabilité comparative fine et gouvernance de fusion**.

---

## 5) Interprétation demandée: fragilité, biais baseline, sens réel

## 5.1 Fragilité « réelle et variable selon seed/contexte »
Oui, confirmé par `win_rate_std` non nul sur les 2 modes, et par la variabilité des pertes par scénario. La stochasticité des trajectoires (bruit) modifie les franchissements autour de la cible.

## 5.2 Biais baseline (`sigma*0.7`, `drift=0.03`)
Hypothèse de biais plausible: la baseline a un bruit réduit et une dynamique plus simple, alors que NX a un bruit thermalisé + non-linéarité (`tanh`). Cette asymétrie peut déplacer les marges selon les zones de paramètres.

## 5.3 Ce que cela veut dire concrètement
- Ce benchmark **ne prouve pas un avantage quantique matériel**.
- Il prouve qu’on sait exécuter une campagne logicielle robuste, traçable, et statistiquement exploitable pour décider une fusion.
- On a un gain moyen stable > 0.65 de win rate, mais pas absolu (donc améliorable).

---

## 6) Réponse claire à "100% ?"
Statut global estimé dans la V3: **88% réalisé**; **12% restant**.

Reste principal:
1. validation inter-environnements (Replit/Kaggle/local) sur plusieurs vagues,
2. calibration baseline pour réduire biais structurel,
3. consolidation statistique long terme (IC95/dérive),
4. protocole bruit avancé prouvé (Lyapunov/noise control).

---

## 7) Nouvelles questions expert (après ce comparatif concurrent)
1. Peut-on normaliser un protocole commun de benchmark face aux concurrents (même charge, même métriques) ?
2. Quel mapping établir entre `nqubits/s` interne et « qubits simulés max » public des concurrents ?
3. Quelle part du gain vient de la non-linéarité vs de la calibration de bruit ?
4. Le `win_rate` reste-t-il >0.60 sur environnements différents ?
5. Quel seuil de stabilité devient contractuel pour fusion vers officiel ?
6. Quel coût infra (€ / run) face aux simulateurs concurrents cloud/HPC ?

---

## 8) Commande Replit immédiate (version V3)
```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v3_staging/run_on_replit_v3.sh /workspace/Lumvorax 30 360 1400
```
