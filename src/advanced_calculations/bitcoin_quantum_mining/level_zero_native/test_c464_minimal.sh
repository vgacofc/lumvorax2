#!/bin/bash
# TEST C464 - Batch buffer minimal absolu

cd "$(dirname "$0")"

echo "🚀 Exécution TEST C464 - Batch minimal"
echo "emmaus" | sudo -S ./tests/test_c464_minimal_batch 2>&1 | tee logs/execution_c464_minimal_batch.log

echo ""
echo "📊 Vérification dmesg GPU HANG..."
sudo dmesg | tail -20 | grep -i "i915\|gpu\|hang" || echo "Aucun GPU HANG détecté"

echo ""
echo "✅ Test terminé - Logs: logs/execution_c464_minimal_batch.log"

# Made with Bob
