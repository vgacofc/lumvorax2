#!/bin/bash
# Script C256 — Test Validation GPU HANG sans Réouverture
# Objectif: Valider si timeout 650ms cause GPU HANG au premier dispatch
#           et si on peut dépasser 27 dispatches SANS réouverture

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
LOG_DIR="$PROJECT_DIR/logs/validation_c256"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Test C256 — Validation GPU HANG (SANS Réouverture)          ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Créer répertoire logs
mkdir -p "$LOG_DIR"

# Fichiers de log
MAIN_LOG="$LOG_DIR/test_no_reopen_${TIMESTAMP}.log"
DMESG_LOG="$LOG_DIR/dmesg_${TIMESTAMP}.log"
RESULT_LOG="$LOG_DIR/result_${TIMESTAMP}.md"

echo -e "${YELLOW}[INFO]${NC} Configuration du test:"
echo "  - Dispatches: 50 (pour dépasser limite 27)"
echo "  - Réouverture: DÉSACTIVÉE"
echo "  - Batch size: 40M nonces (temps ~700ms > timeout 650ms)"
echo "  - Log principal: $MAIN_LOG"
echo "  - Log dmesg: $DMESG_LOG"
echo ""

# Vérifier que le binaire existe
BINARY="$PROJECT_DIR/test_c255v12_reopen"
if [ ! -f "$BINARY" ]; then
    echo -e "${RED}[ERREUR]${NC} Binaire non trouvé: $BINARY"
    echo "Compilation nécessaire..."
    cd "$PROJECT_DIR"
    make -f Makefile.gen9_native clean
    make -f Makefile.gen9_native test_c255v12_reopen
    if [ ! -f "$BINARY" ]; then
        echo -e "${RED}[ERREUR]${NC} Échec compilation"
        exit 1
    fi
fi

echo -e "${GREEN}[OK]${NC} Binaire trouvé: $BINARY"
echo ""

# Capturer dmesg initial
echo -e "${YELLOW}[INFO]${NC} Capture dmesg initial..."
sudo dmesg -c > /dev/null 2>&1 || true
echo -e "${GREEN}[OK]${NC} dmesg vidé"
echo ""

# Fonction pour monitorer dmesg en temps réel
monitor_dmesg() {
    local dispatch_num=$1
    local gpu_hang_detected=0
    
    # Capturer nouveaux messages dmesg
    sudo dmesg -T | tail -n 50 > "$DMESG_LOG.tmp"
    
    # Chercher GPU HANG
    if grep -i "GPU HANG" "$DMESG_LOG.tmp" > /dev/null 2>&1; then
        echo -e "${RED}[GPU HANG DÉTECTÉ]${NC} Dispatch $dispatch_num" | tee -a "$MAIN_LOG"
        cat "$DMESG_LOG.tmp" >> "$DMESG_LOG"
        gpu_hang_detected=1
    fi
    
    # Chercher context reset
    if grep -i "context reset" "$DMESG_LOG.tmp" > /dev/null 2>&1; then
        echo -e "${YELLOW}[CONTEXT RESET]${NC} Dispatch $dispatch_num" | tee -a "$MAIN_LOG"
        cat "$DMESG_LOG.tmp" >> "$DMESG_LOG"
    fi
    
    # Chercher timeout
    if grep -i "timeout" "$DMESG_LOG.tmp" > /dev/null 2>&1; then
        echo -e "${YELLOW}[TIMEOUT]${NC} Dispatch $dispatch_num" | tee -a "$MAIN_LOG"
        cat "$DMESG_LOG.tmp" >> "$DMESG_LOG"
    fi
    
    rm -f "$DMESG_LOG.tmp"
    return $gpu_hang_detected
}

