# RAPPORT VÉRIFICATION EXPERTE — V1 vs V2 (transfert 9 teachers, dépendances exactes, plan d’apprentissage)
Date: 2026-02-24

## 0) Réponse directe à ta question principale
## Est-ce que la V1 a **déjà réellement commencé** le transfert de connaissances des 9 modèles concurrents ?
**Réponse courte: NON (pas encore).**

### Pourquoi NON ?
- V1 contient un registre des 9 teachers + garde stricte (`assert_9_teacher_models_ready`) + chemins des modèles.
- Mais V1 ne contient pas un pipeline exécuté de distillation (pas de boucle d’entraînement student-from-teachers, pas d’agrégation logits teachers, pas de loss explicite de transfert, pas de checkpoint “student_after_distillation”).
- Donc: V1 est **préparée / verrouillée**, mais le transfert n’est **pas prouvé comme exécuté** dans les artefacts présents.

---

## 1) Ce que j’ai compris de ton plan (reformulé clairement)
Tu demandes une stratégie en 4 étapes obligatoires:
1. Charger les **9 modèles concurrents** comme enseignants.
2. Faire un **transfert de connaissances** vers notre neurone/modèle (distillation teacher→student), en utilisant aussi le TIFF concurrent (référence score 0.552) comme supervision auxiliaire.
3. Faire un **fine-tuning normal sur train**.
4. Faire un **ultra-fine-tuning guidé test** par comparaison slice-à-slice avec le TIFF concurrent, puis seulement après lancer l’inférence test principale et générer la soumission.

Tu imposes aussi une règle métier:
- tant que la proximité avec le TIFF concurrent n’est pas suffisante, **pas de soumission officielle** (pré-runs seulement).

---

## 2) Vérification V1 vs V2 (code réel)

## 2.1 État V1 (constaté)
V1 contient:
- `TEACHER_MODELS_REGISTRY` (9 slots),
- `USER_PROVIDED_MODEL_INPUT_PATHS`,
- `COMPETITOR_MODELS_FOUND_EXACT`,
- `assert_9_teacher_models_ready(...)`.

=> V1 valide la **présence/structure** de l’orchestration teacher.

## 2.2 État V2 avant correction (constaté)
La V2 (`nx46-vx-unified-kaggle.ipynb`) ne contenait pas ces blocs explicites V1 teacher/distillation guardrails.

## 2.3 Correction appliquée en V2 (réintégration V1 + compléments)
J’ai réintégré en V2:
- registre 9 teachers,
- garde stricte `assert_9_teacher_models_ready`,
- références concurrent (`COMPETITOR_MODELS_FOUND_EXACT`),
- manifest dépendances exactes,
- références explicites `competitor_teacher_1407735.tif/.lum`,
- plan d’apprentissage en phases incluant:
  - `phase_1_teacher_distillation`,
  - `phase_2_supervised_train`,
  - `phase_3_test_guided_ultra_finetune`,
  - `phase_4_main_test_inference`.

---

## 3) Vérification des dépendances exactes demandées
Source de vérité: `liste-dependence.md` (19 entrées).

Résultat de vérification après correction V2:
- **19/19 noms exacts** sont maintenant présents dans le code V2 via `REQUIRED_DEPENDENCY_FILENAMES`.
- Les deux artefacts clés sont explicitement référencés:
  - `competitor_teacher_1407735.tif`
  - `competitor_teacher_1407735.lum`

Conclusion: sur la contrainte “noms exacts des dépendances dans V2”, c’est **OK**.

---

## 4) Analyse des résultats par rapport au TIFF concurrent (proximité réelle)
Comparaison binaire effectuée entre:
- TIFF concurrent (issu de `exemple-soumision-notebook-concurrent/results.zip -> submission.zip -> 1407735.tif`),
- TIFFs NX46-VX (`result-nx46-vx` et `result-nx46-vx-v1`).

Résultats:
- IoU = **0.0385106847**
- Dice = **0.0741652162**
- XOR ratio = **0.1088222046**
- densité concurrent ≈ **9.412%**
- densité NX46-VX ≈ **2.342%**

