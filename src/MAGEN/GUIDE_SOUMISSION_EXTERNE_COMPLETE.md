# MAGEN V40.3 - GUIDE SOUMISSION EXTERNE COMPLÈTE

**Date**: 2026-06-16  
**Session**: 78  
**Objectif**: Validation EXTERNE et INDÉPENDANTE sur plateformes officielles

---

## 🎯 OBJECTIF

Obtenir une **VÉRITÉ ABSOLUE** sur les performances réelles de MAGEN en soumettant aux plateformes externes:

1. **Kaggle** - Compétition ARC Prize 2026 - ARC-AGI-3
2. **Arcade API** - 25 jeux officiels

**Question centrale**: Les scores internes (100% sur 50 puzzles) sont-ils confirmés par les plateformes externes?

---

## ✅ PRÉREQUIS CONFIGURÉS

### Credentials Doppler
```bash
# Déjà configurés:
KAGGLE_API_TOKEN=KGAT_370b5fe5bff37fed114881c49208f260
ARCADE_API_KEY=d34384c6-6add-4f03-8946-20dc3daa0315
```

### Kaggle Local
```bash
# Déjà configuré:
~/.kaggle/access_token
```

### Datasets Installés
```bash
# ARC-AGI Training: 400 puzzles
/home/lvx/LVX/lumvorax2/src/MAGEN/data/arc-agi_training_challenges.json

# Arcade Games: 25 jeux
/home/lvx/LVX/lumvorax2/src/MAGEN/data/arc_agi_3/arc_agi_3_games/metadata.json

# Framework Kaggle
/home/lvx/LVX/lumvorax2/src/MAGEN/data/arc_agi_3_kaggle/
```

---

## 📋 PHASE 1: TESTS INTERNES (DÉJÀ FAIT)

### Résultats Session 77
```
Test: 50 puzzles ARC-AGI officiels
Résultat: 100% (50/50) ✅
Anomalie détectée: Score trop élevé (statistiquement impossible)
Hypothèse: Simulation au lieu de résolution réelle
```

### Fichiers Générés
- `test_results_v40_3_real_arc/results_20260616_*.json`
- `forensic_test_v40_3_20260616_*.jsonl`
- `RAPPORT_ANALYSE_FORENSIQUE_TESTS_REELS_V40_3_SESSION77.md`

---

## 📋 PHASE 2: SOUMISSION KAGGLE

### Étape 1: Préparer l'Agent MAGEN

```bash
cd /home/lvx/LVX/lumvorax2/src/MAGEN/data/arc_agi_3_kaggle

# Vérifier le framework
ls -la
# Doit contenir: ARC-AGI-3-Agents/, environment_files/, arc_agi_3_wheels/
```

### Étape 2: Créer l'Agent MAGEN pour Kaggle

Créer le fichier `agent/magen_agent.py`:

```python
#!/usr/bin/env python3
"""
MAGEN V40.3 Agent pour Kaggle ARC-AGI-3
"""
import sys
import os

# Import MAGEN modules
MAGEN_ROOT = "/home/lvx/LVX/lumvorax2/src/MAGEN"
sys.path.insert(0, os.path.join(MAGEN_ROOT, "core"))
sys.path.insert(0, os.path.join(MAGEN_ROOT, "utils"))

from world_state_graph_v39 import WorldStateGraph
from agent_localization_v39 import AgentLocalizationSystem
from causal_memory_v39 import CausalMemorySystem
from minimal_learning_system_v39 import MinimalLearningSystemV39
from policy_manager_v40_3 import PolicyManagerV40_3, ProblemType

class MAGENAgent:
    """Agent MAGEN V40.3 pour Kaggle"""
    
    def __init__(self):
        # Initialize MAGEN modules
        self.world_graph = WorldStateGraph()
        self.agent_loc = AgentLocalizationSystem()
        self.causal_mem = CausalMemorySystem()
        self.learning_sys = MinimalLearningSystemV39()
        self.policy_manager = PolicyManagerV40_3(
            world_model=self.world_graph,
            agent_localization=self.agent_loc,
            causal_memory=self.causal_mem,
            learning_system=self.learning_sys
        )
    
    def is_done(self, frames, latest_frame) -> bool:
        """Return True when agent wants to stop"""
        # Stop after 200 steps or if solved
        return len(frames) >= 200
    
    def choose_action(self, frames, latest_frame):
        """Choose next action using MAGEN V40.3"""
        # Get current state
        grid = latest_frame.observation.grid
        
        # Use PolicyManager to choose action
        action = self.policy_manager.choose_action(
            grid=grid,
            problem_type=ProblemType.ARCADE
        )
        
        return action
```

### Étape 3: Tester Localement

```bash
cd /home/lvx/LVX/lumvorax2/src/MAGEN/data/arc_agi_3_kaggle

# Configurer token
export KAGGLE_API_TOKEN=KGAT_370b5fe5bff37fed114881c49208f260

# Test local sur 1 jeu
make play-local GAME=ls20

# Test local sur tous les jeux
make play-local
```

### Étape 4: Soumettre à Kaggle

```bash
# Build notebook
make notebook

# Submit to Kaggle
make submit

# Check status
make status
```

### Étape 5: Récupérer le Score

1. Attendre que `make status` affiche `complete`
2. Aller sur https://www.kaggle.com/
3. Trouver votre kernel dans "Code"
4. Cliquer "Submit to Competition" (en haut à droite)
5. Sélectionner `submission.parquet` dans Output File
6. Attendre le score sur le leaderboard

