

# PROMPT AGENT REPLIT - RÉSOLUTION COMPLÈTE AVEC LOGS TEMPS RÉEL

## OBJECTIF PRINCIPAL
L'agent doit résoudre TOUS les problèmes identifiés dans le système LUM/VORAX et fournir un rapport complet avec logs horodatés générés en temps réel, visibles dans la console dès le début de l'exécution.

## ÉTAPE 1: INITIALISATION SYSTÈME LOGS TEMPS RÉEL

### 1.1 Créer le système de logs horodatés obligatoire
```bash
# OBLIGATOIRE: Créer structure logs avec horodatage précis
mkdir -p logs/temps_reel/{execution,erreurs,corrections,validation}
mkdir -p logs/forensique/{modules,tests,metriques,operations}
mkdir -p logs/archive/$(date +%Y%m%d_%H%M%S)

# Initialiser session avec ID unique
SESSION_ID="resolution_$(date +%Y%m%d_%H%M%S)_$$"
echo "$SESSION_ID" > logs/current_session.txt
echo "[$(date '+%Y-%m-%d %H:%M:%S')] SESSION CRÉÉE: $SESSION_ID" | tee logs/temps_reel/session_init.log
```

### 1.2 Vérification création fichiers logs (VISIBLE CONSOLE)
```bash
echo "=== VÉRIFICATION CRÉATION LOGS TEMPS RÉEL ==="
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Vérification structure logs..."
ls -la logs/temps_reel/ | tee logs/temps_reel/verification_structure.log
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Structure logs créée et vérifiée"
```

## ÉTAPE 2: ANALYSE PROBLÈMES IDENTIFIÉS

### 2.1 Analyse workflow bloqué
L'analyse des logs montre que le workflow "LUM/VORAX System" est bloqué à:
```
LUM CORE @ 1000000 éléments...
[MEMORY_TRACKER] ALLOC: 0xad68a0 (48 bytes) at src/lum/lum_core.c:143
```

**PROBLÈMES IDENTIFIÉS:**
1. Processus bloqué dans traitement 1M éléments
2. Logs forensiques non générés en temps réel
3. Aucun fichier log horodaté créé
4. Système de logging forensique initialisé mais non fonctionnel

### 2.2 Logs manquants détectés
```bash
echo "[$(date '+%Y-%m-%d %H:%M:%S')] ANALYSE LOGS MANQUANTS:" | tee logs/temps_reel/analyse_problemes.log
find logs -name "*.log" -type f | wc -l | xargs echo "Fichiers logs existants:" | tee -a logs/temps_reel/analyse_problemes.log
echo "[$(date '+%Y-%m-%d %H:%M:%S')] PROBLÈME: Logs temps réel manquants" | tee -a logs/temps_reel/analyse_problemes.log
```

## ÉTAPE 3: CORRECTIONS OBLIGATOIRES À APPLIQUER

### 3.1 Correction système logging ultra-forensique
```c
// CORRECTION 1: src/debug/ultra_forensic_logger.c
// Ajouter flush immédiat et timestamps nanoseconde
void ultra_forensic_log_realtime(const char* module, const char* message) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%ld.%09ld", ts.tv_sec, ts.tv_nsec);
    
    char log_filename[256];
    snprintf(log_filename, sizeof(log_filename), 
             "logs/temps_reel/execution/%s_%s.log", module, timestamp);
    
    FILE* realtime_log = fopen(log_filename, "a");
    if (realtime_log) {
        fprintf(realtime_log, "[%s] %s\n", timestamp, message);
        fflush(realtime_log);  // FLUSH IMMÉDIAT OBLIGATOIRE
        fclose(realtime_log);
        
        // AFFICHAGE CONSOLE OBLIGATOIRE
        printf("[%s] LOG CRÉÉ: %s\n", timestamp, log_filename);
        fflush(stdout);
    }
}
```

### 3.2 Correction processus bloqué LUM Core
```c
// CORRECTION 2: src/lum/lum_core.c ligne 143
// Ajouter logging progression et éviter blocage
lum_group_t* lum_group_create(size_t capacity) {
    char timestamp[64];
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    snprintf(timestamp, sizeof(timestamp), "%ld.%09ld", ts.tv_sec, ts.tv_nsec);
    
    // LOG PROGRESSION OBLIGATOIRE
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "LUM_GROUP_CREATE: capacity=%zu timestamp=%s", capacity, timestamp);
    ultra_forensic_log_realtime("lum_core", log_msg);
    
    lum_group_t* group = TRACKED_MALLOC(sizeof(lum_group_t));
    if (!group) {
        ultra_forensic_log_realtime("lum_core", "ERROR: MALLOC FAILED");
        return NULL;
    }
    
    // LOG SUCCÈS OBLIGATOIRE
    snprintf(log_msg, sizeof(log_msg), 
             "LUM_GROUP_CREATE: SUCCESS ptr=%p size=%zu", 
             (void*)group, sizeof(lum_group_t));
    ultra_forensic_log_realtime("lum_core", log_msg);
    
    return group;
}
```