# Lancer le test SANS réouverture
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  LANCEMENT TEST — 50 Dispatches SANS Réouverture${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Modifier le binaire pour désactiver réouverture (si nécessaire)
# Pour ce test, on va utiliser une variable d'environnement
export BTC_DISABLE_REOPEN=1
export BTC_NUM_DISPATCHES=50
export BTC_BATCH_SIZE=40265318  # 40M nonces = ~700ms

# Lancer le test avec monitoring
echo -e "${YELLOW}[INFO]${NC} Démarrage test..." | tee -a "$MAIN_LOG"
START_TIME=$(date +%s)

# Variables de suivi
DISPATCH_COUNT=0
GPU_HANG_DISPATCH=0
CRASH_DISPATCH=0
SUCCESS=1

# Lancer le test en arrière-plan et monitorer
(
    cd "$PROJECT_DIR"
    sudo "$BINARY" --no-reopen --dispatches 50 --batch-size 40265318 2>&1
) | while IFS= read -r line; do
    echo "$line" | tee -a "$MAIN_LOG"
    
    # Détecter numéro de dispatch
    if echo "$line" | grep -q "\[Dispatch"; then
        DISPATCH_COUNT=$(echo "$line" | grep -oP '\[Dispatch \K[0-9]+' || echo "0")
        
        # Monitorer dmesg après chaque dispatch
        if monitor_dmesg "$DISPATCH_COUNT"; then
            GPU_HANG_DISPATCH=$DISPATCH_COUNT
            echo -e "${RED}[CRITIQUE]${NC} GPU HANG détecté au dispatch $DISPATCH_COUNT!" | tee -a "$MAIN_LOG"
            SUCCESS=0
        fi
        
        # Vérifier si on a dépassé la limite 27
        if [ "$DISPATCH_COUNT" -eq 28 ]; then
            echo -e "${GREEN}[SUCCÈS]${NC} Limite 27 dispatches DÉPASSÉE!" | tee -a "$MAIN_LOG"
        fi
    fi
    
    # Détecter crash
    if echo "$line" | grep -qi "crash\|fatal\|segfault"; then
        CRASH_DISPATCH=$DISPATCH_COUNT
        echo -e "${RED}[CRASH]${NC} Détecté au dispatch $DISPATCH_COUNT" | tee -a "$MAIN_LOG"
        SUCCESS=0
        break
    fi
done

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# Capturer dmesg final
echo "" | tee -a "$MAIN_LOG"
echo -e "${YELLOW}[INFO]${NC} Capture dmesg final..." | tee -a "$MAIN_LOG"
sudo dmesg -T > "$DMESG_LOG.final"

# Analyser les résultats
echo "" | tee -a "$MAIN_LOG"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}" | tee -a "$MAIN_LOG"
echo -e "${BLUE}  ANALYSE DES RÉSULTATS${NC}" | tee -a "$MAIN_LOG"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

# Compter GPU HANG dans dmesg
GPU_HANG_COUNT=$(grep -c "GPU HANG" "$DMESG_LOG.final" 2>/dev/null || echo "0")
TIMEOUT_COUNT=$(grep -c -i "timeout" "$DMESG_LOG.final" 2>/dev/null || echo "0")
RESET_COUNT=$(grep -c -i "context reset" "$DMESG_LOG.final" 2>/dev/null || echo "0")

echo "Durée totale: ${DURATION}s" | tee -a "$MAIN_LOG"
echo "Dispatches réussis: $DISPATCH_COUNT/50" | tee -a "$MAIN_LOG"
echo "GPU HANG détectés: $GPU_HANG_COUNT" | tee -a "$MAIN_LOG"
echo "Timeouts détectés: $TIMEOUT_COUNT" | tee -a "$MAIN_LOG"
echo "Context resets: $RESET_COUNT" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

# Générer rapport markdown
cat > "$RESULT_LOG" << EOF
# RAPPORT C256 — Validation GPU HANG (Test Réel)

**Date**: $(date -Iseconds)  
**Durée**: ${DURATION}s  
**Configuration**: SANS réouverture périodique

---

## 🎯 OBJECTIF DU TEST

Valider deux hypothèses:
1. **Le timeout 650ms cause-t-il un GPU HANG au premier dispatch?**
2. **Peut-on dépasser 27 dispatches sans réouverture?**

---

## 📊 RÉSULTATS

### Métriques Globales
- **Dispatches réussis**: $DISPATCH_COUNT/50
- **GPU HANG détectés**: $GPU_HANG_COUNT
- **Timeouts détectés**: $TIMEOUT_COUNT
- **Context resets**: $RESET_COUNT
- **Durée totale**: ${DURATION}s

### Analyse par Dispatch

EOF

# Analyser dispatch par dispatch
if [ "$GPU_HANG_COUNT" -gt 0 ]; then
    echo "#### ❌ GPU HANG Détectés" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    if [ "$GPU_HANG_DISPATCH" -eq 1 ]; then
        echo "**CONFIRMATION**: GPU HANG au **premier dispatch**" >> "$RESULT_LOG"
        echo "→ Le timeout 650ms est **CONFIRMÉ** comme cause du problème" >> "$RESULT_LOG"
    else
        echo "**GPU HANG au dispatch $GPU_HANG_DISPATCH**" >> "$RESULT_LOG"
        echo "→ Le problème n'est **PAS** lié au timeout du premier dispatch" >> "$RESULT_LOG"
    fi
    echo "" >> "$RESULT_LOG"
