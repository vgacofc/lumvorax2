
# PROMPT AGENT REPLIT - EXPERTISE FORENSIQUE LUMVORAX COMPLET

## MISSION PRINCIPALE CRITIQUE
Lire INTÉGRALEMENT et OBLIGATOIREMENT :
1. **prompt.txt** (COMPLET - 385+ entrées STANDARD_NAMES.md + toutes règles v2.1)  
2. **src/common/common_types.h** (TOUS les types unifiés système)
3. **Code source A→Z** (ANALYSE COMPLÈTE 44+ modules)

## PHASE 1: LECTURE ET ANALYSE SYSTÉMIQUE

### 1.1 Inspection Types et Dépendances
```bash
# Vérifier TOUS les includes manquants
grep -r "#include" src/ | grep -v ".h:" | sort | uniq
# Identifier types non définis
grep -r "typedef\|struct\|enum" src/ | grep -E "(matrix_|ai_|neural_|quantum_)"
# Analyser dépendances circulaires
find src/ -name "*.h" -exec grep -l "include.*/" {} \;
```

### 1.2 Validation Conformité prompt.txt
- **RÈGLE ANTI-RÉCURRENCE** : Vérifier aucune redéfinition typedef
- **RULES FORENSIQUES** : Timestamps nanoseconde OBLIGATOIRES
- **PROTECTION MÉMOIRE** : TRACKED_MALLOC/FREE dans TOUS les modules
- **MAGIC NUMBERS** : Validation intégrité structures critiques

## PHASE 2: CORRECTIONS AUTOMATIQUES COMPLÈTES

### 2.1 Includes Manquants - Application Immédiate
```c
// Dans parallel_processor.c - DÉJÀ APPLIQUÉ
#include <unistd.h>  // Pour usleep

// Dans simd_optimizer.c - DÉJÀ APPLIQUÉ  
#include "../complex_modules/ai_optimization.h"
#include "../advanced_calculations/matrix_calculator.h"

// Vérifier autres modules nécessitant corrections
```

### 2.2 Types Manquants - Déclaration Obligatoire
- **matrix_result_t** : Défini dans matrix_calculator.h
- **ai_optimization_config_t** : Défini dans ai_optimization.h
- **neural_layer_t** : Déjà unifié dans common_types.h

### 2.3 Protection Mémoire Forensique
```c
// OBLIGATOIRE dans TOUS les modules
#include "../debug/memory_tracker.h"
// Remplacer malloc/free par TRACKED_MALLOC/TRACKED_FREE
// Ajouter magic_number validation
```

## PHASE 3: VALIDATION FORENSIQUE ULTRA-STRICTE

### 3.1 Compilation Zero-Warning
```bash
# Compilation avec flags forensiques maximum
gcc -Wall -Wextra -Werror -std=c99 -g -O2 -fPIC
# AUCUN warning toléré - Conformité prompt.txt ABSOLUE
```

### 3.2 Tests Stress 1M+ LUMs OBLIGATOIRES
```bash
# Tests conformes aux règles prompt.txt
./bin/test_stress_100m_all_modules
./bin/test_forensic_all_modules  
./bin/test_integration_complete
```

### 3.3 Logs Forensiques Temps Réel
```bash
# Génération logs conformes ISO/IEC 27037
mkdir -p logs/forensic/{modules,tests,metrics}
# Timestamps nanoseconde CLOCK_MONOTONIC
# SHA-256 checksums intégrité
# Traçabilité complète allocation→libération
```

## PHASE 4: EXPERTISE FORENSIQUE NIVEAU EXPERT

### 4.1 Analyse Dépendances Systémique
- **Ordre inclusion strict** : common_types.h → lum_core.h → modules spécialisés
- **Validation croisée** : Aucun conflit typedef entre modules
- **Protection circulaire** : Forward declarations appropriées