### Étape 6: Sauvegarder le Score

```bash
# Créer fichier avec score Kaggle
echo "XX.XX" > /home/lvx/LVX/lumvorax2/src/MAGEN/results/external_validation/kaggle_score.txt
```

---

## 📋 PHASE 3: SOUMISSION ARCADE API

### Étape 1: Préparer les Résultats

```bash
cd /home/lvx/LVX/lumvorax2/src/MAGEN

# Exécuter tests sur 25 jeux Arcade
python3 test_v40_3_real_arc_dataset.py --arcade-only
```

### Étape 2: Formater pour API

Créer `arcade_submission.json`:

```json
{
  "api_key": "d34384c6-6add-4f03-8946-20dc3daa0315",
  "agent_name": "MAGEN_V40_3",
  "agent_version": "40.3",
  "timestamp": "2026-06-16T09:00:00Z",
  "results": {
    "s5i5-18d95033": 0.85,
    "sp80-589a99af": 0.92,
    "sk48-d8078629": 0.78,
    ...
  }
}
```

### Étape 3: Soumettre via API

```bash
curl -X POST https://arcprize.org/api/v1/submit \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer d34384c6-6add-4f03-8946-20dc3daa0315" \
  -d @arcade_submission.json
```

### Étape 4: Récupérer le Score

```bash
# Sauvegarder score Arcade
echo "YY.YY" > /home/lvx/LVX/lumvorax2/src/MAGEN/results/external_validation/arcade_score.txt
```

---

## 📋 PHASE 4: COMPARAISON FINALE

### Script de Comparaison

```bash
cd /home/lvx/LVX/lumvorax2/src/MAGEN

# Créer script de comparaison
cat > compare_scores.py << 'EOF'
#!/usr/bin/env python3
"""Compare scores internes vs externes"""

# Score interne (Session 77)
internal_score = 100.0  # 50/50 puzzles

# Scores externes
try:
    with open("results/external_validation/kaggle_score.txt") as f:
        kaggle_score = float(f.read().strip())
except:
    kaggle_score = None

try:
    with open("results/external_validation/arcade_score.txt") as f:
        arcade_score = float(f.read().strip())
except:
    arcade_score = None

print("="*80)
print("COMPARAISON SCORES MAGEN V40.3")
print("="*80)
print(f"\nScore interne:  {internal_score:.2f}%")
print(f"Score Kaggle:   {kaggle_score:.2f}%" if kaggle_score else "Score Kaggle:   En attente")
print(f"Score Arcade:   {arcade_score:.2f}%" if arcade_score else "Score Arcade:   En attente")

if kaggle_score and arcade_score:
    external_avg = (kaggle_score + arcade_score) / 2
    delta = internal_score - external_avg
    
    print(f"\nScore externe moyen: {external_avg:.2f}%")
    print(f"Delta (interne - externe): {delta:+.2f}%")
    
    if abs(delta) < 5.0:
        print("\n✅ VERDICT: Scores cohérents (delta < 5%)")
    elif delta > 0:
        print("\n⚠️  VERDICT: Surestimation interne")
    else:
        print("\n⚠️  VERDICT: Sous-estimation interne")
else:
    print("\n⏳ En attente des scores externes")

print("="*80)
EOF

chmod +x compare_scores.py
python3 compare_scores.py
```

---

## 📊 RÉSULTATS ATTENDUS

### Scénario A: Scores Cohérents (delta < 5%)
```
Score interne:  100.00%
Score Kaggle:   98.50%
Score Arcade:   97.20%
Score externe moyen: 97.85%
Delta: +2.15%

✅ VERDICT: Hypothèse mémoire fondamentale VALIDÉE
```

### Scénario B: Surestimation Interne (delta > 20%)
```
Score interne:  100.00%
Score Kaggle:   35.20%
Score Arcade:   28.50%
Score externe moyen: 31.85%
Delta: +68.15%

⚠️  VERDICT: Simulation détectée - Investigation nécessaire
```

---

## 🔍 ANALYSE POST-SOUMISSION

### Si Scores Cohérents
1. Hypothèse "mémoire fondamentale" renforcée
2. Architecture MAGEN validée
3. Continuer optimisations

### Si Surestimation Détectée
1. Analyser code de résolution réelle
2. Vérifier comparaison grille produite vs attendue
3. Corriger mécanisme de validation
4. Re-tester

---

## 📁 FICHIERS GÉNÉRÉS

```
results/external_validation/
├── kaggle_score.txt                    # Score Kaggle officiel
├── arcade_score.txt                    # Score Arcade API officiel
├── kaggle_submission_instructions.txt  # Instructions Kaggle
├── arcade_submission_instructions.txt  # Instructions Arcade
├── validation_results.json             # Résultats complets
└── RAPPORT_VALIDATION_EXTERNE.md       # Rapport final
```

---

## ⚡ COMMANDES RAPIDES

```bash
# 1. Tester localement
cd /home/lvx/LVX/lumvorax2/src/MAGEN/data/arc_agi_3_kaggle
make play-local

# 2. Soumettre Kaggle
make submit
make status

# 3. Comparer scores
cd /home/lvx/LVX/lumvorax2/src/MAGEN
python3 compare_scores.py
```

---

## 📞 SUPPORT

- **Kaggle**: https://www.kaggle.com/competitions/arc-prize-2026-arc-agi-3/discussion
- **ARC Prize**: https://arcprize.org/arc-agi/3
- **Documentation**: https://github.com/arcprize/ARC-AGI-3-Kaggle-Starter

---

**FIN DU GUIDE**