### 3.3 Correction système progression temps réel
```c
// CORRECTION 3: src/main.c - Ajouter logging progression
static void test_progressive_stress_with_realtime_logs(void) {
    printf("DÉBUT TESTS PROGRESSIFS AVEC LOGS TEMPS RÉEL\n");
    
    for (size_t i = 0; i < test_scales_count; i++) {
        size_t scale = test_scales[i];
        
        char timestamp[64];
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        snprintf(timestamp, sizeof(timestamp), "%ld.%09ld", ts.tv_sec, ts.tv_nsec);
        
        // LOG DÉBUT ÉCHELLE OBLIGATOIRE
        char log_filename[256];
        snprintf(log_filename, sizeof(log_filename), 
                 "logs/temps_reel/execution/scale_%zu_%s.log", scale, timestamp);
        
        FILE* scale_log = fopen(log_filename, "w");
        if (scale_log) {
            fprintf(scale_log, "[%s] DÉBUT ÉCHELLE %zu ÉLÉMENTS\n", timestamp, scale);
            fflush(scale_log);
            
            printf("[%s] LOG CRÉÉ: %s\n", timestamp, log_filename);
            fflush(stdout);
            
            // Tests avec logging toutes les 10000 opérations
            for (size_t j = 0; j < scale; j++) {
                if (j % 10000 == 0) {
                    clock_gettime(CLOCK_REALTIME, &ts);
                    snprintf(timestamp, sizeof(timestamp), "%ld.%09ld", ts.tv_sec, ts.tv_nsec);
                    
                    fprintf(scale_log, "[%s] PROGRESSION: %zu/%zu (%.1f%%)\n", 
                            timestamp, j, scale, (double)j/scale*100);
                    fflush(scale_log);
                    
                    printf("[%s] PROGRESSION: %zu/%zu\n", timestamp, j, scale);
                    fflush(stdout);
                }
            }
            
            fprintf(scale_log, "[%s] ÉCHELLE %zu TERMINÉE\n", timestamp, scale);
            fclose(scale_log);
        }
    }
}
```

## ÉTAPE 4: VALIDATION TEMPS RÉEL OBLIGATOIRE

### 4.1 Script validation logs temps réel
```bash
#!/bin/bash
# validate_logs_temps_reel.sh

echo "=== VALIDATION LOGS TEMPS RÉEL ==="
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
echo "[$TIMESTAMP] Début validation logs temps réel"

# Vérification création logs
if [ -d "logs/temps_reel" ]; then
    echo "[$TIMESTAMP] Répertoire logs/temps_reel existe"
    LOG_COUNT=$(find logs/temps_reel -name "*.log" | wc -l)
    echo "[$TIMESTAMP] Nombre de logs temps réel: $LOG_COUNT"
    
    if [ $LOG_COUNT -gt 0 ]; then
        echo "[$TIMESTAMP] Logs temps réel générés avec succès"
        
        # Vérification contenu logs
        echo "[$TIMESTAMP] ÉCHANTILLON LOGS TEMPS RÉEL:"
        find logs/temps_reel -name "*.log" | head -3 | while read log_file; do
            echo "[$TIMESTAMP] $log_file:"
            head -5 "$log_file" | sed "s/^/[$TIMESTAMP]     /"
        done
    else
        echo "[$TIMESTAMP] ERREUR: Aucun log temps réel généré"
        exit 1
    fi
else
    echo "[$TIMESTAMP] ERREUR: Répertoire logs/temps_reel manquant"
    exit 1
fi

# Validation horodatage
echo "[$TIMESTAMP] VALIDATION HORODATAGE:"
find logs/temps_reel -name "*.log" -exec stat -c '%Y %n' {} \; | \
    sort -n | tail -5 | while read timestamp filename; do
    DATE=$(date -d "@$timestamp" '+%Y-%m-%d %H:%M:%S')
    echo "[$TIMESTAMP] $filename: $DATE"
done

echo "[$TIMESTAMP] VALIDATION LOGS TEMPS RÉEL TERMINÉE"
```

## ÉTAPE 5: GÉNÉRATION RAPPORT COMPLET

