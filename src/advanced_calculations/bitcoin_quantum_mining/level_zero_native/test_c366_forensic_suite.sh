#!/bin/bash
#
# TEST C366 - SUITE FORENSIQUE COMPLÈTE
# ======================================
# Compile et exécute les tests T1, T2, T11 avec instrumentation maximale
#
# TESTS:
#   T11 - Compute minimal (PRIORITÉ ABSOLUE)
#   T1  - Signature ISA unique
#   T2  - Cache kernel (2 kernels différents)
#
# OBJECTIF: Isoler définitivement la couche Gen9 défaillante

set -e  # Exit on error

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Répertoires
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/tests"
KERNEL_DIR="$SCRIPT_DIR/kernels"
LOG_DIR="$SCRIPT_DIR/logs/forensic"
BIN_DIR="$SCRIPT_DIR/bin"

# Créer répertoires si nécessaire
mkdir -p "$LOG_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$KERNEL_DIR/compiled"

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  TEST C366 - SUITE FORENSIQUE COMPLÈTE                       ║${NC}"
echo -e "${BLUE}║  Instrumentation: CRC32 + Timestamps + Hex Dumps             ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 1: COMPILATION RUNNER
# ═══════════════════════════════════════════════════════════════════

echo -e "${YELLOW}[1/6] Compilation du runner forensique...${NC}"

gcc -o "$BIN_DIR/test_c366_forensic_runner" \
    "$TEST_DIR/test_c366_forensic_runner.c" \
    -I"$SCRIPT_DIR/src" \
    -I/usr/include/libdrm \
    -ldrm \
    -O2 \
    -Wall \
    -Wextra

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Runner compilé: $BIN_DIR/test_c366_forensic_runner${NC}"
else
    echo -e "${RED}❌ Erreur compilation runner${NC}"
    exit 1
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 2: COMPILATION KERNELS OPENCL → GEN9 ISA
# ═══════════════════════════════════════════════════════════════════

echo -e "${YELLOW}[2/6] Compilation des kernels OpenCL → Gen9 ISA...${NC}"

# Fonction compilation kernel
compile_kernel() {
    local kernel_cl="$1"
    local kernel_name="$2"
    local kernel_bin="$KERNEL_DIR/compiled/${kernel_name}.bin"
    
    echo -e "  Compilation: ${kernel_name}..."
    
    # Compiler avec ocloc (Intel Offline Compiler)
    ocloc compile \
        -file "$kernel_cl" \
        -device skl \
        -output "$kernel_bin" \
        -output_no_suffix \
        2>&1 | grep -v "warning" || true
    
    if [ -f "$kernel_bin" ]; then
        local size=$(stat -c%s "$kernel_bin")
        echo -e "${GREEN}    ✅ $kernel_name: $size bytes${NC}"
        return 0
    else
        echo -e "${RED}    ❌ Échec compilation $kernel_name${NC}"
        return 1
    fi
}

# Compiler T11 (PRIORITÉ ABSOLUE)
if ! compile_kernel "$TEST_DIR/test_c366_t11_compute_minimal.cl" "t11_compute_minimal"; then
    echo -e "${RED}❌ ERREUR CRITIQUE: T11 non compilé${NC}"
    exit 1
fi

# Compiler T1
if ! compile_kernel "$TEST_DIR/test_c366_t1_isa_signature.cl" "t1_isa_signature"; then
    echo -e "${YELLOW}⚠️  T1 non compilé (non bloquant)${NC}"
fi

# Compiler T2
if ! compile_kernel "$TEST_DIR/test_c366_t2_kernel_cache.cl" "t2_kernel_cache"; then
    echo -e "${YELLOW}⚠️  T2 non compilé (non bloquant)${NC}"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 3: EXÉCUTION TEST T11 (PRIORITÉ ABSOLUE)
# ═══════════════════════════════════════════════════════════════════

echo -e "${YELLOW}[3/6] Exécution TEST T11 - Compute Minimal (PRIORITÉ ABSOLUE)...${NC}"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_T11="$LOG_DIR/test_c366_t11_${TIMESTAMP}.log"

