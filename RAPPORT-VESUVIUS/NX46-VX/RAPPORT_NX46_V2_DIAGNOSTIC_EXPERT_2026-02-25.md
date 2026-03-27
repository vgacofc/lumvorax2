# RAPPORT EXPERT — NX46-VX V2 (analyse corrective suite commentaires)

Date: 2026-02-25

## 1) Correction majeure: `golden_nonce_topk` (fixe) ≠ `golden_nonce_detected` (runtime)

Tu as raison de demander la distinction exacte.

- `golden_nonce_topk = 11` = **paramètre BOOT** (combien de meilleurs points on garde).
- `golden_nonce_detected = 11` = **résultat runtime** (combien de points effectivement sortis).

Donc le nombre de points trouvés est un résultat d’exécution, mais limité/filtré par `topk`.

## 2) Où sont les 11 golden nonces et à quel moment ils sont trouvés ?

J’ai ajouté un export dédié:

- `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v2/GOLDEN_NONCE_TIMELINE_V2.md`

Ce fichier donne:
- timestamp BOOT,
- timestamp `NX47_METRICS` (moment où la liste top-11 est publiée),
- timestamp `GLOBAL_STATS`,
- table complète des 11 points `(y, x, score)`.

Important: dans ce run, les logs publient les nonces **en bloc final** (`NX47_METRICS`) et non en 11 événements unitaires.

## 3) Signification réelle des 11 golden nonces (dans ce run)

Les 11 golden nonces sont les **11 pixels candidats les plus scorés** au moment de la consolidation des métriques:

- Rang 1: `(y=118, x=301, score=0.1458509862)`
- …
- Rang 11: `(y=264, x=227, score=0.1432795376)`

Interprétation pratique:
- ce ne sont pas 11 “événements magiques” indépendants,
- ce sont 11 maxima du champ de probabilité final (top-K), injectés dans le masque final selon la logique du pipeline.

## 4) Pourquoi on ne voit pas « le moment exact » de chaque nonce ?

Parce que la version V2 actuelle loggue:
- la config initiale,
- puis un snapshot final top-K.

Elle ne loggue pas encore `NONCE_FOUND` point-par-point en streaming.

➡️ C’est une vraie limitation d’observabilité. Elle est traitée dans le nouveau cahier des charges V3.

## 5) Tu as demandé auto-tuning dynamique: état réel actuel

Tu as raison sur le besoin.

### 5.1 Ce qui est dynamique aujourd’hui
- sélection des meilleurs scores/ratios,
- exécution de boucles internes d’optimisation,
- choix d’un best ratio (`best_ratio ≈ 0.2059`).

### 5.2 Ce qui reste statique aujourd’hui (et te gêne, à juste titre)
- `dust_min_size`, `mutation_noise`, `convergence_patience`, `meta_neurons` sont chargés au BOOT.
- Le code actuel les lit via config/env (pas de contrôleur auto-adaptatif robuste en cours de run).

### 5.3 Pourquoi `convergence_patience=5` est trop contraignant dans ta vision
Tu demandes un comportement autonome et non plafonné. C’est cohérent si l’objectif est:
- apprentissage prolongé,
- exploration plus large,
- adaptation à la difficulté réelle du fragment.

La V2 actuelle est orientée “run sûr / préflight borné”, pas “autonomie longue”.

## 6) Couverture entraînement: point bloquant confirmé

Tu vises 100% (normal vu le mandat), mais le run V2 montre:

- paires découvertes: `786`
- paires sélectionnées: `32`
- couverture effective: `4.0712468193%`

Donc le pipeline déclare une complétion de process, mais **pas une couverture data complète**.

## 7) Transfert des 9 professeurs: diagnostic sans ambiguïté

Ce que j’ai vérifié:
- Le notebook décrit bien le plan 4 phases (dont distillation des 9 teachers).
- Les registries teachers existent.
- Mais dans les logs runtime V2 (`nx46vx_v2_execution_logs.json`), on ne trouve pas d’événements explicites type:
  - `TEACHER_DISTILLATION_START`
  - `TEACHER_DISTILLATION_DONE`
  - `TRANSFER_MEMORY_PERSISTED`

Conclusion: **la preuve forensic d’un transfert effectif teacher→student n’est pas matérialisée dans ce run**.

## 8) TIFF concurrent pour apprentissage: où est la phase ?

Le plan notebook mentionne une phase “test-guided ultra-finetune vs competitor_teacher_1407735.tif”.

Mais côté logs runtime V2 actuels, il manque des preuves explicites de passage effectif de cette phase avec métriques dédiées (ex: alignment gain before/after).

➡️ Donc tu as raison: “annoncé dans le plan” ≠ “prouvé en exécution”.

## 9) Pourquoi 1h30 de pré-exec + 7h submission sans gain de score visible ?

Hypothèse principale étayée:
1. couverture train trop basse (4.07%),
2. absence de preuve de distillation teacher réellement appliquée,
3. sortie masque quasi stable entre runs (plateau),
4. observabilité incomplète sur phases critiques.

## 10) Action immédiate livrée dans ce commit

1. Script d’extraction timeline nonces ajouté:
   - `RAPPORT-VESUVIUS/NX46-VX/tools/extract_golden_nonce_timeline.py`
2. Rapport des 11 nonces généré:
   - `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v2/GOLDEN_NONCE_TIMELINE_V2.md`
3. Nouveau cahier des charges V3 (sans régression) ajouté:
   - `RAPPORT-VESUVIUS/NX46-VX/CAHIER_DES_CHARGES_NX46_V3_TRANSFERT_AUTONOME.md`

## 11) Réponse courte à tes questions (direct)

- “golden nonce doit être résultat runtime ?” → **Oui**. Le `detected` est runtime; `topk` est la capacité/limite.
- “où sont les 11 ?” → dans le fichier timeline ajouté (table complète).
- “pourquoi pas moment exact de chaque nonce ?” → logging actuel en snapshot final; à corriger en V3.
- “auto-tuning sur ces paramètres ?” → pas complet en V2; prévu explicitement en V3.
- “phase transfert 9 modèles + TIFF concurrent ?” → planée mais preuve d’exécution insuffisante dans logs V2.

## 12) Verdict

Ton diagnostic de fond est pertinent:
- il faut passer d’un pipeline “préflight/log-heavy” à un pipeline “apprentissage réellement traçable et démontrable”.

Le cahier des charges V3 fourni cible exactement ça, **sans casser la V2** (activation progressive via flags et invariants de compatibilité).
