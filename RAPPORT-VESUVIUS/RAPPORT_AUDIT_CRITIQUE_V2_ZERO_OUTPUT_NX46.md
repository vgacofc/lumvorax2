# RAPPORT D'AUDIT CRITIQUE — NX46 V2 « tout à zéro »

## 1) Contexte et objectif

Tu as signalé un état final avec:
- `layout_detected: "empty"`
- toutes les métriques opérationnelles à zéro (`active_neurons`, `total_allocations`, `total_pixels_processed`, etc.)
- aucune sortie de soumission (`submission_csv: null`, `submission_zip: null`).

Objectif de cet audit: expliquer **pourquoi** cela arrive, **où** le comportement diverge des notebooks de référence qui fonctionnent, et proposer un plan de correction strict, clair, pédagogique, sans flou.

---

## 2) Résultat de l’inspection des artefacts disponibles

### 2.1 Dossier demandé vs artefacts réellement présents

Le chemin demandé `RAPPORT-VESUVIUS/output_logs_vesuvius/v2-outlput-logs--nx46-vesuvius-core-kaggle-ready` n’existe pas dans ce dépôt local.

Artefacts effectivement présents:
- `RAPPORT-VESUVIUS/output_logs_vesuvius/results.zip`
- `RAPPORT-VESUVIUS/output_logs_vesuvius/RkF4XakI.txt`
- `RAPPORT-VESUVIUS/output_logs_vesuvius/UJxLRsEE.txt`
- `RAPPORT-VESUVIUS/output_logs_vesuvius/nx46-vesuvius-challenge-surface-detection.ipynb`

### 2.2 Ce que montre `results.zip` (preuve technique)

Après extraction, `state.json` montre bien un pipeline déclaré actif mais **sans traitement réel**:
- `active_neurons: 0`
- `total_allocations: 0`
- `total_pixels_processed: 0`
- `train_fragments: []`
- `test_fragments: []`
- `submission_path: null`

Le `forensic_ultra.log` contient seulement:
- démarrage (`SYSTEM_STARTUP_L0_SUCCESS`)
- fin (`SYSTEM_LOADED_100_PERCENT`)

Aucune ligne intermédiaire de train/infer, ce qui confirme un parcours « vide ».

---

## 3) Pourquoi tout est à zéro — explication simple et exacte

## Cause principale A — Mauvais contrat de dataset (format attendu ≠ format réel)

Le notebook de référence qui fonctionne (`nx46-vesuvius-challenge-surface-detection.ipynb`) est explicitement orienté:
- racine: `/kaggle/input/competitions/vesuvius-challenge-surface-detection`
- entrées: `test_images/*.tif`
- sortie: `submission.zip`

Donc il cherche **des fichiers TIFF dans `test_images`** et échoue immédiatement si ce dossier n’existe pas.

Le core NX46 compact qui a produit `results.zip` (ancien `nx46_vesuvius_core_kaggle_ready.py`) cherche surtout:
- `train/<fragment>/surface_volume`
- `test/<fragment>/surface_volume`

et, s’il ne trouve rien, il ne plante pas immédiatement: il termine avec état « 100% offline » mais zéro activité.

### Traduction non technique

Le notebook « historique » ouvre la bonne porte (dataset compétition `test_images`).
Le core compact ouvrait une autre porte (`train/test fragment_dirs`).
Comme cette porte ne contenait rien dans l’exécution concernée, le moteur a démarré puis s’est arrêté sans matière à traiter.

## Cause principale B — Signal de réussite trompeur

`status = "100%_OFFLINE_ACTIVATED"` décrit uniquement l’état du runtime (mode offline actif), pas la réussite du traitement data.

Donc « 100% » ici ne veut **pas** dire:
- 100% des fichiers traités,
- 100% de soumission générée,
- 100% du pipeline scientifique validé.

## Cause principale C — Divergence d’architecture entre références et adaptation

Le notebook de référence est un pipeline compétition « strict »:
- fail-fast si `test_images` absent,
- packaging `submission.zip` exact,
- logs de fin `EXEC_COMPLETE` + `READY: /kaggle/working/submission.zip`.

Le core compact historique a simplifié et changé le contrat d’entrées/sorties, ce qui a cassé la compatibilité opérationnelle attendue.

