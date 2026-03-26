# RAPPORT D'AUDIT D'AUTHENTICITÉ — NQubit_v2

## 1. Contexte et objectif

Ce rapport est une **nouvelle analyse indépendante** produite dans `src/projetx_NQubit NX/NQubit_v2`, sans modification des rapports précédents.

Objectif : vérifier ce que les résultats générés montrent réellement, inspecter le code pour détecter **smoke tests insuffisants, placeholders, stubs et hardcoding** pouvant compromettre la crédibilité scientifique, et proposer une suite d’actions expertes.

---

## 2. Ce qui a été audité

### 2.1 Artefacts d’exécution
- `results/nqbit_benchmark.csv`
- `results/nqbit_stats_v2.csv`
- `results/rapport_final.md`
- `results/nqbit_forensic_ns.log`
- `results/test_extended_forensic.log`

### 2.2 Code source
- `main.c`
- `nqbit_nx.c/.h`
- `nx_forensic.c/.h`
- `tests/test_nqbit_nx.c`
- `tests/test_nqbit_nx_extended.c`

---

## 3. Résultats réellement générés (constat factuel)

## 3.1 KPI benchmark principal (360 scénarios)

- Scénarios : **360**
- Victoires NX : **360/360**
- Win rate : **1.0000**
- `nx_score` moyen : **0.361543481940**
- `classical_score` moyen : **0.189412669968**
- Marge moyenne : **+0.172130811972**
- Marge min/max : **0.111300922702 / 0.242608181587**
- Delta énergie moyen (NX - baseline) : **+13.916848103278**

## 3.2 Robustesse testée

- Tests unitaires : **360/360 pass**
- Tests progressifs étendus : **11110/11110 pass** (paliers 10, 100, 1000, 10000)

## 3.3 Reproductibilité observée

Le benchmark est **déterministe** dans l’état actuel : hash SHA-256 identique de `nqbit_benchmark.csv` sur ré-exécution.

---

## 4. Inspection d’authenticité (smoke / placeholder / stub / hardcoding)

## 4.1 Placeholders / stubs

### Verdict
Aucun stub explicite de type `TODO`, `FIXME`, `placeholder`, `dummy`, `return fake` n’a été détecté dans le cœur de calcul.

### Nuance critique
Le terme `fake_superposition` existe dans la baseline (`nqbit_nx.c`) : ce n’est pas un stub cassé, mais un signal explicite que la baseline est une **approximation simulée** et non un modèle physique complet.

## 4.2 Hardcoding détecté (impact sur crédibilité)

Hardcoding significatif identifié :
1. **Nombre de scénarios fixé à 360**.
2. **Formules de génération des cas** (seed, bruit sigma, thermique) déterministes et bornées.
3. **Baseline volontairement simple** (`0.03 * erreur + bruit*0.7`).
4. **Fonctions de score asymétriques** (pénalité énergie 0.05 côté NX vs 0.20 côté baseline).

### Risque d’interprétation
Ces hardcodings ne falsifient pas les résultats, mais **orientent** la comparaison. Le 360/360 doit donc être lu comme :
- excellent résultat sur **ce protocole interne**,
- pas preuve de supériorité universelle sans protocole externe équitable.

## 4.3 Smoke tests

Les smoke tests sont dépassés (on a plus que du smoke) :
- contrôle de finitude numérique,
- contrôle positivité des scores,
- contrôle plage de steps,
- campagne jusqu’à 10k cas.

Mais il manque encore :
- tests adversariaux non gaussiens,
- tests avec symétrisation stricte des métriques,
- tests statistiques multi-runs avec IC95%.

---

## 5. Ce que signifient vraiment ces résultats

1. **Validé** : la mécanique algorithmique NX (bruit + rappel Lyapunov) est stable et performante sur le protocole implémenté.
2. **Non validé à ce stade** : preuve hardware réelle de rupture thermodynamique.
3. **Interprétation correcte** : démonstrateur logiciel solide, pas encore preuve expérimentale en laboratoire.

---

## 6. Comparaison avec l’état de l’art (position prudente)

Ce prototype apporte une base crédible de recherche sur le calcul stochastique guidé.

Mais les "découvertes inconnues" ne peuvent pas être revendiquées comme nouvelles preuves de littérature sans :
- comparaison à baselines fortes standardisées,
- protocole externe publié,
- reproduction indépendante inter-environnements,
- validation sur bruit physique mesuré.

---

## 7. Forces, faiblesses, anomalies

## 7.1 Forces
- Pipeline complet et traçable (CSV + forensic + rapport).
- Reproductibilité forte (seed + déterminisme).
- Couverture de tests étendue (11 110 cas).

## 7.2 Faiblesses
- Asymétrie de scoring énergétique.
- Baseline "classique" limitée.
- Pas de validation physique réelle.

## 7.3 Anomalies / vigilance
- 100% de victoire est scientifiquement suspect tant qu’on n’élargit pas les baselines.
- Le gain s’accompagne d’un coût énergétique moyen supérieur côté NX.

---

## 8. Proto validé ou non ?

### Décision experte nuancée
- **Oui** : prototype validé comme démonstrateur logiciel.
- **Non (encore)** : prototype non validé comme preuve hardware scientifique définitive.

---

## 9. Questions déjà répondues

1. Le système s’exécute-t-il de façon stable ? Oui.
2. Les résultats sont-ils reproductibles dans ce cadre ? Oui.
3. Le modèle NX bat-il la baseline interne actuelle ? Oui, 360/360.

---

## 10. Nouvelles questions que les experts poseront

1. Le résultat tient-il avec une baseline forte et équitable ?
2. Le gain persiste-t-il en symétrisant la fonction de score ?
3. Le gain persiste-t-il avec bruit non-gaussien adversarial ?
4. Le gain persiste-t-il avec injection de bruit matériel réel ?
5. Quel compromis précision/énergie/latence en conditions embarquées ?
6. Quelle sensibilité aux dérives thermiques et à l’EMI ?

---

## 11. Suite logique de tests (priorités)

## Priorité P0 (immédiat)
- Ablations : sans Lyapunov / sans bruit / bruit impulsionnel.
- Symétrisation métrique score/énergie.
- 30 runs indépendants par scénario + IC95%.

## Priorité P1 (validation robuste)
- Baselines fortes (optimiseurs stochastiques adaptatifs).
- Tests OOD : distributions de bruit lourdes et drifts thermiques.

## Priorité P2 (transition hardware)
- Acquisition ADC de bruit physique.
- Injection signal réel dans le solveur.
- Reproductibilité sur plusieurs machines/capteurs.

---

## 12. Compléments experts souvent oubliés (ajoutés)

- Gouvernance des preuves : manifester hash + versions + seeds dans chaque run.
- Politique anti-biais : protocole d’évaluation figé avant exécution.
- Critère go/no-go formel pour bascule hardware.
- Section "threats to validity" obligatoire à chaque rapport.

---

## 13. Conclusion opérationnelle

Le dossier `NQubit_v2` est aujourd’hui un **bon socle d’audit logiciel reproductible**.  
Il est suffisamment propre pour une phase de validation scientifique renforcée, mais il faut éviter toute sur-affirmation hardware avant la campagne instrumentée réelle.

**Date** : 2026-02-28
