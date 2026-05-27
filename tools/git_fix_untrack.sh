#!/usr/bin/env bash
# ============================================================
# LumVorax — Fix unique : désindexer les fichiers runtime Ubuntu de git
#
# À exécuter UNE SEULE FOIS sur Ubuntu après git pull/reset.
# Après ce script, les conflits git pull disparaissent définitivement.
#
# PROBLÈME résolu :
#   - .o compilés Replit → commités → SIGILL Ubuntu (AVX-512 incompatible)
#   - CSV/logs runtime Ubuntu → conflit merge à chaque git pull
#
# Usage (Fish shell Ubuntu) :
#   bash tools/git_fix_untrack.sh
# ============================================================

set -euo pipefail
echo "[GIT-FIX] Désindexage fichiers runtime Ubuntu..."

# ─── Désindexer les .o pré-compilés QDAYPRIZE ───────────────
git rm --cached src/qdayprize/main_qdayprize.o 2>/dev/null && echo "[OK] main_qdayprize.o désindexé" || echo "[SKIP] main_qdayprize.o non tracé"
git rm --cached src/qdayprize/qdayprize_engine.o 2>/dev/null && echo "[OK] qdayprize_engine.o désindexé" || echo "[SKIP] qdayprize_engine.o non tracé"

# ─── Désindexer les binaires compilés ───────────────────────
git rm --cached src/qdayprize/qdayprize_runner 2>/dev/null && echo "[OK] qdayprize_runner désindexé" || echo "[SKIP]"
git rm --cached src/advanced_calculations/bitcoin_quantum_mining/btc_mining_runner 2>/dev/null && echo "[OK] btc_mining_runner désindexé" || echo "[SKIP]"

# ─── Désindexer le CSV NX48 (modifié à chaque run Ubuntu) ───
git rm --cached src/advanced_calculations/bitcoin_quantum_mining/config/btc_nx48_last.csv 2>/dev/null && echo "[OK] btc_nx48_last.csv désindexé" || echo "[SKIP] btc_nx48_last.csv non tracé"

# ─── Désindexer les logs runtime (metrics, hw, anomalies) ───
git rm --cached --quiet src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/anomalies/btc_qm_engine_anomalies.log 2>/dev/null && echo "[OK] anomalies.log désindexé" || echo "[SKIP]"
git rm --cached --quiet src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/hw_samples/btc_qm_engine_hw.log 2>/dev/null && echo "[OK] hw.log désindexé" || echo "[SKIP]"
git rm --cached --quiet src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/metrics/btc_qm_engine_metrics.log 2>/dev/null && echo "[OK] metrics.log désindexé" || echo "[SKIP]"

echo ""
echo "[GIT-FIX] ✅ Désindexage terminé."
echo "[GIT-FIX] Ces fichiers sont maintenant ignorés par git (via .gitignore)."
echo "[GIT-FIX] Les prochains 'git pull' seront sans conflit."
echo ""
echo "[GIT-FIX] → Lance maintenant un commit pour confirmer :"
echo "   git add .gitignore && git commit -m 'fix: untrack runtime files (C59)' && git push origin main"
