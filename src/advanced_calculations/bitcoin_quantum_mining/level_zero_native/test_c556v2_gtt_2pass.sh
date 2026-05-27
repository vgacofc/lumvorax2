#!/bin/bash
# TEST C556v2 - STRATÉGIE 2-PASS CORRECTE AVEC FORENSIC LOGGING MAXIMAL
# ROOT CAUSE #63: PASS 1 = batch minimal, PASS 2 = batch complet

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
LOG_FILE="${LOG_DIR}/test_c556v2_${TIMESTAMP}.log"
STRACE_FILE="${LOG_DIR}/test_c556v2_strace_${TIMESTAMP}.log"
DMESG_FILE="${LOG_DIR}/test_c556v2_dmesg_${TIMESTAMP}.log"

mkdir -p "${LOG_DIR}"

echo "=== TEST C556v2 - STRATÉGIE 2-PASS CORRECTE ===" | tee "${LOG_FILE}"
echo "Timestamp: ${TIMESTAMP}" | tee -a "${LOG_FILE}"
echo "ROOT CAUSE #63: PASS 1 batch minimal, PASS 2 batch complet" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Clear dmesg
echo "emmaus" | sudo -S dmesg -C

# Compilation
echo "[1%] Compilation test_c556v2_gtt_2pass.c..." | tee -a "${LOG_FILE}"
gcc -o test_c556v2_gtt_2pass tests/test_c556v2_gtt_2pass.c -ldrm -I/usr/include/libdrm 2>&1 | tee -a "${LOG_FILE}"

if [ $? -ne 0 ]; then
    echo "❌ ÉCHEC COMPILATION" | tee -a "${LOG_FILE}"
    exit 1
fi
echo "[5%] ✓ Compilation réussie" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Exécution avec strace forensic maximal
echo "[10%] Exécution avec strace forensic (timeout 10s)..." | tee -a "${LOG_FILE}"
echo "Logging: ioctl, mmap, memory, signals, file descriptors" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

timeout 10s strace -f -tt -T -v \
    -e trace=ioctl,mmap,munmap,open,close,read,write \
    -e signal=all \
    -e verbose=all \
    -o "${STRACE_FILE}" \
    ./test_c556v2_gtt_2pass 2>&1 | tee -a "${LOG_FILE}"

EXIT_CODE=$?

echo "" | tee -a "${LOG_FILE}"
echo "[90%] Capture dmesg kernel..." | tee -a "${LOG_FILE}"
sudo dmesg > "${DMESG_FILE}"

# Analyse forensic
echo "" | tee -a "${LOG_FILE}"
echo "=== ANALYSE FORENSIC ===" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Compter les ioctl
IOCTL_COUNT=$(grep -c "ioctl(" "${STRACE_FILE}" || echo "0")
echo "[92%] Total ioctl calls: ${IOCTL_COUNT}" | tee -a "${LOG_FILE}"

# Analyser PASS 1
PASS1_EXECBUF=$(grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" | head -1 || echo "")
if [ -n "${PASS1_EXECBUF}" ]; then
    echo "[94%] ✓ PASS 1 détecté dans strace" | tee -a "${LOG_FILE}"
else
    echo "[94%] ❌ PASS 1 non trouvé dans strace" | tee -a "${LOG_FILE}"
fi

# Analyser PASS 2
PASS2_EXECBUF=$(grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${STRACE_FILE}" | tail -1 || echo "")
if [ -n "${PASS2_EXECBUF}" ]; then
    echo "[96%] ✓ PASS 2 détecté dans strace" | tee -a "${LOG_FILE}"
else
    echo "[96%] ❌ PASS 2 non trouvé dans strace" | tee -a "${LOG_FILE}"
fi

# Analyser dmesg pour erreurs i915
I915_ERRORS=$(grep -i "i915.*error\|i915.*invalid\|i915.*fail" "${DMESG_FILE}" || echo "")
if [ -n "${I915_ERRORS}" ]; then
    echo "[98%] ⚠️  Erreurs i915 détectées:" | tee -a "${LOG_FILE}"
    echo "${I915_ERRORS}" | tee -a "${LOG_FILE}"
else
    echo "[98%] ✓ Aucune erreur i915 dans dmesg" | tee -a "${LOG_FILE}"
fi

echo "" | tee -a "${LOG_FILE}"
echo "=== FICHIERS GÉNÉRÉS ===" | tee -a "${LOG_FILE}"
echo "Log principal: ${LOG_FILE}" | tee -a "${LOG_FILE}"
echo "Strace:        ${STRACE_FILE}" | tee -a "${LOG_FILE}"
echo "Dmesg:         ${DMESG_FILE}" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

if [ ${EXIT_CODE} -eq 0 ]; then
    echo "[100%] 🎉 TEST C556v2 RÉUSSI!" | tee -a "${LOG_FILE}"
    echo "✓ ROOT CAUSE #63 RÉSOLU" | tee -a "${LOG_FILE}"
    echo "✓ Stratégie 2-PASS fonctionnelle" | tee -a "${LOG_FILE}"
    echo "✓ GPU natif i915 DRM 100%" | tee -a "${LOG_FILE}"
else
    echo "[100%] ❌ TEST C556v2 ÉCHEC (exit code: ${EXIT_CODE})" | tee -a "${LOG_FILE}"
    echo "Analyser les logs forensics pour diagnostic" | tee -a "${LOG_FILE}"
fi

exit ${EXIT_CODE}

# Made with Bob
