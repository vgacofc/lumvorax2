# Rapport NX-47 : apprentissage réel, arrêt sans fallback, et continuité NX-1 → NX47-VESU-kernel-new-v132

## 1) Réponse directe à la question: « est-ce que NX-47 apprend réellement ? »

Sur la base stricte des métriques fournies (issues de `v132 3.log` + `results 10.zip`), **l’apprentissage paramétrique n’est pas démontré**.

Indicateurs clés observés:
- `supervised_train: true`, `supervised_epochs: 3`, `unet_epochs: 2` (entraînement déclaré)
- mais `val_f1_mean_supervised: 0.0`, `val_iou_mean_supervised: 0.0`, `unet_25d_best_fbeta: 0.0`
- absence de trace de loss par epoch
- `active_neurons_start_total: 0` puis `6` en milieu/fin (activation dynamique, pas preuve d’optimisation des poids)

Conclusion factuelle:
- Le pipeline exécute une chaîne de traitement/segmentation/calibration.
- **La preuve de mise à jour de poids (train effectif) n’apparaît pas dans les artefacts fournis.**

---

## 2) Processus A → Z de NX-47 (version réelle observée v132)

### A. Ingestion volume
1. Chargement du volume 3D (320 tranches).
2. Comptage pixels: `32768000`.
3. Préparation des slices pour segmentation/inférence.

### B. Prétraitement
4. Normalisation/standardisation des intensités (implicite pipeline).
5. Fenêtrage par tranches pour extraction de motifs.

### C. Détection structurelle
6. Détection d’anchors: `pixels_anchor_detected: 0`.
7. Détection papyrus sans anchor: `6144` pixels.
8. Détection matériaux/patterns: `933` / `933`.

### D. Inférence modèle
9. Passage UNet 2.5D déclaré `ok`.
10. Distribution de probabilités basse confiance:
   - `max=0.14585`
   - `mean=0.09743`
   - `std=0.03557`
11. Aucun score > 0.2 signalé.

### E. Calibration
12. Courbe ratio F1 interne:
   - best ratio ≈ `0.205918`
   - best F1 ≈ `0.686275`
13. Cette F1 est une calibration interne de ratio, **pas une validation supervisée prouvée**.

### F. Mécanique neuronale NX
14. Neurones actifs au départ: `0`.
15. Activation en cours: `6`.
16. Fin: `6`.
17. `mutation_events: 0`, `pruning_events: 1`.

### G. Forensic / traçabilité
18. Génération logs JSON + roadmap + Merkle + forensic report.
19. Production `submission.zip`.
20. Progression roadmap incohérente (train done 100%, segment 20%, val 0).

---

## 3) Comparaison claire avec modèles connus (PyTorch standard, U-Net, ViT/SegFormer)

## Ce qu’un pipeline standard montre normalement
- Loss train/val par epoch.
- Courbes learning rate.
- checkpoints `best.pt` / `last.pt`.
- métriques validation non nulles (F1/IoU/Dice).
- confusion matrix / PR curve / ROC (selon tâche).
- journal split train/val reproductible.

## Ce que NX-47 v132 montre actuellement
- Infrastructure d’audit forte (forensic, Merkle, metadata).
- Pipeline d’inférence-calibration opérationnel.
- Pas de preuve explicite d’entraînement effectif multi-epoch.
- Métriques supervisées nulles malgré mode supervisé activé.

## Différence principale
- **Modèles connus**: orientés preuve d’apprentissage mesurable.
- **NX-47 v132**: orienté robustesse d’exécution + audit + calibration, avec apprentissage déclaré mais non prouvé par logs/métriques.

---

## 4) Exigence demandée: suppression des fallbacks (arrêt net si échec)

Politique proposée pour V133:

1. `FALLBACK_DISABLED = true` global.
2. Si labels absents/corrompus → `raise FatalPipelineError("LABELS_MISSING")`.
3. Si split invalide (train ou val vide) → arrêt immédiat.
4. Si loss non calculable (`NaN`, `inf`, `None`) → arrêt.
5. Si aucun `optimizer.step()` exécuté à la fin epoch 1 → arrêt.
6. Si `val_f1_mean_supervised == 0` ET aucune métrique brute de validation présente → arrêt + code erreur.
7. Si UNet status != `ok` → arrêt, pas de soumission.
8. Interdiction de produire `submission.zip` si phase train/val non certifiée.

Objectif:
- **Zéro simulation de progrès**.
- **Zéro “mode autonome fallback” silencieux**.
- **Traçabilité binaire: réussi prouvé / stoppé prouvé**.

---

## 5) Continuité NX-1 → NX-47 → `nx47-vesu-kernel-new-v132`

Cadre de compatibilité pour ne pas perdre la base historique:

