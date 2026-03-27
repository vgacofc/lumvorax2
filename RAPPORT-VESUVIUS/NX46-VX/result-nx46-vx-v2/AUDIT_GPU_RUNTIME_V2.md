# Audit GPU et budget runtime — NX46-VX V2

## 1) Détection hardware

- GPU(s) détecté(s) au BOOT: `['Tesla P100-PCIE-16GB, 16384 MiB, 15564 MiB']`

## 2) Utilisation GPU prouvée dans les logs V2

- Des traces contenant des tokens GPU/CUDA existent, événements uniques:
  - `BOOT` @ `1771982327815910668`

## 3) Risque dépassement limite 9h Kaggle

- `elapsed_total_s`: `2542.322` s
- `elapsed_total_h`: `0.706` h
- marge vs 9h: `8.294` h
- ✅ Run en-dessous de 9h avec marge significative.

## 4) Conclusion

- V2 détecte la P100 au BOOT, mais la preuve d’utilisation GPU complète A→Z n’est pas assez explicite dans les traces publiques actuelles.
- Il faut ajouter des événements `GPU_PHASE_START/END` et un compteur `gpu_utilization_trace` en V3 pour lever toute ambiguïté.
