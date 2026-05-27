# NX48 Unified Ultra

Cette version crée un **run unifié** dans un **dossier unique** (`artifacts/unified_run_<UTC>_<pid>/`) pour éviter tout mélange avec les autres versions NX.

## Objectifs couverts
- Lecture et inventaire ligne-par-ligne des versions de `src/nx_versions` (`.cpp`, `.py`, `.lean`, `.md`).
- Traçabilité brute totale: hashes SHA256, versions environnement, provenance, métriques, journal d'exécution.
- Intégration des résultats précédents Hubbard HTS via détection du dernier rapport `research_*`.
- Sortie de preuves formelles Lean (axiomes, lemmes, théorèmes) dans le même dossier d'artefacts.
- Questions expertes + nouveaux tests + anomalies dans des fichiers dédiés, tous au même endroit.

## Exécution
```bash
bash src/nx_versions/nx48_unified_ultra/run_nx48_unified_ultra.sh
```

Le script n'écrase jamais les exécutions précédentes.
