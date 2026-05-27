# Explication détaillée (nouveau fichier) — smoke, manifest/signature, gate de fusion et préparation benchmark réel

Date: 2026-03-03  
Scope: `src/advanced_calculations/quantum_simulator_v4_staging_next/`  
Objectif: expliquer précisément *de quoi on parle*, ce qui est déjà inclus dans V4-next, ce qui manque encore pour des comparaisons externes strictes, et confirmer si les derniers tests analysés sont bien sur la version V4.

---

## 1) Définition claire des trois blocs demandés

### A. `smoke test`
Un smoke test est un test rapide “est-ce que le système démarre et fait le minimum vital sans casser ?”.  
Dans V4-next, le smoke test compile puis lance le test officiel C (`test_quantum_simulator_complete.c`) avant la campagne.

Ce que ça valide concrètement:
- création/détruction des structures;
- application d’opérations quantiques de base;
- mesure/collapse;
- absence d’échec immédiat (`rc=0`).

### B. `manifest/signature`
- **Manifest**: fichier de traçabilité du run (artefacts, paramètres, hash/chemins).
- **Signature**: preuve cryptographique que le manifest n’a pas été modifié après génération.
- **Verify**: vérification signature (`verify_rc=0`) avant de déclarer le run “intègre”.

En pratique: si signature KO, la gate peut forcer `pass=false` (selon politique).

### C. `gate de fusion`
La gate est une règle Go/No-Go automatique.  
Dans V4-next, elle combine:
- seuil min de `win_rate_mean`;
- seuil max de `win_rate_std`;
- seuils max latence `p95` et `p99`;
- exigence d’intégrité (`integrity_required=true`).

Si tous les critères passent → `pass=true`.

---

## 2) État réel de ce qui est inclus aujourd’hui dans V4-next

## ✅ Inclus et vérifié
1. Smoke test officiel exécuté automatiquement dans la campagne (`official_smoke.rc=0`).
2. Manifest + signature + verify exécutés (`build_rc=0`, `sign_rc=0`, `verify_rc=0`).
3. Gate Go/No-Go active (win-rate, std, p95/p99, intégrité).
4. 3 modes RNG présents:
   - `hardware_preferred`
   - `deterministic_seeded`
   - `baseline_neutralized`
5. Métrique `max_qubits_width` exportée par mode.
6. Logs forensics JSONL + résumé JSON/MD + CSV campagne.

## ⚠️ Inclus partiellement (prêts à compléter pour benchmark “marché scientifique”)
1. **Largeur max de circuit comparable aux concurrents**:
   - présent comme champ `max_qubits_width` (déclaré/paramétré),
   - mais il manque encore une batterie standard “QV/MQTBench-like” publiée avec protocole strict et reproductible inter-outils.
2. **Temps/mémoire sur circuits standardisés (QV, random, Hamiltonian)**:
   - latence/throughput présents sur le pipeline actuel,
   - mais pas encore de suite standard complète alignée sur publications concurrentes.
3. **Stack matérielle explicite CPU/GPU/HPC**:
   - CPU/RAM local détectables et documentables,
   - GPU/HPC multi-node non disponibles dans cet environnement local actuel.

---

## 3) Détection hardware disponible (local/Replit-like actuel)

Relevé exécuté localement:
- CPU: `Intel(R) Xeon(R) Platinum 8370C @ 2.80GHz`
- Coeurs visibles: `3`
- RAM: `17 GiB`
- GPU NVIDIA: non détecté (`nvidia-smi` indisponible)
- Python: `3.10.19`

Interprétation:
- Oui pour benchmark CPU mono-noeud et comparaison interne.
- Non pour benchmark GPU natif tant que GPU non disponible.
- Non pour benchmark HPC multi-node tant qu’on n’a pas d’infra distribuée.

---

## 4) Réponse à ta question clé: “as-tu inclus les mêmes tests et logs nécessaires ?”

Réponse stricte: **oui, pour démarrer les vrais benchmarks sur les ressources disponibles ici**.

Déjà inclus:
- test unitaire/protocole (`pytest`) ;
- smoke C officiel ;
- campagne orchestrée multi-modes ;
- logs forensics par scénario ;
- résumé agrégé avec CI95/p95/p99 ;
- manifest/signature/verify ;
- gate de fusion ;
- export des métriques nécessaires au suivi de régression.

Ce qu’il faut encore ajouter pour être “au même format publication concurrente”:
1. batterie QV/MQTBench-like versionnée;
2. profils mémoire standardisés par workload;
3. runs sur environnement GPU/HPC quand disponible;
4. tableau de comparaison final harmonisé (même circuits, mêmes tailles, même budget temps).

---

## 5) Réponse à “les dernières analyses sont-elles bien sur la V4 ?”

**Oui.** Les derniers tests vérifiés ici sont exécutés sur:
- `src/advanced_calculations/quantum_simulator_v4_staging_next/tests/...`
- `src/advanced_calculations/quantum_simulator_v4_staging_next/run_on_replit_v4_next.sh`
- sortie de run: `.../quantum_simulator_v4_staging_next/results/20260303_114149`

Donc la validation citée ci-dessus correspond bien à la branche *V4-next* (nouvelle version de travail), pas à l’ancienne V3 seule.

---

## 6) Commande unique de démarrage (Replit)

```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v4_staging_next/run_on_replit_v4_next.sh /workspace/Lumvorax 30 360 1400 36
```

Paramètres:
1. `ROOT`
2. `RUNS` (runs par mode)
3. `SCENARIOS`
4. `STEPS`
5. `MAX_QUBITS_WIDTH`

---

## 7) Conclusion opérationnelle

- Tu as maintenant le triptyque demandé **smoke + manifest/signature + gate** avec logs complets en V4-next.
- Tu peux lancer de vrais benchmarks **sur hardware disponible** immédiatement.
- Pour “faire exactement comme la concurrence scientifique”, la prochaine étape n’est pas de rajouter des logs génériques, mais de verrouiller une suite QV/MQTBench-like + profils mémoire + exécutions sur stack GPU/HPC quand accessible.
