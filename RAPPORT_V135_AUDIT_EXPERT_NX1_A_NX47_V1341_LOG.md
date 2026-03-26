# Rapport expert V135 — Audit total NX-1 → NX-47 + analyse du log distant `nx47-vesu-kernel-new-v134.1.log`

## 0) Preuve de récupération du log distant
- Source: dépôt GitHub `lumc01/Lumvorax`.
- Fichier récupéré: `nx47-vesu-kernel-new-v134.1.log`.
- Taille observée localement après pull: `3480` lignes.

## 1) Résultats factuels v134.1 (point par point)
1. Continuité NX active: `NX_CONTINUITY_OK` présent.
2. Dataset découvert: `total_assets=1615`, `file_count=1` test.
3. Train pairs découverts: `pair_count=786`.
4. Entraînement supervisé exécuté: `status=ok`, `train_samples=960000`, `val_samples=320000`, `train_volumes=24`, `val_volumes=8`.
5. Pas de fallback autonome: `files_supervised_mode=1`, `files_autonomous_fallback=0`.
6. Qualité validation toujours nulle: `val_f1_mean_supervised=0.0`, `val_iou_mean_supervised=0.0`, `unet_25d_best_fbeta=0.0`.
7. Exécution complète: `EXEC_COMPLETE` + `submission.zip`.

### Diagnostic des problèmes rencontrés
- Problème principal: **apprentissage exécuté mais non validé en performance** (métriques val nulles).
- Problème structurel: **seuil fixe 1500** incohérent avec dataset réel observé (786 pairs).

---

## 2) Mise à jour V135 demandée (implémentée)

### 2.1 Adaptation automatique des vérifications à la taille réelle dataset
- Ajout `adapt_train_threshold_to_dataset_size`.
- Ajout `train_pair_coverage_target_pct`.
- Nouvelle logique: le seuil requis de paires est dérivé du **pair_count réel** au runtime.

### 2.2 Audit explicite de la taille des fichiers train/labels
- Ajout `_audit_train_dataset_size(...)`:
  - `pair_count_discovered`
  - `pair_count_selected_for_training`
  - `coverage_pct_selected_vs_discovered`
  - `total_train_image_bytes`
  - `total_train_label_bytes`
  - `required_pair_count`

### 2.3 Garde-fous anti-falsification renforcés
- Pas de fallback autonome (`forbid_autonomous_mode`).
- Détection pattern métrique suspect (`enforce_no_hardcoded_metrics`).
- Calcul `learning_percent_real` + `reasoning_trace_events`.

---

## 3) Traçabilité complète du neurone NX (exigence inspecteur)

### 3.1 Tableau de correspondance (synthèse)
- NX-1..NX-10: invariants entrée/prétraitement.
- NX-11..NX-20: signatures/features intermédiaires.
- NX-21..NX-35: audit/hash/intégrité.
- NX-36..NX-47: Merkle/roadmap/forensic.
- v115..v135: supervision stricte, no-fallback, audit apprentissage réel.

### 3.2 Conservé / modifié / désactivé
- Conservé: pipeline multi-étapes, neurone NX (w,b,alpha,beta), Merkle, logs.
- Modifié: gates de validation et seuil train adaptatif.
- Désactivé/interdit: mode autonome en production stricte.

---

## 4) Définition mathématique & algorithmique du neurone NX

### 4.1 Formulation
Soit une entrée feature `x`, gradient local `g`, opérateur laplacien `l`:

`z = w·x + b + alpha·g + beta·l`

`p = sigmoid(z)`

Décision via seuil calibré sur validation (`threshold_scan`).

### 4.2 Apprentissage
- Optimisation proximale (L1/L2) multi-candidats `(l1,l2)`.
- Score objectif = combinaison `cross_entropy`, sparsité, F1, Fbeta, IoU.
- Sélection meilleur hyperparamètre + pruning + mutation contrôlée si stagnation.

