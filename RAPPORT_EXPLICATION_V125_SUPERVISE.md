# RAPPORT V125 — AUDIT COMPLET ET CORRECTIONS A→Z

## Objectif
V125 corrige le point critique identifié: **absence d'apprentissage supervisé réel** malgré présence de `train_images/` et `train_labels/`.

## Corrections structurelles intégrées
- Activation d'un mode supervisé réel (`supervised_train`) avec découverte automatique des paires train image/label.
- Construction d'un dataset pixel-level équilibré (positif/négatif) avec sampling contrôlé par volume.
- Split train/validation (volumes distincts), puis entraînement du neurone ATOM avec objectif val CE/F1/IoU.
- Fallback automatique vers mode autonome uniquement si labels indisponibles.
- Maintien de toute la couche forensic/Merkle/hardware déjà demandée.

## Golden nonces — extraction spatiale
V125 exporte maintenant la liste spatiale des meilleurs golden nonces:
- `golden_nonce_points_topk`: coordonnées `(y, x)` + score proba
- top-K configurable (`V125_GOLDEN_NONCE_TOPK`, défaut 11)

## Réponse directe aux critiques V123/V124
- "1 seul fichier traité" : V125 entraîne d'abord sur train pairs, puis infère sur test.
- "proxy F1 artificiel" : en mode supervisé, métriques validation réelles sont calculées.
- "masque vide" : seuils par défaut assouplis (`strong_th=0.65`, `dust_min_size=24`) + ratio adaptatif hybride.
- "golden nonce non exploités" : désormais tracés spatialement et persistés dans `NX47_METRICS`.

## Métriques attendues en logs V125
- `SUPERVISED_TRAIN` avec `train_samples`, `val_samples`, `leaderboard_top5`.
- `NX47_METRICS.train_mode` = `supervised` ou `autonomous_fallback`.
- `GLOBAL_STATS.files_supervised_mode` / `files_autonomous_fallback`.
- Liste `golden_nonce_points_topk` pour audit manuel/forensic.

## Limites assumées
- Cette V125 reste un modèle atomique léger (pas encore U-Net 2.5D).
- Mais elle supprime l'erreur fondamentale: entraînement test-only sans labels.
