#!/usr/bin/env bash
# ============================================================
# LumVorax C47 — Nettoyage Ubuntu (exécuter sur lvx@lvx-Vostro-5481)
# Supprime les anciens fichiers envoyés morceau par morceau
# et les copies obsolètes de l'agent Ubuntu.
#
# Usage :
#   bash ~/LVX/lumvorax2/tools/ubuntu_cleanup.sh
# ============================================================

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'

echo -e "${YELLOW}=== LumVorax C47 — Nettoyage Ubuntu ===${NC}"
echo -e "${YELLOW}Host : $(hostname) — $(date '+%Y-%m-%dT%H:%M:%SZ')${NC}"

removed=0

# ─── 1. Supprimer ~/agent_ubuntu.sh (copie obsolète dans home) ───
if [ -f "$HOME/agent_ubuntu.sh" ]; then
    rm -f "$HOME/agent_ubuntu.sh"
    echo -e "${GREEN}[OK] Supprimé : ~/agent_ubuntu.sh${NC}"
    ((removed++))
else
    echo -e "${YELLOW}[SKIP] ~/agent_ubuntu.sh absent (déjà nettoyé)${NC}"
fi

# ─── 2. Supprimer scripts temporaires /tmp/lumvorax_* ────────────
TMP_FILES=$(ls /tmp/lumvorax_* 2>/dev/null)
if [ -n "$TMP_FILES" ]; then
    rm -f /tmp/lumvorax_*
    echo -e "${GREEN}[OK] Supprimé : /tmp/lumvorax_* ($(echo "$TMP_FILES" | wc -l) fichiers)${NC}"
    ((removed++))
else
    echo -e "${YELLOW}[SKIP] /tmp/lumvorax_* absent${NC}"
fi

# ─── 3. Supprimer ~/lumvorax_patch*.sh (patches envoyés morceau par morceau) ───
PATCH_FILES=$(ls "$HOME"/lumvorax_patch*.sh 2>/dev/null)
if [ -n "$PATCH_FILES" ]; then
    rm -f "$HOME"/lumvorax_patch*.sh
    echo -e "${GREEN}[OK] Supprimé : ~/lumvorax_patch*.sh${NC}"
    ((removed++))
else
    echo -e "${YELLOW}[SKIP] Aucun lumvorax_patch*.sh dans ~/${NC}"
fi

# ─── 4. Supprimer ~/lumvorax_*.py (scripts Python temporaires) ───
PY_FILES=$(ls "$HOME"/lumvorax_*.py 2>/dev/null)
if [ -n "$PY_FILES" ]; then
    rm -f "$HOME"/lumvorax_*.py
    echo -e "${GREEN}[OK] Supprimé : ~/lumvorax_*.py${NC}"
    ((removed++))
else
    echo -e "${YELLOW}[SKIP] Aucun lumvorax_*.py dans ~/${NC}"
fi

# ─── 5. Supprimer ~/lumvorax_*.sh (autres scripts temporaires) ───
SH_FILES=$(ls "$HOME"/lumvorax_*.sh 2>/dev/null)
if [ -n "$SH_FILES" ]; then
    rm -f "$HOME"/lumvorax_*.sh
    echo -e "${GREEN}[OK] Supprimé : ~/lumvorax_*.sh${NC}"
    ((removed++))
else
    echo -e "${YELLOW}[SKIP] Aucun lumvorax_*.sh dans ~/${NC}"
fi

# ─── 6. Supprimer ~/btc_*.c, ~/btc_*.h (sources C temporaires) ──
C_FILES=$(ls "$HOME"/btc_*.c "$HOME"/btc_*.h 2>/dev/null)
if [ -n "$C_FILES" ]; then
    rm -f "$HOME"/btc_*.c "$HOME"/btc_*.h
    echo -e "${GREEN}[OK] Supprimé : ~/btc_*.c/h${NC}"
    ((removed++))
else
    echo -e "${YELLOW}[SKIP] Aucun btc_*.c/h dans ~/${NC}"
fi

echo ""
echo -e "${GREEN}=== Nettoyage terminé — ${removed} catégories traitées ===${NC}"
echo -e "${GREEN}Source unique à utiliser : ~/LVX/lumvorax2/ (dépôt git complet)${NC}"
echo ""
echo -e "${YELLOW}Prochaine étape — Lancer l'agent C47 :${NC}"
echo -e "  cd ~/LVX/lumvorax2 && git pull origin main"
echo -e "  doppler run -- bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh"
