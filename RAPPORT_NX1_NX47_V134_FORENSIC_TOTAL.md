# Rapport forensic total NX-1 → NX-47 → v134

## Portée de l’audit effectué
- Vérification code exécutable v134 (`nx47_vesu_kernel_v134.py`).
- Vérification run réel v133 via pull GitHub du log `nx47-vesu-kernel-new-v133.log`.
- Vérification des couches de continuité NX (matrice explicite + assertions fail-fast).

## Continuité version par version (synthèse technique)
- **NX-1..NX-10**: invariants d’entrée/prétraitement conservés via chemins de lecture robustes + normalisation volume.
- **NX-11..NX-20**: signature/selection features conservées (`extract_multi_features`, `auto_select_features`).
- **NX-21..NX-35**: traçabilité, hash/signatures, intégrité des événements.
- **NX-36..NX-47**: Merkle chain, roadmap realtime, forensic/memory tracking.
- **v115..v134**: train supervisé, blocage fallback, audit d’apprentissage réel, seuil minimal de paires train, garde anti-pattern métrique hardcodée.

## Vérifications réelles v133 (pull distant)
- `TRAIN_DISCOVERY.pair_count=786` -> inférieur à 1500.
- `SUPERVISED_TRAIN.status=ok`, mais `val_f1_mean_supervised=0.0` et `val_iou_mean_supervised=0.0`.
- `NX_CONTINUITY_OK` présent dans le run.

## Renforcements v134 implémentés
- `min_train_pairs_required=1500` (bloque si seuil non atteint).
- `require_train_completion_100=True` (bloque si epochs observées < epochs configurées).
- `forbid_autonomous_mode=True` (interdit mode autonome).
- `enforce_no_hardcoded_metrics=True` (bloque motif d’objectifs d’epoch identiques).
- `learning_percent_real` calculé et injecté dans `global_stats` + metadata.
- `reasoning_trace_events` injecté dans `global_stats`.

## Comparaison avec modèles existants connus
- **Pipelines standards (U-Net/SegFormer/PyTorch)**: exigent courbes loss/val, checkpoints, métriques val non-nulles pour valider l’apprentissage.
- **NX v134**: ajoute forensic bit-level + Merkle + traces ns + garde-fous d’intégrité/continuité; mais l’apprentissage n’est considéré valide que si les métriques runtime sont observées et cohérentes.

## Verdict expert
- Continuité technologique NX: **oui (codée et auditée)**.
- Traçabilité bit/ns: **forte (logs signés + Merkle + timestamps ns + array bit trace)**.
- Apprentissage “réel validé”: **conditionnel aux métriques runtime non nulles**.
- Falsification/placeholder/fallback: **fortement réduits en v134 par design fail-fast**.
