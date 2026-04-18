#!/usr/bin/env bash
# ============================================================
# LumVorax C63 — Rotation des fichiers volumineux (max 50 MB)
# ============================================================
# Objectif : Aucun fichier tracké ne dépasse 50 MB dans le repo.
#            Évite les warnings GitHub GH001 et supprime le besoin de LFS.
#
# Usage :
#   bash tools/rotate_large_files.sh               # toutes les cibles par défaut
#   bash tools/rotate_large_files.sh --dir /chemin # cibler un répertoire
#   bash tools/rotate_large_files.sh --dry-run     # simulation sans modification
#
# Cibles : logs forensic, métriques, nano-ring, pt_mc_swap, bench_raw
# Stratégie :
#   - Fichiers texte (*.log, *.txt, *.csv) : garder les dernières 10 000 lignes
#   - Fichiers JSON : compresser si >50 MB (rare) → .gz + réinitialiser
#   - Fichiers binaires / .o / .so : suppression directe (non trackés)
#
# STANDARD_NAMES.md v4.4 §F-ROTATE-C63

set -euo pipefail

MAX_MB="${ROTATE_MAX_MB:-50}"
MAX_BYTES=$(( MAX_MB * 1024 * 1024 ))
DRY_RUN=0
TARGET_DIRS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run) DRY_RUN=1; shift ;;
        --dir) shift; TARGET_DIRS+=("$1"); shift ;;
        --max-mb) shift; MAX_MB="$1"; MAX_BYTES=$(( MAX_MB * 1024 * 1024 )); shift ;;
        *) shift ;;
    esac
done

# Dossiers par défaut (relatifs à la racine du repo)
if [ -d "$HOME/LVX/lumvorax2" ]; then
    REPO_ROOT="$HOME/LVX/lumvorax2"
elif [ -d "/home/runner/workspace" ]; then
    REPO_ROOT="/home/runner/workspace"
else
    REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fi

