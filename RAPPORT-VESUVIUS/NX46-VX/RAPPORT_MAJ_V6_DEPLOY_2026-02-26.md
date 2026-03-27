# Mise à jour ciblée NX46-VX vers V6 (base V5 conservée)

## Ce qui a été fait
- Scan du périmètre `RAPPORT-VESUVIUS/NX46-VX` (notebooks, rapports `.md`, résultats V3/V4/V5) pour réaligner le point de départ sur `result-nx46-vx-v5/nx46-vx-unified-kaggle-v5.ipynb`.
- Création de la **V6 par copie directe de la V5** via l'outil `tools/create_v6_from_v5_notebook.py`.
- Génération du notebook cible: `result-nx46-vx-v6/nx46-vx-unified-kaggle-v6.ipynb`.

## Garanties respectées
- Le nom du fichier source V5 n'a pas été modifié.
- La V5 (12k+ lignes) reste intacte, et la V6 est dérivée de cette base complète.
- Aucun renommage parasite de type `nx47_vesu_kernel_v138*.py` n'a été introduit.

## Patch V6 intégré
1. Version runtime alignée en `NX46-VX V6`.
2. `log_version_tag` migré en `nx46vx_v6` pour éviter mélange des sorties V2/V5.
3. Politique GPU au démarrage:
   - priorité `H100` si détecté,
   - fallback `P100` sinon,
   - fallback `CUDA_GENERIC` sinon,
   - puis `CPU` si aucune carte visible.
4. Variables de continuité activées:
   - `NX46VX_FORENSIC_MODE=FULL`
   - `NX46VX_RESUME_STRICT=1`
   - `NX46VX_CHECKPOINT_MODE=REALTIME`

## Clarification sur `nx47_vesu_kernel_v138.py`
- Cette mise à jour V6 demandée est centrée sur `NX46-VX` et le notebook V5 ciblé.
- Le fichier `nx47_vesu_kernel_v138.py` n'a pas été utilisé comme base de la V6 ici pour éviter la dérive hors périmètre demandé.
