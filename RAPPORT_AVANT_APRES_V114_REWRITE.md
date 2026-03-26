# Rapport avant / après — revue complète V114

Ce document synthétise **avant/après chaque modification majeure** introduite entre l’ancienne base (`nx47_vesu_kernel.py`) et la réécriture standalone (`nx47_vesu_kernel_v114_rewrite.py`).

## 1) Stratégie de livraison du code

### Avant
- Le code V114 était mêlé à l’ancien fichier principal, ce qui complexifiait le suivi des régressions.

### Après
- Une version réécrite isolée est livrée dans un nouveau fichier : `nx47_vesu_kernel_v114_rewrite.py`.
- L’entrée principale est explicite via la classe `NX47V114RebuildKernel`.

**Impact**: meilleure traçabilité des changements et rollback simplifié.

---

## 2) Configuration explicite du pipeline

### Avant
- Plusieurs paramètres étaient dispersés et moins centralisés.

### Après
- Les paramètres critiques sont centralisés dans `V114Config` (`t_low`, `t_high`, `z_radius`, `xy_radius`, `dust_min_size`, `max_layers`, etc.).

**Impact**: tuning plus fiable et reproductible.

---

## 3) Robustesse Kaggle offline (dépendances)

### Avant
- Risque d’échec si l’environnement n’exposait pas directement `imagecodecs`/`tifffile` compatibles.

### Après
- Réintégration d’un bootstrap offline robuste:
  - `install_offline`
  - `bootstrap_dependencies_fail_fast`
  - `ensure_imagecodecs`
- Gestion des roues locales Kaggle + fallback `--find-links`.

**Impact**: démarrage plus robuste en environnement Kaggle offline.

---

## 4) I/O TIFF LZW résilient

### Avant
- Le décodage/encodage pouvait casser en absence de codec chargé dynamiquement.

### Après
- `read_tiff_lzw_safe` tente `tifffile`, puis réinstalle/rafraîchit les codecs, puis fallback Pillow multi-frame.
- `write_tiff_lzw_safe` tente LZW `tifffile`, puis fallback Pillow (`tiff_lzw`).

**Impact**: réduction forte des erreurs I/O en production.

---

## 5) Résolution de racine dataset tolérante aux variations

### Avant
- Un seul chemin invalide pouvait stopper le run (`FileNotFoundError`).

### Après
- `_resolve_root` teste plusieurs chemins standards Kaggle et ajoute une recherche fallback par similarité du nom de dossier.

**Impact**: tolérance aux variantes/typos de chemin.

---

## 6) Observabilité et forensic logs

### Avant
- Logs moins structurés pour audit mémoire reproductible.

### Après
- `PlanTracker` écrit l’avancement temps-réel par étape.
- `MemoryTracker` fingerprint les tenseurs (shape/dtype/min/max/mean/hash SHA-512).
- `log()` signe chaque événement avec SHA-512.

**Impact**: auditabilité déterministe, plus simple pour analyses d’écart.

---

## 7) Segmentation et post-traitement

### Avant
- Pipeline moins explicite sur le chaînage “public-anchor + private-prob + hystérésis + topo”.

### Après
- `segment_volume` applique:
  1. lissage multi-échelle,
  2. score `private_prob`,
  3. masques `strong/weak` avec `public_anchor`,
  4. `binary_propagation` (hystérésis),
  5. fermeture 3D (`_build_struct` + `binary_closing`),
  6. suppression petites composantes (`_remove_small_components`),
  7. pixel-trace optionnel.

**Impact**: segmentation plus stable et plus explicable dans les logs.

---

## 8) Packaging compétition + artefacts

### Avant
- Sorties parfois moins standardisées selon version en cours.

### Après
- Pipeline complet `discovery -> load -> features -> segment -> overlay -> package`.
- Génère `submission.zip` + metadata + logs d’exécution + mémoire + rapport diff notebooks concurrents.

**Impact**: conformité compétition et meilleur diagnostic post-run.

---

## 9) Avant / Après synthèse rapide

- **Avant**: version V114 partiellement intégrée, plus sensible aux variations d’environnement/chemin.
- **Après**: version V114 standalone, robuste offline, I/O LZW sécurisé, segmentation explicite, logs forensic signés, sortie compétition standard.

