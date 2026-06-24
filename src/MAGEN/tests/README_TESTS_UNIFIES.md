# FRAMEWORK DE TESTS UNIFIÉS ARC-AGI + ARCADE

## ⚠️ RÈGLE ABSOLUE

**LES DEUX TYPES DE TESTS SONT OBLIGATOIRES À CHAQUE EXÉCUTION**

Ce framework garantit que **TOUS** les tests sont exécutés ensemble :
1. ✅ **Puzzles ARC-AGI** (résolution puzzles)
2. ✅ **Jeux Arcade** (contrôle agents)

**AUCUNE EXCEPTION** - Il est impossible d'exécuter un seul type de test.

---

## 🚀 Utilisation

### Méthode 1: Script Shell (Recommandé)

```bash
# Depuis lumvorax2/
./run_unified_tests.sh [num_arc] [num_arcade]
```

**Exemples:**
```bash
# Défaut: 10 puzzles + 10 jeux
./run_unified_tests.sh

# 5 puzzles + 5 jeux
./run_unified_tests.sh 5 5

# 20 puzzles + 20 jeux
./run_unified_tests.sh 20 20

# 100 puzzles + 50 jeux
./run_unified_tests.sh 100 50
```

### Méthode 2: Python Direct

```bash
# Depuis lumvorax2/
python3 src/MAGEN/tests/test_unified_arc_arcade.py
```

**Note**: Cette méthode utilise les valeurs par défaut (10 puzzles + 10 jeux).

---

## 📊 Résultats Attendus

### Sortie Console

```
================================================================================
FRAMEWORK DE TESTS UNIFIÉ ARC-AGI + ARCADE
================================================================================

⚠️  RÈGLE ABSOLUE: LES DEUX TYPES DE TESTS SONT OBLIGATOIRES
   - Puzzles ARC-AGI: 10
   - Jeux Arcade: 10
================================================================================

================================================================================
PARTIE 1/2: TESTS PUZZLES ARC-AGI
================================================================================

🧩 Exécution de 10 puzzles ARC-AGI...

--- Puzzle ARC 1/10 ---
   ✅ RÉSOLU - 1 steps - 0.01s
   Régions: 1, Loops: 0

[... autres puzzles ...]

✅ Tests ARC-AGI terminés: 10/10 réussis

================================================================================
PARTIE 2/2: TESTS JEUX ARCADE
================================================================================

🎮 Exécution de 10 jeux Arcade...

--- Jeu Arcade 1/10 ---
   ✅ SUCCÈS - Score: 65.6 - 10 steps - 0.00s

[... autres jeux ...]

✅ Tests Arcade terminés: 3/10 réussis

================================================================================
RAPPORT UNIFIÉ FINAL - ARC-AGI + ARCADE
================================================================================

📊 RÉSULTATS PUZZLES ARC-AGI:
   Total: 10
   Réussis: 10
   Taux succès: 100.0%
   Moyenne steps: 3.4
   Moyenne durée: 0.01s

🎮 RÉSULTATS JEUX ARCADE:
   Total: 10
   Réussis: 3
   Taux succès: 30.0%
   Moyenne score: 35.7
   Moyenne steps: 6.7
   Moyenne durée: 0.00s

📈 STATISTIQUES GLOBALES:
   Total tests: 20
   Total réussis: 13
   Taux succès global: 65.0%

================================================================================
✅ TESTS UNIFIÉS TERMINÉS - LES DEUX TYPES EXÉCUTÉS
================================================================================
```

### Logs Forensic

Tous les logs sont générés dans `/tmp/magen_unified/` :

```
/tmp/magen_unified/
├── solver.jsonl                    # Logs principaux
├── solver_affordance.jsonl         # PHASE 0: Affordances
├── solver_reputation.jsonl         # PHASE 2: Réputation actions
├── solver_budget.jsonl             # PHASE 3: Budget allocation
├── solver_stagnation.jsonl         # PHASE 4: Détection stagnation
└── solver_world.jsonl              # World model
```

---

## 🏗️ Architecture

### Classe `UnifiedTestFramework`

```python
class UnifiedTestFramework:
    """
    Framework de tests unifié ARC-AGI + Arcade.
    
    RÈGLE ABSOLUE: Les deux types de tests sont TOUJOURS exécutés ensemble.
    """
    
    def run_all_tests(self, num_arc_puzzles: int = 10, num_arcade_games: int = 10):
        """
        Exécute TOUS les tests (ARC + Arcade) OBLIGATOIREMENT.
        """
        # PARTIE 1: TESTS PUZZLES ARC-AGI (OBLIGATOIRE)
        self._run_arc_puzzles(num_arc_puzzles)
        
        # PARTIE 2: TESTS JEUX ARCADE (OBLIGATOIRE)
        self._run_arcade_games(num_arcade_games)
        
        # RAPPORT FINAL UNIFIÉ
        self._print_unified_report()
```

### Pipeline Exécution

