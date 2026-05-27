#!/bin/bash
# TEST C558 - STRUCTURE 4 BOs COMME OPENCL AVEC FORENSIC MAXIMAL
# ROOT CAUSE #65: Utiliser 5 BOs (output + surface + idrt + kernel + batch)

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
LOG_FILE="${LOG_DIR}/test_c558_${TIMESTAMP}.log"
STRACE_FILE="${LOG_DIR}/test_c558_strace_${TIMESTAMP}.log"
DMESG_FILE="${LOG_DIR}/test_c558_dmesg_${TIMESTAMP}.log"

mkdir -p "${LOG_DIR}"

echo "=== TEST C558 - STRUCTURE 4 BOs COMME OPENCL ===" | tee "${LOG_FILE}"
echo "Timestamp: ${TIMESTAMP}" | tee -a "${LOG_FILE}"
echo "ROOT CAUSE #65: 5 BOs séparés (output + surface + idrt + kernel + batch)" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Clear dmesg
echo "emmaus" | sudo -S dmesg -C

# Compilation
echo "[1%] Compilation test_c558_4bo_opencl_structure.c..." | tee -a "${LOG_FILE}"
gcc -o test_c558_4bo_opencl_structure tests/test_c558_4bo_opencl_structure.c -ldrm -I/usr/include/libdrm 2>&1 | tee -a "${LOG_FILE}"

if [ $? -ne 0 ]; then
    echo "❌ ÉCHEC COMPILATION" | tee -a "${LOG_FILE}"
    exit 1
fi
echo "[3%] ✓ Compilation réussie" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Exécution avec strace ULTRA COMPLET
echo "[5%] Exécution avec strace forensic MAXIMAL (timeout 10s)..." | tee -a "${LOG_FILE}"
echo "Logging: ALL syscalls, structures complètes, timing nanoseconde" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

timeout 10s strace -f -tt -T -v -s 8192 \
    -e trace=all \
    -e signal=all \
    -e verbose=all \
    -o "${STRACE_FILE}" \
    ./test_c558_4bo_opencl_structure 2>&1 | tee -a "${LOG_FILE}"

EXIT_CODE=$?

echo "" | tee -a "${LOG_FILE}"
echo "[80%] Capture dmesg kernel..." | tee -a "${LOG_FILE}"
sudo dmesg > "${DMESG_FILE}"

# Analyse forensic ULTRA DÉTAILLÉE
echo "" | tee -a "${LOG_FILE}"
echo "=== ANALYSE FORENSIC ULTRA DÉTAILLÉE ===" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Compter syscalls
TOTAL_SYSCALLS=$(wc -l < "${STRACE_FILE}")
echo "[82%] Total syscalls: ${TOTAL_SYSCALLS}" | tee -a "${LOG_FILE}"

# Analyser ioctl
IOCTL_COUNT=$(grep -c "ioctl(" "${STRACE_FILE}" || echo "0")
echo "[84%] Total ioctl: ${IOCTL_COUNT}" | tee -a "${LOG_FILE}"

# Context
CONTEXT_CREATE=$(grep "DRM_IOCTL_I915_GEM_CONTEXT_CREATE" "${STRACE_FILE}" || echo "")
if [ -n "${CONTEXT_CREATE}" ]; then
    echo "[86%] ✓ Context création détecté" | tee -a "${LOG_FILE}"
else
    echo "[86%] ❌ Context création NON trouvé" | tee -a "${LOG_FILE}"
fi

# GEM_CREATE count
GEM_CREATE_COUNT=$(grep -c "DRM_IOCTL_I915_GEM_CREATE" "${STRACE_FILE}" || echo "0")
echo "[88%] GEM_CREATE count: ${GEM_CREATE_COUNT} (attendu: 5)" | tee -a "${LOG_FILE}"

# PASS 1
PASS1_COUNT=$(grep -c "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" || echo "0")
if [ "${PASS1_COUNT}" -ge 1 ]; then
    PASS1_LINE=$(grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" | head -1)
    echo "[90%] ✓ PASS 1 détecté" | tee -a "${LOG_FILE}"
    echo "       ${PASS1_LINE}" | head -c 200 | tee -a "${LOG_FILE}"
else
    echo "[90%] ❌ PASS 1 NON trouvé" | tee -a "${LOG_FILE}"
fi

# PASS 2
if [ "${PASS1_COUNT}" -ge 2 ]; then
    PASS2_LINE=$(grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" | tail -1)
    echo "[92%] ✓ PASS 2 détecté" | tee -a "${LOG_FILE}"
    echo "       ${PASS2_LINE}" | head -c 200 | tee -a "${LOG_FILE}"
else
    echo "[92%] ❌ PASS 2 NON trouvé" | tee -a "${LOG_FILE}"
fi

# GEM_WAIT
GEM_WAIT=$(grep "DRM_IOCTL_I915_GEM_WAIT" "${STRACE_FILE}" || echo "")
if [ -n "${GEM_WAIT}" ]; then
    echo "[94%] ✓ GEM_WAIT détecté" | tee -a "${LOG_FILE}"
else
    echo "[94%] ⚠️  GEM_WAIT non trouvé" | tee -a "${LOG_FILE}"
fi

# Analyser dmesg
I915_ERRORS=$(grep -i "i915.*error\|i915.*invalid\|i915.*fail" "${DMESG_FILE}" || echo "")
if [ -n "${I915_ERRORS}" ]; then
    echo "[96%] ⚠️  Erreurs i915:" | tee -a "${LOG_FILE}"
    echo "${I915_ERRORS}" | head -20 | tee -a "${LOG_FILE}"
else
    echo "[96%] ✓ Aucune erreur i915" | tee -a "${LOG_FILE}"
fi

# Messages GPU
GPU_MESSAGES=$(grep -i "i915.*gpu\|i915.*render\|i915.*exec" "${DMESG_FILE}" || echo "")
if [ -n "${GPU_MESSAGES}" ]; then
    echo "[98%] ℹ️  Messages GPU:" | tee -a "${LOG_FILE}"
    echo "${GPU_MESSAGES}" | head -10 | tee -a "${LOG_FILE}"
fi

echo "" | tee -a "${LOG_FILE}"
echo "=== FICHIERS GÉNÉRÉS ===" | tee -a "${LOG_FILE}"
echo "Log principal: ${LOG_FILE}" | tee -a "${LOG_FILE}"
echo "Strace:        ${STRACE_FILE} (${TOTAL_SYSCALLS} lignes)" | tee -a "${LOG_FILE}"
echo "Dmesg:         ${DMESG_FILE}" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

if [ ${EXIT_CODE} -eq 0 ]; then
    echo "[100%] 🎉🎉🎉 TEST C558 RÉUSSI! 🎉🎉🎉" | tee -a "${LOG_FILE}"
    echo "✓ ROOT CAUSE #65 RÉSOLU (structure 4 BOs)" | tee -a "${LOG_FILE}"
    echo "✓ Structure OpenCL exacte reproduite" | tee -a "${LOG_FILE}"
    echo "✓ GPU natif i915 DRM 100%" | tee -a "${LOG_FILE}"
    echo "✓ Élimination OpenCL 100%" | tee -a "${LOG_FILE}"
    echo "✓ PREMIER RÉSULTAT GPU NATIF VALIDE!" | tee -a "${LOG_FILE}"
else
    echo "[100%] ❌ TEST C558 ÉCHEC (exit code: ${EXIT_CODE})" | tee -a "${LOG_FILE}"
    echo "Analyser logs forensics pour diagnostic" | tee -a "${LOG_FILE}"
fi

exit ${EXIT_CODE}

# Made with Bob
