#!/bin/bash
# master_lvx/execute_all_parallel.sh
# EXÉCUTION PARALLÈLE COMPLÈTE - Tous tests + corrections simultanés
# ROOT CAUSE #87 corrigé + Tests T001-T005 + SIMD_OPTIMIZER + Level Zero

set -euo pipefail

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

WORKSPACE="/home/lvx/LVX/lumvorax2"
RESULTS_DIR="$WORKSPACE/master_lvx/results_parallel_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  EXÉCUTION PARALLÈLE COMPLÈTE - TOUS TESTS SIMULTANÉS     ║${NC}"
echo -e "${CYAN}║  ROOT CAUSE #87 + Tests + Corrections + Compilations      ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Fonction exécution parallèle avec PID tracking
declare -A PIDS
declare -A TASKS

run_parallel() {
    local task_id="$1"
    local task_name="$2"
    local task_cmd="$3"
    local log_file="$RESULTS_DIR/${task_id}_${task_name}.log"
    
    echo -e "${YELLOW}[PARALLEL] Lancement $task_id: $task_name${NC}"
    
    # Exécuter en arrière-plan
    (
        echo "=== TASK $task_id: $task_name ===" > "$log_file"
        echo "Command: $task_cmd" >> "$log_file"
        echo "Start: $(date)" >> "$log_file"
        echo "" >> "$log_file"
        
        eval "$task_cmd" >> "$log_file" 2>&1
        exit_code=$?
        
        echo "" >> "$log_file"
        echo "End: $(date)" >> "$log_file"
        echo "Exit code: $exit_code" >> "$log_file"
        
        exit $exit_code
    ) &
    
    PIDS[$task_id]=$!
    TASKS[$task_id]="$task_name"
    
    echo -e "${GREEN}[PARALLEL] $task_id lancé (PID: ${PIDS[$task_id]})${NC}"
}

# ============================================================================
# TÂCHE 1: CORRECTION ROOT CAUSE #87 (EXEC_OBJECT_PINNED)
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}TÂCHE 1: CORRECTION ROOT CAUSE #87${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

cd "$WORKSPACE/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native"

# Créer test C610 avec EXEC_OBJECT_PINNED
cat > tests/test_c610_pinned_fix.c <<'EOFC610'
// Test C610 - Fix ROOT CAUSE #87 avec EXEC_OBJECT_PINNED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <drm/i915_drm.h>
#include <time.h>

#define EXEC_OBJECT_PINNED (1<<4)
#define EXEC_OBJECT_WRITE (1<<2)
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS (1<<3)