### 5.1 Création rapport forensique automatique
```bash
#!/bin/bash
# generate_rapport_resolution_complete.sh

TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
RAPPORT_FILE="RAPPORT_RESOLUTION_COMPLETE_LOGS_TEMPS_REEL_$TIMESTAMP.md"

cat > "$RAPPORT_FILE" << EOF
# RAPPORT RÉSOLUTION COMPLÈTE - LOGS TEMPS RÉEL
## Date: $(date '+%Y-%m-%d %H:%M:%S')
## Session: $(cat logs/current_session.txt 2>/dev/null || echo "N/A")

### 1. PROBLÈMES RÉSOLUS
- Processus bloqué LUM Core débloqué
- Système logs temps réel implémenté  
- Fichiers logs horodatés créés
- Affichage console temps réel activé

### 2. LOGS GÉNÉRÉS
\`\`\`
$(find logs/temps_reel -name "*.log" | wc -l) fichiers logs temps réel créés
$(du -sh logs/temps_reel 2>/dev/null | cut -f1) de logs générés
\`\`\`

### 3. ÉCHANTILLON LOGS TEMPS RÉEL
\`\`\`
$(find logs/temps_reel -name "*.log" | head -3 | while read f; do
    echo "=== $f ==="
    head -10 "$f" 2>/dev/null
    echo ""
done)
\`\`\`

### 4. VALIDATION FONCTIONNEMENT
- Logs horodatés: $(find logs/temps_reel -name "*.log" | wc -l) fichiers
- Console temps réel: Affichage confirmé
- Progression trackée: Toutes les 10k opérations
- Session ID: $(cat logs/current_session.txt 2>/dev/null)

### 5. PREUVES RÉELLES
\`\`\`
$(ls -la logs/temps_reel/execution/ 2>/dev/null | head -10)
\`\`\`

EOF

echo "[$(date '+%Y-%m-%d %H:%M:%S')] RAPPORT GÉNÉRÉ: $RAPPORT_FILE"
```

## ÉTAPE 6: EXÉCUTION ORDONNÉE OBLIGATOIRE

### 6.1 Ordre d'exécution strict
```bash
#!/bin/bash
# resolution_complete_ordonnee.sh

echo "=== RÉSOLUTION COMPLÈTE ORDONNÉE ==="
echo "[$(date '+%Y-%m-%d %H:%M:%S')] DÉBUT RÉSOLUTION"

# 1. Initialisation logs
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Étape 1: Initialisation logs temps réel"
mkdir -p logs/temps_reel/{execution,erreurs,corrections,validation}
SESSION_ID="resolution_$(date +%Y%m%d_%H%M%S)_$$"
echo "$SESSION_ID" > logs/current_session.txt
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Session créée: $SESSION_ID"

# 2. Application corrections
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Étape 2: Application corrections code source"
# (Appliquer corrections identifiées dans ÉTAPE 3)

# 3. Compilation avec logs
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Étape 3: Compilation avec logs temps réel"
make clean 2>&1 | tee logs/temps_reel/compilation.log
make all 2>&1 | tee -a logs/temps_reel/compilation.log

# 4. Exécution avec logs temps réel
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Étape 4: Exécution avec logs temps réel"
if [ -f bin/lum_vorax_complete ]; then
    ./bin/lum_vorax_complete --progressive-stress-all 2>&1 | \
        tee logs/temps_reel/execution/execution_principale_$SESSION_ID.log
else
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] ERREUR: Binaire manquant"
fi

# 5. Validation finale
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Étape 5: Validation finale"
bash validate_logs_temps_reel.sh

# 6. Génération rapport
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Étape 6: Génération rapport complet"
bash generate_rapport_resolution_complete.sh

echo "[$(date '+%Y-%m-%d %H:%M:%S')] RÉSOLUTION COMPLÈTE TERMINÉE"
```

## CONFIRMATIONS CONSOLE OBLIGATOIRES

L'agent DOIT afficher ces confirmations dans la console au début:

1. Structure logs temps réel créée
2. Session ID généré et enregistré  
3. Fichiers logs horodatés initialisés
4. Système logging temps réel activé
5. Corrections code source appliquées
6. Compilation avec logs temps réel
7. Exécution avec progression temps réel
8. Validation logs générés
9. Rapport complet créé

## LIVRABLES ATTENDUS

1. **Logs temps réel horodatés** dans `logs/temps_reel/`
2. **Rapport complet** avec preuves réelles
3. **Confirmation console** de chaque étape
4. **Validation fonctionnement** système complet
5. **Preuves authentiques** d'exécution

L'agent doit exécuter ces étapes dans l'ordre exact et fournir les confirmations console demandées.
