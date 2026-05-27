#!/bin/bash
# Script C256 — Test Validation GPU HANG sans Réouverture (Version sans sudo)
# L'utilisateur doit lancer ce script avec sudo

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
NC='\033[0m'

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Test C256 — Validation GPU HANG (SANS Réouverture)          ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Vérifier si lancé avec sudo
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}[ERREUR]${NC} Ce script doit être lancé avec sudo pour accéder à dmesg"
    echo ""
    echo "Commande à exécuter:"
    echo -e "${YELLOW}sudo $0${NC}"
    echo ""
    exit 1
fi

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
    echo ""
    echo "Veuillez compiler d'abord:"
    echo "  cd $PROJECT_DIR"
    echo "  make -f Makefile.gen9_native test_c255v12_reopen"
    echo ""
    exit 1
fi

echo -e "${GREEN}[OK]${NC} Binaire trouvé: $BINARY"
echo ""

# Capturer dmesg initial
echo -e "${YELLOW}[INFO]${NC} Vidage dmesg initial..."
dmesg -c > /dev/null 2>&1 || true
echo -e "${GREEN}[OK]${NC} dmesg vidé"
echo ""

# Lancer le test SANS réouverture
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  LANCEMENT TEST — 50 Dispatches SANS Réouverture${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Variables d'environnement pour désactiver réouverture
export BTC_DISABLE_REOPEN=1
export BTC_NUM_DISPATCHES=50
export BTC_BATCH_SIZE=40265318

echo -e "${YELLOW}[INFO]${NC} Démarrage test..." | tee "$MAIN_LOG"
echo "Variables d'environnement:" | tee -a "$MAIN_LOG"
echo "  BTC_DISABLE_REOPEN=1" | tee -a "$MAIN_LOG"
echo "  BTC_NUM_DISPATCHES=50" | tee -a "$MAIN_LOG"
echo "  BTC_BATCH_SIZE=40265318" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

START_TIME=$(date +%s)

# Lancer le test
cd "$PROJECT_DIR"
"$BINARY" --no-reopen --dispatches 50 --batch-size 40265318 2>&1 | tee -a "$MAIN_LOG"

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# Capturer dmesg final
echo "" | tee -a "$MAIN_LOG"
echo -e "${YELLOW}[INFO]${NC} Capture dmesg final..." | tee -a "$MAIN_LOG"
dmesg -T > "$DMESG_LOG"

# Analyser les résultats
echo "" | tee -a "$MAIN_LOG"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}" | tee -a "$MAIN_LOG"
echo -e "${BLUE}  ANALYSE DES RÉSULTATS${NC}" | tee -a "$MAIN_LOG"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

# Compter dispatches réussis
DISPATCH_COUNT=$(grep -c "Dispatch.*Succès" "$MAIN_LOG" 2>/dev/null || echo "0")

# Compter GPU HANG dans dmesg
GPU_HANG_COUNT=$(grep -c "GPU HANG" "$DMESG_LOG" 2>/dev/null || echo "0")
TIMEOUT_COUNT=$(grep -c -i "timeout" "$DMESG_LOG" 2>/dev/null || echo "0")
RESET_COUNT=$(grep -c -i "context reset" "$DMESG_LOG" 2>/dev/null || echo "0")

# Détecter premier GPU HANG
FIRST_GPU_HANG=0
if [ "$GPU_HANG_COUNT" -gt 0 ]; then
    # Chercher le premier GPU HANG dans dmesg et corréler avec les dispatches
    FIRST_GPU_HANG_TIME=$(grep "GPU HANG" "$DMESG_LOG" | head -n 1 | awk '{print $1" "$2" "$3}')
    echo "Premier GPU HANG détecté à: $FIRST_GPU_HANG_TIME" | tee -a "$MAIN_LOG"
    
    # Estimer le dispatch (approximatif basé sur le timing)
    if [ "$DISPATCH_COUNT" -lt 5 ]; then
        FIRST_GPU_HANG=1
    fi
fi

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

### Analyse par Hypothèse

EOF