if [ -f "$KERNEL_DIR/compiled/t11_compute_minimal.bin" ]; then
    echo -e "  Kernel: t11_compute_minimal.bin"
    echo -e "  Log: $LOG_T11"
    echo -e "  Résultat attendu: output[i] = 0xCAFE0000 + i"
    echo ""
    
    "$BIN_DIR/test_c366_forensic_runner" \
        "$KERNEL_DIR/compiled/t11_compute_minimal.bin" \
        "$LOG_T11"
    
    T11_RESULT=$?
    
    if [ $T11_RESULT -eq 0 ]; then
        echo -e "${GREEN}✅ TEST T11 RÉUSSI - Compute pipeline VALIDÉ${NC}"
    else
        echo -e "${RED}❌ TEST T11 ÉCHOUÉ - Analyse logs: $LOG_T11${NC}"
    fi
else
    echo -e "${RED}❌ Kernel T11 non trouvé${NC}"
    T11_RESULT=1
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 4: EXÉCUTION TEST T1 (Signature ISA)
# ═══════════════════════════════════════════════════════════════════

echo -e "${YELLOW}[4/6] Exécution TEST T1 - Signature ISA Unique...${NC}"

LOG_T1="$LOG_DIR/test_c366_t1_${TIMESTAMP}.log"

if [ -f "$KERNEL_DIR/compiled/t1_isa_signature.bin" ]; then
    echo -e "  Kernel: t1_isa_signature.bin"
    echo -e "  Log: $LOG_T1"
    echo -e "  Résultat attendu: signatures uniques (0x11111111, 0x22222222...)"
    echo ""
    
    "$BIN_DIR/test_c366_forensic_runner" \
        "$KERNEL_DIR/compiled/t1_isa_signature.bin" \
        "$LOG_T1"
    
    T1_RESULT=$?
    
    if [ $T1_RESULT -eq 0 ]; then
        echo -e "${GREEN}✅ TEST T1 RÉUSSI - ISA kernel VALIDÉ${NC}"
    else
        echo -e "${RED}❌ TEST T1 ÉCHOUÉ - Analyse logs: $LOG_T1${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  Kernel T1 non trouvé (skip)${NC}"
    T1_RESULT=2
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 5: EXÉCUTION TEST T2 (Cache Kernel)
# ═══════════════════════════════════════════════════════════════════

echo -e "${YELLOW}[5/6] Exécution TEST T2 - Cache Kernel...${NC}"

LOG_T2="$LOG_DIR/test_c366_t2_${TIMESTAMP}.log"

if [ -f "$KERNEL_DIR/compiled/t2_kernel_cache.bin" ]; then
    echo -e "  Kernel: t2_kernel_cache.bin"
    echo -e "  Log: $LOG_T2"
    echo -e "  Résultat attendu: kernel A=0xAAAA****, kernel B=0xBBBB****"
    echo ""
    
    "$BIN_DIR/test_c366_forensic_runner" \
        "$KERNEL_DIR/compiled/t2_kernel_cache.bin" \
        "$LOG_T2"
    
    T2_RESULT=$?
    
    if [ $T2_RESULT -eq 0 ]; then
        echo -e "${GREEN}✅ TEST T2 RÉUSSI - Cache ISA VALIDÉ${NC}"
    else
        echo -e "${RED}❌ TEST T2 ÉCHOUÉ - Analyse logs: $LOG_T2${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  Kernel T2 non trouvé (skip)${NC}"
    T2_RESULT=2
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 6: RAPPORT FINAL
# ═══════════════════════════════════════════════════════════════════

echo -e "${YELLOW}[6/6] Génération rapport final...${NC}"

REPORT_FILE="$LOG_DIR/test_c366_report_${TIMESTAMP}.md"

cat > "$REPORT_FILE" << EOF
# RAPPORT TEST C366 - SUITE FORENSIQUE COMPLÈTE
## Timestamp: $(date '+%Y-%m-%d %H:%M:%S')

## RÉSUMÉ EXÉCUTION

| Test | Statut | Log |
|------|--------|-----|
| T11 - Compute Minimal | $([ $T11_RESULT -eq 0 ] && echo "✅ RÉUSSI" || echo "❌ ÉCHOUÉ") | $LOG_T11 |
| T1 - Signature ISA | $([ $T1_RESULT -eq 0 ] && echo "✅ RÉUSSI" || [ $T1_RESULT -eq 2 ] && echo "⚠️ SKIP" || echo "❌ ÉCHOUÉ") | $LOG_T1 |
| T2 - Cache Kernel | $([ $T2_RESULT -eq 0 ] && echo "✅ RÉUSSI" || [ $T2_RESULT -eq 2 ] && echo "⚠️ SKIP" || echo "❌ ÉCHOUÉ") | $LOG_T2 |

## DIAGNOSTIC DÉFINITIF

EOF

