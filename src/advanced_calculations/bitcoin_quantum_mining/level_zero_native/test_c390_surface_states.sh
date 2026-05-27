#!/bin/bash
# LumVorax C390 - Test Surface States Gen9 Complets
# 
# OBJECTIF: Valider implémentation Surface States + Binding Table
# ATTENDU: output[0] = 0xC390ABCD (au lieu de 0x00000000)

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  LUMVORAX C390 - TEST SURFACE STATES GEN9 COMPLETS"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Répertoires
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="$SCRIPT_DIR/kernels"
LOG_DIR="$SCRIPT_DIR/logs"
BIN_DIR="$SCRIPT_DIR/bin"

# Créer répertoires si nécessaire
mkdir -p "$LOG_DIR"
mkdir -p "$BIN_DIR"

# Fichiers
KERNEL_CL="$KERNEL_DIR/test_c390_surface_states.cl"
KERNEL_BIN="$KERNEL_DIR/test_c390_surface_states.bin"
LOG_FILE="$LOG_DIR/test_c390_$(date +%Y%m%d_%H%M%S).log"

echo "[C390] Étape 1: Compilation kernel OpenCL → Gen9 ISA"
echo "  Source: $KERNEL_CL"
echo "  Output: $KERNEL_BIN"
echo ""

# Compiler avec ocloc (Intel Offline Compiler)
if ! command -v ocloc &> /dev/null; then
    echo "❌ ERREUR: ocloc non trouvé"
    echo "   Installation: sudo apt install intel-ocloc"
    exit 1
fi

# Compiler pour Gen9 (Skylake/Kaby Lake)
ocloc compile -file "$KERNEL_CL" \
    -device kbl \
    -output "$KERNEL_BIN" \
    -options "-cl-std=CL2.0" \
    2>&1 | tee -a "$LOG_FILE"

if [ ! -f "$KERNEL_BIN" ]; then
    echo "❌ ERREUR: Compilation kernel échouée"
    exit 1
fi

echo "✅ Kernel compilé: $(stat -c%s "$KERNEL_BIN") bytes"
echo ""

# Désassembler avec IGA pour validation
echo "[C390] Étape 2: Désassemblage ISA avec IGA"
if command -v iga64 &> /dev/null; then
    iga64 -d "$KERNEL_BIN" > "${KERNEL_BIN}.asm" 2>&1 || true
    if [ -f "${KERNEL_BIN}.asm" ]; then
        echo "✅ ISA désassemblé: ${KERNEL_BIN}.asm"
        echo "  Premières lignes:"
        head -n 20 "${KERNEL_BIN}.asm" | sed 's/^/    /'
    fi
else
    echo "⚠️  IGA non disponible (optionnel)"
fi
echo ""

# Compiler le runner de test (si nécessaire)
echo "[C390] Étape 3: Compilation runner natif"
RUNNER_SRC="$SCRIPT_DIR/src/btc_gen9_native_runner.c"
RUNNER_BIN="$BIN_DIR/test_c390_runner"

if [ ! -f "$RUNNER_BIN" ] || [ "$RUNNER_SRC" -nt "$RUNNER_BIN" ]; then
    echo "  Compilation du runner..."
    gcc -o "$RUNNER_BIN" \
        "$RUNNER_SRC" \
        -I"$SCRIPT_DIR/src" \
        -I/usr/include/libdrm \
        -ldrm \
        -lpthread \
        -lm \
        -O2 \
        -Wall \
        -DTEST_MODE_C390 \
        2>&1 | tee -a "$LOG_FILE"
    
    if [ $? -ne 0 ]; then
        echo "❌ ERREUR: Compilation runner échouée"
        exit 1
    fi
    echo "✅ Runner compilé"
else
    echo "✅ Runner déjà compilé (à jour)"
fi
echo ""

# Exécuter le test
echo "[C390] Étape 4: Exécution test GPU"
echo "  Kernel: $KERNEL_BIN"
echo "  Log: $LOG_FILE"
echo "  Durée: 10 secondes"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo ""

# Lancer avec timeout 10s
timeout 10s "$RUNNER_BIN" \
    --kernel "$KERNEL_BIN" \
    --batch-size 256 \
    --log "$LOG_FILE" \
    2>&1 | tee -a "$LOG_FILE"

EXIT_CODE=$?

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "[C390] Étape 5: Analyse résultats"
echo ""

# Analyser les logs
if [ -f "$LOG_FILE" ]; then
    echo "📊 Métriques extraites:"
    echo ""
    
    # Chercher magic number
    MAGIC_FOUND=$(grep -c "0xC390ABCD" "$LOG_FILE" || true)
    if [ "$MAGIC_FOUND" -gt 0 ]; then
        echo "  ✅ Magic number 0xC390ABCD trouvé ($MAGIC_FOUND occurrences)"
    else
        echo "  ❌ Magic number 0xC390ABCD NON trouvé"
    fi
    
    # Chercher Surface States configurés
    SURFACE_OK=$(grep -c "C390_SUCCESS: Surface States initialized" "$LOG_FILE" || true)
    if [ "$SURFACE_OK" -gt 0 ]; then
        echo "  ✅ Surface States initialisés correctement"
    else
        echo "  ❌ Surface States NON initialisés"
    fi
    
    # Chercher Binding Table configuré
    BINDING_OK=$(grep -c "C390_SUCCESS: Binding Table validated" "$LOG_FILE" || true)
    if [ "$BINDING_OK" -gt 0 ]; then
        echo "  ✅ Binding Table validé"
    else
        echo "  ❌ Binding Table NON validé"
    fi
    
    # Chercher erreurs
    ERROR_COUNT=$(grep -c "ERROR\|FAILED" "$LOG_FILE" || true)
    if [ "$ERROR_COUNT" -gt 0 ]; then
        echo "  ⚠️  $ERROR_COUNT erreurs détectées"
        echo ""
        echo "  Dernières erreurs:"
        grep "ERROR\|FAILED" "$LOG_FILE" | tail -n 5 | sed 's/^/    /'
    else
        echo "  ✅ Aucune erreur détectée"
    fi
    
    echo ""
    echo "📁 Log complet: $LOG_FILE"
    echo "   Lignes: $(wc -l < "$LOG_FILE")"
fi

echo ""
echo "════════════════════════════════════════════════════════════════"

if [ $EXIT_CODE -eq 0 ] && [ "$MAGIC_FOUND" -gt 0 ]; then
    echo "✅ TEST C390 RÉUSSI - Surface States fonctionnent!"
    echo ""
    echo "RÉSULTAT: GPU a écrit 0xC390ABCD au lieu de 0x00000000"
    echo "CONCLUSION: Problème C288-C389 RÉSOLU"
    exit 0
else
    echo "❌ TEST C390 ÉCHOUÉ - Surface States ne fonctionnent pas encore"
    echo ""
    echo "PROCHAINES ACTIONS:"
    echo "  1. Analyser logs: $LOG_FILE"
    echo "  2. Vérifier Surface State DW8-9 (base address)"
    echo "  3. Vérifier Binding Table offsets"
    echo "  4. Comparer avec trace OpenCL"
    exit 1
fi

# Made with Bob
