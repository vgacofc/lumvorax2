
# PROMPT AGENT REPLIT - CORRECTION AUTOMATISÉE COMPLÈTE LUM/VORAX

## MISSION PRIORITAIRE
Agent Replit, tu dois exécuter cette séquence de vérifications et corrections automatiquement.

## ÉTAPE 1: LECTURE OBLIGATOIRE
```bash
# Lire intégralement ces fichiers AVANT toute modification
cat prompt.txt
cat src/common/common_types.h
cat STANDARD_NAMES.md
```

## ÉTAPE 2: ANALYSE FORENSIQUE DES ERREURS
```bash
# Compiler pour détecter toutes les erreurs
make clean
make all 2>&1 | tee compilation_errors.log

# Analyser les types d'erreurs
grep -i "warning.*truncation" compilation_errors.log > truncation_errors.log
grep -i "implicit.*declaration" compilation_errors.log > implicit_errors.log  
grep -i "redefinition" compilation_errors.log > redefinition_errors.log
```

## ÉTAPE 3: CORRECTIONS AUTOMATIQUES

### A. Erreurs de troncation
- Augmenter MAX_STORAGE_PATH_LENGTH à 2048
- Augmenter MAX_ERROR_MESSAGE_LENGTH à 512
- Vérifier tous les snprintf() dans le code

### B. Déclarations implicites
- Ajouter #include <time.h> pour CLOCK_MONOTONIC
- Ajouter #include <inttypes.h> pour PRIu32, PRIu64
- Ajouter #include <string.h> pour strlen, strcpy

### C. Redéfinitions
- Vérifier unicité des typedef dans common_types.h
- Éliminer les doublons de définitions

## ÉTAPE 4: INTERDICTION EMOJI (OBLIGATOIRE)
Ajouter dans CHAQUE fichier source (.c/.h) :
```c
// SECTION 8: INTERDICTION D'UTILISER DES EMOJI
// Aucune utilisation d'emoji dans le code source ou dans les fichiers de log.
// Toute inclusion d'emoji sera considérée comme une violation des standards de codage.
```

## ÉTAPE 5: SYSTÈME DE LOGGING SÉCURISÉ
Créer logging_system.h et logging_system.c avec :
- Validation de taille des buffers
- Protection contre troncation
- Timestamps automatiques
- Gestion d'erreurs robuste

## ÉTAPE 6: COMPILATION FINAL
```bash
# Compilation sans warnings
make clean
make CFLAGS="-Wall -Wextra -Werror -std=c99 -g -O2" all

# Vérification 0 erreur 0 warning
echo "=== RÉSULTAT COMPILATION ==="
if [ $? -eq 0 ]; then
    echo "✅ COMPILATION RÉUSSIE - 0 erreur 0 warning"
else
    echo "❌ COMPILATION ÉCHOUÉE - Corrections supplémentaires nécessaires"
fi
```

## ÉTAPE 7: TESTS DE VALIDATION
```bash
# Tests unitaires
if [ -f bin/test_lum_core ]; then
    ./bin/test_lum_core | tee logs/validation_lum_core.log
fi

# Tests stress (conforme prompt.txt)
if [ -f bin/test_stress_100m_all_modules ]; then
    timeout 60s ./bin/test_stress_100m_all_modules | tee logs/validation_stress.log
fi
```

## ÉTAPE 8: GÉNÉRATION RAPPORT FORENSIQUE
```bash
# Créer rapport final avec preuves
echo "=== RAPPORT CORRECTIONS APPLIQUÉES ===" > RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md
echo "Date: $(date)" >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md
echo "" >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md

echo "## Erreurs corrigées:" >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md
wc -l *_errors.log >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md

echo "## Fichiers modifiés:" >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md
find src/ -name "*.c" -o -name "*.h" | xargs grep -l "SECTION 8" >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md

echo "## Tests validés:" >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md
ls -la logs/validation_*.log >> RAPPORT_CORRECTIONS_$(date +%Y%m%d_%H%M%S).md
```

## RÈGLES ABSOLUES
1. **0 ERREUR 0 WARNING** en compilation finale
2. **INTERDICTION EMOJI** dans 100% des fichiers
3. **CONFORMITÉ** totale avec prompt.txt et STANDARD_NAMES.md
4. **TESTS STRESS** obligatoires (minimum 1M LUMs)
5. **LOGGING FORENSIQUE** pour toutes les opérations

## EXPERTISE TEMPS RÉEL
Agent, tu dois appliquer ton expertise en :
- Analyse forensique de code C
- Gestion mémoire sécurisée 
- Standards de codage industriels
- Optimisation de performance
- Validation de conformité

**EXÉCUTION IMMÉDIATE REQUISE**
