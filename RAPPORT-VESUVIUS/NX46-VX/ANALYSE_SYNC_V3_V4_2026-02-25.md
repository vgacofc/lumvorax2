# Analyse de synchronisation dépôt + contrôle des premiers logs (V3 vs V4)

Date: 2026-02-25

## 1) Mise à jour avec le dépôt distant

- Remote configuré: `origin = https://github.com/lumc01/Lumvorax.git`.
- La branche locale courante `work` pointe déjà sur le même commit que `origin/main` (`19ac71ed`), donc le dépôt local est **à jour**.

## 2) Vérification des premiers logs d’exécution

Fichiers inspectés:
- `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v3/logV3.md`
- `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v4/LOGS v4.md`

### Constat commun (V3 et V4)

- Même environnement Kaggle GPU `Tesla P100`, image conteneur « Latest Container Image ».
- Même série d’avertissements de démarrage CUDA/XLA (cuFFT/cuDNN/cuBLAS + computation placer dupliqué) dans les premières secondes.
- Aucun `Traceback`, aucune `Exception`, aucun `OOM`, aucun `Killed` trouvé dans les logs analysés.
- Les deux exécutions entrent ensuite dans le pipeline instrumenté (événements JSON signés / merkle chain), puis continuent en préflight/chargement volumique.

### Différences utiles V3 vs V4

1. **Nettoyage partiel de warning notebook en V4**
   - V3 contient `MissingIDFieldWarning` (`nbformat`), absent en V4.

2. **Patch V4 bien chargé**
   - V4 contient un événement `V4_RUNTIME_PATCH_BOOT` avec overrides GPU/RAM/nonce persist.
   - V3 contient uniquement `V3_RUNTIME_PATCH_BOOT`.

3. **Anomalie de version potentielle (importante)**
   - Dans les deux logs, l’événement `BOOT` annonce `"version": "NX46-VX V2"` alors que les runs sont nommés `...V3` et `...v4`.
   - Cela suggère une **incohérence d’étiquetage de version** (version logique interne non alignée avec la version de run).

4. **Signature d’artefact précoce inchangée**
   - Les deux runs montrent `FUSION_SCORE_GLOBAL` à `0.0` pour `1407735.tif` et un `SUBMISSION_READY` très tôt (~60.7s), puis continuent vers des étapes volumétriques lourdes.
   - Ce comportement est cohérent entre V3 et V4 (pas de régression visible dans le démarrage).

## 3) Détection d’anomalies

### Anomalies confirmées

- **Incohérence de version runtime**: `BOOT.version = NX46-VX V2` dans des runs V3/V4.
- **Warnings CUDA/XLA dupliqués** au démarrage (bruit technique connu, non bloquant mais polluant).

### Points à surveiller (non bloquants sur ce contrôle)

- Les exports de logs Markdown contiennent des lignes tronquées (`... chars truncated ...`) qui limitent l’audit forensique complet de certains événements JSON.
- Dans l’extrait contrôlé, on voit les chemins de trace V4 (`gpu_ram_trace_v4.jsonl`, `golden_nonce_events_v4.jsonl`) mais pas encore d’événements explicites de télémétrie GPU/RAM dans la portion observée.

## 4) Nouvelles découvertes (inconnues) issues de ce contrôle

- **Découverte A**: La chaîne runtime V4 charge simultanément les patches V3 et V4 au boot (`V3_RUNTIME_PATCH_BOOT` puis `V4_RUNTIME_PATCH_BOOT`).
  - Interprétation: compatibilité ascendante volontaire possible, mais à confirmer pour éviter doubles effets de patch.

- **Découverte B**: Le marquage de version interne (`NX46-VX V2`) persiste à travers V3 et V4.
  - C’est probablement un héritage de constante non mise à jour, avec risque de confusion dans les audits/versioning.

## 5) Recommandations rapides

1. Corriger le champ de version interne exposé dans `BOOT` pour refléter réellement V3/V4.
2. Réduire le bruit CUDA init (si possible via ordre d’import/initialisation unifié) pour clarifier les logs.
3. Ajouter un événement explicite périodique de télémétrie V4 (GPU/RAM) visible dans le log principal, pas seulement via fichier annexe.
4. Éviter l’export de lignes JSON tronquées pour les rapports d’audit (ou publier aussi les `.jsonl` bruts signés).
