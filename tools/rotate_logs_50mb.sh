#!/bin/bash
# ============================================================
# LumVorax — Rotation logs 50MB — C65
# Applique une rotation automatique a TOUS les fichiers log > 50MB
# Aucun fichier de log ne depasse 50MB — pas de LFS
# Usage : bash tools/rotate_logs_50mb.sh [--dry-run]
# ============================================================

set -euo pipefail

DRY_RUN=0
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=1
    echo "[ROTATE] Mode --dry-run : aucune modification effectuee"
fi

MAX_SIZE_MB=50
MAX_SIZE_BYTES=$((MAX_SIZE_MB * 1024 * 1024))
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROTATED=0
SKIPPED=0
ERRORS=0

log_rotate() {
    local file="$1"
    local size_bytes
    size_bytes=$(stat -c%s "$file" 2>/dev/null || echo 0)
    local size_mb=$(( size_bytes / 1024 / 1024 ))

    if (( size_bytes > MAX_SIZE_BYTES )); then
        local ts
        ts=$(date -u +%Y%m%dT%H%M%SZ)
        local archive="${file}.rotated_${ts}.gz"

        echo "[ROTATE] ${file##$REPO_ROOT/} : ${size_mb}MB > ${MAX_SIZE_MB}MB → rotation"

        if [[ $DRY_RUN -eq 0 ]]; then
            # Comprimer les 90% anciens, garder les 10% recents (dernieres lignes)
            local total_lines
            total_lines=$(wc -l < "$file" 2>/dev/null || echo 1000)
            local keep_lines=$(( total_lines / 10 ))
            keep_lines=$(( keep_lines < 100 ? 100 : keep_lines ))

            # Archive des lignes anciennes
            head -n "-${keep_lines}" "$file" | gzip -9 > "${archive}" 2>/dev/null || true

            # Garder uniquement les dernieres lignes dans le fichier actif
            tail -n "${keep_lines}" "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"

            local new_size_mb=$(( $(stat -c%s "$file" 2>/dev/null || echo 0) / 1024 / 1024 ))
            echo "[ROTATE] Apres rotation : ${new_size_mb}MB | Archive : ${archive##$REPO_ROOT/}"
            ROTATED=$(( ROTATED + 1 ))
        fi
    else
        SKIPPED=$(( SKIPPED + 1 ))
    fi
}

echo "[ROTATE] ============================================"
echo "[ROTATE] LumVorax Log Rotation 50MB — C65"
echo "[ROTATE] REPO_ROOT : $REPO_ROOT"
echo "[ROTATE] Seuil : ${MAX_SIZE_MB}MB"
echo "[ROTATE] ============================================"

# Rechercher tous les fichiers log dans src/ et a la racine
while IFS= read -r -d '' file; do
    log_rotate "$file"
done < <(find "$REPO_ROOT/src" "$REPO_ROOT/logs" "$REPO_ROOT/logs_AIMO3" \
    -maxdepth 10 \
    -type f \
    \( -name "*.log" -o -name "*.csv" -o -name "*.json" \) \
    -size +"${MAX_SIZE_MB}M" \
    -print0 2>/dev/null)

# Rechercher aussi dans les logs forensic modules specifiquement
for logdir in \
    "$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/logs" \
    "$REPO_ROOT/src/advanced_calculations/quantum_problem_hubbard_hts/logs" \
    "$REPO_ROOT/src/qdayprize/logs"; do
    if [[ -d "$logdir" ]]; then
        while IFS= read -r -d '' file; do
            log_rotate "$file"
        done < <(find "$logdir" -type f -print0 2>/dev/null)
    fi
done

echo "[ROTATE] ============================================"
echo "[ROTATE] Rotation terminee : ${ROTATED} fichiers traites, ${SKIPPED} ok, ${ERRORS} erreurs"
echo "[ROTATE] ============================================"