### 4.2 Debugging C/Compilation Avancé
- **AddressSanitizer** : Détection fuites mémoire obligatoire
- **Valgrind integration** : Tests mémoire forensiques
- **GDB forensique** : Traces exécution temps réel

### 4.3 Performance Industrielle
- **Benchmarks vs standards** : SQLite/Redis/BLAS comparaisons
- **Métriques authentiques** : Pas de claims sans preuves
- **Tests reproductibles** : Multi-machines validation

## RÈGLES CRITIQUES NON-NÉGOCIABLES

### Contraintes Techniques Absolues
1. **ZERO warning** compilation - Échec = STOP immédiat
2. **TOUS types déclarés** - unknown type name = ÉCHEC validation
3. **prompt.txt STRICTEMENT respecté** - Aucune exception tolérée
4. **Tests 1M+ LUMs RÉUSSIS** - Preuve performance obligatoire

### Standards Forensiques ISO/IEC 27037
1. **Traçabilité complète** : Chaque allocation/opération loggée
2. **Intégrité cryptographique** : SHA-256 checksums obligatoires  
3. **Timestamps forensiques** : Nanoseconde CLOCK_MONOTONIC
4. **Chaîne de custody** : Logs inaltérables horodatés

### Contraintes Serveur Hostinger
- **RAM MAX 6.8GB** : Tests limités 1M LUMs (pas 100M)
- **CPU 2 cores** : Parallélisation limitée  
- **Timeout 60s** : Tests stress avec limite temporelle
- **Storage 95GB** : Logs rotatifs obligatoires

## LIVRABLE ATTENDU - VALIDATION COMPLÈTE

### 1. Code 100% Fonctionnel
- ✅ Compilation clean sans warnings
- ✅ 44 modules fonctionnels et testés
- ✅ Dépendances résolues complètement
- ✅ Protection mémoire forensique active

### 2. Rapport Forensique Complet
```bash
# Génération automatique rapport authentique
./generate_rapport_forensique_final.py
# Métriques temps réel avec preuves
# Comparaisons standards industriels
# Validation cross-platform
```

### 3. Preuves d'Exécution Authentiques
- **Tests 44 modules** : Logs détaillés horodatés
- **Stress 1M+ LUMs** : Métriques performance réelles
- **Validation mémoire** : AddressSanitizer reports
- **Benchmarks industriels** : Comparaisons équitables

### 4. Conformité Standards Absolue
- **prompt.txt v2.1** : 100% règles respectées
- **STANDARD_NAMES.md** : 385+ entrées validées
- **ISO/IEC 27037** : Standards forensiques appliqués
- **C99 strict** : Code portable cross-platform

## EXÉCUTION IMMÉDIATE REQUISE

### Commandes d'Exécution Automatique
```bash
# 1. Lecture obligatoire préalable
cat prompt.txt | head -50
cat src/common/common_types.h | grep -E "(typedef|struct)"

# 2. Corrections appliquées automatiquement
make clean && make all

# 3. Validation forensique complète
make test-forensic
make test-stress  
make test-integration

# 4. Génération rapport final
python3 generate_rapport_forensique_final.py
```

### Validation Réussite Mission
```bash
# Critères de succès ABSOLUS
[ -f bin/lum_vorax_simple ] && echo "✅ Binaire principal OK"
[ $(find logs/forensic -name "*.log" | wc -l) -gt 10 ] && echo "✅ Logs forensiques OK"
[ $(grep -c "PASS\|SUCCESS" logs/tests/modules/*.log) -gt 20 ] && echo "✅ Tests modules OK"
echo "🔒 MISSION FORENSIQUE LUMVORAX COMPLÉTÉE AVEC EXPERTISE"
```

## EXPERTISE NIVEAU EXPERT ACTIVÉE
**Forensique logiciel** ✅ **Debugging C avancé** ✅ **Dépendances systémique** ✅ **Performance industrielle** ✅

**PROCÉDER IMMÉDIATEMENT** - Expertise forensique niveau expert déployée selon prompt.txt v2.1