# Analyser hypothèse 1: GPU HANG au premier dispatch?
if [ "$GPU_HANG_COUNT" -gt 0 ]; then
    echo "#### ❌ GPU HANG Détectés" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    if [ "$FIRST_GPU_HANG" -eq 1 ]; then
        echo "**CONFIRMATION**: GPU HANG détecté dans les **premiers dispatches**" >> "$RESULT_LOG"
        echo "→ Le timeout 650ms est **PROBABLEMENT** la cause du problème" >> "$RESULT_LOG"
    else
        echo "**GPU HANG détecté après plusieurs dispatches**" >> "$RESULT_LOG"
        echo "→ Le problème n'est **PAS** lié au timeout du premier dispatch" >> "$RESULT_LOG"
    fi
    echo "" >> "$RESULT_LOG"
else
    echo "#### ✅ Aucun GPU HANG Détecté" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    echo "**RÉFUTATION**: Le timeout 650ms ne cause **PAS** de GPU HANG" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
fi

# Analyser hypothèse 2: Dépasser 27 dispatches?
if [ "$DISPATCH_COUNT" -gt 27 ]; then
    echo "#### ✅ Limite 27 Dispatches DÉPASSÉE" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    echo "**Dispatches réussis**: $DISPATCH_COUNT (> 27)" >> "$RESULT_LOG"
    echo "→ La limite 27 peut être dépassée sans réouverture" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
elif [ "$DISPATCH_COUNT" -ge 20 ]; then
    echo "#### ⚠️ Proche de la Limite 27" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    echo "**Dispatches réussis**: $DISPATCH_COUNT (proche de 27)" >> "$RESULT_LOG"
    echo "→ Test interrompu avant d'atteindre la limite" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
else
    echo "#### ❌ Crash Avant Limite 27" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
    echo "**Dispatches réussis**: $DISPATCH_COUNT (< 27)" >> "$RESULT_LOG"
    echo "→ Crash précoce, impossible de tester la limite 27" >> "$RESULT_LOG"
    echo "" >> "$RESULT_LOG"
fi

cat >> "$RESULT_LOG" << EOF
---

## 🔍 LOGS DMESG (100 dernières lignes)

\`\`\`
$(tail -n 100 "$DMESG_LOG")
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

**Recommandation**: La solution C255V13 (réouverture périodique) n'est peut-être pas nécessaire. Le système peut fonctionner sans workaround.
EOF
elif [ "$FIRST_GPU_HANG" -eq 1 ]; then
    cat >> "$RESULT_LOG" << EOF
### ❌ HYPOTHÈSE TIMEOUT CONFIRMÉE

**Preuves**:
1. GPU HANG détecté dans les **premiers dispatches**
2. Temps d'exécution ~700ms > timeout 650ms
3. Driver i915 a déclenché GPU HANG rapidement

**Recommandation**: 
- Option 1: Augmenter timeout driver i915 (650ms → 5000ms)
- Option 2: Utiliser solution C255V13 (réouverture périodique)
- Option 3: Réduire batch size pour rester sous 650ms
EOF
else
    cat >> "$RESULT_LOG" << EOF
### ⚠️ RÉSULTATS À ANALYSER

**Observations**:
1. Dispatches réussis: $DISPATCH_COUNT/50
2. GPU HANG détectés: $GPU_HANG_COUNT
3. Context resets: $RESET_COUNT

**Recommandation**: Analyser logs détaillés pour comprendre le comportement exact.
EOF
fi

cat >> "$RESULT_LOG" << EOF

---

**Logs complets**: 
- Principal: \`$MAIN_LOG\`
- dmesg: \`$DMESG_LOG\`

**Commande pour relancer**:
\`\`\`bash
sudo $0
\`\`\`
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
elif [ "$FIRST_GPU_HANG" -eq 1 ]; then
    echo -e "${RED}[ÉCHEC]${NC} GPU HANG dans les premiers dispatches"
    echo -e "  → ${RED}Hypothèse timeout PROBABLEMENT CONFIRMÉE${NC}"
    echo -e "  → Solution: Augmenter timeout driver ou utiliser C255V13"
else
    echo -e "${YELLOW}[PARTIEL]${NC} Résultats à analyser"
    echo -e "  → Dispatches réussis: $DISPATCH_COUNT/50"
    echo -e "  → GPU HANG détectés: $GPU_HANG_COUNT"
    echo -e "  → Analyse détaillée nécessaire"
fi

echo ""
echo -e "${BLUE}Rapport complet:${NC} $RESULT_LOG"
echo ""
echo "Pour voir le rapport:"
echo "  cat $RESULT_LOG"
echo ""

exit 0

# Made with Bob
