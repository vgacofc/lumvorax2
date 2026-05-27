# Mise à jour distante + analyse des nouveaux résultats V2 (Replit)

Date: 2026-03-03  
Dépôt distant: `https://github.com/lumc01/Lumvorax.git` (`origin/main`).

## 1) Synchronisation distante effectuée
- `git fetch origin --prune` exécuté.
- État observé au moment de la synchro: `work...origin/main = 1|43`.
- Artefact V2 confirmé sur le distant:
  - `src/advanced_calculations/quantum_simulator_v2_staging/results/fusion_forensic_full.jsonl`

## 2) Données V2 extraites du distant (run forensic)
Fichier source: `fusion_forensic_full.jsonl`.

- `run_config`: `rng_mode=1` (hardware_preferred), `used_hardware_entropy=true`, `scenarios=360`, `steps=1400`.
- `summary`:
  - `nqubits_simulated=504000`
  - `nqubits_per_sec=10029954.738`
  - `elapsed_ns=50249479`
  - `nqubit_avg_score=0.960481999898`
  - `baseline_qubit_avg_score=0.939847964408`
  - `nqubit_win_rate=0.625000000000`
  - `nqubit_wins=225`, `baseline_wins=135`

## 3) Interprétation des logs (ligne par ligne, en synthèse)
1. `event=run_config`:
   - confirme le mode RNG réellement utilisé;
   - confirme les dimensions du run (360 × 1400).
2. `event=scenario_margin` (360 lignes):
   - `margin > 0` = NX gagne ce scénario;
   - `margin <= 0` = baseline gagne ce scénario.
3. `event=summary`:
   - consolide le run complet en métriques exploitables.

## 4) Statistiques consolidées calculées sur les 360 scénarios
- Gagnés NX: **225** (62.5%)
- Perdus NX: **135** (37.5%)
- Marge moyenne: `+0.020634`
- Médiane: `+0.017349`
- `p05`: `-0.056563`
- `p10`: `-0.041581`
- `p90`: `+0.079781`
- `p95`: `+0.105584`
- Min: `-0.115622` (scénario `310`)
- Max: `+0.199299` (scénario `275`)

### Top fragilités (marges les plus négatives)
- `310: -0.115622`
- `117: -0.087861`
- `328: -0.085211`
- `35: -0.081449`
- `292: -0.079133`

### Top forces (marges les plus positives)
- `275: +0.199299`
- `257: +0.161959`
- `55: +0.159389`
- `71: +0.153775`
- `219: +0.137466`

## 5) Comparaison avec les résultats récents V3 distants
Comparaison informative (pas totalement 1:1 car V3 est une campagne multi-run/multi-modes):

- V2 (run unique hardware_preferred):
  - `win_rate = 62.50%`
  - `throughput = 10.03M nqubits/s`
- V3 distant (campaign `20260303_051554`, moyenne hardware_preferred):
  - `win_rate_mean = 65.57%`
  - `throughput_mean = 8.59M`
- V3 distant (campaign `20260303_053436`, moyenne hardware_preferred):
  - `win_rate_mean = 65.19%`
  - `throughput_mean = 8.76M`

### Lecture concrète
- V2 est **plus rapide en débit brut** sur ce run unique.
- V3 est **plus robuste/statistiquement gouverné** (multi-runs, std/CI95, gate, latence p95/p99, intégrité manifest/signature).
- Donc:
  - V2 = bon signal de performance instantanée;
  - V3/V4 = meilleure validité décisionnelle pour fusion officielle.

## 6) Réponses directes aux questions de fond

### Qu’est-ce qu’on a réellement produit concrètement ?
- Un benchmark logiciel NX vs baseline avec traces scénario par scénario,
- et un résultat chiffré distant vérifiable: **225/360 scénarios gagnés (62.5%)**.

### Le plan est-il 100% terminé ?
- **Non** (comme déjà établi dans les rapports précédents).
- Principaux manques pour 100%:
  1. protocole standard QV/MQTBench-like exécuté de bout en bout,
  2. comparaisons strictement harmonisées avec concurrents (mêmes workloads + même hardware),
  3. couverture GPU/HPC quand disponible.

### Différences techno V2 vs V3/V4
- V2: run forensic unitaire très lisible, rapide à rejouer.
- V3/V4: orchestration campagne, statistiques de stabilité, gate Go/No-Go, intégrité renforcée.

## 7) Nouvelles questions expertes à poser après ce run V2
1. Les scénarios `310/117/328` sont-ils aussi fragiles en mode deterministic_seeded ?
2. Le top-5 des pertes V2 reste-t-il stable sur 30 runs ?
3. Le gain de débit V2 vient-il d’un coût analytique plus faible (moins de contrôles) ?
4. Quel compromis cible: débit max instantané ou robustesse décisionnelle ?

## 8) Commande de reproduction Replit (V2)
```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v2_staging/run_on_replit.sh /workspace/Lumvorax
```

## 9) Conclusion
- Les nouveaux résultats V2 distants confirment un run valide et utile (62.5% de wins).
- Pour la comparaison externe “marché”, il faut continuer avec le pipeline V3/V4 pour la couche de gouvernance statistique et d’intégrité.
