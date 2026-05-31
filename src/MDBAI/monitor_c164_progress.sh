#!/bin/bash
# MDBAI C164 - Monitoring progression build kernel + validation système
# Affiche l'avancement en temps réel en %

echo "=== MDBAI C164 - MONITORING PROGRESSION ==="
echo "Timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo ""

# Étapes totales
TOTAL_STEPS=15
COMPLETED=0

# Vérification étape par étape
echo "📊 PROGRESSION GLOBALE C164:"
echo ""

# 1. Clone Firecracker
if [ -d "/tmp/firecracker_c164" ]; then
    echo "✅ [1/15] Clone Firecracker C164"
    ((COMPLETED++))
else
    echo "❌ [1/15] Clone Firecracker C164"
fi

# 2. Build kernel 6.1
if [ -f "/tmp/firecracker_c164/resources/x86_64/vmlinux-6.1" ] || [ -f "/tmp/firecracker_c164/resources/x86_64/vmlinux-6.1.bin" ]; then
    echo "✅ [2/15] Build kernel Linux 6.1"
    ((COMPLETED++))
elif pgrep -f "devtool build_ci_artifacts" > /dev/null; then
    echo "⏳ [2/15] Build kernel Linux 6.1 (EN COURS)"
    # Vérifier progression dans le log
    if [ -f "/tmp/firecracker_kernel_build_c164.log" ]; then
        LINES=$(wc -l < /tmp/firecracker_kernel_build_c164.log)
        echo "   └─ Log: $LINES lignes générées"
        LAST_LINE=$(tail -1 /tmp/firecracker_kernel_build_c164.log)
        echo "   └─ Dernière ligne: $LAST_LINE"
    fi
else
    echo "❌ [2/15] Build kernel Linux 6.1"
fi

# 3. Compilation Firecracker
if [ -f "/tmp/firecracker_c164/build/cargo_target/x86_64-unknown-linux-musl/release/firecracker" ]; then
    echo "✅ [3/15] Compilation Firecracker release"
    ((COMPLETED++))
else
    echo "⏸️  [3/15] Compilation Firecracker release"
fi

# 4. cargo-llvm-cov
if command -v cargo-llvm-cov &> /dev/null; then
    echo "✅ [4/15] Installation cargo-llvm-cov"
    ((COMPLETED++))
else
    echo "⏸️  [4/15] Installation cargo-llvm-cov"
fi

# 5-13. Autres étapes
for i in {5..13}; do
    echo "⏸️  [$i/15] En attente..."
done

# 14. Logs forensiques
if [ -f "/tmp/firecracker_c164_forensic_complete.log" ]; then
    echo "✅ [14/15] Analyse forensique logs"
    ((COMPLETED++))
else
    echo "⏸️  [14/15] Analyse forensique logs"
fi

# 15. Rapport final
if [ -f "src/MDBAI/RAPPORT_FORENSIQUE_COMPLET_C164_*.md" ]; then
    echo "✅ [15/15] Rapport final C164"
    ((COMPLETED++))
else
    echo "⏸️  [15/15] Rapport final C164"
fi

echo ""
PERCENT=$((COMPLETED * 100 / TOTAL_STEPS))
echo "🎯 AVANCEMENT GLOBAL: $COMPLETED/$TOTAL_STEPS étapes ($PERCENT%)"
echo ""

# Détails processus actifs
echo "🔄 PROCESSUS ACTIFS:"
if pgrep -f "devtool" > /dev/null; then
    echo "   • devtool build_ci_artifacts (kernel 6.1)"
fi
if pgrep -f "cargo build" > /dev/null; then
    echo "   • cargo build (compilation Firecracker)"
fi
if pgrep -f "cargo test" > /dev/null; then
    echo "   • cargo test (tests unitaires)"
fi

echo ""
echo "=== FIN MONITORING ==="

# Made with Bob
