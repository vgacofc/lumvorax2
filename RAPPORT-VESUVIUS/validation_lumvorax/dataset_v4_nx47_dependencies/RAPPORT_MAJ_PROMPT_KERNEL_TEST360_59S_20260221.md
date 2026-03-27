# Mise à jour confirmée du prompt — Kernel `ndarray2000/test-360-dep-lumvorax`

Date: 2026-02-21

La demande est prise en compte: les prompts V4 incluent maintenant explicitement:
1. push dataset V4,
2. pull du kernel `ndarray2000/test-360-dep-lumvorax`,
3. remplacement du code par le script V3 complet,
4. push kernel,
5. attente obligatoire de 59 secondes,
6. récupération du status et des outputs/logs,
7. rapport détaillé des incidents,
8. génération/contrôle de logs persistants complets:
   - événements ns JSONL,
   - métriques runtime CSV,
   - latences par étape,
   - fingerprint hardware,
   - checksums artefacts,
   - stdout/stderr persistants.

Fichiers prompt mis à jour:
- `PROMPT_AGENT_PUSH_DATASET_V4_NX47_DEPENDENCIES_20260221.md`
- `PROMPT_AGENT_PUSH_ONLY_DATASET_V4_READY_20260221.md`
