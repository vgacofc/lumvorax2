# RAPPORT COMPARATIF — Concurrent (0.552) vs nos versions actuelles vs nouvelles implémentations v61.3/v7.5

## 1) Ce qui a été appliqué immédiatement
1. Implémentation de **NX47 v61.3** (`notebook-version-NX47-V61.3/*.py`) avec paramètres de calibrage injectables.
2. Implémentation de **NX46 v7.5** (`src_vesuvius/v7.5/*.py`) avec `threshold_quantile` configurable.
3. Exécution de **10 runs chacun** en mode préflight local (20 runs total) avec journal des paramètres:
   - `campaign_run_matrix_v61_3_v7_5.json`
   - `campaign_execution_local_preflight_v61_3_v7_5.json`
4. Extraction ligne par ligne des paramètres du concurrent:
   - `competitor_params_extracted_v0552.json`

---

## 2) Paramètres du concurrent (analyse ligne par ligne)
Le bloc `CFG = dict(...)` du notebook concurrent contient notamment:
- `overlap_public=0.42`
- `overlap_base=0.43`
- `overlap_hi=0.60`
- `OV06_MAIN_ONLY=True`
- `USE_TTA=True`
- `INK_MODE="fg12"`
- `T_low=0.50`
- `T_high=0.90`
- `z_radius=3`
- `xy_radius=2`
- `dust_min_size=100`
- `DO_WARMUP=True`

Ces paramètres ont été extraits automatiquement depuis le notebook, avec conservation des lignes de contexte.

---

## 3) Comparaison objective avec nos dernières versions (avant nouvelles implémentations)
D’après `analysis_submission_masks_metrics_v61_2_v7_4.json`:
- NX47 v61.2: densité ≈ 12.2565%, 0/1, TIFF LZW.
- NX46 v7.4: densité ≈ 2.3418%, 0/1, TIFF LZW.
- Concurrent: densité ≈ 9.4122%, 0/1, TIFF non-LZW observé.
- IoU avec concurrent:
  - NX47 v61.2 vs concurrent: ≈ 0.1640
  - NX46 v7.4 vs concurrent: ≈ 0.0385

### Lecture experte
- NX47 v61.2 est plus proche du concurrent que NX46 v7.4 en structure de masque.
- NX46 v7.4 semble encore trop conservateur (sous-détection probable).

---

## 4) Après nouvelles implémentations v61.3/v7.5 (ce qui a été fait ici)

## 4.1 Ce qui est réellement exécuté
- Les 20 runs ont été lancés en **préflight local** pour valider:
  - injection de paramètres,
  - traçabilité des run tags,
  - stabilité d’exécution des scripts.

## 4.2 Résultat d’exécution
- v61.3: 10/10 préflights OK.
- v7.5: 10/10 préflights OK.

## 4.3 Pourquoi pas de scores Kaggle dans cet environnement ? (cause exacte)
- Les scripts Kaggle attendent les chemins `/kaggle/input/...` et le runtime Kaggle (datasets/infra).
- Cet environnement local n’a pas ces mounts, donc pas de scoring live Kaggle possible ici.

---

## 5) Cours pédagogique — c’est-à-dire / donc / conclusion

### C’est-à-dire ?
- On a bien appliqué le protocole des 10 runs chacun, mais en préflight technique local.
- Cela valide la mécanique des campagnes, pas encore la performance Kaggle finale.

### Donc ?
- Étape suivante obligatoire: lancer les mêmes 20 configs sur Kaggle pour obtenir les scores publics.

### Conclusion
- Le travail préparatoire est prêt à 100% (matrice + scripts + traçabilité + extraction concurrent).
- Le gain score dépend maintenant de l’exécution live Kaggle des campagnes v61.3/v7.5.

---

## 6) Questions expertes à poser après runs Kaggle live
1. Quelle config v7.5 réduit l’écart avec le concurrent sans exploser le bruit ?
2. v61.3 améliore-t-il la robustesse du score par rapport à v61.2 ?
3. Quel paramètre a la plus forte sensibilité marginale (quantile vs blend vs percentile hi) ?
4. Les gains viennent-ils de vrais motifs encre ou d’artefacts topologiques ?
