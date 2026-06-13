# RAPPORT ANALYSE FORENSIQUE V23 - 400 PUZZLES

**Date**: 2026-06-13T21:30:05.882362

**Total événements**: 625

## 1. STATISTIQUES GLOBALES

### Événements par type

| Type | Count |
|------|-------|
| pattern_detection | 330 |
| transformation | 255 |
| puzzle_start | 10 |
| adaptive_recoveries | 10 |
| validation | 10 |
| explanation_generated | 6 |
| symbolic_verifications | 4 |

### Événements par composant

| Composant | Count |
|-----------|-------|
| advanced_pattern_detectors | 330 |
| transformation_learning_engine | 255 |
| None | 40 |

## 2. ANOMALIES DÉTECTÉES

### 2.1 Échecs Transformations (154)

| Ligne | Action | Success | Error |
|-------|--------|---------|-------|
| 55 | unknown | False | 0.000 |
| 59 | rotate_90 | False | 0.120 |
| 64 | rotate_180 | False | 0.132 |
| 65 | rotate_180 | False | 0.101 |
| 69 | mirror_horizontal | False | 0.120 |
| 74 | mirror_vertical | False | 0.114 |
| 178 | unknown | False | 0.000 |
| 179 | rotate_180 | False | 0.127 |
| 180 | rotate_180 | False | 0.311 |
| 182 | mirror_horizontal | False | 0.200 |
| 183 | mirror_vertical | False | 0.127 |
| 184 | mirror_vertical | False | 0.267 |
| 186 | identity | False | 0.133 |
| 187 | rotate_180 | False | 0.127 |
| 188 | rotate_180 | False | 0.311 |
| 190 | mirror_horizontal | False | 0.200 |
| 191 | mirror_vertical | False | 0.127 |
| 192 | mirror_vertical | False | 0.267 |
| 194 | identity | False | 0.133 |
| 195 | rotate_180 | False | 0.127 |
| 196 | rotate_180 | False | 0.311 |
| 198 | mirror_horizontal | False | 0.200 |
| 199 | mirror_vertical | False | 0.127 |
| 200 | mirror_vertical | False | 0.267 |
| 202 | identity | False | 0.133 |
| 203 | rotate_180 | False | 0.127 |
| 204 | rotate_180 | False | 0.311 |
| 206 | mirror_horizontal | False | 0.200 |
| 207 | mirror_vertical | False | 0.127 |
| 208 | mirror_vertical | False | 0.267 |
| 210 | identity | False | 0.133 |
| 211 | rotate_180 | False | 0.127 |
| 212 | rotate_180 | False | 0.311 |
| 214 | mirror_horizontal | False | 0.200 |
| 215 | mirror_vertical | False | 0.127 |
| 216 | mirror_vertical | False | 0.267 |
| 218 | identity | False | 0.133 |
| 242 | unknown | False | 0.000 |
| 300 | unknown | False | 0.000 |
| 372 | unknown | False | 0.000 |
| 373 | rotate_90 | False | 0.398 |
| 374 | rotate_90 | False | 0.206 |
| 375 | rotate_90 | False | 0.408 |
| 376 | rotate_180 | False | 0.414 |
| 377 | rotate_180 | False | 0.218 |
| 378 | rotate_180 | False | 0.449 |
| 379 | mirror_horizontal | False | 0.398 |
| 380 | mirror_horizontal | False | 0.206 |
| 381 | mirror_horizontal | False | 0.408 |
| 382 | mirror_vertical | False | 0.398 |

*... et 104 autres*

### 2.2 Détections Pattern Vides (0)

✅ Aucune détection vide

### 2.3 Échecs Validation (6)

Total: 6 échecs

*Détails dans section suivante*

### 2.4 Anomalies Génériques (247)

| Ligne | Type | Composant |
|-------|------|----------|
| 56 | transformation | transformation_learning_engine |
| 57 | transformation | transformation_learning_engine |
| 58 | transformation | transformation_learning_engine |
| 59 | transformation | transformation_learning_engine |
| 60 | transformation | transformation_learning_engine |
| 61 | transformation | transformation_learning_engine |
| 62 | transformation | transformation_learning_engine |
| 63 | transformation | transformation_learning_engine |
| 64 | transformation | transformation_learning_engine |
| 65 | transformation | transformation_learning_engine |
| 66 | transformation | transformation_learning_engine |
| 67 | transformation | transformation_learning_engine |
| 68 | transformation | transformation_learning_engine |
| 69 | transformation | transformation_learning_engine |
| 70 | transformation | transformation_learning_engine |
| 71 | transformation | transformation_learning_engine |
| 72 | transformation | transformation_learning_engine |
| 73 | transformation | transformation_learning_engine |
| 74 | transformation | transformation_learning_engine |
| 75 | transformation | transformation_learning_engine |
| 76 | transformation | transformation_learning_engine |
| 77 | transformation | transformation_learning_engine |
| 78 | transformation | transformation_learning_engine |
| 79 | transformation | transformation_learning_engine |
| 80 | transformation | transformation_learning_engine |
| 179 | transformation | transformation_learning_engine |
| 180 | transformation | transformation_learning_engine |
| 181 | transformation | transformation_learning_engine |
| 182 | transformation | transformation_learning_engine |
| 183 | transformation | transformation_learning_engine |
| 184 | transformation | transformation_learning_engine |
| 185 | transformation | transformation_learning_engine |
| 186 | transformation | transformation_learning_engine |
| 187 | transformation | transformation_learning_engine |
| 188 | transformation | transformation_learning_engine |
| 189 | transformation | transformation_learning_engine |
| 190 | transformation | transformation_learning_engine |
| 191 | transformation | transformation_learning_engine |
| 192 | transformation | transformation_learning_engine |
| 193 | transformation | transformation_learning_engine |
| 194 | transformation | transformation_learning_engine |
| 195 | transformation | transformation_learning_engine |
| 196 | transformation | transformation_learning_engine |
| 197 | transformation | transformation_learning_engine |
| 198 | transformation | transformation_learning_engine |
| 199 | transformation | transformation_learning_engine |
| 200 | transformation | transformation_learning_engine |
| 201 | transformation | transformation_learning_engine |
| 202 | transformation | transformation_learning_engine |
| 203 | transformation | transformation_learning_engine |

## 3. ERREURS CRITIQUES

✅ Aucune erreur critique

## 4. CORRECTIONS RECOMMANDÉES

### P1 - Priorité Haute

### P2 - Priorité Moyenne

- **P2.1**: Optimiser logging (9MB pour 400 puzzles)
- **P2.2**: Ajouter métriques de performance


## 5. MÉTRIQUES TEMPORELLES

- Durée totale: 0.96s
- Événements/seconde: 650.42

---

*Rapport généré automatiquement par analyze_forensic_logs_v23.py*