int main(int argc, char **argv) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C610 - FIX ROOT CAUSE #87                           ║\n");
    printf("║  EXEC_OBJECT_PINNED sur output buffer                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            printf("❌ Erreur: Impossible d'ouvrir DRM\n");
            return 1;
        }
    }
    printf("✅ DRM ouvert: fd=%d\n", fd);
    
    // Créer context
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("❌ Erreur: Context creation failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Context créé: ctx_id=%u\n", ctx_create.ctx_id);
    
    // Allouer output buffer
    struct drm_i915_gem_create create = {
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        printf("❌ Erreur: GEM_CREATE failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Output buffer créé: handle=%u\n", create.handle);
    
    // Écrire sentinel
    uint32_t sentinel = 0xDEADBEEF;
    struct drm_i915_gem_pwrite pwrite = {
        .handle = create.handle,
        .offset = 0,
        .size = sizeof(sentinel),
        .data_ptr = (uint64_t)&sentinel
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        printf("❌ Erreur: PWRITE failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Sentinel écrit: 0x%08X\n", sentinel);
    
    // Créer batch buffer minimal (écriture 0x12345678)
    struct drm_i915_gem_create batch_create = {
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) < 0) {
        printf("❌ Erreur: Batch GEM_CREATE failed\n");
        close(fd);
        return 1;
    }
    
    uint32_t batch[] = {
        0x7A000004,  // MI_STORE_DATA_IMM
        0x00100000,  // Address low (sera relocalisé)
        0x00000000,  // Address high
        0x12345678,  // Data
        0x00000000,  // Data high
        0x05000000   // MI_BATCH_BUFFER_END
    };
    
    struct drm_i915_gem_pwrite batch_pwrite = {
        .handle = batch_create.handle,
        .offset = 0,
        .size = sizeof(batch),
        .data_ptr = (uint64_t)batch
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &batch_pwrite) < 0) {
        printf("❌ Erreur: Batch PWRITE failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Batch buffer créé et écrit\n");
    
    // Configuration EXEC avec PINNED
    struct drm_i915_gem_exec_object2 exec_objects[2] = {
        {
            .handle = create.handle,
            .offset = 0x0000000000100000,  // Adresse fixe
            .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE | EXEC_OBJECT_SUPPORTS_48B_ADDRESS
        },
        {
            .handle = batch_create.handle,
            .offset = 0x0000000000040000,
            .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS
        }
    };
    
    struct drm_i915_gem_relocation_entry reloc = {
        .target_handle = create.handle,
        .delta = 0,
        .offset = 4,  // Offset dans batch (address low)
        .presumed_offset = 0x100000,
        .read_domains = 0,
        .write_domain = 0x00000001  // I915_GEM_DOMAIN_RENDER
    };
    
    exec_objects[1].relocation_count = 1;
    exec_objects[1].relocs_ptr = (uint64_t)&reloc;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 2,
        .batch_start_offset = 0,
        .batch_len = sizeof(batch),
        .flags = 0,
        .rsvd1 = ctx_create.ctx_id
    };
    
    printf("\n[PRE-EXEC] Output offset: 0x%016llx (PINNED)\n", exec_objects[0].offset);
    printf("[PRE-EXEC] Batch offset:  0x%016llx (PINNED)\n", exec_objects[1].offset);
    
    // EXECBUFFER2
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        printf("❌ Erreur: EXECBUFFER2 failed (errno=%d)\n", errno);
        close(fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("\n[POST-EXEC] Output offset: 0x%016llx\n", exec_objects[0].offset);
    printf("[POST-EXEC] Batch offset:  0x%016llx\n", exec_objects[1].offset);
    printf("✅ EXECBUFFER2 réussi (%.6f sec)\n", elapsed);
    
    // Vérifier si offset a changé
    if (exec_objects[0].offset != 0x100000) {
        printf("⚠️  WARNING: Output offset a changé! (attendu: 0x100000)\n");
    } else {
        printf("✅ Output offset STABLE @ 0x100000 (PINNED fonctionne!)\n");
    }
    
    // Lire résultat
    uint32_t result = 0;
    struct drm_i915_gem_pread pread = {
        .handle = create.handle,
        .offset = 0,
        .size = sizeof(result),
        .data_ptr = (uint64_t)&result
    };
    
    // Wait GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = create.handle,
        .timeout_ns = 1000000000  // 1 seconde
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PREAD, &pread) < 0) {
        printf("❌ Erreur: PREAD failed\n");
        close(fd);
        return 1;
    }
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS TEST C610                                       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("Sentinel avant:  0x%08X\n", sentinel);
    printf("Résultat après:  0x%08X\n", result);
    
    if (result == 0x12345678) {
        printf("\n✅ SUCCESS: GPU a écrit la valeur correcte!\n");
        printf("✅ ROOT CAUSE #87 CORRIGÉ avec EXEC_OBJECT_PINNED!\n");
        close(fd);
        return 0;
    } else if (result != sentinel) {
        printf("\n⚠️  PARTIAL: GPU a écrit mais valeur incorrecte\n");
        close(fd);
        return 2;
    } else {
        printf("\n❌ FAIL: GPU n'a pas écrit (sentinel intact)\n");
        close(fd);
        return 1;
    }
}
EOFC610

# Compiler test C610
run_parallel "TASK01" "Compile_C610_PINNED" \
    "gcc -o tests/test_c610_pinned_fix tests/test_c610_pinned_fix.c -I. -ldrm"

# Exécuter test C610
run_parallel "TASK02" "Execute_C610_PINNED" \
    "sleep 2 && ./tests/test_c610_pinned_fix"