else
    echo "#### ✅ Aucun GPU HANG Détecté" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    echo "**RÉFUTATION**: Le timeout 650ms ne cause **PAS** de GPU HANG" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
fi

if [ "$DISPATCH_COUNT" -gt 27 ]; then
    echo "#### ✅ Limite 27 Dispatches DÉPASSÉE" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    echo "**Dispatches réussis**: $DISPATCH_COUNT (> 27)" >> "$RESULT_LOG"
    echo "→ La limite 27 peut être dépassée sans réouverture" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
else
    echo "#### ❌ Crash Avant Limite 27" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    echo "**Crash au dispatch**: $CRASH_DISPATCH" >> "$RESULT_LOG"
    echo "→ Impossible de dépasser 27 dispatches sans réouverture" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
fi

cat >> "$RESULT_LOG" << EOF
---

## 🔍 LOGS DMESG

\`\`\`
$(tail -n 100 "$DMESG_LOG.final")
\`\`\`

---

## 📋 CONCLUSION

EOF

if [ "$GPU_HANG_COUNT" -eq 0 ] && [ "$DISPATCH_COUNT" -gt 27 ]; then
    cat >> "$RESULT_LOG" << EOF
### ✅ HYPOTHÈSE TIMEOUT RÉFUTÉE

**Preuves**:
1. Aucun GPU HANG détecté sur $DISPATCH_COUNT dispatches
2. Limite 27 dispatches dépassée sans réouverture
3. Temps d'exécution ~700ms > timeout 650ms sans crash

**Recommandation**: La solution C255V13 (réouverture périodique) reste optimale mais n'est peut-être pas nécessaire.
EOF
elif [ "$GPU_HANG_DISPATCH" -eq 1 ]; then
    cat >> "$RESULT_LOG" << EOF
### ❌ HYPOTHÈSE TIMEOUT CONFIRMÉE

**Preuves**:
1. GPU HANG détecté au **premier dispatch**
2. Temps d'exécution ~700ms > timeout 650ms
3. Driver i915 a déclenché GPU HANG immédiatement

**Recommandation**: Augmenter timeout driver i915 ou utiliser solution C255V13.
EOF
else
    cat >> "$RESULT_LOG" << EOF
### ⚠️ RÉSULTATS MIXTES

**Observations**:
1. GPU HANG détecté au dispatch $GPU_HANG_DISPATCH (pas au premier)
2. Dispatches réussis: $DISPATCH_COUNT/50
3. Context resets: $RESET_COUNT

**Recommandation**: Analyser logs détaillés pour comprendre le comportement.
EOF
fi

cat >> "$RESULT_LOG" << EOF

---

**Logs complets**: 
- Principal: \`$MAIN_LOG\`
- dmesg: \`$DMESG_LOG.final\`
EOF

# Afficher résumé
echo "" | tee -a "$MAIN_LOG"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  RÉSUMÉ FINAL${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

if [ "$GPU_HANG_COUNT" -eq 0 ] && [ "$DISPATCH_COUNT" -gt 27 ]; then
    echo -e "${GREEN}[SUCCÈS]${NC} Test validé avec succès!"
    echo -e "  → Aucun GPU HANG détecté"
    echo -e "  → Limite 27 dispatches dépassée ($DISPATCH_COUNT dispatches)"
    echo -e "  → ${GREEN}Hypothèse timeout RÉFUTÉE${NC}"
elif [ "$GPU_HANG_DISPATCH" -eq 1 ]; then
    echo -e "${RED}[ÉCHEC]${NC} GPU HANG au premier dispatch"
    echo -e "  → ${RED}Hypothèse timeout CONFIRMÉE${NC}"
    echo -e "  → Solution: Augmenter timeout driver ou utiliser C255V13"
else
    echo -e "${YELLOW}[PARTIEL]${NC} Résultats mixtes"
    echo -e "  → GPU HANG au dispatch $GPU_HANG_DISPATCH"
    echo -e "  → Dispatches réussis: $DISPATCH_COUNT/50"
    echo -e "  → Analyse détaillée nécessaire"
fi

echo ""
echo -e "${BLUE}Rapport complet:${NC} $RESULT_LOG"
echo ""

# Ouvrir le rapport dans l'éditeur
if command -v code &> /dev/null; then
    code "$RESULT_LOG"
fi

exit 0

# Made with Bob
