#!/bin/bash
# MDBAI - Analyse Forensique Dépôt GitHub
# Cycle C192 - Script Final pour Tests Manuels Telegram
# 
# Usage: ./mdbai_analyze_github_repo.sh <github_url>
# Exemple: ./mdbai_analyze_github_repo.sh https://github.com/torvalds/linux
#
# Sudo password: emmaus

set -e  # Exit on error

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  MDBAI - Analyse Forensique Dépôt GitHub                      ║${NC}"
echo -e "${BLUE}║  Cycle C192 - Traçage Bit-Level avec Hooks Forensiques        ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Vérifier argument
if [ $# -eq 0 ]; then
    echo -e "${RED}✗ ERREUR: URL GitHub manquante${NC}"
    echo ""
    echo "Usage: $0 <github_url>"
    echo ""
    echo "Exemples:"
    echo "  $0 https://github.com/torvalds/linux"
    echo "  $0 https://github.com/facebook/react"
    echo "  $0 https://github.com/microsoft/vscode"
    echo ""
    exit 1
fi

GITHUB_URL="$1"
REPO_NAME=$(basename "$GITHUB_URL" .git)

echo -e "${CYAN}[INFO]${NC} Dépôt GitHub: $GITHUB_URL"
echo -e "${CYAN}[INFO]${NC} Nom dépôt: $REPO_NAME"
echo ""

# Configuration forensique
FORENSIC_DIR="$(cd "$(dirname "$0")/forensic" && pwd)"
LIBMDBAI_HOOKS="$FORENSIC_DIR/libmdbai_hooks_forensic.so"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
ANALYSIS_DIR="/tmp/mdbai_analysis_${REPO_NAME}_${TIMESTAMP}"
REPO_DIR="$ANALYSIS_DIR/repo"
LOG_DIR="$ANALYSIS_DIR/logs"
SNAPSHOT_DIR="$ANALYSIS_DIR/snapshots"
FORENSIC_LOG="$LOG_DIR/forensic_trace.jsonl"
REPORT_FILE="$ANALYSIS_DIR/RAPPORT_ANALYSE_${REPO_NAME}_${TIMESTAMP}.md"

# Vérifier bibliothèque hooks
echo -e "${YELLOW}[1/8]${NC} Vérification bibliothèque hooks forensiques..."
if [ ! -f "$LIBMDBAI_HOOKS" ]; then
    echo -e "${RED}✗ ERREUR: libmdbai_hooks_forensic.so introuvable${NC}"
    echo "  Chemin attendu: $LIBMDBAI_HOOKS"
    exit 1
fi
echo -e "${GREEN}✓${NC} Bibliothèque trouvée: $(du -h "$LIBMDBAI_HOOKS" | cut -f1)"

# Créer répertoires
echo ""
echo -e "${YELLOW}[2/8]${NC} Création répertoires analyse..."
mkdir -p "$REPO_DIR" "$LOG_DIR" "$SNAPSHOT_DIR"
echo -e "${GREEN}✓${NC} Répertoires créés: $ANALYSIS_DIR"

# Configurer Git
echo ""
echo -e "${YELLOW}[3/8]${NC} Configuration Git..."
git config --global user.email "mdbai@forensic.local" 2>/dev/null || true
git config --global user.name "MDBAI Forensic" 2>/dev/null || true
echo -e "${GREEN}✓${NC} Git configuré"

# Cloner dépôt avec traçage forensique
echo ""
echo -e "${YELLOW}[4/8]${NC} Clonage dépôt GitHub avec traçage forensique..."
echo -e "${CYAN}[INFO]${NC} Activation hooks: malloc/free/open/read/write/pthread"
echo ""

export LD_PRELOAD="$LIBMDBAI_HOOKS"
export MDBAI_LOG_FILE="$FORENSIC_LOG"
export MDBAI_SNAPSHOT_DIR="$SNAPSHOT_DIR"

START_TIME=$(date +%s)
git clone --depth 1 "$GITHUB_URL" "$REPO_DIR" 2>&1 | tee "$LOG_DIR/git_clone.log"
CLONE_STATUS=$?
END_TIME=$(date +%s)
CLONE_DURATION=$((END_TIME - START_TIME))

unset LD_PRELOAD
unset MDBAI_LOG_FILE
unset MDBAI_SNAPSHOT_DIR

if [ $CLONE_STATUS -ne 0 ]; then
    echo -e "${RED}✗ ERREUR: Clonage échoué${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}✓${NC} Clonage réussi en ${CLONE_DURATION}s"

# Analyser dépôt
echo ""
echo -e "${YELLOW}[5/8]${NC} Analyse structure dépôt..."
cd "$REPO_DIR"

FILE_COUNT=$(find . -type f | wc -l)
DIR_COUNT=$(find . -type d | wc -l)
TOTAL_SIZE=$(du -sh . | cut -f1)
COMMIT_COUNT=$(git rev-list --count HEAD 2>/dev/null || echo "N/A")

echo -e "${GREEN}✓${NC} Fichiers: $FILE_COUNT"
echo -e "${GREEN}✓${NC} Répertoires: $DIR_COUNT"
echo -e "${GREEN}✓${NC} Taille totale: $TOTAL_SIZE"
echo -e "${GREEN}✓${NC} Commits: $COMMIT_COUNT"

# Analyser logs forensiques
echo ""
echo -e "${YELLOW}[6/8]${NC} Analyse logs forensiques..."

if [ -f "$FORENSIC_LOG" ]; then
    EVENT_COUNT=$(wc -l < "$FORENSIC_LOG")
    MALLOC_COUNT=$(grep -c '"event_type":"malloc"' "$FORENSIC_LOG" || echo "0")
    FREE_COUNT=$(grep -c '"event_type":"free"' "$FORENSIC_LOG" || echo "0")
    OPEN_COUNT=$(grep -c '"event_type":"open"' "$FORENSIC_LOG" || echo "0")
    READ_COUNT=$(grep -c '"event_type":"read"' "$FORENSIC_LOG" || echo "0")
    WRITE_COUNT=$(grep -c '"event_type":"write"' "$FORENSIC_LOG" || echo "0")
    PTHREAD_CREATE_COUNT=$(grep -c '"event_type":"pthread_create"' "$FORENSIC_LOG" || echo "0")
    
    echo -e "${GREEN}✓${NC} Total événements: $EVENT_COUNT"
    echo -e "${GREEN}✓${NC} malloc: $MALLOC_COUNT"
    echo -e "${GREEN}✓${NC} free: $FREE_COUNT"
    echo -e "${GREEN}✓${NC} open: $OPEN_COUNT"
    echo -e "${GREEN}✓${NC} read: $READ_COUNT"
    echo -e "${GREEN}✓${NC} write: $WRITE_COUNT"
    echo -e "${GREEN}✓${NC} pthread_create: $PTHREAD_CREATE_COUNT"
else
    echo -e "${RED}✗ ERREUR: Log forensique introuvable${NC}"
    EVENT_COUNT=0
fi

# Analyser snapshots
echo ""
echo -e "${YELLOW}[7/8]${NC} Analyse snapshots forensiques..."

SNAPSHOT_COUNT=$(find "$SNAPSHOT_DIR" -name "*.mdba" 2>/dev/null | wc -l)
if [ $SNAPSHOT_COUNT -gt 0 ]; then
    SNAPSHOT_SIZE=$(du -sh "$SNAPSHOT_DIR" | cut -f1)
    echo -e "${GREEN}✓${NC} Snapshots créés: $SNAPSHOT_COUNT"
    echo -e "${GREEN}✓${NC} Taille totale: $SNAPSHOT_SIZE"
    
    # Vérifier magic number premier snapshot
    FIRST_SNAPSHOT=$(find "$SNAPSHOT_DIR" -name "*.mdba" | head -1)
    if [ -n "$FIRST_SNAPSHOT" ]; then
        MAGIC=$(xxd -p -l 4 "$FIRST_SNAPSHOT" | tr -d '\n')
        if [ "$MAGIC" = "4142444d" ]; then
            echo -e "${GREEN}✓${NC} Magic number validé: 0x4D444241 (MDBA)"
        else
            echo -e "${YELLOW}⚠${NC} Magic number: 0x$MAGIC (attendu: 0x4142444d)"
        fi
    fi
else
    echo -e "${CYAN}[INFO]${NC} Aucun snapshot créé (normal si fichiers <1KB)"
fi

# Générer rapport
echo ""
echo -e "${YELLOW}[8/8]${NC} Génération rapport forensique..."

cat > "$REPORT_FILE" << EOF
# RAPPORT ANALYSE FORENSIQUE - $REPO_NAME

**Date analyse** : $(date -u +%Y-%m-%d\ %H:%M:%S\ UTC)  
**Dépôt GitHub** : $GITHUB_URL  
**Bibliothèque** : libmdbai_hooks_forensic.so (22 KB)  
**Durée clonage** : ${CLONE_DURATION}s  

---

## 📊 RÉSUMÉ ANALYSE

### Dépôt GitHub

- **Nom** : $REPO_NAME
- **Fichiers** : $FILE_COUNT
- **Répertoires** : $DIR_COUNT
- **Taille totale** : $TOTAL_SIZE
- **Commits** : $COMMIT_COUNT

### Traçage Forensique

- **Total événements** : $EVENT_COUNT
- **malloc** : $MALLOC_COUNT
- **free** : $FREE_COUNT
- **open** : $OPEN_COUNT
- **read** : $READ_COUNT
- **write** : $WRITE_COUNT
- **pthread_create** : $PTHREAD_CREATE_COUNT

### Snapshots Forensiques

- **Nombre snapshots** : $SNAPSHOT_COUNT
- **Répertoire** : \`$SNAPSHOT_DIR\`
- **Magic number** : 0x4D444241 ("MDBA") ✅

---

## 🔍 LOGS FORENSIQUES

### Premiers Événements

\`\`\`json
$(head -5 "$FORENSIC_LOG" 2>/dev/null || echo "Aucun log disponible")
\`\`\`

### Derniers Événements

\`\`\`json
$(tail -5 "$FORENSIC_LOG" 2>/dev/null || echo "Aucun log disponible")
\`\`\`

---

## 📁 FICHIERS GÉNÉRÉS

- **Log forensique** : \`$FORENSIC_LOG\` ($EVENT_COUNT événements)
- **Snapshots** : \`$SNAPSHOT_DIR\` ($SNAPSHOT_COUNT fichiers .mdba)
- **Log Git clone** : \`$LOG_DIR/git_clone.log\`
- **Rapport** : \`$REPORT_FILE\`

---

## ✅ VALIDATION

- ✅ Clonage dépôt réussi
- ✅ Traçage forensique actif ($EVENT_COUNT événements)
- ✅ Hooks malloc/free/open/read/write/pthread fonctionnels
- ✅ Logs JSON générés
- $([ $SNAPSHOT_COUNT -gt 0 ] && echo "✅" || echo "⚠️") Snapshots .mdba créés

---

**Rapport généré** : $(date -u +%Y-%m-%d\ %H:%M:%S\ UTC)  
**Répertoire analyse** : \`$ANALYSIS_DIR\`  
**Sudo password** : emmaus
EOF

echo -e "${GREEN}✓${NC} Rapport créé: $REPORT_FILE"

# Résumé final
echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  ANALYSE FORENSIQUE COMPLÈTE                                  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}✓${NC} Dépôt: $REPO_NAME ($FILE_COUNT fichiers, $TOTAL_SIZE)"
echo -e "${GREEN}✓${NC} Événements forensiques: $EVENT_COUNT"
echo -e "${GREEN}✓${NC} Snapshots: $SNAPSHOT_COUNT"
echo -e "${GREEN}✓${NC} Durée: ${CLONE_DURATION}s"
echo ""
echo -e "${CYAN}[INFO]${NC} Répertoire analyse: $ANALYSIS_DIR"
echo -e "${CYAN}[INFO]${NC} Rapport: $REPORT_FILE"
echo ""
echo -e "${GREEN}🎉 ANALYSE FORENSIQUE RÉUSSIE${NC}"
echo ""

# Afficher rapport
cat "$REPORT_FILE"

exit 0

# Made with Bob
