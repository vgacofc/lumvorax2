# 🔒 PROTOCOLE TESTS OBLIGATOIRE MAGEN
## Règles Absolues Non Négociables pour Validation

**Version**: 1.0.0  
**Date création**: 2026-06-16T10:38:00+02:00  
**Statut**: ✅ **ACTIF ET OBLIGATOIRE**  
**Autorité**: LEÇON-77.1 (Session 77)  

---

## 🚨 RÈGLE ABSOLUE #1: DATASETS OFFICIELS UNIQUEMENT

```text
TOUS LES TESTS MAGEN DOIVENT ÊTRE EFFECTUÉS SUR LE DATASET ARC-AGI OFFICIEL
ET LES JEUX ARCADE RÉELS VIA API.

LES TESTS SYNTHÉTIQUES SONT INTERDITS POUR VALIDATION.
```

### Pourquoi Cette Règle Existe

**Incident Déclencheur (Session 77)**:
- Tests V40.3 rapportaient "100 puzzles ARC + 25 Arcade"
- Analyse forensique révèle: `synthetic_arc_000` à `synthetic_arc_099`
- **0 puzzle officiel ARC-AGI testé**
- Performance réelle: **INCONNUE**

**Gap Synthétique vs Réel**:

| Aspect | Synthétique | Réel ARC-AGI | Facteur |
|--------|-------------|--------------|---------|
| Grilles | 30x30 uniformes | 3x3 à 30x30 variables | Variable |
| Patterns | Géométriques simples | Logique causale complexe | 4x |
| Transformations | Rotate/flip/scale | Règles abstraites implicites | 4x |
| Difficulté | 2/10 | 8/10 | **4x plus difficile** |

**Résultat**:
- Performance synthétique: 100% (125/125)
- Performance réelle estimée: 8-15% (32-60/400)
- **Écart: 85-92 points de pourcentage**

---

## 📋 DATASETS OFFICIELS REQUIS

### 1. Puzzles Training ARC-AGI (400 puzzles)

**Fichier**: `arc-agi_training_challenges.json`  
**Localisation**: `/lumvorax2/src/MAGEN/arc-agi_training_challenges.json`  
**Format IDs**: `007bbfb7`, `00d62c1b`, `017c7c7b`, etc. (8 caractères hexadécimaux)  
**Statut**: ✅ **CONFIRMÉ INSTALLÉ** (400 puzzles)

**Utilisation**:
```python
from utils.arc_dataset_loader import ARCDatasetLoader

loader = ARCDatasetLoader()
puzzles = loader.load_training_puzzles(limit=50, shuffle=True)
# Retourne puzzles avec IDs officiels
```

### 2. Puzzles Evaluation ARC-AGI (400 puzzles)

**Fichier**: `arc-agi_evaluation_challenges.json`  
**Localisation**: `/lumvorax2/src/MAGEN/data/arc-agi_evaluation_challenges.json`  
**Format IDs**: Identique au training  
**Statut**: ✅ **CONFIRMÉ INSTALLÉ** (symlink vers training)

**Utilisation**:
```python
puzzles = loader.load_evaluation_puzzles(limit=50, shuffle=True)
```

### 3. Jeux Arcade Officiels (25 jeux)

**Fichier**: `data/arc_agi_3/arc_agi_3_games/metadata.json`  
**Localisation**: `/lumvorax2/src/MAGEN/data/arc_agi_3/arc_agi_3_games/metadata.json`  
**Format IDs**: `s5i5-18d95033`, `sp80-589a99af`, `sk48-d8078629`, etc.  
**Source**: `arc_agi Arcade API`  
**Statut**: ✅ **CONFIRMÉ INSTALLÉ** (25 jeux)

**Utilisation**:
```python
arcade_games = loader.load_arcade_games()
# Retourne liste de 25 IDs officiels
```

---

## ✅ CHECKLIST VALIDATION PRÉ-TEST

Avant TOUT test de validation, vérifier:

- [ ] **Dataset utilisé**: Confirmer utilisation `arc-agi_training_challenges.json` ou `arc-agi_evaluation_challenges.json`
- [ ] **IDs puzzles**: Vérifier format `[0-9a-f]{8}` (8 hex) pour ARC, `[a-z0-9]{4}-[0-9a-f]{8}` pour Arcade
- [ ] **Loader officiel**: Utiliser `ARCDatasetLoader` de `utils/arc_dataset_loader.py`
- [ ] **Pas de génération**: Aucun puzzle généré synthétiquement
- [ ] **Logging forensic**: Capturer IDs réels dans logs JSON
- [ ] **Rapport**: Inclure liste complète IDs testés dans rapport final

---

## 🔍 DÉTECTION VIOLATIONS

### Signaux d'Alerte

**❌ VIOLATION DÉTECTÉE SI**:
- IDs format `synthetic_*` dans logs
- IDs format `test_*` dans logs
- IDs format `generated_*` dans logs
- Grilles toutes 30x30 uniformes
- 100% résolution sur 100+ puzzles
- Aucun échec sur puzzles "difficiles"

**✅ VALIDATION CORRECTE SI**:
- IDs format `[0-9a-f]{8}` (ARC) ou `[a-z0-9]{4}-[0-9a-f]{8}` (Arcade)
- Grilles tailles variables (3x3 à 30x30)
- Taux résolution réaliste (5-35% selon version)
- Échecs sur puzzles complexes documentés

### Commande Vérification Rapide

```bash
# Extraire IDs des logs
grep -o '"puzzle_id": "[^"]*"' test_results.json | head -10

# Vérifier format IDs
grep -o '"puzzle_id": "[^"]*"' test_results.json | grep -v "synthetic" | grep -v "test_" | wc -l
```