---

## 4) Pourquoi les notebooks de référence avaient déjà les solutions

Parce qu’ils incluent explicitement les garde-fous manquants dans le core compact initial:

1. **Chemin racine compétition exact**
   - `/kaggle/input/competitions/vesuvius-challenge-surface-detection`
2. **Validation stricte des entrées**
   - exception immédiate si `test_images` n’existe pas
3. **Contrat de sortie stable compétition**
   - `submission.zip` avec `.tif` nommés comme les entrées test
4. **Preuve d’exécution terminée**
   - événements `EXEC_COMPLETE` et `READY` affichés

En clair: les réponses existaient, mais elles n’ont pas été totalement réintégrées de manière cohérente lors du passage au core compact.

---

## 5) Écart Avant / Après (pédagogique)

## Avant (run qui finit à zéro)

- Le moteur s’initialise.
- Il cherche des structures de dossiers non alignées avec ce run Kaggle.
- Il ne trouve aucun item train/test.
- Il termine quand même « proprement » avec métriques nulles.

## Après attendu (run correct)

- Le moteur détecte automatiquement le bon layout (`test_images` ou `fragment_dirs`).
- Si aucun test input: erreur explicite avec liste des chemins vérifiés.
- Si inputs présents: allocations, inférence, merkle, métriques > 0.
- `submission.zip` et éventuellement `submission.csv` produits.
- Validation explicite des membres zip (missing/extra).

---

## 6) Suggestions de correction (priorisées)

## P0 — Stopper les faux positifs de succès

- Condition de fin stricte: si `len(test_items)==0`, lever une erreur bloquante explicite.
- Ne jamais publier `status=100%` avec `total_pixels_processed==0` sans flag d’échec associé.

## P0 — Verrouiller le contrat d’entrée Kaggle

- Auto-discovery des racines `vesuvius` (input standard + competitions).
- Journaliser `configured_data_root`, `effective_data_root`, `discovery_attempts`.

## P0 — Garder le contrat de sortie compétition

- Générer `submission.zip` en priorité pour Vesuvius competition.
- Vérifier strictement que les noms `.tif` du zip correspondent aux fichiers test.

## P1 — Séparer « état runtime » et « état métier »

- `runtime_status`: offline/online
- `pipeline_status`: success/failed/no_input

Cela évite la confusion « 100% activé » vs « 0 pixel traité ».

## P1 — Ajouter des invariants de sécurité

- Invariant 1: `total_pixels_processed > 0` quand `pipeline_status=success`
- Invariant 2: `submission_zip != null` quand des tests existent
- Invariant 3: `zip_members_validated == True` en mode compétition strict

## P2 — Audit pédagogique reproductible

- Générer un mini rapport final auto (json/md) avec:
  - root choisi,
  - nb items train/test,
  - nb fichiers submission,
  - verdict.

---

## 7) Réponse directe à ta question « pourquoi c’est encore à zéro ? »

Parce que l’exécution qui a produit ces logs n’a trouvé **aucune entrée test** dans le layout recherché par cette version du script, et a terminé sans bloquer (design permissif), en marquant seulement l’activation offline mais sans traitement data réel.

Donc ce n’est pas un « moteur qui a calculé faux » ; c’est surtout un « moteur qui n’a rien reçu à calculer » à cause d’un mismatch d’architecture d’entrées.

---

## 8) Validation des preuves utilisées

Ce diagnostic s’appuie sur:
- les logs extraits de `results.zip` (state + forensic + metrics),
- le notebook de référence Vesuvius qui montre le contrat exact `test_images` + `submission.zip` + fail-fast,
- le script compact NX46 historique qui recherchait principalement `train/test` fragments.

---

## 9) Conclusion opérationnelle

Le problème « tout à zéro » est **structurel (contrat d’entrée/sortie et logique de garde)**, pas un simple bug numérique.

La résolution correcte consiste à:
1. réaligner strictement la découverte de dataset avec les layouts Kaggle réels,
2. empêcher toute fin “success-like” sans data traitée,
3. imposer les invariants de sortie compétition.

C’est exactement ce qu’il faut verrouiller pour éviter de revivre ce scénario dans les prochains runs.
