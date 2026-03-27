# Rapport détaillé résultats + code (concurrent ancien vs concurrent dernier vs notre version)

## 1) Synchronisation dépôt
- Analyse recalculée depuis `origin/main` (dépôt distant mis à jour).

## 2) Comparaison résultats TIFF (dernier concurrent vs results02)
- Fichier results02: `/tmp/newcompare2/results02/1407735.tif`
- Fichier concurrent: `/tmp/newcompare2/results-concurrent/1407735.tif`
- SHA256 results02 TIFF: `1daafbb1828b76d345cae1b1227a89e10cdc4a54ab8976adb81354db1df6834b`
- SHA256 concurrent TIFF: `4135090f39acbf3f0be3af3999af1a8d7685e43e471cf1d13007fda2282f9a3d`
- shape: `(320, 320, 320)` vs `(320, 320, 320)`
- nonzero: `3054752` vs `3084176`
- densité globale: `0.093223632812` vs `0.094121582031`
- XOR: `41496` | IoU: `0.986571795074` | Dice: `0.993240513653`

## 3) Densité par slice
- CSV produit: `slice_density_results02_vs_concurrent_1407735.csv`
- 320 slices, colonnes: `z, our_nonzero, comp_nonzero, our_density, comp_density, abs_density_diff, xor_nonzero`.

## 4) Comparaison code source (ancien concurrent -> dernier concurrent -> notre version)

### 4.1 Snapshots source (ligne par ligne)
- Ancien concurrent: `code_snapshot_competitor_old_f26d45.txt`
- Dernier concurrent: `code_snapshot_competitor_new_latest.txt`
- Notre version courante: `code_snapshot_ours_current.txt`

### 4.2 Indicateurs rapides
- Ancien concurrent: shim protobuf ligne `None`, LUM_CFG ligne `94`, USE_LUM ligne `96`
- Dernier concurrent: shim protobuf ligne `32`, LUM_CFG ligne `None`, USE_LUM ligne `None`
- Notre version: shim protobuf ligne `14`, LUM_CFG ligne `91`, USE_LUM ligne `93`

### 4.3 Diffs extraits
- Diff ancien concurrent vs dernier concurrent: `diff_competitor_old_vs_new.txt`
- Diff dernier concurrent vs notre version: `diff_competitor_new_vs_ours.txt`

## 5) Conclusion opérationnelle
1. Le dernier concurrent diffère du résultat `results02` (XOR non nul, IoU/Dice < 1).
2. Les écarts de densité sont faibles mais systématiques (voir CSV slice-by-slice).
3. Le comparatif code est matérialisé par snapshots numérotés + diff textuel pour audit précis des modifications.
