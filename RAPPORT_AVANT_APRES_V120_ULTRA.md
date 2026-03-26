# Rapport avant / après — NX47 V120 Ultra Authentic 360

## Pourquoi les logs n’apparaissaient plus sous la cellule ?

### Avant
- Le kernel journalisait peu d’événements (BOOT, DATASET, FILE_START/END, métriques finales), donc peu de lignes visibles en sortie cellule.
- Le traçage mémoire était écrit surtout en JSON final, pas en flux ultra détaillé continu.

### Après
- Chaque événement passe par un logger Merkle `UltraAuthentic360Merkle` qui **imprime en temps réel** (`print(..., flush=True)`) et écrit en `.jsonl`.
- Ajout de logs d’étapes intermédiaires (`STEP`) et traces d’array détaillées (`ARRAY_TRACE`) pendant l’exécution.

## Modifications clés

### 1) Logger Merkle 360 streaming
- Ajout de `UltraAuthentic360Merkle` avec chaînage `prev_merkle -> merkle`.
- Sortie continue en `v120_ultra_authentic_360_merkle.jsonl`.

### 2) Traces mémoire bit-level
- Ajout de `_log_array_ultra(stage, arr)`:
  - fingerprint mémoire classique,
  - stats bits preview (ones/zeros + hash preview),
  - émission événement `ARRAY_TRACE`.

### 3) Étapes ultra verboses
- Ajout de logs `STEP` au démarrage de `load/features/train/segment`.

### 4) Paramètres de contrôle
- Ajout de flags config/env:
  - `V120_ULTRA_CONSOLE_LOG`
  - `V120_ULTRA_STEP_LOG`
  - `V120_ULTRA_BIT_TRACE_ARRAYS`
  - `V120_ULTRA_BIT_TRACE_LIMIT`

### 5) Artefacts
- Le metadata inclut maintenant le chemin de log ultra (`ultra_log`).

## Résultat
- Les logs reviennent sous la cellule (streaming).
- Le traçage est renforcé (nanoseconde + merkle + bit preview) sans casser le format `submission.zip`.
