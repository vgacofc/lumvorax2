# PROMPT REPLIT AGENT — NQubit_v4 (Forensic Nano-precision)

Tu dois exécuter **strictement** le module `src/projetx_NQubit NX/NQubit_v4` et produire des logs forensiques profonds avec détection matériel Replit.

## Objectifs
1. Détecter matériel/runtime Replit (CPU, RAM, kernel, fréquence, VM/container info).
2. Exécuter toute la suite NQubit_v4.
3. Capturer et préserver les artefacts forensiques nanoseconde.
4. Produire un rapport final comparatif V2/V3/V4 + comparaison online disponible.

## Étapes obligatoires
1. `cd src/projetx_NQubit NX/NQubit_v4`
2. `make clean all test_all run`
3. `python tools/generate_v4_report.py`
4. Exécuter 5 runs consécutifs de stabilité:
   - `for i in 1 2 3 4 5; do make run; done`
5. Générer manifest hash:
   - `sha256sum results/* > results/sha256_replit_v4.txt`
6. Collecter infos hardware/env:
   - `uname -a > results/replit_env_info.txt`
   - `lscpu >> results/replit_env_info.txt || true`
   - `cat /proc/meminfo >> results/replit_env_info.txt`
   - `cat /etc/os-release >> results/replit_env_info.txt || true`

## Rapport attendu
Créer `RAPPORT_REPLIT_EXECUTION_V4_FINAL.md` contenant:
- Résumé ET EXPLUQUE EN DETAIL KPI (benchmark, P0/P1, bruit hardware)
- Différences en % vs V3 et V2
- Analyse de stabilité sur 5 runs
- Liste anomalies/écarts
- Threats to validity
- Comparaison avec systèmes/références online disponibles (citer sources)
- Recommandation GO/NO-GO

## Règles strictes
- Ne pas modifier les versions antérieures.
- Ne pas supprimer d’artefacts existants.
- Tout nouveau fichier doit être dans `src/projetx_NQubit NX/NQubit_v4`.
- Si une commande manque (ex: lscpu), continuer et noter l’écart dans le rapport.
