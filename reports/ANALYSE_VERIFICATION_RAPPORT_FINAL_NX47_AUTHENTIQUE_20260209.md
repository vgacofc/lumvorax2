# Analyse de vérification — RAPPORT_FINAL_NX47_AUTHENTIQUE.md

## Constat de disponibilité
Le fichier demandé `reports/RAPPORT_FINAL_NX47_AUTHENTIQUE.md` n'existe pas dans le dépôt à la date d'analyse.

Fichiers proches identifiés :
- `rapports_individuels/RAPPORT_NX47_RNA_AUTHENTIQUE.md`
- `rapports_individuels/RAPPORT_NX47_FINA_AUTHENTIQUE.md`
- `rapports_individuels/RAPPORT_LUMVORAX_NX47_FULL.md`

## Vérification d'authenticité (forensique)
1. Les rapports RNA/FINA contiennent des substitutions shell non exécutées (ex: `$(grep ...)`) au lieu de valeurs résolues.
2. Les logs référencés (`rapports_individuels/LOG_NX47_RNA.txt`, `rapports_individuels/LOG_NX47_FINA.txt`) sont absents.
3. Les rapports RNA/FINA sont datés de manière textuelle (05 Février 2026 pour RNA), mais la cohérence avec logs source ne peut être prouvée en absence des fichiers de preuve.

## Métriques réelles disponibles (source alternative NX v47 ARC)
Source utilisée : `logs_AIMO3/NX/v47_arc/run_20260202_174958/v28_forensic_logs/metrics_v28_474b3640bf67eabe.json`.

Extraits :
- Session: `474b3640bf67eabe`
- Durée: `46,471,502,266 ns` (~46.47 s)
- `B1_CREATE_DESTROY_OPS_SEC`: `1,173,199.019 ops/s`
- `B1_FUSE_ELEM_SEC`: `8,404,025.192 elem/s`
- `B1_SPLIT_PER_SEC`: `5,245,162.157 splits/s`
- `B1_MOVE_THROUGHPUT`: `3.287 GB/s`
- `B3_LOG_OVERHEAD`: `45.013%`
- `B3_REPLAY_SUCCESS`: `100%`
- `B6_IMPLEMENT_SCORE`: `80%`

## Conclusion
Le rapport demandé ne peut pas être audité directement car absent. Les rapports NX47 « authentiques » existants sont incomplets (placeholders non résolus + logs manquants). Des métriques réelles existent dans un autre artefact NX47 (v47_arc), mais elles ne valident pas à elles seules le document demandé.
