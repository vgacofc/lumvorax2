#!/bin/bash
# Script C620 - Exécution parallèle de toutes les validations
# Date: 20 juin 2026

set -e

BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$BASEDIR"

LOG_DIR="logs/c620_validation_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$LOG_DIR"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  C620 VALIDATION PARALLÈLE COMPLÈTE                        ║"
echo "║  Logs: $LOG_DIR                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Fonction pour exécuter une tâche en arrière-plan
run_task() {
    local task_name="$1"
    local task_cmd="$2"
    local log_file="$LOG_DIR/${task_name}.log"
    
    echo "[TASK] Lancement: $task_name"
    {
        echo "═══════════════════════════════════════════════════════════"
        echo "TASK: $task_name"
        echo "DATE: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "═══════════════════════════════════════════════════════════"
        echo ""
        eval "$task_cmd" 2>&1
        echo ""
        echo "═══════════════════════════════════════════════════════════"
        echo "TASK TERMINÉE: $task_name"
        echo "═══════════════════════════════════════════════════════════"
    } > "$log_file" 2>&1 &
    
    echo "$!" >> "$LOG_DIR/pids.txt"
}

# TASK 1: Vérifier BATCH_POOL_SIZE dans code actuel vs backup
run_task "TASK01_Check_BATCH_POOL_SIZE" \
    "grep -n 'BATCH_POOL_SIZE' src/btc_gen9_native_runner.c backup_pre_optim_20260515_235125/src/btc_gen9_native_runner.c"

# TASK 2: Analyser impact VM support
run_task "TASK02_Analyze_VM_Support" \
    "grep -n 'vm_pool\|VM' src/btc_gen9_native_runner.c | head -20"

# TASK 3: Lister tous les kernels disponibles
run_task "TASK03_List_Kernels" \
    "ls -lh kernels/*.bin kernels/*.cl 2>/dev/null | head -30"

# TASK 4: Vérifier signature kernel btc_sha256_gen9.cl
run_task "TASK04_Check_Kernel_Signature" \
    "sed -n '190,220p' kernels/btc_sha256_gen9.cl"

# TASK 5: Compiler test SHA-256 ajusté (sera fait après ajustement)
echo "[INFO] TASK05 sera exécutée après ajustement du test SHA-256"

# TASK 6: Vérifier différences structures Gen9
run_task "TASK06_Check_Gen9_Structures" \
    "ls -la src/btc_gen9_structures.h src/btc_gen9_surface_state.h 2>/dev/null || echo 'Fichiers non trouvés'"

# TASK 7: Analyser logs précédents hashrate
run_task "TASK07_Analyze_Previous_Hashrate" \
    "grep -i 'mh/s\|hashrate' logs/execution_c289_10s_20260520_231258.log 2>/dev/null | head -20 || echo 'Log non trouvé'"

# TASK 8: Vérifier version driver i915
run_task "TASK08_Check_i915_Driver" \
    "dmesg | grep -i 'i915\|drm' | tail -20"

echo ""
echo "[INFO] Toutes les tâches lancées en parallèle"
echo "[INFO] Attente de la fin des tâches..."
echo ""

# Attendre que toutes les tâches se terminent
wait

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  TOUTES LES TÂCHES TERMINÉES                               ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Afficher résumé
echo "[RÉSUMÉ] Logs générés:"
ls -lh "$LOG_DIR"/*.log

echo ""
echo "[NEXT] Consultez les logs dans: $LOG_DIR"
echo "[NEXT] Ajustez test SHA-256 avec les informations collectées"

# Made with Bob
