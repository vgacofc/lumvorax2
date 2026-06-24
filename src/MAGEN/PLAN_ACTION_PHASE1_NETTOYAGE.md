# PLAN D'ACTION PHASE 1: NETTOYAGE MAGEN
**Date: 2026-06-18**
**Objectif: Éliminer duplications et créer fichiers manquants**
**Économie attendue: ~6,000 lignes**

---

## 📊 ÉTAT INITIAL (Audit 100% complété)

- **99 fichiers** analysés (40,665 lignes)
- **3 duplications majeures** identifiées
- **4 fichiers manquants** critiques
- **Taux duplication**: ~15% du code

---

## 🎯 PHASE 1.1: Fusionner Avatar Identification (4→1)

### Fichiers à fusionner:
1. `avatar_identifier.py` (467 lignes)
2. `behavioral_entity_detector.py` (428 lignes)
3. `agent_self_anchor.py` (421 lignes)
4. `agent_localization_v39.py` (437 lignes)

**Total**: 1,753 lignes → **~500 lignes** (économie 1,253 lignes)

### Nouveau module: `avatar_unified.py`
**Fonctionnalités consolidées**:
- Détection avatar par mouvement cohérent
- Ancrage spatial multi-critères (threshold 0.6→0.8)
- Localisation robuste avec confiance
- Détection entités comportementales

**Architecture**:
```python
class AvatarUnified:
    def __init__(self):
        self.detector = MovementDetector()
        self.anchor = SpatialAnchor()
        self.localizer = RobustLocalizer()
    
    def identify_avatar(self, grid_sequence):
        # Détection mouvement cohérent
        candidates = self.detector.detect_moving_entities()
        # Ancrage spatial
        avatar = self.anchor.lock_identity(candidates)
        # Localisation
        position = self.localizer.get_position(avatar)
        return avatar, position
```

---

## 🎯 PHASE 1.2: Fusionner Decision Kernels (6→1)

### Fichiers à fusionner:
1. `decision_kernel_minimal.py` (234 lignes)
2. `decision_kernel_v29.py` (312 lignes)
3. `decision_kernel_v30.py` (345 lignes)
4. `decision_kernel_v31.py` (389 lignes)
5. `decision_kernel_v32.py` (412 lignes)
6. `decision_kernel_v34.py` (456 lignes)

**Total**: 2,148 lignes → **~500 lignes** (économie 1,648 lignes)

### Module final: `decision_kernel_v34.py` (garder uniquement)
**Action**: Supprimer V29-V32 + minimal

**Raison**: V34 contient toutes améliorations cumulatives:
- Sterility memory (V34)
- Metacognitive "NO" (V31)
- Budget dynamique (V32)
- Reputation system (V29)

---

## 🎯 PHASE 1.3: Fusionner Metacognitive Controllers (2→1)

### Fichiers à fusionner:
1. `metacognitive_controller.py` (V31, 445 lignes)
2. `metacognitive_controller_v34.py` (V34, 523 lignes)

**Total**: 968 lignes → **~550 lignes** (économie 418 lignes)

### Module final: `metacognitive_controller_v34.py` (garder uniquement)
**Action**: Supprimer V31

**Raison**: V34 inclut toutes capacités V31 + améliorations:
- Sterility detection
- Stop function
- Enhanced monitoring

---

## 🎯 PHASE 1.4: Supprimer policy_manager_v39

### Fichiers:
1. `policy_manager_v39.py` (378 lignes) → **SUPPRIMER**
2. `policy_manager_v40_3.py` (412 lignes) → **GARDER**

**Économie**: 378 lignes

**Raison**: V40.3 corrige bug critique 32/36 APIs inutilisées (World Model consultation forcée)

---

## 🎯 PHASE 1.5: Supprimer state_encoder_v24

### Fichiers:
1. `state_encoder.py` (V24, 322 lignes) → **SUPPRIMER**
2. `state_encoder_v25.py` (V25, 535 lignes) → **GARDER**

**Économie**: 322 lignes

**Raison**: V25 embeddings enrichis 256D (vs 128D V24):
- Relations spatiales
- Hash topologique SHA256
- Action context
- Augmentation 128→256D

---

## 🎯 PHASE 1.6-1.9: Créer 4 Fichiers Manquants