1. **NX-1..NX-10 (socle)**
   - conserver les invariants de prétraitement et format d’entrée.
2. **NX-11..NX-20 (stabilisation)**
   - conserver signature des features et schéma de sortie intermédiaire.
3. **NX-21..NX-35 (industrialisation)**
   - conserver protocole d’audit, hashes, checks d’intégrité.
4. **NX-36..NX-47 (adaptativité/forensic avancé)**
   - conserver moteur de traçabilité + Merkle + roadmap temps réel.
5. **V132 actuelle**
   - garder ce qui marche (forensic/exécution),
   - remplacer la partie “train déclaré” par un train prouvé/strict.

Règle d’or:
- Aucun module legacy retiré sans test de non-régression fonctionnelle.
- Toute évolution doit être additif + drapeaux de compatibilité versionnés.

---

## 6) Avant / Après (d’après ton analyse réelle)

## Avant (V132 observée)
- Pipeline exécuté bout en bout.
- Segmentation/inférence partielle.
- Métriques supervision à `0.0`.
- Fallback autonome à `0` (bien), mais pas d’arrêt fatal sur incohérences.

## Après (V133 cible corrigée)
- Arrêt net si labels/split/metrics invalides.
- Logs obligatoires par epoch: `train_loss`, `val_loss`, `val_f1`, `val_iou`, `optimizer_steps`.
- Checkpointing obligatoire (`best`, `last`) + hash modèle.
- Blocage de la soumission si apprentissage non certifié.
- Rapport final “preuve d’apprentissage” auto-généré.

---

## 7) Diagnostic synthétique final

- Ton constat est correct: **NX-47 v132 se comporte surtout comme moteur d’inférence calibrée auditable**.
- L’apprentissage réel n’est pas falsifié, mais **non démontré** avec les preuves actuelles.
- La priorite V133: transformer “train déclaré” en “train vérifiable mathématiquement et loggable” avec politique **fail-fast sans fallback**.

---

## 8) Réintégration effective NX-1 → NX-47 (implémentation V132 stricte)

Pour répondre au point critique: **non, NX-47 v132 ne doit pas perdre l’héritage NX-1**.
La version corrigée réintègre explicitement les couches suivantes dans une chaîne de compatibilité vérifiée au démarrage:

- NX-1..NX-10: invariants prétraitement + garde format input.
- NX-11..NX-20: signature features + schéma intermédiaire.
- NX-21..NX-35: audit hash-chain + intégrité.
- NX-36..NX-47: forensic + Merkle + roadmap temps réel + télémétrie neurones.
- NX-47 v115..v132: gate strict d’évidence train + seuil adaptatif + roadmap.

Si une capacité manque, le pipeline s’arrête immédiatement (`FatalPipelineError`).

---

## 9) Logs stricts + tests unitaires exigés

Implémentation cible validée:
- Export automatique `v132_roadmap_realtime.json` à chaque étape.
- Export `v132_execution_logs.json` (hash-chain) et `v132_execution_metadata.json` (digest + Merkle root).
- Tests unitaires:
  1. échec immédiat si dossier `/test` absent;
  2. succès contrôlé avec fragments mock + génération submission + roadmap 100% + forensic metadata.

---

## 10) Avant / Après final

### Avant
- Héritage NX historique décrit mais non verrouillé en exécution.
- Train déclaré mais non prouvé.
- Incohérences roadmap possibles.

### Après
- Héritage NX-1→NX-47 verrouillé par `compatibility_layers` obligatoire.
- Politique fail-fast active et vérifiée.
- Roadmap temps réel cohérente et calculée automatiquement jusqu’à 100%.
- Logs forensic stricts + tests unitaires reproductibles.

---

## 11) Correctif demandé: réintégration directement dans `nx47_vesu_kernel_v132.py` (sans rétrogradation)

Mise à jour appliquée **dans le noyau historique 1579 lignes** `nx47_vesu_kernel_v132.py` (et non en remplacement d’un fichier réduit):

- Correction de cohérence de type/config vers `V132Config` sur les signatures et l’initialisation noyau.
- Ajout d’une matrice explicite de continuité NX (`_build_continuity_matrix`) couvrant NX-1..NX-47 + v115..v132.
- Vérification d’intégrité de continuité au démarrage (`_assert_continuity_integrity`) avec log `NX_CONTINUITY_OK`.
- Activation stricte anti-rétrogradation: si `supervised_train=True` et données train absentes, arrêt immédiat en mode `strict_no_fallback`.
- Injection de la matrice de continuité dans les métadonnées d’exécution.

Objectif respecté:
- **Pas de destruction des versions antérieures v132**.
- **Réintégration effective des héritages NX dans le fichier v132 principal**.
