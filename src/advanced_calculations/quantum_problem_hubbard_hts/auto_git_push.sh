#!/usr/bin/env bash
# ============================================================================
# auto_git_push.sh — C26-GITPUSH : Push GitHub automatique
# Gère automatiquement les fichiers > 95 MB (limite GitHub = 100 MB)
# Découpe, nettoie l'historique git, push — AUCUNE INTERVENTION MANUELLE
# ============================================================================
# INSTRUCTION PERMANENTE : Répondre TOUJOURS en français dans les rapports.
# INSTRUCTION PERMANENTE : NE JAMAIS RÉDUIRE LES LOGS — augmenter la traçabilité.
# INSTRUCTION PERMANENTE : Si un run est arrêté, relancer automatiquement depuis
#   le point d'arrêt jusqu'à finalisation complète à 100%.
# ============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GITIGNORE="$REPO_ROOT/.gitignore"
MAX_SIZE_BYTES=$((20 * 1024 * 1024))   # 20 MB max par fichier (réduit de 90→20 MB, C42)
SPLIT_SIZE="20m"                        # taille des morceaux pour la commande split
GIT_LOG="$SCRIPT_DIR/logs/auto_git_push_$(date -u +%Y%m%dT%H%M%SZ).log"

mkdir -p "$SCRIPT_DIR/logs"
exec > >(tee -a "$GIT_LOG") 2>&1

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] =========================================="
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] auto_git_push.sh démarré"
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] REPO_ROOT=$REPO_ROOT"
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] =========================================="

cd "$REPO_ROOT"

# ── ÉTAPE 1 : Détecter les fichiers > 90 MB dans l'espace de travail ──────
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ÉTAPE 1 : Détection des fichiers > 90 MB..."

LARGE_FILES=()
while IFS= read -r -d '' f; do
    size=$(stat -c%s "$f" 2>/dev/null || echo 0)
    if [ "$size" -gt "$MAX_SIZE_BYTES" ]; then
        LARGE_FILES+=("$f")
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   GRAND FICHIER : $f ($(( size / 1024 / 1024 )) MB)"
    fi
done < <(find . -type f \( -name "*.csv" -o -name "*.log" -o -name "*.bin" \) ! -path "./.git/*" -print0 2>/dev/null)

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ${#LARGE_FILES[@]} fichier(s) > 90 MB détecté(s)"

# ── ÉTAPE 2 : Pour chaque gros fichier — découper + gitignore ─────────────
for BIG_FILE in "${LARGE_FILES[@]}"; do
    REL_PATH="${BIG_FILE#./}"
    BASENAME="$(basename "$BIG_FILE")"
    DIRNAME="$(dirname "$BIG_FILE")"
    PREFIX="${DIRNAME}/${BASENAME%.csv}_part_"

    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] TRAITEMENT : $REL_PATH"

    # 2a. Découper le fichier en morceaux < 90 MB
    if [ ! -f "${PREFIX}aa" ]; then
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   Découpage en morceaux ${SPLIT_SIZE}..."
        split -b "$SPLIT_SIZE" "$BIG_FILE" "$PREFIX"
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   Morceaux créés : $(ls "${PREFIX}"* 2>/dev/null | wc -l)"
        # Ajouter les morceaux à git
        for PART in "${PREFIX}"*; do
            git add "$PART" 2>/dev/null && echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   git add : $PART" || true
        done
    else
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   Morceaux déjà existants — skip découpage"
    fi

    # 2b. Ajouter le gros fichier au .gitignore (si pas déjà présent)
    if ! grep -qF "$REL_PATH" "$GITIGNORE" 2>/dev/null; then
        echo "$REL_PATH" >> "$GITIGNORE"
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   Ajouté à .gitignore : $REL_PATH"
    fi

    # 2c. Retirer le gros fichier de l'index git (sans supprimer le fichier)
    git rm --cached "$REL_PATH" 2>/dev/null && \
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   git rm --cached : $REL_PATH" || \
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   $REL_PATH non tracké dans git — OK"
done

# ── ÉTAPE 3 : Nettoyer l'historique git des fichiers > 90 MB ──────────────
# (retire les gros fichiers des commits précédents pour pouvoir pusher)
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ÉTAPE 3 : Nettoyage historique git des fichiers > 90 MB..."