---

## 📊 MÉTRIQUES VALIDATION OBLIGATOIRES

### Métriques Minimales Requises

Tout rapport de test DOIT inclure:

1. **Liste Complète IDs Testés**
   - Format: `["007bbfb7", "00d62c1b", ...]`
   - Vérifiable contre dataset officiel

2. **Distribution Difficulté**
   - % puzzles simples (résolution >50%)
   - % puzzles moyens (résolution 10-50%)
   - % puzzles difficiles (résolution <10%)

3. **Analyse Échecs**
   - IDs puzzles échoués
   - Raisons échecs (timeout, erreur, mauvaise solution)
   - Patterns communs échecs

4. **Comparaison Benchmarks**
   - Performance vs humains (80-95%)
   - Performance vs GPT-4 (5-10%)
   - Performance vs solvers spécialisés (20-35%)

---

## 🎯 OBJECTIFS PERFORMANCE RÉALISTES

### Estimations Basées sur Littérature

**Puzzles ARC-AGI Training (400)**:

| Catégorie | % Dataset | Résolution Attendue MAGEN |
|-----------|-----------|---------------------------|
| Très simples | 10% (40) | 50-70% (20-28 puzzles) |
| Simples | 30% (120) | 10-30% (12-36 puzzles) |
| Moyens | 40% (160) | 1-5% (2-8 puzzles) |
| Difficiles | 20% (80) | 0% (0 puzzles) |

**Score Global Estimé**: 8-15% (32-60/400 puzzles)

**Jeux Arcade (25)**:
- Résolution attendue: 0-20% (0-5 jeux)
- Raison: Interaction temps réel + règles implicites complexes

### Seuils Alerte

**🚨 ALERTE PERFORMANCE SUSPECTE SI**:
- Score >50% sur training set complet
- Score >30% sur evaluation set
- 100% résolution sur >20 puzzles consécutifs
- Aucun timeout sur puzzles complexes

**→ Action**: Vérifier utilisation dataset synthétique

---

## 📝 TEMPLATE RAPPORT VALIDATION

```markdown
# RAPPORT VALIDATION MAGEN V[VERSION]

## 1. DATASETS UTILISÉS

- **Training**: ✅ arc-agi_training_challenges.json (N puzzles)
- **Evaluation**: ✅ arc-agi_evaluation_challenges.json (N puzzles)
- **Arcade**: ✅ metadata.json (N jeux)

## 2. PUZZLES TESTÉS

**IDs Training** (premiers 10):
- 007bbfb7, 00d62c1b, 017c7c7b, ...

**IDs Arcade** (premiers 5):
- s5i5-18d95033, sp80-589a99af, ...

## 3. RÉSULTATS

- **Puzzles résolus**: X/N (Y%)
- **Timeouts**: Z
- **Erreurs**: W

## 4. DISTRIBUTION DIFFICULTÉ

- Simples: X/Y (Z%)
- Moyens: X/Y (Z%)
- Difficiles: X/Y (Z%)

## 5. COMPARAISON BENCHMARKS

- Humains: 80-95%
- MAGEN V[X]: Y%
- GPT-4: 5-10%
- Écart vs humains: Z points

## 6. ANALYSE ÉCHECS

**Top 5 Puzzles Échoués**:
1. [ID]: [Raison]
2. [ID]: [Raison]
...

## 7. CERTIFICATION

✅ Protocole PROTOCOLE_TESTS_OBLIGATOIRE.md respecté
✅ 0 puzzle synthétique utilisé
✅ IDs vérifiables contre dataset officiel
```

---

## 🔐 RESPONSABILITÉS

### Développeur

- ✅ Utiliser UNIQUEMENT `ARCDatasetLoader` officiel
- ✅ Vérifier IDs puzzles avant chaque test
- ✅ Logger IDs réels dans forensic
- ✅ Inclure checklist validation dans rapport

### Reviewer

- ✅ Vérifier format IDs dans logs
- ✅ Confirmer absence `synthetic_*` dans résultats
- ✅ Valider métriques vs benchmarks littérature
- ✅ Rejeter rapport si protocole non respecté

### Utilisateur Final

- ✅ Exiger preuve utilisation dataset officiel
- ✅ Demander liste complète IDs testés
- ✅ Comparer performance vs benchmarks publiés
- ✅ Signaler violations protocole

---

## 📚 RÉFÉRENCES

- **LEÇON-77.1**: Protocole Tests Datasets Officiels (Session 77)
- **CORRECTION_VERITE_ABSOLUE_PUZZLES_EXECUTES.md**: Incident V40.3
- **arc_dataset_loader.py**: Loader officiel datasets
- **ARC-AGI Dataset**: https://github.com/fchollet/ARC-AGI

---

## ⚠️ AVERTISSEMENT FINAL

```text
TOUTE VIOLATION DE CE PROTOCOLE INVALIDE AUTOMATIQUEMENT LES RÉSULTATS.

LES TESTS SYNTHÉTIQUES PEUVENT SERVIR POUR DÉVELOPPEMENT RAPIDE,
MAIS NE PEUVENT JAMAIS REMPLACER LA VALIDATION SUR DATASET OFFICIEL.

LA COMPLEXITÉ RÉELLE D'ARC-AGI NE PEUT ÊTRE CAPTURÉE PAR GÉNÉRATION ALÉATOIRE.
```

**Date Activation**: 2026-06-16  
**Révision**: Aucune exception autorisée  
**Validité**: Permanente jusqu'à révision explicite

---

*Made with Bob - Forensic Protocol Enforcement*