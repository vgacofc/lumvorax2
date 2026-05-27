# Comparaison détaillée — valeurs « qubits » Lumvorax vs benchmarks classiques concurrents (2026)

## 1) Objet exact de cette comparaison
Tu demandes une comparaison claire entre:
1. nos résultats mesurés (Lumvorax V3 staging),
2. les benchmarks publics de simulateurs classiques concurrents (sans matériel quantique) que tu as listés.

Ce rapport répond précisément à cette question **en évitant les comparaisons trompeuses**.

---

## 2) Point critique: nos métriques et celles des concurrents ne mesurent pas la même chose

### 2.1 Ce que mesurent les concurrents (dans ton benchmark)
Les concurrents sont majoritairement évalués sur:
- nombre de qubits simulables en état-vector/tensor-network,
- temps/mémoire sur circuits de référence (QV, random circuits, etc.),
- montée en charge CPU/GPU/multi-node.

### 2.2 Ce que mesure Lumvorax V3 aujourd’hui
Notre V3 mesure surtout:
- un benchmark A/B NX vs baseline sur 360 scénarios × 1400 steps,
- `nqubits_per_sec` (débit interne de simulation),
- `win_rate` et dispersion inter-runs,
- latences p50/p95/p99 par scénario,
- fréquence des scénarios perdants + raisons d’échec,
- intégrité artefacts via manifest/signature.

👉 Donc: **nos “nqubits” actuels sont des unités de charge de benchmark interne**, pas des “qubits physiques state-vector exacts comparables 1:1” à cuQuantum/Qiskit-Aer/IQS.

---

## 3) Nos chiffres réels (run Replit récent, Kaggle exclu)
D’après le rapport V3:
- `hardware_preferred`: win_rate_mean **0.6625926**, throughput_mean **5,800,990 nqubits/s**, p95 **277,993 ns**, p99 **314,578 ns**.
- `deterministic_seeded`: win_rate_mean **0.6562963**, throughput_mean **5,824,676 nqubits/s**, p95 **271,645 ns**, p99 **300,716 ns**.
- gate de fusion: **pass=true**.
- intégrité V6-style: manifest/sign/verify **0/0/0**.

Interprétation:
- le pipeline est stable et traçable,
- le gain NX est majoritaire mais non absolu,
- la variabilité seed/contexte existe (std non nul).

---

## 4) Tableau comparatif explicatif (apples-to-apples partiel)

| Axe | Concurrents classiques (ton benchmark) | Lumvorax V3 actuel |
|---|---|---|
| Cible principale | Simuler des circuits quantiques “standards” avec max qubits | Comparer dynamiques NX vs baseline en forensic |
| KPI dominant | qubits max simulables, temps/mémoire HPC | win_rate, nqubits/s interne, p95/p99, causes d’échec |
| Échelle qubits publiée | oui (≈30–40+ state-vector; >100 MPS selon cas) | non normalisée state-vector aujourd’hui |
| Comparabilité directe | bonne entre frameworks benchmarkés pareil | partielle (protocole différent) |
| Gouvernance forensic | variable selon outil | forte: JSONL scenario-level + signature + gate |

Conclusion clinique:
- Sur **échelle qubits brute standardisée**, les frameworks HPC concurrents sont en avance (car c’est leur objectif principal).
- Sur **observabilité décisionnelle / forensic de fusion algorithme**, Lumvorax V3 est très structuré.

---

## 5) Différence technologique claire (origine vs nouveau)

## 5.1 Origine `src/quantum/v_kernel_quantum.c`
- prototype simple,
- quelques métriques pseudo-aléatoires en log texte,
- pas de campagne multi-run ni p95/p99 ni manifest signé.

## 5.2 Nouveau V3 staging
- benchmark massif automatisé multi-runs,
- logs JSONL par scénario avec causes d’échec,
- modes RNG (hardware/déterministe),
- latences p50/p95/p99,
- manifest/signature/verify intégrés,
- gate de fusion formalisée.

---

## 6) Explication demandée: valeur “qubits” de nos résultats vs concurrents

## 6.1 Ce que **nous pouvons dire avec certitude**
- `504000 nqubits_simulated` = volume de travail interne (scenarios × steps),
- ~5.8M `nqubits/s` = débit de notre modèle de benchmark,
- ce n’est pas équivalent à “40 qubits state-vector exacts”.

## 6.2 Ce qu’on **ne doit pas prétendre**
- On ne peut pas annoncer “Lumvorax simule X qubits state-vector” tant qu’on n’a pas un benchmark standardisé type QV/MQTBench avec protocole comparable à cuQuantum/Qiskit-Aer/IQS.

## 6.3 Conversion honnête
- Aujourd’hui: métrique opérationnelle interne = excellente pour pilotage produit,
- Demain: ajouter un benchmark “state-vector compatible” pour produire une valeur qubits comparable marché.

---

## 7) Fragilité et biais baseline (rappel explicite)

### 7.1 Fragilité variable seed/contexte
Oui, confirmée:
- variabilité inter-runs mesurée (`win_rate_std`),
- scénarios fragiles récurrents (fréquences élevées),
- bruit hardware modifie les trajectoires.

### 7.2 Biais baseline (`sigma*0.7`, drift `0.03`)
Biais plausible et déjà identifié:
- baseline plus “douce” sur le bruit,
- NX avec dynamique plus non linéaire/thermique.
- cela peut déplacer les marges selon les zones.

Action recommandée: créer une baseline neutralisée pour comparaison plus neutre.

---

## 8) Réponse nette à “100% du plan ?”
- Non, pas 100%.
- Estimation actuelle: **88% réalisé**, **12% restant**.

Reste principal:
1. benchmark standardisé comparables marché (QV/MQTBench-like),
2. validation multi-environnements longue durée,
3. recalibration baseline anti-biais,
4. consolidation statistique (IC95, drift).

---

## 9) Ce que nous avons produit concrètement
1. Une chaîne Replit reproductible de benchmark et analyse.
2. Une observabilité forensic très fine (chaque scénario).
3. Une intégrité cryptographique des artefacts.
4. Une gouvernance de fusion mesurable (gate).
5. Une base solide pour passer à un benchmark concurrentiel standardisé “qubits comparables”.