### 1.6: `semantic_memory_v39.py` (~400 lignes)
**Fonction**: Mémoire sémantique Layer 6
**Capacités**:
- Stockage concepts abstraits
- Associations sémantiques
- Généralisation cross-puzzle
- Retrieval par similarité

### 1.7: `spatial_compression_engine.py` (~350 lignes)
**Fonction**: Compression trajectoires en patterns cognitifs
**Capacités**:
- 5 types patterns (linear_probe, loop, zigzag, cluster, dead_end)
- Information_gained + strategic_value
- Compression géométrique

### 1.8: `state_transition_validator.py` (~300 lignes)
**Fonction**: Validation transitions causales
**Capacités**:
- Vérification cohérence transitions
- Détection violations physiques
- Validation contraintes

### 1.9: `topological_hash_v39.py` (~250 lignes)
**Fonction**: Hash topologique pour états
**Capacités**:
- Hash SHA256 structure spatiale
- Invariance transformations
- Détection états équivalents

---

## 📈 RÉSUMÉ ÉCONOMIES PHASE 1

| Action | Fichiers | Lignes avant | Lignes après | Économie |
|--------|----------|--------------|--------------|----------|
| Avatar Unified | 4→1 | 1,753 | 500 | **1,253** |
| Decision Kernels | 6→1 | 2,148 | 500 | **1,648** |
| Metacognitive | 2→1 | 968 | 550 | **418** |
| Policy Manager | 2→1 | 790 | 412 | **378** |
| State Encoder | 2→1 | 857 | 535 | **322** |
| **TOTAL SUPPRESSIONS** | **16→5** | **6,516** | **2,497** | **4,019** |
| Fichiers manquants | +4 | 0 | 1,300 | **-1,300** |
| **TOTAL NET** | **99→87** | **40,665** | **37,946** | **+2,719** |

**Réduction nette**: 2,719 lignes (6.7%)
**Modules**: 99 → 87 (-12 fichiers)

---

## 🔧 ORDRE D'EXÉCUTION RECOMMANDÉ

### Étape 1: Suppressions simples (rapide)
1. ✅ Supprimer `policy_manager_v39.py`
2. ✅ Supprimer `state_encoder.py` (V24)
3. ✅ Supprimer decision_kernel V29-V32 + minimal

### Étape 2: Fusions complexes (moyen)
4. ✅ Fusionner Metacognitive Controllers → V34
5. ✅ Fusionner Avatar Identification → avatar_unified.py

### Étape 3: Créations (long)
6. ✅ Créer `semantic_memory_v39.py`
7. ✅ Créer `spatial_compression_engine.py`
8. ✅ Créer `state_transition_validator.py`
9. ✅ Créer `topological_hash_v39.py`

### Étape 4: Validation
10. ✅ Tests imports tous modules
11. ✅ Vérification dépendances
12. ✅ Update documentation

---

## ⚠️ PRÉCAUTIONS

### Avant suppression:
- ✅ Vérifier aucune référence externe
- ✅ Backup fichiers originaux
- ✅ Tester imports après chaque suppression

### Après fusion:
- ✅ Mettre à jour tous imports
- ✅ Vérifier compatibilité API
- ✅ Tests unitaires modules fusionnés

---

## 🎯 CRITÈRES DE SUCCÈS PHASE 1

- [ ] 12 fichiers supprimés
- [ ] 4 fichiers créés
- [ ] 2,719 lignes économisées
- [ ] 0 régression fonctionnelle
- [ ] Tous tests passent
- [ ] Documentation à jour

---

## 📅 TIMELINE ESTIMÉE

- **Étape 1** (Suppressions): 30 min
- **Étape 2** (Fusions): 2h
- **Étape 3** (Créations): 4h
- **Étape 4** (Validation): 1h
- **TOTAL**: ~7h30

---

## 🚀 PROCHAINE ÉTAPE

Après Phase 1 complétée → **Phase 2: Implémentation Architecture V2**
- Layer 0: Sensorimotor
- Layer 9: Prediction (CORE)
- Layer 3: Attention

---

*Plan créé par Bob (Expert Architecture Logicielle)*
*Méthodologie: Refactoring progressif avec validation continue*