# Diagnostic basé sur résultats
if [ $T11_RESULT -eq 0 ]; then
    cat >> "$REPORT_FILE" << EOF
### ✅ COMPUTE PIPELINE VALIDÉ

Le test T11 a réussi, ce qui signifie que **TOUTE** la pile compute Gen9 fonctionne:
- ✅ ISA kernel execution
- ✅ Thread payload (GID)
- ✅ Binding table (BTI 0)
- ✅ Memory writeback

**CONCLUSION**: Le problème C364 (output=0) n'est PAS dans le pipeline compute Gen9.
Le problème est ailleurs (kernel minage production, NX48/NX49, ou autre).

### PROCHAINES ACTIONS

1. Valider kernel minage production avec ce même runner
2. Activer NX48/NX49 après validation
3. Tester avec vrai algorithme SHA256 Bitcoin
EOF
else
    cat >> "$REPORT_FILE" << EOF
### ❌ COMPUTE PIPELINE DÉFAILLANT

Le test T11 a échoué. Analyse des logs pour identifier la couche défaillante:

**Patterns à chercher dans $LOG_T11:**

1. **Si output[0-63] = 0x00000000**
   → Kernel non exécuté (ISA problem)
   → Vérifier: KERNEL_CRC32, BATCH_CRC32, EXECBUFFER2_SUCCESS

2. **Si output[0-63] = même valeur**
   → Payload cassé (GID=0 partout)
   → Vérifier: GPGPU_WALKER, Thread Group dimensions

3. **Si output[0-63] = valeurs incohérentes**
   → Thread dispatch cassé
   → Vérifier: MEDIA_VFE_STATE, Scoreboard

4. **Si output[0-63] = 0xDEADBEEF (inchangé)**
   → GPU n'a pas écrit (surface states NULL)
   → Vérifier: SSH_SURFACE[0], relocations

### PROCHAINES ACTIONS

1. Analyser logs T11 ligne par ligne
2. Identifier couche Gen9 défaillante
3. Corriger problème identifié
4. Re-tester jusqu'à succès
EOF
fi

cat >> "$REPORT_FILE" << EOF

## LOGS FORENSIQUES

Tous les logs contiennent:
- ✅ Checksums CRC32 (kernel, batch, SSH, output)
- ✅ Timestamps nanoseconde (chaque étape)
- ✅ Hex dumps complets (tous les buffers)
- ✅ Validation bit-level (chaque dword)

## FICHIERS GÉNÉRÉS

- Runner: $BIN_DIR/test_c366_forensic_runner
- Kernels: $KERNEL_DIR/compiled/*.bin
- Logs: $LOG_DIR/test_c366_*_${TIMESTAMP}.log
- Rapport: $REPORT_FILE

---
*Généré automatiquement par test_c366_forensic_suite.sh*
EOF

echo -e "${GREEN}✅ Rapport généré: $REPORT_FILE${NC}"
echo ""

# ═══════════════════════════════════════════════════════════════════
# RÉSUMÉ FINAL
# ═══════════════════════════════════════════════════════════════════

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  RÉSUMÉ FINAL                                                 ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "  T11 (Compute Minimal): $([ $T11_RESULT -eq 0 ] && echo -e "${GREEN}✅ RÉUSSI${NC}" || echo -e "${RED}❌ ÉCHOUÉ${NC}")"
echo -e "  T1  (Signature ISA):   $([ $T1_RESULT -eq 0 ] && echo -e "${GREEN}✅ RÉUSSI${NC}" || [ $T1_RESULT -eq 2 ] && echo -e "${YELLOW}⚠️ SKIP${NC}" || echo -e "${RED}❌ ÉCHOUÉ${NC}")"
echo -e "  T2  (Cache Kernel):    $([ $T2_RESULT -eq 0 ] && echo -e "${GREEN}✅ RÉUSSI${NC}" || [ $T2_RESULT -eq 2 ] && echo -e "${YELLOW}⚠️ SKIP${NC}" || echo -e "${RED}❌ ÉCHOUÉ${NC}")"
echo ""
echo -e "  Rapport: ${BLUE}$REPORT_FILE${NC}"
echo ""

if [ $T11_RESULT -eq 0 ]; then
    echo -e "${GREEN}🎉 SUCCÈS: Compute pipeline Gen9 VALIDÉ!${NC}"
    exit 0
else
    echo -e "${RED}⚠️  ÉCHEC: Analyser logs pour identifier problème${NC}"
    exit 1
fi

# Made with Bob
