#!/usr/bin/env bash
# =============================================================================
# fix_git_lock_and_track.sh — Nettoyage du lock git stale + ajout fichiers
#   manquants dans git + correction .gitignore + push GitHub
#
# USAGE : bash fix_git_lock_and_track.sh
# À exécuter depuis le shell Replit (pas via l'agent — le lock bloque l'agent)
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"

echo "======================================================================"
echo "FIX GIT LOCK + TRACK FICHIERS MANQUANTS — $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "======================================================================"

# ── ÉTAPE 1 : Supprimer le lock git stale ─────────────────────────────────
echo ""
echo "[ÉTAPE 1] Vérification et suppression du lock git stale..."

LOCK="$REPO_ROOT/.git/index.lock"
if [ -f "$LOCK" ]; then
    rm -f "$LOCK"
    echo "  ✅ Lock supprimé : $LOCK"
else
    echo "  ℹ️  Pas de lock détecté — OK"
fi

# ── ÉTAPE 2 : Vérifier l'état git ─────────────────────────────────────────
echo ""
echo "[ÉTAPE 2] État git actuel..."
git status --short 2>&1 | head -30 || true
echo ""
echo "Fichiers non trackés dans CHAT/ :"
git ls-files --others --exclude-standard \
    src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/ 2>/dev/null || true

# ── ÉTAPE 3 : Ajouter les rapports CHAT manquants ─────────────────────────
echo ""
echo "[ÉTAPE 3] Ajout des fichiers CHAT non trackés..."

CHAT_DIR="src/advanced_calculations/quantum_problem_hubbard_hts/CHAT"
UNTRACKED_CHAT=$(git ls-files --others --exclude-standard "$CHAT_DIR/" 2>/dev/null || true)

if [ -z "$UNTRACKED_CHAT" ]; then
    echo "  ℹ️  Tous les fichiers CHAT/ sont déjà trackés"
else
    for f in $UNTRACKED_CHAT; do
        git add "$f" && echo "  ✅ git add : $f"
    done
fi

# ── ÉTAPE 4 : Ajouter le .gitignore corrigé ───────────────────────────────
echo ""
echo "[ÉTAPE 4] Ajout du .gitignore corrigé..."
git add .gitignore && echo "  ✅ git add .gitignore"

# ── ÉTAPE 5 : Vérifier les morceaux Lumvorax non trackés ──────────────────
echo ""
echo "[ÉTAPE 5] Vérification des morceaux Lumvorax non trackés..."

UNTRACKED_PARTS=$(git ls-files --others --exclude-standard \
    --exclude='.gitignore' \
    src/advanced_calculations/quantum_problem_hubbard_hts/results/ 2>/dev/null | \
    grep "_part_" || true)

if [ -z "$UNTRACKED_PARTS" ]; then
    echo "  ℹ️  Aucun morceau Lumvorax non tracké"
else
    echo "  Morceaux Lumvorax non trackés trouvés — ajout forcé..."
    for f in $UNTRACKED_PARTS; do
        git add -f "$f" && echo "  ✅ git add -f (morceau) : $f"
    done
fi

# ── ÉTAPE 6 : Diagnostiquer les fichiers ignorés par erreur ───────────────
echo ""
echo "[ÉTAPE 6] Audit des fichiers ignorés (vérification .gitignore)..."

echo "  Fichiers ignorés dans results/ :"
git ls-files --others --ignored --exclude-standard \
    src/advanced_calculations/quantum_problem_hubbard_hts/results/ 2>/dev/null | \
    head -20 || true

echo ""
echo "  Fichiers ignorés dans CHAT/ :"
git ls-files --others --ignored --exclude-standard \
    src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/ 2>/dev/null || true

# ── ÉTAPE 7 : Commit ──────────────────────────────────────────────────────
echo ""
echo "[ÉTAPE 7] Vérification des changements à committer..."

if git diff --cached --quiet 2>/dev/null; then
    echo "  ℹ️  Aucun changement stagé — rien à committer"
else
    COMMIT_MSG="fix(git): ajout analysechatgpt46.1+47 CHAT non trackés + correction .gitignore lumvorax_part — $(date -u +%Y%m%dT%H%M%SZ)"
    git commit -m "$COMMIT_MSG"
    echo "  ✅ Commit créé : $COMMIT_MSG"
fi

# ── ÉTAPE 8 : Push ────────────────────────────────────────────────────────
echo ""
echo "[ÉTAPE 8] Push vers GitHub (origin main)..."

if git push origin main 2>&1; then
    echo "  ✅ Push GitHub réussi"
else
    echo "  ⚠️  Push échoué — vérifier les credentials SSH GitHub"
    echo "      Pour vérifier la clé SSH : ssh -T git@github.com"
fi

# ── RAPPORT FINAL ─────────────────────────────────────────────────────────
echo ""
echo "======================================================================"
echo "RAPPORT FINAL — Fichiers trackés dans git après correction :"
echo "======================================================================"
echo ""
echo "CHAT/ :"
git ls-files src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/ 2>/dev/null | \
    grep "analysechatgpt" | tail -10

echo ""
echo "Run 4200 :"
git ls-files \
    src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260318T165728Z_4200/ \
    2>/dev/null | wc -l | xargs echo "  Fichiers trackés :"

echo ""
echo "Résumé :"
git log --oneline -5 2>/dev/null || true
echo ""
echo "======================================================================"
echo "TERMINÉ — $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "======================================================================"