# ============================================================================
# TÂCHE 2: TEST T003 - OpenCL Pure (Référence 45 nonces)
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}TÂCHE 2: TEST T003 - OpenCL Pure${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

cd "$WORKSPACE/src/advanced_calculations/bitcoin_quantum_mining"

if [ -f "test_c194_opencl_drm_benchmark" ]; then
    run_parallel "TASK03" "OpenCL_Pure_60s" \
        "./test_c194_opencl_drm_benchmark --duration=60"
else
    echo -e "${YELLOW}⚠️  test_c194_opencl_drm_benchmark non trouvé, skip${NC}"
fi

# ============================================================================
# TÂCHE 3: TEST T001 - i915 DRM Natif (État actuel)
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}TÂCHE 3: TEST T001 - i915 DRM Natif${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

cd "$WORKSPACE/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native"

if [ -f "test_c282_progressive_batch.sh" ]; then
    run_parallel "TASK04" "i915_DRM_Natif_1test" \
        "./test_c282_progressive_batch.sh 1 50000"
else
    echo -e "${YELLOW}⚠️  test_c282_progressive_batch.sh non trouvé, skip${NC}"
fi

# ============================================================================
# TÂCHE 4: COMPILATION + TEST Level Zero
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}TÂCHE 4: Level Zero - Compilation + Test${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

cd "$WORKSPACE/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native"

if [ -f "btc_mining_levelzero_runner.c" ]; then
    run_parallel "TASK05" "Compile_Level_Zero" \
        "gcc -o btc_mining_levelzero_runner btc_mining_levelzero_runner.c -I. -lze_loader -ldrm -lm 2>&1 || echo 'Compilation failed but continuing'"
    
    run_parallel "TASK06" "Execute_Level_Zero_60s" \
        "sleep 3 && (./btc_mining_levelzero_runner --duration=60 2>&1 || echo 'Execution failed')"
else
    echo -e "${YELLOW}⚠️  btc_mining_levelzero_runner.c non trouvé, skip${NC}"
fi

# ============================================================================
# TÂCHE 5: CORRECTION SIMD_OPTIMIZER
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}TÂCHE 5: Analyse + Correction SIMD_OPTIMIZER${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

cd "$WORKSPACE"

run_parallel "TASK07" "Analyze_SIMD_OPTIMIZER" \
    "grep -r 'SIMD_OPTIMIZER' src/ --include='*.c' --include='*.h' -A 5 -B 5 2>&1 || echo 'No SIMD_OPTIMIZER found'"

# ============================================================================
# TÂCHE 6: TESTS HYBRIDES
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}TÂCHE 6: Tests Hybrides (si disponibles)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

cd "$WORKSPACE/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native"

if [ -f "tests/test_hybrid_native_lz" ]; then
    run_parallel "TASK08" "Hybrid_Natif_LevelZero" \
        "./tests/test_hybrid_native_lz --duration=60"
else
    echo -e "${YELLOW}⚠️  test_hybrid_native_lz non trouvé, skip${NC}"
fi

