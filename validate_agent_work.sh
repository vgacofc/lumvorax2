
#!/bin/bash

echo "🔍 === VALIDATION FORENSIQUE TRAVAIL AGENT REPLIT ==="
echo "Date: $(date)"
echo ""

# Phase 1: Vérification compilation
echo "📋 Phase 1: Vérification compilation..."
make clean > /dev/null 2>&1
if make all > compilation_check.log 2>&1; then
    echo "✅ Compilation: SUCCÈS"
    COMPILATION_OK=true
else
    echo "❌ Compilation: ÉCHEC"
    echo "Erreurs dans compilation_check.log"
    COMPILATION_OK=false
fi

# Phase 2: Test exécution simple
echo ""
echo "📋 Phase 2: Test exécution système..."
if [ "$COMPILATION_OK" = true ] && [ -f bin/lum_vorax_complete ]; then
    timeout 30s ./bin/lum_vorax_complete --progressive-stress-all > execution_test.log 2>&1 &
    EXEC_PID=$!
    sleep 5
    if ps -p $EXEC_PID > /dev/null; then
        echo "✅ Exécution: DÉMARRÉE (PID: $EXEC_PID)"
        kill $EXEC_PID 2>/dev/null
        echo "✅ Test interrompu proprement"
    else
        echo "❌ Exécution: CRASH IMMÉDIAT"
    fi
else
    echo "❌ Binaire manquant"
fi

# Phase 3: Vérification logs individuels
echo ""
echo "📋 Phase 3: Vérification logs individuels..."
LOGS_FOUND=0
LOGS_EMPTY=0

for module_dir in logs/individual/*/; do
    if [ -d "$module_dir" ]; then
        module_name=$(basename "$module_dir")
        log_files=$(find "$module_dir" -name "*.log" | wc -l)
        if [ $log_files -gt 0 ]; then
            LOGS_FOUND=$((LOGS_FOUND + 1))
            echo "✅ Module $module_name: $log_files logs"
        else
            LOGS_EMPTY=$((LOGS_EMPTY + 1))
            echo "❌ Module $module_name: AUCUN LOG"
        fi
    fi
done

# Phase 4: Rapport final
echo ""
echo "📊 === RAPPORT FINAL VALIDATION ==="
echo "Modules avec logs: $LOGS_FOUND"
echo "Modules sans logs: $LOGS_EMPTY"
echo "Compilation: $([ "$COMPILATION_OK" = true ] && echo "OK" || echo "ÉCHEC")"

# Note pour l'utilisateur
echo ""
echo "🎯 RÉSULTAT: $LOGS_FOUND modules validés sur 44 attendus"
if [ $LOGS_FOUND -lt 5 ]; then
    echo "❌ VALIDATION ÉCHOUÉE - Logs individuels non générés"
    exit 1
else
    echo "⚠️ VALIDATION PARTIELLE - Besoin correction logs"
    exit 2
fi
