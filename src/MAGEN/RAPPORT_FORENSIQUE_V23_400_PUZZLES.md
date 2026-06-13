# RAPPORT ANALYSE FORENSIQUE V23 - 400 PUZZLES

**Date**: 2026-06-13T21:09:25.073161

**Total événements**: 36,155

## 1. STATISTIQUES GLOBALES

### Événements par type

| Type | Count |
|------|-------|
| transformation | 22,094 |
| pattern_detection | 12,450 |
| puzzle_start | 400 |
| adaptive_recoveries | 400 |
| validation | 400 |
| explanation_generated | 237 |
| symbolic_verifications | 173 |
| error | 1 |

### Événements par composant

| Composant | Count |
|-----------|-------|
| None | 23,403 |
| advanced_pattern_detectors | 12,450 |
| transformation_learning_engine | 302 |

## 2. ANOMALIES DÉTECTÉES

### 2.1 Échecs Transformations (17345)

| Ligne | Action | Success | Error |
|-------|--------|---------|-------|
| 55 | unknown | False | 0.000 |
| 59 | rotate_90 | False | 0.120 |
| 64 | rotate_180 | False | 0.132 |
| 65 | rotate_180 | False | 0.101 |
| 69 | mirror_horizontal | False | 0.120 |
| 74 | mirror_vertical | False | 0.114 |
| 81 | identity | False | 1.000 |
| 88 | identity | False | 1.000 |
| 89 | identity | False | 1.000 |
| 90 | identity | False | 1.000 |
| 91 | identity | False | 1.000 |
| 92 | identity | False | 1.000 |
| 93 | identity | False | 1.000 |
| 94 | identity | False | 1.000 |
| 95 | identity | False | 1.000 |
| 96 | identity | False | 1.000 |
| 97 | identity | False | 1.000 |
| 98 | identity | False | 1.000 |
| 99 | identity | False | 1.000 |
| 100 | identity | False | 1.000 |
| 101 | identity | False | 1.000 |
| 102 | identity | False | 1.000 |
| 103 | identity | False | 1.000 |
| 104 | identity | False | 1.000 |
| 105 | identity | False | 1.000 |
| 106 | identity | False | 1.000 |
| 107 | identity | False | 1.000 |
| 108 | identity | False | 1.000 |
| 109 | identity | False | 1.000 |
| 110 | identity | False | 1.000 |
| 111 | identity | False | 1.000 |
| 112 | identity | False | 1.000 |
| 113 | identity | False | 1.000 |
| 114 | identity | False | 1.000 |
| 115 | identity | False | 1.000 |
| 116 | identity | False | 1.000 |
| 117 | identity | False | 1.000 |
| 118 | identity | False | 1.000 |
| 119 | identity | False | 1.000 |
| 120 | identity | False | 1.000 |
| 121 | identity | False | 1.000 |
| 122 | identity | False | 1.000 |
| 123 | identity | False | 1.000 |
| 124 | identity | False | 1.000 |
| 125 | identity | False | 1.000 |
| 126 | identity | False | 1.000 |
| 127 | identity | False | 1.000 |
| 128 | identity | False | 1.000 |
| 129 | identity | False | 1.000 |
| 130 | identity | False | 1.000 |

*... et 17295 autres*

### 2.2 Détections Pattern Vides (0)

✅ Aucune détection vide

### 2.3 Échecs Validation (226)

Total: 226 échecs

*Détails dans section suivante*

### 2.4 Anomalies Génériques (21793)

| Ligne | Type | Composant |
|-------|------|----------|
| 56 | transformation | None |
| 57 | transformation | None |
| 58 | transformation | None |
| 59 | transformation | None |
| 60 | transformation | None |
| 61 | transformation | None |
| 62 | transformation | None |
| 63 | transformation | None |
| 64 | transformation | None |
| 65 | transformation | None |
| 66 | transformation | None |
| 67 | transformation | None |
| 68 | transformation | None |
| 69 | transformation | None |
| 70 | transformation | None |
| 71 | transformation | None |
| 72 | transformation | None |
| 73 | transformation | None |
| 74 | transformation | None |
| 75 | transformation | None |
| 76 | transformation | None |
| 77 | transformation | None |
| 78 | transformation | None |
| 79 | transformation | None |
| 80 | transformation | None |
| 81 | transformation | None |
| 88 | transformation | None |
| 89 | transformation | None |
| 90 | transformation | None |
| 91 | transformation | None |
| 92 | transformation | None |
| 93 | transformation | None |
| 94 | transformation | None |
| 95 | transformation | None |
| 96 | transformation | None |
| 97 | transformation | None |
| 98 | transformation | None |
| 99 | transformation | None |
| 100 | transformation | None |
| 101 | transformation | None |
| 102 | transformation | None |
| 103 | transformation | None |
| 104 | transformation | None |
| 105 | transformation | None |
| 106 | transformation | None |
| 107 | transformation | None |
| 108 | transformation | None |
| 109 | transformation | None |
| 110 | transformation | None |
| 111 | transformation | None |

## 3. ERREURS CRITIQUES

✅ Aucune erreur critique

## 4. CORRECTIONS RECOMMANDÉES

### P1 - Priorité Haute

- **P1.1**: Taux d'échec transformations élevé (>1000)
  - Analyser actions les plus échouées
  - Améliorer heuristiques de sélection

- **P1.2**: Taux d'échec validation élevé (>200)
  - Revoir critères de validation
  - Améliorer scoring

### P2 - Priorité Moyenne

- **P2.1**: Optimiser logging (9MB pour 400 puzzles)
- **P2.2**: Ajouter métriques de performance


## 5. MÉTRIQUES TEMPORELLES

- Durée totale: 43.40s
- Événements/seconde: 833.16

---

*Rapport généré automatiquement par analyze_forensic_logs_v23.py*
