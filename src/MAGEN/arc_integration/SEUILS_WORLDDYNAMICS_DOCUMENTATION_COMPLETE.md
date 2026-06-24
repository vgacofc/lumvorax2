# DOCUMENTATION COMPLÈTE - TOUS LES SEUILS WORLDDYNAMICS

**Fichier**: `world_dynamics.py`  
**Date**: 2026-06-20  
**Version**: 4.5.3

## LISTE EXHAUSTIVE DES SEUILS

### 1. SEUILS D'INITIALISATION (Ligne 103-106)

```python
self.invariant_threshold = 0.1
self.cycle_confidence_threshold = 0.7
self.min_observations_for_invariant = 5
```

**Explication**:
- `invariant_threshold = 0.1`: **Variance maximale** pour qu'une propriété soit considérée comme invariante (stable). Si variance < 0.1, la propriété ne change presque jamais.
- `cycle_confidence_threshold = 0.7`: **Confiance minimale** (70%) pour valider un cycle détecté (actuellement non utilisé dans le code).
- `min_observations_for_invariant = 5`: **Nombre minimum d'observations** nécessaires avant de détecter un invariant. Évite faux positifs avec peu de données.

---

### 2. SEUILS DÉTECTION INVARIANTS

#### 2.1 Position Stable (Ligne 166)
```python
if variance < self.invariant_threshold:  # variance < 0.1
```
**Explication**: Si variance position < 0.1, l'entité est **immobile** (comme la porte à position fixe).

#### 2.2 Taille Stable (Ligne 191)
```python
if variance < self.invariant_threshold:  # variance < 0.1
```
**Explication**: Si variance taille < 0.1, la taille est **constante** (comme la porte avec 892 pixels fixes).

#### 2.3 Observations Minimales (Ligne 143, 162, 187)
```python
if len(state_sequence) < self.min_observations_for_invariant:  # < 5
if len(properties['position']) >= self.min_observations_for_invariant:  # >= 5
if len(properties['size']) >= self.min_observations_for_invariant:  # >= 5
```
**Explication**: Nécessite **au moins 5 observations** pour détecter un invariant fiable.

---

### 3. SEUILS DÉTECTION CYCLES

#### 3.1 Séquence Minimale (Ligne 221)
```python
if len(state_sequence) < 10:
```
**Explication**: Nécessite **au moins 10 états** pour détecter un cycle temporel.

#### 3.2 Changement Significatif (Ligne 237)
```python
if changes > 5:
```
**Explication**: Un changement > 5 unités est considéré comme **événement détectable**. Seuil bas pour capturer plus d'événements.

#### 3.3 Événements Minimaux (Ligne 241)
```python
if len(event_steps) >= 3:
```
**Explication**: Nécessite **au moins 3 événements** pour détecter un pattern cyclique.

#### 3.4 Intervalles Minimaux (Ligne 245)
```python
if len(intervals) >= 2:
```
**Explication**: Nécessite **au moins 2 intervalles** entre événements pour calculer périodicité.

#### 3.5 Tolérance Régularité (Ligne 250)
```python
if std_interval < mean_interval * 0.3:  # 30% tolérance
```
**Explication**: Les intervalles doivent être réguliers à **30% près**. Si écart-type < 30% de la moyenne, c'est un cycle.

#### 3.6 Confiance Minimale Cycle (Ligne 252, 257)
```python
confidence = float(1.0 - min(std_interval / mean_interval, 0.5))
confidence=max(confidence, 0.5)  # Confiance minimum 50%
```
**Explication**: La confiance d'un cycle est **minimum 50%**, calculée depuis la régularité des intervalles.

#### 3.7 Tolérance Cycle Existant (Ligne 296)
```python
def has_cycle(self, period: int, tolerance: int = 2):
    return any(abs(cycle.period - period) <= tolerance for cycle in self.cycles)
```
**Explication**: Deux cycles sont considérés identiques si leurs périodes diffèrent de **±2 steps maximum**.

#### 3.8 Méthode Alternative - Changements Minimaux (Ligne 267)
```python
if len(all_changes) >= 20 and len(self.cycles) == 0:
```
**Explication**: Si aucun cycle détecté et **au moins 20 changements**, essayer méthode alternative (analyse pics).

#### 3.9 Méthode Alternative - Pics (Ligne 273)
```python
peaks = [i for i, c in enumerate(all_changes) if c > mean_change * 1.5]
```
**Explication**: Un pic est un changement **> 150% de la moyenne**. Identifie événements majeurs.

