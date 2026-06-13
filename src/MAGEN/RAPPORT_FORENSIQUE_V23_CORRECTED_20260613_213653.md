# RAPPORT ANALYSE FORENSIQUE V23 - 400 PUZZLES

**Date**: 2026-06-13T21:38:51.094013

**Total événements**: 36,275

## 1. STATISTIQUES GLOBALES

### Événements par type

| Type | Count |
|------|-------|
| transformation | 22,094 |
| pattern_detection | 12,570 |
| puzzle_start | 400 |
| adaptive_recoveries | 400 |
| validation | 400 |
| explanation_generated | 237 |
| symbolic_verifications | 173 |
| error | 1 |

### Événements par composant

| Composant | Count |
|-----------|-------|
| transformation_learning_engine | 22,094 |
| advanced_pattern_detectors | 12,570 |
| None | 1,611 |

## 2. ANOMALIES DÉTECTÉES

### 2.1 Échecs Transformations (17345)

| Ligne | Action | Success | Error |
|-------|--------|---------|-------|
| 115 | unknown | False | 0.000 |
| 119 | rotate_90 | False | 0.120 |
| 124 | rotate_180 | False | 0.132 |
| 125 | rotate_180 | False | 0.101 |
| 129 | mirror_horizontal | False | 0.120 |
| 134 | mirror_vertical | False | 0.114 |
| 141 | identity | False | 1.000 |
| 148 | identity | False | 1.000 |
| 149 | identity | False | 1.000 |
| 150 | identity | False | 1.000 |
| 151 | identity | False | 1.000 |
| 152 | identity | False | 1.000 |
| 153 | identity | False | 1.000 |
| 154 | identity | False | 1.000 |
| 155 | identity | False | 1.000 |
| 156 | identity | False | 1.000 |
| 157 | identity | False | 1.000 |
| 158 | identity | False | 1.000 |
| 159 | identity | False | 1.000 |
| 160 | identity | False | 1.000 |
| 161 | identity | False | 1.000 |
| 162 | identity | False | 1.000 |
| 163 | identity | False | 1.000 |
| 164 | identity | False | 1.000 |
| 165 | identity | False | 1.000 |
| 166 | identity | False | 1.000 |
| 167 | identity | False | 1.000 |
| 168 | identity | False | 1.000 |
| 169 | identity | False | 1.000 |
| 170 | identity | False | 1.000 |
| 171 | identity | False | 1.000 |
| 172 | identity | False | 1.000 |
| 173 | identity | False | 1.000 |
| 174 | identity | False | 1.000 |
| 175 | identity | False | 1.000 |
| 176 | identity | False | 1.000 |
| 177 | identity | False | 1.000 |
| 178 | identity | False | 1.000 |
| 179 | identity | False | 1.000 |
| 180 | identity | False | 1.000 |
| 181 | identity | False | 1.000 |
| 182 | identity | False | 1.000 |
| 183 | identity | False | 1.000 |
| 184 | identity | False | 1.000 |
| 185 | identity | False | 1.000 |
| 186 | identity | False | 1.000 |
| 187 | identity | False | 1.000 |
| 188 | identity | False | 1.000 |
| 189 | identity | False | 1.000 |
| 190 | identity | False | 1.000 |

*... et 17295 autres*

### 2.2 Détections Pattern Vides (0)

✅ Aucune détection vide

### 2.3 Échecs Validation (226)

Total: 226 échecs

*Détails dans section suivante*

### 2.4 Anomalies Génériques (21793)

| Ligne | Type | Composant |
|-------|------|----------|
| 116 | transformation | transformation_learning_engine |
| 117 | transformation | transformation_learning_engine |
| 118 | transformation | transformation_learning_engine |
| 119 | transformation | transformation_learning_engine |
| 120 | transformation | transformation_learning_engine |
| 121 | transformation | transformation_learning_engine |
| 122 | transformation | transformation_learning_engine |
| 123 | transformation | transformation_learning_engine |
| 124 | transformation | transformation_learning_engine |
| 125 | transformation | transformation_learning_engine |
| 126 | transformation | transformation_learning_engine |
| 127 | transformation | transformation_learning_engine |
| 128 | transformation | transformation_learning_engine |
| 129 | transformation | transformation_learning_engine |
| 130 | transformation | transformation_learning_engine |
| 131 | transformation | transformation_learning_engine |
| 132 | transformation | transformation_learning_engine |
| 133 | transformation | transformation_learning_engine |
| 134 | transformation | transformation_learning_engine |
| 135 | transformation | transformation_learning_engine |
| 136 | transformation | transformation_learning_engine |
| 137 | transformation | transformation_learning_engine |
| 138 | transformation | transformation_learning_engine |
| 139 | transformation | transformation_learning_engine |
| 140 | transformation | transformation_learning_engine |
| 141 | transformation | transformation_learning_engine |
| 148 | transformation | transformation_learning_engine |
| 149 | transformation | transformation_learning_engine |
| 150 | transformation | transformation_learning_engine |
| 151 | transformation | transformation_learning_engine |
| 152 | transformation | transformation_learning_engine |
| 153 | transformation | transformation_learning_engine |
| 154 | transformation | transformation_learning_engine |
| 155 | transformation | transformation_learning_engine |
| 156 | transformation | transformation_learning_engine |
| 157 | transformation | transformation_learning_engine |
| 158 | transformation | transformation_learning_engine |
| 159 | transformation | transformation_learning_engine |
| 160 | transformation | transformation_learning_engine |
| 161 | transformation | transformation_learning_engine |
| 162 | transformation | transformation_learning_engine |
| 163 | transformation | transformation_learning_engine |
| 164 | transformation | transformation_learning_engine |
| 165 | transformation | transformation_learning_engine |
| 166 | transformation | transformation_learning_engine |
| 167 | transformation | transformation_learning_engine |
| 168 | transformation | transformation_learning_engine |
| 169 | transformation | transformation_learning_engine |
| 170 | transformation | transformation_learning_engine |
| 171 | transformation | transformation_learning_engine |

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

- Durée totale: 44.31s
- Événements/seconde: 818.66

---

*Rapport généré automatiquement par analyze_forensic_logs_v23.py*
