# RAPPORT ANALYSE FORENSIQUE PROFONDE V24

**Date**: 2026-06-13T21:45:19.016034

**Total lignes analysées**: 36,275
**Total événements**: 36,275

## 1. STATISTIQUES GLOBALES

### Événements par type

| Type | Count | % |
|------|-------|---|
| transformation | 22,094 | 60.9% |
| pattern_detection | 12,570 | 34.7% |
| puzzle_start | 400 | 1.1% |
| adaptive_recoveries | 400 | 1.1% |
| validation | 400 | 1.1% |
| explanation_generated | 237 | 0.7% |
| symbolic_verifications | 173 | 0.5% |
| error | 1 | 0.0% |

### Événements par composant

| Composant | Count | % |
|-----------|-------|---|
| transformation_learning_engine | 22,094 | 60.9% |
| advanced_pattern_detectors | 12,570 | 34.7% |
| None | 1,611 | 4.4% |

## 2. ANALYSE TRANSFORMATIONS

- **Total**: 21,792
- **Succès**: 4,749 (21.8%)
- **Erreur moyenne**: 0.4951
- **Erreur médiane**: 0.2667
- **Timing moyen**: 0.03 ms
- **Timing max**: 0.51 ms
- **Timing min**: 0.0038 ms
- **Chaînes causales utilisées**: 0
- **Longueur moyenne chaîne**: 0.00

## 3. ANALYSE PUZZLES

- **Total**: 400
- **Succès**: 174 (43.5%)
- **Échecs**: 226 (56.5%)

### Comparaison Succès vs Échec

- **Succès - Transformations moyennes**: 56.4
- **Échecs - Transformations moyennes**: 53.0
- **Différence**: -3.4

## 4. ANOMALIES DÉTECTÉES

**Total anomalies**: 1,611

### Par type

| Type | Count | Sévérité |
|------|-------|----------|
| missing_component | 1,611 | low |

## 5. CORRECTIONS NÉCESSAIRES

**Total corrections identifiées**: 2

### C6 - low_success_rate [HIGH]

**Description**: Taux succès transformations: 21.8% (< 30%)

**Fix**: Améliorer sélection transformations + pruning intelligent

**Fichier**: `transformation_learning_engine.py`

**Impact**: Efficacité +30% + réduction exploration inutile

---

### C5 - missing_component [LOW]

**Description**: 1611 événements avec component=None

**Fix**: Ajouter composant à tous les types d'événements

**Fichier**: `test_phase2_400_puzzles.py`

**Impact**: Traçabilité complète + indexation améliorée

---


## 6. MÉTRIQUES TEMPORELLES

- **Durée totale**: 44.31s
- **Événements/seconde**: 818.66
- **Premier timestamp**: 1781379414008239799
- **Dernier timestamp**: 1781379458318689658