Lecture métier:
- la proximité actuelle est faible (IoU 3.85%),
- NX46-VX sous-détecte fortement vs concurrent (densité trop basse),
- donc ta règle “pas de soumission officielle tant que pas assez proche” est cohérente et justifiée.

---

## 5) Ce qui est fait vs ce qui reste à faire (état d’avancement)

## 5.1 Fait
- Fusion technique des versions dans notebook unifié.
- V1: registre 9 teachers + verrou.
- V2 corrigée: réintégration V1 + dépendances exactes + plan double fine-tuning explicite.
- Vérification de proximité concurrent réalisée sur les TIFFs disponibles (IoU/Dice/XOR).

## 5.2 Pas encore fait / non prouvé
- Distillation réelle exécutée (teacher logits/probas -> student loss).
- Journal traçable “ce que l’étudiant a appris de chaque teacher” (par zone/slice).
- Ultra-fine-tuning test guidé concurrent avec protocole anti-fuite formalisé.
- Critère GO/NO-GO quantitatif final (“soumission autorisée si ...”).

---

## 6) Questions d’experts identifiées + réponses

## Q1 — Les 9 teachers sont-ils juste listés ou réellement utilisés pour apprendre ?
**Réponse actuelle:** listés + verrouillés, mais apprentissage effectif non prouvé dans les artefacts.

## Q2 — Le TIFF concurrent est-il branché comme cible auxiliaire d’apprentissage ?
**Réponse actuelle:** référence ajoutée dans la config V2, mais absence de preuve d’un entraînement exécuté qui l’exploite de bout en bout.

## Q3 — Quelle est la proximité actuelle au concurrent ?
**Réponse actuelle:** IoU 0.0385 (faible), donc insuffisant pour une stratégie “équivalent/supérieur concurrent”.

## Q4 — Où est l’écart majeur ?
**Réponse actuelle:** densité binaire sortie NX46-VX (2.34%) très inférieure à la densité concurrent (9.41%).

## Q5 — Peut-on expliquer “ce que le modèle a appris des 9 teachers” ?
**Réponse actuelle:** pas encore; il manque des logs de transfert dédiés par teacher/slice/région et des KPI de gain par phase.

---

## 7) Questions encore sans réponse (à résoudre pour progresser score)
1. Quelle pondération optimale teacher par teacher (globale + par tranche Z) ?
2. Quel gain marginal de chaque teacher (ablation 9->8->...->1) ?
3. Le TIFF concurrent doit-il être utilisé en hard target, soft target, ou calibration topologique seulement ?
4. Quelle contrainte anti-fuite exacte pour le fine-tuning test guidé ?
5. Quels seuils GO/NO-GO quantifiés bloquent/débloquent la soumission ?
6. Quelle métrique “raisonnement appris” pour tracer concrètement le transfert de connaissance ?

---

## 8) Plan opérationnel court (prochaines actions)
1. Implémenter distillation explicite (teachers -> student) avec logs:
   - `teacher_consensus_map`,
   - `student_delta_vs_consensus`,
   - `gain_per_teacher`.
2. Fine-tuning train classique (phase 2) avec checkpoints comparatifs.
3. Ultra-fine-tuning test guidé concurrent (phase 3) sous protocole anti-fuite documenté.
4. Calibration finale par tranche avec objectif d’augmentation IoU concurrent.
5. GO/NO-GO submission:
   - no-go si proximité concurrent < seuil fixé,
   - go seulement si stabilité multi-run + proximité validée.

---

## 9) Conclusion décisionnelle
- **V1**: prête pour l’enseignement (structure), mais l’enseignement réel n’est pas encore démontré.
- **V2**: corrigée pour ne rien oublier de V1 + dépendances exactes + plan double fine-tuning.
- **État score/proximité**: encore trop loin du concurrent (IoU 0.0385), donc stratégie pré-exécution seulement est pertinente tant que l’apprentissage teacher→student n’est pas effectivement réalisé et tracé.