# Trouver les chemins dans l'historique git qui dépassent la limite
HIST_LARGE=()
while IFS= read -r line; do
    # Format: <size> <blob> <path>
    size=$(echo "$line" | awk '{print $1}')
    path=$(echo "$line" | awk '{print $3}')
    if [ "$size" -gt "$MAX_SIZE_BYTES" ]; then
        HIST_LARGE+=("$path")
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   Dans historique : $path ($(( size / 1024 / 1024 )) MB)"
    fi
done < <(git rev-list --objects --all 2>/dev/null | \
    git cat-file --batch-check='%(objecttype) %(objectname) %(objectsize) %(rest)' 2>/dev/null | \
    awk '/^blob/{print $3, $2, $4}' | \
    sort -rn | head -50 2>/dev/null || true)

if [ ${#HIST_LARGE[@]} -gt 0 ]; then
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ${#HIST_LARGE[@]} gros fichier(s) dans l'historique git — réécriture..."
    for HIST_PATH in "${HIST_LARGE[@]}"; do
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   Suppression historique : $HIST_PATH"
        git filter-branch --force --index-filter \
            "git rm --cached --ignore-unmatch '$HIST_PATH'" \
            --prune-empty --tag-name-filter cat -- --all 2>/dev/null || \
            echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   filter-branch échoué pour $HIST_PATH — continuer quand même"
    done
    # Nettoyer les refs et objets orphelins
    git for-each-ref --format='delete %(refname)' refs/original/ 2>/dev/null | \
        git update-ref --stdin 2>/dev/null || true
    git reflog expire --expire=now --all 2>/dev/null || true
    git gc --prune=now --aggressive 2>/dev/null || true
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Nettoyage historique terminé"
else
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Aucun gros fichier dans l'historique git — OK"
fi

# ── ÉTAPE 4 : Mettre à jour le .gitignore avec le pattern générique ────────
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ÉTAPE 4 : Mise à jour .gitignore..."

# Pattern générique pour tous les gros CSV Lumvorax futurs
PATTERNS=(
    "# Lumvorax CSV > 90 MB : trackés uniquement sous forme de morceaux découpés"
    "**/lumvorax_*_part_*"
    "# Les gros CSV bruts Lumvorax sont découpés automatiquement"
    "**/lumvorax_hubbard_hts_advanced_parallel_*.csv"
    "**/lumvorax_hubbard_hts_fullscale_*.csv"
)

for PATTERN in "${PATTERNS[@]}"; do
    if ! grep -qF "$PATTERN" "$GITIGNORE" 2>/dev/null; then
        echo "$PATTERN" >> "$GITIGNORE"
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)]   Ajouté à .gitignore : $PATTERN"
    fi
done

# Ré-ajouter .gitignore mis à jour
git add "$GITIGNORE" 2>/dev/null || true

# ── ÉTAPE 5 : Git add + commit + push ─────────────────────────────────────
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ÉTAPE 5 : Git add / commit / push..."

# Ajouter tous les nouveaux fichiers (morceaux, résultats, rapports)
git add --all 2>/dev/null || true

# Vérifier s'il y a des changements à committer
if git diff --cached --quiet 2>/dev/null; then
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Aucun changement à committer"
else
    COMMIT_MSG="C26-GITPUSH: run auto $(date -u +%Y%m%dT%H%M%SZ) — morceaux Lumvorax + rapports"
    git commit -m "$COMMIT_MSG" 2>/dev/null || \
        echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] commit échoué — peut-être déjà committé"
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Commit : $COMMIT_MSG"
fi

# Push (force si réécriture historique)
PUSH_FLAGS=""
if [ ${#HIST_LARGE[@]} -gt 0 ]; then
    PUSH_FLAGS="--force"
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Push FORCE requis (historique réécrit)"
fi

if git push origin main $PUSH_FLAGS 2>&1; then
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ✅ Push GitHub réussi"
else
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ⚠️  Push GitHub échoué — log conservé pour diagnostic"
    # Ne pas faire échouer le script — le push sera retenté au prochain run
fi

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] =========================================="
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] auto_git_push.sh terminé"
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] =========================================="