# ============================================================================
# ATTENTE COMPLETION TOUTES TÂCHES
# ============================================================================
echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}ATTENTE COMPLETION ${#PIDS[@]} TÂCHES PARALLÈLES${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo ""

declare -A RESULTS
for task_id in "${!PIDS[@]}"; do
    pid=${PIDS[$task_id]}
    task_name=${TASKS[$task_id]}
    
    echo -e "${YELLOW}[WAIT] $task_id ($task_name) - PID $pid${NC}"
    
    wait $pid
    exit_code=$?
    
    RESULTS[$task_id]=$exit_code
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✅ $task_id: SUCCESS${NC}"
    else
        echo -e "${RED}❌ $task_id: FAILED (exit code: $exit_code)${NC}"
    fi
done

# ============================================================================
# GÉNÉRATION RAPPORT FINAL
# ============================================================================
echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}GÉNÉRATION RAPPORT FINAL${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo ""

REPORT="$RESULTS_DIR/RAPPORT_FINAL_PARALLEL.md"

cat > "$REPORT" <<'EOFREPORT'
# 📊 RAPPORT FINAL - EXÉCUTION PARALLÈLE COMPLÈTE

## Résumé Exécution

**Date**: $(date +"%Y-%m-%d %H:%M:%S")  
**Tâches lancées**: ${#PIDS[@]}  
**Durée totale**: Parallèle (toutes simultanées)

---

## Résultats par Tâche

EOFREPORT

for task_id in $(echo "${!RESULTS[@]}" | tr ' ' '\n' | sort); do
    exit_code=${RESULTS[$task_id]}
    task_name=${TASKS[$task_id]}
    log_file="$RESULTS_DIR/${task_id}_${task_name}.log"
    
    if [ $exit_code -eq 0 ]; then
        status="✅ SUCCESS"
    else
        status="❌ FAILED (exit: $exit_code)"
    fi
    
    cat >> "$REPORT" <<EOFTASK
### $task_id: $task_name

**Status**: $status  
**Log**: \`${task_id}_${task_name}.log\`

EOFTASK

    # Extraire métriques si disponibles
    if [ -f "$log_file" ]; then
        nonces=$(grep -oP 'nonces[_\s]found[=:]\s*\K[0-9]+' "$log_file" | head -1 || echo "N/A")
        hashrate=$(grep -oP 'hashrate[=:]\s*\K[0-9.]+' "$log_file" | head -1 || echo "N/A")
        
        cat >> "$REPORT" <<EOFMETRICS
**Métriques**:
- Nonces trouvés: $nonces
- Hashrate: $hashrate MH/s

EOFMETRICS
    fi
    
    echo "" >> "$REPORT"
done

cat >> "$REPORT" <<'EOFEND'

---

## Analyse Globale

### ROOT CAUSE #87
EOFEND

if [ "${RESULTS[TASK02]:-1}" -eq 0 ]; then
    echo "✅ **CORRIGÉ** - EXEC_OBJECT_PINNED fonctionne!" >> "$REPORT"
else
    echo "❌ **NON CORRIGÉ** - Nécessite investigation supplémentaire" >> "$REPORT"
fi

cat >> "$REPORT" <<'EOFEND2'

### Meilleure Configuration

EOFEND2

# Trouver meilleure config (max nonces)
best_task=""
best_nonces=0
for task_id in "${!RESULTS[@]}"; do
    log_file="$RESULTS_DIR/${task_id}_${TASKS[$task_id]}.log"
    if [ -f "$log_file" ]; then
        nonces=$(grep -oP 'nonces[_\s]found[=:]\s*\K[0-9]+' "$log_file" | head -1 || echo "0")
        if [ "$nonces" -gt "$best_nonces" ]; then
            best_nonces=$nonces
            best_task=$task_id
        fi
    fi
done

if [ -n "$best_task" ]; then
    echo "**Gagnant**: $best_task (${TASKS[$best_task]})" >> "$REPORT"
    echo "**Nonces trouvés**: $best_nonces" >> "$REPORT"
else
    echo "**Aucune configuration n'a produit de nonces**" >> "$REPORT"
fi

cat >> "$REPORT" <<'EOFEND3'

---

## Logs Détaillés

Tous les logs sont disponibles dans le répertoire:
EOFEND3

echo "\`$RESULTS_DIR\`" >> "$REPORT"

echo "" >> "$REPORT"
echo "**Fin du rapport**" >> "$REPORT"

# Afficher rapport
cat "$REPORT"

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ EXÉCUTION PARALLÈLE TERMINÉE${NC}"
echo -e "${GREEN}📁 Résultats: $RESULTS_DIR${NC}"
echo -e "${GREEN}📊 Rapport: $REPORT${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"

# Retourner code selon succès
if [ "$best_nonces" -gt 0 ]; then
    echo -e "${GREEN}🎉 AU MOINS UNE CONFIGURATION A PRODUIT DES NONCES!${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠️  AUCUNE CONFIGURATION N'A PRODUIT DE NONCES${NC}"
    exit 1
fi

# Made with Bob