### 4.3 Pseudocode canonique
1. Extraire features + gradient.
2. Pour chaque epoch / neurone / (l1,l2):
   - Fit proximal,
   - Prédire sur val,
   - Scanner seuils,
   - Calculer objectif.
3. Conserver meilleur état.
4. Appliquer pruning/mutation selon règles.
5. Produire modèle final + trace forensic.

---

## 5) Preuve d’exécution réelle (non décorative)
- Événements runtime présents: `SUPERVISED_TRAIN`, `NX47_METRICS`, `GLOBAL_STATS`, `EXEC_COMPLETE`.
- Compteurs réels: volumes, samples, temps, ops, neurones actifs.
- Limite observée: métriques val nulles => exécution oui, qualité apprentissage non prouvée.

---

## 6) Dynamique interne et stabilité
- Mécanismes présents: seuil/hystérésis/calibration ratio global + slice-wise.
- Contrôle stabilité: pruning + quantile + mutation conditionnelle.
- Cas limites à monitorer: labels rares, mismatch train/val, confiance logit basse.

---

## 7) Comparaison avec standards connus
- Neurone linéaire: pas de mémoire gradient/laplacien native.
- Conv standard: capture locale mais pas ce mix explicite `w + alpha + beta` avec audit forensique intégré.
- Attention/récurrent: plus expressifs en séquence, mais coûts et traçabilité causale différents.
- NX: compromis explicabilité + forensic + contrôle heuristique domaine.

---

## 8) Invariants / garanties / contraintes
- Invariants: logs signés, Merkle chain, timestamps ns.
- Garanties v135: seuil train adaptatif dataset, no autonomous, anti-métriques identiques.
- Contraintes: la validité finale dépend toujours des métriques val observées.

---

## 9) Lien causal architecture -> résultat
- Causalité attendue: `train supervision + calibration seuil` -> segmentation plus stable.
- Résultat v134.1 observé: exécution complète mais score val nul; causalité performance non validée.
- Action v135: adaptation seuil paires + audits bytes + vérification stricte, pour empêcher faux “succès”.

---

## 10) Robustesse / reproductibilité / falsifiabilité
- Reproductibilité: seed, logs, metadata, script pull remote.
- Falsification explicite:
  - `TRAIN_PAIRS_BELOW_THRESHOLD`
  - `TRAIN_COMPLETION_100_FAILED`
  - `HARD_METRIC_PATTERN_DETECTED`
  - `AUTONOMOUS_MODE_FORBIDDEN`

---

## 11) Positionnement épistémologique
- NX ne remplace pas tous les modèles DL standards.
- NX complète un cadre forensic strict orienté traçabilité.
- NX ne prétend pas résoudre seul les limites d’annotation/qualité labels.

---

## 12) Checklist d’audit final (inspecteur)
- [x] Le neurone NX est formellement défini
- [x] Son exécution est prouvée
- [x] Son évolution est traçable
- [x] Sa dynamique est analysée
- [ ] Son apport est causalement démontré (bloqué par métriques val nulles v134.1)
- [x] Ses limites sont explicites

---

## 13) Plan d’audit formel / structure article-brevet / revue comité

### 13.1 Plan d’audit formel
1. Acquisition artefacts run (log, metadata, forensic, roadmap).
2. Vérification intégrité cryptographique.
3. Vérification exécution train (epochs, samples, coverage dataset).
4. Vérification causalité performance (val metrics, ablation).
5. Verdict signé.

### 13.2 Structure article technique / brevet
- Problème.
- Architecture NX formelle.
- Méthodes d’entraînement + calibration.
- Système forensic bit/ns.
- Résultats et limites.

### 13.3 Checklist comité experts
- Exécution prouvée ?
- Métriques val non nulles ?
- Effet causal après ablation ?
- Invariants forensic satisfaits ?
- Reproductibilité inter-runs ?
