# MAJ distante + analyse experte Replit V3 (runs distants 30/360/1400)

## 1) Mise à jour dépôt distant (GitHub)

Commandes exécutées:
- `git remote add origin https://github.com/lumc01/Lumvorax.git` (si absent)
- `git fetch origin --prune`
- `git rev-list --left-right --count HEAD...origin/main`
- `git ls-tree -r --name-only origin/main src/advanced_calculations/quantum_simulator_v3_staging/results | rg '20260303_05[0-9]{4}/campaign_summary.json'`

Constat:
- divergence observée: `1|27`.
- les runs distants détectés pour la série demandée:
  - `20260303_051554`
  - `20260303_053436` (deuxième exécution)

---

## 2) Résultats distants consolidés (les 2 exécutions)

Paramètres communs:
- `runs_per_mode=30`, `scenarios=360`, `steps=1400`.

### Run `20260303_051554`
- smoke: `rc=0`
- gate: `pass=true`
- intégrité manifest/sign/verify: `0/0/0`
- hardware_preferred: `win_rate=0.655741`, `std=0.029335`, `throughput=8,587,502`, `p99=214,552`
- deterministic_seeded: `win_rate=0.656296`, `std=0.024023`, `throughput=8,213,471`, `p99=220,376`

### Run `20260303_053436` (2ème exécution)
- smoke: `rc=0`
- gate: `pass=true`
- intégrité manifest/sign/verify: `0/0/0`
- hardware_preferred: `win_rate=0.651944`, `std=0.023131`, `throughput=8,760,064`, `p99=210,198`
- deterministic_seeded: `win_rate=0.656296`, `std=0.024023`, `throughput=7,885,635`, `p99=247,613`

---

## 3) Interprétation pédagogique (ce que ça veut dire réellement)

1. **Ce qui est réussi concrètement**
   - campagne Replit lourde exécutée complètement (60 runs/campagne),
   - pipeline fiable (smoke + manifest signé + vérification),
   - décision automatique reproductible (`fusion_gate.pass=true`).

2. **Ce que les chiffres montrent**
   - le signal de gain reste stable autour de **~65.2–65.6%**,
   - mode deterministic_seeded reste très stable en score,
   - la latence extrême est maîtrisée sur ces deux runs (pas de dérive majeure type p99=843k observée auparavant).

3. **Ce que ce n'est PAS**
   - ce n'est pas une preuve d'avantage quantique matériel,
   - c'est une preuve d'un **benchmark logiciel gouverné et auditable**.

---

## 4) Comparaison avec historiques distants (contextualisation)

Références antérieures (origin/main):
- `20260303_035715`, `20260303_040253`, `20260303_040706`.

Lecture comparative:
1. Win-rate global reste dans la même bande (~0.652 à ~0.660).
2. Throughput des runs `051554/053436` est élevé (retour vers ~8M+) et meilleur que certaines dérives antérieures.
3. L'anomalie extrême historique p99 deterministic (~843k sur `040706`) n'est pas reproduite sur `051554/053436`.

Conclusion comparative:
- la 2ème exécution confirme une **stabilité fonctionnelle** et une latence extrême bien plus saine que le pire cas historique.

---

## 5) Anomalies et découvertes (ligne à ligne synthétisées)

## 5.1 Cause d'échec dominante
- `nx_noise_amplification` reste unique/dominante:
  - `7430` (run 051554)
  - `7471` (run 053436)

## 5.2 Scénarios fragiles
- run 051554: `133`, `270`, `148`, `2`, `74`...
- run 053436: `226`, `69`, `131`, `293`, `74`, `144`, `148`...

Découverte:
- certains scénarios persistent (ex: `74`, `148`),
- d'autres bougent selon exécution (dépendance au contexte/bruit),
- donc fragilité mixte: composante structurelle + composante stochastique.

---

## 6) Réponses directes aux questions d'expert

1. **Plan réalisé à 100% ?**
   - Non.
2. **Tout a été répondu des anciens rapports ?**
   - Majoritairement oui sur l'instrumentation/gouvernance; non sur la neutralisation baseline + comparatif concurrent exécuté apples-to-apples.
3. **Nouvelles questions possibles ?**
   - effet d'une baseline neutralisée,
   - dérive throughput inter-lots,
   - robustesse top scénarios fragiles quand `steps` augmente,
   - protocole concurrent standard prioritaire (QV/MQTBench/Hamiltonian).
4. **Différence techno entre origine / officiel / V6 / V3 nouveau ?**
   - origine: prototype métriques simples,
   - officiel: moteur simulation robuste,
   - V6: chaîne forensic/intégrité A->Z,
   - V3 staging: benchmark comparatif NX-vs-baseline + gate décisionnel + logs per-scenario.
5. **Comparaison avec concurrents (Top 10 2026)**
   - Oui en méthodologie (protocole concurrent préparé),
   - Non en verdict brut final tant que l'exécution harmonisée même hardware/workload n'est pas terminée.

---

## 7) Questions expertes à poser après lecture des logs

1. Quelle part de la variabilité vient du runtime (CPU contention) vs du modèle de bruit ?
2. Le top-10 scénarios perdants est-il stable sur 10 campagnes hebdo ?
3. Quels seuils CI throughput/p99 doivent bloquer une fusion automatique ?
4. Quel gain réel après ablation (avec/sans tanh, avec/sans noise shaping) ?
5. Quel mapping interne `nqubits/s` vers benchmarks publics concurrents est défendable ?

---

## 8) Commandes de reproduction (Replit)

Exécution:
```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v3_staging/run_on_replit_v3.sh /workspace/Lumvorax 30 360 1400
```

Inspection:
```bash
RUN=src/advanced_calculations/quantum_simulator_v3_staging/results/<run_id>
sed -n '1,180p' "$RUN"/campaign_summary.md
jq '.fusion_gate,.modes,.watchlist_loss_frequency_by_mode' "$RUN"/campaign_summary.json
```