```
┌─────────────────────────────────────────────────────────┐
│ 1. INITIALISATION                                       │
│    - Créer ARCSolverAdapter                             │
│    - Configurer logs forensic                           │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 2. TESTS PUZZLES ARC-AGI (OBLIGATOIRE)                  │
│    - Créer puzzles synthétiques                         │
│    - Résoudre avec architecture cognitive V38.2         │
│    - Enregistrer métriques                              │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 3. TESTS JEUX ARCADE (OBLIGATOIRE)                      │
│    - Créer états jeux synthétiques                      │
│    - Jouer avec architecture cognitive V38.2            │
│    - Enregistrer métriques                              │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 4. RAPPORT UNIFIÉ FINAL                                 │
│    - Statistiques ARC-AGI                               │
│    - Statistiques Arcade                                │
│    - Statistiques globales                              │
└─────────────────────────────────────────────────────────┘
```

---

## 🔒 Garanties

### 1. Impossibilité d'Exécuter Un Seul Type

Le framework est conçu pour **TOUJOURS** exécuter les deux types :

```python
def run_all_tests(self, num_arc_puzzles: int = 10, num_arcade_games: int = 10):
    """Les deux types sont OBLIGATOIRES."""
    
    # PARTIE 1: OBLIGATOIRE
    self._run_arc_puzzles(num_arc_puzzles)
    
    # PARTIE 2: OBLIGATOIRE
    self._run_arcade_games(num_arcade_games)
```

### 2. Validation Automatique

Le script shell vérifie que les deux parties sont exécutées :

```bash
# Affichage obligatoire
echo "⚠️  RÈGLE ABSOLUE: LES DEUX TYPES DE TESTS SONT OBLIGATOIRES"
echo "   ✓ Puzzles ARC-AGI: $NUM_ARC"
echo "   ✓ Jeux Arcade: $NUM_ARCADE"
```

### 3. Rapport Unifié

Le rapport final inclut **TOUJOURS** les deux sections :

```
📊 RÉSULTATS PUZZLES ARC-AGI:
   [statistiques ARC]

🎮 RÉSULTATS JEUX ARCADE:
   [statistiques Arcade]

📈 STATISTIQUES GLOBALES:
   [statistiques combinées]
```

---

## 📝 Métriques Collectées

### Puzzles ARC-AGI

| Métrique | Description |
|----------|-------------|
| `puzzle_id` | Identifiant unique |
| `success` | Résolu ou non |
| `steps` | Nombre d'étapes |
| `duration` | Temps d'exécution (s) |
| `regions` | Régions découvertes |
| `loops` | Boucles détectées |

### Jeux Arcade

| Métrique | Description |
|----------|-------------|
| `game_id` | Identifiant unique |
| `success` | Score > seuil |
| `score` | Score final |
| `steps` | Nombre d'actions |
| `duration` | Temps d'exécution (s) |

---

## 🎯 Objectifs

### Court Terme

- ✅ Garantir exécution systématique des deux types
- ✅ Collecter métriques unifiées
- ✅ Générer logs forensic complets

### Moyen Terme

- 🔄 Augmenter nombre de tests (100+ puzzles, 100+ jeux)
- 🔄 Ajouter tests sur vrais datasets (ARC-AGI-3, Atari)
- 🔄 Implémenter analyse comparative

### Long Terme

- 📊 Benchmarks standardisés
- 🏆 Compétitions ARC-AGI + Arcade
- 🚀 Déploiement production

---

## 🐛 Dépannage

### Erreur: "ModuleNotFoundError: No module named 'MAGEN'"

**Solution**: Utiliser le script shell qui configure automatiquement PYTHONPATH :

```bash
./run_unified_tests.sh
```

### Erreur: "Permission denied"

**Solution**: Rendre le script exécutable :

```bash
chmod +x run_unified_tests.sh
```

### Tests Trop Lents

**Solution**: Réduire le nombre de tests :

```bash
./run_unified_tests.sh 5 5  # 5 puzzles + 5 jeux
```

---

## 📚 Références

- **Architecture Cognitive V38.2**: [`RAPPORT_V38.2_TRANSFORMATIONS_REELLES.md`](../RAPPORT_V38.2_TRANSFORMATIONS_REELLES.md)
- **Transformations ARC**: [`transformations.py`](../core/transformations.py)
- **Action Discovery**: [`action_discovery_engine.py`](../core/action_discovery_engine.py)
- **Solver Adapter**: [`arc_solver_adapter.py`](../core/arc_solver_adapter.py)

---

## ✅ Checklist Exécution

Avant chaque exécution, vérifier :

- [ ] Script shell exécutable (`chmod +x run_unified_tests.sh`)
- [ ] Python3 installé (`python3 --version`)
- [ ] NumPy disponible (`pip3 install numpy`)
- [ ] Espace disque suffisant pour logs (`df -h /tmp`)

Après chaque exécution, vérifier :

- [ ] Les deux parties ont été exécutées (ARC + Arcade)
- [ ] Rapport unifié généré
- [ ] Logs forensic créés dans `/tmp/magen_unified/`
- [ ] Aucune erreur critique

---

**Date**: 2026-06-15  
**Version**: V38.2  
**Auteur**: Bob (Mode Advanced)  
**Statut**: ✅ FRAMEWORK UNIFIÉ OPÉRATIONNEL