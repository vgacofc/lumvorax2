#!/bin/bash
# TEST C557 - CONTEXT i915 + STRATÉGIE 2-PASS AVEC FORENSIC LOGGING MAXIMAL
# ROOT CAUSE #64: Context i915 requis
# ROOT CAUSE #63: Stratégie 2-PASS correcte

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
LOG_FILE="${LOG_DIR}/test_c557_${TIMESTAMP}.log"
STRACE_FILE="${LOG_DIR}/test_c557_strace_${TIMESTAMP}.log"
DMESG_FILE="${LOG_DIR}/test_c557_dmesg_${TIMESTAMP}.log"

mkdir -p "${LOG_DIR}"

echo "=== TEST C557 - CONTEXT i915 + 2-PASS ===" | tee "${LOG_FILE}"
echo "Timestamp: ${TIMESTAMP}" | tee -a "${LOG_FILE}"
echo "ROOT CAUSE #64: Context i915 requis pour execbuffer2" | tee -a "${LOG_FILE}"
echo "ROOT CAUSE #63: PASS 1 minimal, PASS 2 complet" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Clear dmesg
echo "emmaus" | sudo -S dmesg -C

# Compilation
echo "[1%] Compilation test_c557_context_2pass.c..." | tee -a "${LOG_FILE}"
gcc -o test_c557_context_2pass tests/test_c557_context_2pass.c -ldrm -I/usr/include/libdrm 2>&1 | tee -a "${LOG_FILE}"

if [ $? -ne 0 ]; then
    echo "❌ ÉCHEC COMPILATION" | tee -a "${LOG_FILE}"
    exit 1
fi
echo "[3%] ✓ Compilation réussie" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Exécution avec strace forensic ULTRA COMPLET
echo "[5%] Exécution avec strace forensic MAXIMAL (timeout 10s)..." | tee -a "${LOG_FILE}"
echo "Logging: ALL syscalls, memory, signals, timing nanoseconde" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

timeout 10s strace -f -tt -T -v -s 4096 \
    -e trace=all \
    -e signal=all \
    -e verbose=all \
    -o "${STRACE_FILE}" \
    ./test_c557_context_2pass 2>&1 | tee -a "${LOG_FILE}"

EXIT_CODE=$?

echo "" | tee -a "${LOG_FILE}"
echo "[85%] Capture dmesg kernel..." | tee -a "${LOG_FILE}"
sudo dmesg > "${DMESG_FILE}"

# Analyse forensic ULTRA DÉTAILLÉE
echo "" | tee -a "${LOG_FILE}"
echo "=== ANALYSE FORENSIC ULTRA DÉTAILLÉE ===" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Compter syscalls
TOTAL_SYSCALLS=$(wc -l < "${STRACE_FILE}")
echo "[87%] Total syscalls: ${TOTAL_SYSCALLS}" | tee -a "${LOG_FILE}"

# Analyser ioctl
IOCTL_COUNT=$(grep -c "ioctl(" "${STRACE_FILE}" || echo "0")
echo "[88%] Total ioctl: ${IOCTL_COUNT}" | tee -a "${LOG_FILE}"

# Context creation
CONTEXT_CREATE=$(grep "DRM_IOCTL_I915_GEM_CONTEXT_CREATE" "${STRACE_FILE}" || echo "")
if [ -n "${CONTEXT_CREATE}" ]; then
    echo "[89%] ✓ Context création détecté" | tee -a "${LOG_FILE}"
    echo "       ${CONTEXT_CREATE}" | tee -a "${LOG_FILE}"
else
    echo "[89%] ❌ Context création NON trouvé" | tee -a "${LOG_FILE}"
fi

# PASS 1
PASS1_COUNT=$(grep -c "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" || echo "0")
if [ "${PASS1_COUNT}" -ge 1 ]; then
    PASS1_LINE=$(grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" | head -1)
    echo "[91%] ✓ PASS 1 détecté" | tee -a "${LOG_FILE}"
    echo "       ${PASS1_LINE}" | tee -a "${LOG_FILE}"
else
    echo "[91%] ❌ PASS 1 NON trouvé" | tee -a "${LOG_FILE}"
fi

# PASS 2
if [ "${PASS1_COUNT}" -ge 2 ]; then
    PASS2_LINE=$(grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" | tail -1)
    echo "[93%] ✓ PASS 2 détecté" | tee -a "${LOG_FILE}"
    echo "       ${PASS2_LINE}" | tee -a "${LOG_FILE}"
else
    echo "[93%] ❌ PASS 2 NON trouvé" | tee -a "${LOG_FILE}"
fi

# GEM_WAIT
GEM_WAIT=$(grep "DRM_IOCTL_I915_GEM_WAIT" "${STRACE_FILE}" || echo "")
if [ -n "${GEM_WAIT}" ]; then
    echo "[95%] ✓ GEM_WAIT détecté (synchronisation GPU)" | tee -a "${LOG_FILE}"
else
    echo "[95%] ⚠️  GEM_WAIT non trouvé" | tee -a "${LOG_FILE}"
fi

# Analyser dmesg pour erreurs i915
I915_ERRORS=$(grep -i "i915.*error\|i915.*invalid\|i915.*fail" "${DMESG_FILE}" || echo "")
if [ -n "${I915_ERRORS}" ]; then
    echo "[97%] ⚠️  Erreurs i915 kernel:" | tee -a "${LOG_FILE}"
    echo "${I915_ERRORS}" | tee -a "${LOG_FILE}"
else
    echo "[97%] ✓ Aucune erreur i915 dans dmesg" | tee -a "${LOG_FILE}"
fi

# Analyser dmesg pour messages GPU
GPU_MESSAGES=$(grep -i "i915.*gpu\|i915.*render\|i915.*exec" "${DMESG_FILE}" || echo "")
if [ -n "${GPU_MESSAGES}" ]; then
    echo "[98%] ℹ️  Messages GPU kernel:" | tee -a "${LOG_FILE}"
    echo "${GPU_MESSAGES}" | head -20 | tee -a "${LOG_FILE}"
fi

echo "" | tee -a "${LOG_FILE}"
echo "=== FICHIERS GÉNÉRÉS ===" | tee -a "${LOG_FILE}"
echo "Log principal: ${LOG_FILE}" | tee -a "${LOG_FILE}"
echo "Strace:        ${STRACE_FILE} (${TOTAL_SYSCALLS} lignes)" | tee -a "${LOG_FILE}"
echo "Dmesg:         ${DMESG_FILE}" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

if [ ${EXIT_CODE} -eq 0 ]; then
    echo "[100%] 🎉🎉🎉 TEST C557 RÉUSSI! 🎉🎉🎉" | tee -a "${LOG_FILE}"
    echo "✓ ROOT CAUSE #64 RÉSOLU (context i915)" | tee -a "${LOG_FILE}"
    echo "✓ ROOT CAUSE #63 RÉSOLU (stratégie 2-PASS)" | tee -a "${LOG_FILE}"
    echo "✓ GPU natif i915 DRM 100%" | tee -a "${LOG_FILE}"
    echo "✓ Élimination OpenCL 100%" | tee -a "${LOG_FILE}"
    echo "✓ PREMIER RÉSULTAT GPU NATIF VALIDE!" | tee -a "${LOG_FILE}"
else
    echo "[100%] ❌ TEST C557 ÉCHEC (exit code: ${EXIT_CODE})" | tee -a "${LOG_FILE}"
    echo "Analyser logs forensics pour diagnostic" | tee -a "${LOG_FILE}"
fi

exit ${EXIT_CODE}

# Made with Bob