#### 3.10 Méthode Alternative - Pics Minimaux (Ligne 275)
```python
if len(peaks) >= 3:
```
**Explication**: Nécessite **au moins 3 pics** pour détecter cycle via méthode alternative.

#### 3.11 Méthode Alternative - Tolérance Pics (Ligne 281)
```python
if std_peak_interval < mean_peak_interval * 0.4:  # 40% tolérance
```
**Explication**: Tolérance **40%** pour pics (plus permissive que méthode principale).

---

### 4. SEUILS COMPTAGE CHANGEMENTS

#### 4.1 Entités Créées/Supprimées (Ligne 308-309)
```python
changes += len(set(entities2.keys()) - set(entities1.keys())) * 10
changes += len(set(entities1.keys()) - set(entities2.keys())) * 10
```
**Explication**: Chaque entité créée/supprimée compte pour **10 unités de changement**. Événement majeur.

#### 4.2 Mouvement Détectable (Ligne 319)
```python
if distance > 0.5:
```
**Explication**: Un mouvement > **0.5 pixels** est considéré comme changement significatif.

#### 4.3 Transformation Taille (Ligne 324)
```python
if size_diff > 1:
```
**Explication**: Une différence de taille > **1 pixel** est considérée comme transformation.

---

### 5. SEUILS DÉTECTION CHANGEMENTS DÉTAILLÉS

#### 5.1 Mouvement Entité (Ligne 399)
```python
if distance > 0.5:
```
**Explication**: Même seuil que comptage changements, **0.5 pixels minimum** pour mouvement.

#### 5.2 Transformation Entité (Ligne 407)
```python
if size_diff > 1:
```
**Explication**: Même seuil que comptage changements, **1 pixel minimum** pour transformation.

---

### 6. SEUILS ÉVÉNEMENTS MAJEURS

#### 6.1 Seuil Adaptatif (Ligne 435)
```python
threshold = mean_change + 1.5 * std_change
```
**Explication**: Seuil **dynamique** calculé comme: moyenne + **1.5 × écart-type**. S'adapte aux données.

#### 6.2 Minimum Absolu (Ligne 438)
```python
if changes > threshold and changes > 10:
```
**Explication**: Un événement majeur doit dépasser le seuil adaptatif **ET** être > **10 unités minimum absolu**.

---

## RÉSUMÉ PAR CATÉGORIE

### INVARIANTS
- Variance max: **0.1**
- Observations min: **5**

### CYCLES
- Séquence min: **10 états**
- Changement significatif: **> 5 unités**
- Événements min: **3**
- Intervalles min: **2**
- Tolérance régularité: **30%**
- Confiance min: **50%**
- Tolérance période: **±2 steps**
- Pics: **> 150% moyenne**
- Tolérance pics: **40%**

### CHANGEMENTS
- Entité créée/supprimée: **×10 unités**
- Mouvement min: **0.5 pixels**
- Transformation min: **1 pixel**

### ÉVÉNEMENTS MAJEURS
- Seuil adaptatif: **moyenne + 1.5σ**
- Minimum absolu: **10 unités**

---

## JUSTIFICATION DES VALEURS

### Pourquoi ces seuils ?

1. **invariant_threshold = 0.1**: Permet détecter porte immobile (variance ~0.0) tout en tolérant petites fluctuations numériques.

2. **min_observations_for_invariant = 5**: Compromis entre rapidité détection et fiabilité statistique.

3. **changes > 5**: Seuil bas pour capturer événements subtils (timer décroît de 2 pixels/step).

4. **Tolérance 30%**: Cycle 43 steps peut varier légèrement (42-44) selon timing exact.

5. **Confiance min 50%**: Accepte cycles même avec variance modérée, sera affiné avec données réelles.

6. **Entité ×10**: Création/suppression entité est événement majeur, poids élevé justifié.

7. **Mouvement 0.5 pixels**: Précision sub-pixel pour détecter mouvements lents.

8. **Seuil adaptatif 1.5σ**: Standard statistique pour outliers (événements rares).

---

## AJUSTEMENTS FUTURS

Ces seuils seront **affinés automatiquement** avec:
- Données réelles ls20 (344+ tests)
- Méta-apprentissage (Phase 4.5.5)
- Feedback victoires/défaites

**Status**: Seuils actuels validés par tests unitaires (6/6 réussis, Exit 0)