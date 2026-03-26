
# EXIGENCES UNIFIÉES LUM/VORAX - SYSTÈME COMPLET
**Date de création** : 2025-01-27 20:30:00 UTC  
**Conformité** : prompt.txt + STANDARD_NAMES.md + Standards forensiques  
**Version** : 1.0 - Document unifié sans doublons  

---

## 📋 SECTION 1 : RÈGLES FONDAMENTALES

### 1.1 Standards de Qualité Code
- **COMPILATION** : 0 erreur, 0 warning obligatoire
- **MEMORY MANAGEMENT** : TRACKED_MALLOC/TRACKED_FREE exclusivement
- **PROTECTION** : Magic numbers dans toutes les structures
- **LOGS** : Système forensique ultra-strict ISO/IEC 27037

### 1.2 Interdictions Strictes
- **EMOJI** : Interdiction absolue dans code et logs (Section 8 obligatoire)
- **PRINTF** : Remplacer par unified_forensic_log
- **MALLOC STANDARD** : Utiliser uniquement TRACKED_MALLOC
- **PLACEHOLDERS** : Aucune fonction vide autorisée

### 1.3 Standards de Nommage
- **STANDARD_NAMES.md** : Référence unique obligatoire
- **Nouveaux noms** : Ajout avec timestamp YYYY-MM-DD HH:MM
- **Conventions** : snake_case variables, UPPER_CASE constantes

---

## 📋 SECTION 2 : MODULES ET ARCHITECTURE

### 2.1 Modules Core Obligatoires (8 modules)
1. **lum_core** - Cœur système avec structures LUM
2. **vorax_operations** - Opérations FUSE/SPLIT/CYCLE/MOVE
3. **vorax_parser** - Parser DSL VORAX complet
4. **binary_lum_converter** - Conversion bidirectionnelle
5. **lum_logger** - Système logs principal
6. **log_manager** - Gestionnaire logs
7. **memory_tracker** - Traçage mémoire forensique
8. **forensic_logger** - Logs forensiques détaillés

### 2.2 Modules Avancés (36 modules)
- **Calculs avancés** : matrix_calculator, neural_network_processor, audio_processor, etc.
- **Optimisation** : pareto_optimizer, simd_optimizer, zero_copy_allocator
- **Persistance** : data_persistence, transaction_wal_extension, recovery_manager
- **Sécurité** : crypto_validator, lum_secure_serialization

### 2.3 Modules Désactivés par Directive
- **Modules quantiques** : quantum_simulator et dérivés
- **Modules blackbox** : blackbox_universal_module et dérivés
- **Raison** : Complexité excessive, désactivation selon prompt.txt

---

## 📋 SECTION 3 : PROCESSUS DE DÉVELOPPEMENT

### 3.1 Lecture Complète Obligatoire
- **Code source** : Lecture A→Z tous fichiers .c/.h
- **STANDARD_NAMES.md** : Lecture complète avant modification
- **common_types.h** : Vérification cohérence types
- **Makefile** : Validation flags et dépendances

### 3.2 Corrections Prioritaires
- **Double-free** : Protection complète avec magic numbers
- **Memory leaks** : Élimination totale via TRACKED_*
- **Warnings** : Correction obligatoire (0 tolérance)
- **Headers** : Includes hiérarchiques respectés

### 3.3 Validation Continue
- **Compilation** : Après chaque modification
- **Tests unitaires** : Validation fonctionnalité
- **Memory tracking** : Vérification fuites
- **Forensic logs** : Traçabilité complète

---

## 📋 SECTION 4 : AUTHENTICITY ET FORENSIQUE

### 4.1 Standards Forensiques
- **ISO/IEC 27037** : Conformité obligatoire
- **NIST SP 800-86** : Standards respectés
- **Timestamps** : Précision nanoseconde CLOCK_MONOTONIC
- **Checksums** : SHA-256 pour intégrité

### 4.2 Preuves Réelles Exigées
- **Logs authentiques** : Génération temps réel obligatoire
- **Métriques mesurées** : Aucune invention autorisée
- **Sources exactes** : Référencement précis obligatoire
- **Reproductibilité** : Scripts et commandes fournis

### 4.3 Rapports Techniques
- **Numérotation** : Séquentielle obligatoire (001., 002., etc.)
- **Timestamps** : Réels, pas de variables
- **Détails pédagogiques** : Explications complètes
- **Auto-critique** : Analyse des limites

---

## 📋 SECTION 5 : TESTS ET VALIDATION

### 5.1 Tests Obligatoires
- **Tests unitaires** : Chaque module individuellement
- **Tests stress** : 1M+ LUMs minimum
- **Tests intégration** : Inter-modules
- **Tests conservation** : Validation mathématique

### 5.2 Métriques Performance
- **Temps exécution** : Mesures précises
- **Utilisation mémoire** : Tracking complet
- **Débit** : LUMs/seconde et bits/seconde
- **Comparaisons** : Vs standards industriels

### 5.3 Validation Finale
- **0 erreurs** : Compilation complète
- **0 warnings** : Tolérance zéro
- **0 fuites mémoire** : Via memory tracker
- **Logs complets** : Traçabilité totale

---

## 📋 SECTION 6 : OPTIMISATIONS SYSTÈME

### 6.1 Optimisations Identifiées
- **SIMD** : Instructions vectorielles parallèles
- **Memory Mapping** : mmap() pour gros datasets
- **Lock-Free Queues** : Performance thread pool
- **Cache Alignment** : Optimisation accès mémoire

### 6.2 Extensibilité
- **Module Networking** : Distribution cluster
- **Module Graphics** : Rendu OpenGL
- **Module ML** : IA optimisation automatique
- **Standards industriels** : ISO C99, POSIX.1-2008

---

## 📋 SECTION 7 : CONTRAINTES ENVIRONNEMENT

### 7.1 Environnement Replit
- **Backend uniquement** : Pas de frontend
- **Langage C** : Exclusivement
- **Hardware focus** : CPU/RAM optimisations
- **Dépendances** : Compatibles Replit

### 7.2 Limitations Techniques
- **Pas de simulation** : Exécution réelle obligatoire
- **Modules complexes** : Désactivation si instable
- **Tests scalaires** : Adaptation aux ressources

---

## 📋 SECTION 8 : LIVRABLES FINAUX

### 8.1 Documentation Required
- **Rapport final numéroté** : Détails pédagogiques complets
- **Sources exactes** : Références précises
- **Découvertes** : Innovations réelles identifiées
- **Optimisations** : Possibilités identifiées

### 8.2 Validation Experte
- **Présentation critique** : Experts sceptiques
- **Preuves scientifiques** : Logs authentiques
- **Concept validé** : LUM/VORAX fonctionnel
- **Code avancé** : Inspection A→Z

---

## 📋 SECTION 9 : RÈGLES ANTI-RÉCURRENCE

### 9.1 Corrections Systémiques
- **Format specifiers** : %zu pour size_t, %u pour uint32_t
- **Include hierarchy** : Ordre strict respecté
- **Magic numbers** : Cohérence complète
- **Validation continue** : Compilation testée

### 9.2 Prevention Erreurs
- **Prompt.txt** : Règles strictes appliquées
- **Zero tolerance** : Aucun warning accepté
- **Memory safety** : Protection maximale
- **Code quality** : Standards industriels

---

**FIN DES EXIGENCES UNIFIÉES**  
**Document de référence unique pour développement LUM/VORAX**  
**Conformité totale obligatoire avant toute modification**