if [ ${#TARGET_DIRS[@]} -eq 0 ]; then
    TARGET_DIRS=(
        "$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/logs"
        "$REPO_ROOT/logs"
        "$REPO_ROOT/src/qdayprize/logs"
        "$REPO_ROOT/trou_noir_sim/logs"
    )
fi

echo "[ROTATE] ============================================"
echo "[ROTATE] LumVorax — Rotation fichiers > ${MAX_MB} MB"
echo "[ROTATE] DRY_RUN : $DRY_RUN"
echo "[ROTATE] REPO    : $REPO_ROOT"
echo "[ROTATE] ============================================"

TOTAL_ROTATED=0
TOTAL_FREED_MB=0

rotate_text_file() {
    local f="$1"
    local size_bytes
    size_bytes=$(stat -c%s "$f" 2>/dev/null || echo 0)

    if [ "$size_bytes" -le "$MAX_BYTES" ]; then
        return
    fi

    local size_mb=$(( size_bytes / 1024 / 1024 ))
    local lines
    lines=$(wc -l < "$f" 2>/dev/null || echo 0)
    local keep_lines=10000

    echo "[ROTATE] 📄 ${f##$REPO_ROOT/} — ${size_mb} MB / ${lines} lignes → garder ${keep_lines} dernières"

    if [ "$DRY_RUN" = "1" ]; then
        echo "[ROTATE]   (DRY-RUN — aucune modification)"
        return
    fi

    # Conserver l'en-tête (première ligne = metadata/headers) + dernières N lignes
    local header
    header=$(head -1 "$f" 2>/dev/null || echo "")
    local tmp
    tmp=$(mktemp)

    if echo "$header" | grep -qiE "^(#|//|timestamp|time|cycle|version|module|METRIC|ANOMALY|HW_SAMPLE)"; then
        # Fichier avec en-tête métadata — garder header + dernières lignes
        { echo "$header"; tail -n "$keep_lines" "$f"; } > "$tmp"
    else
        # Garder uniquement les dernières lignes
        tail -n "$keep_lines" "$f" > "$tmp"
    fi

    local new_size
    new_size=$(stat -c%s "$tmp")
    local freed=$(( (size_bytes - new_size) / 1024 / 1024 ))

    mv "$tmp" "$f"
    TOTAL_ROTATED=$(( TOTAL_ROTATED + 1 ))
    TOTAL_FREED_MB=$(( TOTAL_FREED_MB + freed ))
    echo "[ROTATE]   ✅ ${size_mb} MB → $(( new_size / 1024 / 1024 )) MB (libéré: ${freed} MB)"
}

rotate_csv_file() {
    local f="$1"
    local size_bytes
    size_bytes=$(stat -c%s "$f" 2>/dev/null || echo 0)

    if [ "$size_bytes" -le "$MAX_BYTES" ]; then
        return
    fi

    local size_mb=$(( size_bytes / 1024 / 1024 ))
    echo "[ROTATE] 📊 CSV ${f##$REPO_ROOT/} — ${size_mb} MB → garder 5000 dernières lignes"

    if [ "$DRY_RUN" = "1" ]; then return; fi

    local header
    header=$(head -1 "$f" 2>/dev/null || echo "")
    local tmp
    tmp=$(mktemp)

    { echo "$header"; tail -n 5000 "$f"; } > "$tmp"
    mv "$tmp" "$f"
    TOTAL_ROTATED=$(( TOTAL_ROTATED + 1 ))
}

# Supprimer les anciens logs forensic > 50 MB (garder les 5 plus récents)
prune_old_forensic_logs() {
    local dir="$1"
    [ -d "$dir" ] || return

    # Compter les fichiers > MAX_BYTES
    local all_logs
    all_logs=$(ls -t "$dir"/*.log 2>/dev/null || true)
    if [ -z "$all_logs" ]; then return; fi

    local count=0
    while IFS= read -r f; do
        local size_bytes
        size_bytes=$(stat -c%s "$f" 2>/dev/null || echo 0)
        count=$(( count + 1 ))

        # Garder les 5 plus récents même s'ils sont gros
        if [ "$count" -le 5 ]; then
            rotate_text_file "$f"
            continue
        fi

        # Supprimer les anciens > MAX_BYTES
        if [ "$size_bytes" -gt "$MAX_BYTES" ]; then
            local size_mb=$(( size_bytes / 1024 / 1024 ))
            echo "[ROTATE] 🗑️  Ancien log supprimé : ${f##$REPO_ROOT/} (${size_mb} MB)"
            if [ "$DRY_RUN" = "0" ]; then
                rm -f "$f"
                TOTAL_FREED_MB=$(( TOTAL_FREED_MB + size_mb ))
                TOTAL_ROTATED=$(( TOTAL_ROTATED + 1 ))
            fi
        fi
    done <<< "$all_logs"
}

# ── Parcours des répertoires cibles ──────────────────────────────────
for tdir in "${TARGET_DIRS[@]}"; do
    [ -d "$tdir" ] || continue

    # Logs forensic modulaires (fichiers les plus gros)
    for subdir in modules metrics hw_samples anomalies; do
        if [ -d "$tdir/forensic/$subdir" ]; then
            prune_old_forensic_logs "$tdir/forensic/$subdir"
        fi
    done

    # Logs texte simples dans le dossier
    while IFS= read -r -d '' f; do
        ext="${f##*.}"
        case "$ext" in
            log|txt)  rotate_text_file "$f" ;;
            csv)      rotate_csv_file  "$f" ;;
        esac
    done < <(find "$tdir" -maxdepth 4 -type f \( -name "*.log" -o -name "*.txt" -o -name "*.csv" \) -print0 2>/dev/null)
done

# ── Supprimer les fichiers _part_ab > 50 MB (fragments worm) ─────────
while IFS= read -r -d '' f; do
    local_size=$(stat -c%s "$f" 2>/dev/null || echo 0)
    if [ "$local_size" -gt "$MAX_BYTES" ]; then
        echo "[ROTATE] 🗑️  Fragment supprimé : ${f##$REPO_ROOT/} ($(( local_size/1024/1024 )) MB)"
        if [ "$DRY_RUN" = "0" ]; then rm -f "$f"; fi
    fi
done < <(find "$REPO_ROOT" -maxdepth 6 -type f -name "*_part_a*" -print0 2>/dev/null)

echo "[ROTATE] ============================================"
echo "[ROTATE] ✅ ${TOTAL_ROTATED} fichiers traités — ${TOTAL_FREED_MB} MB libérés"
echo "[ROTATE] ============================================"
