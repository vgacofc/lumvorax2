
#!/bin/bash
# Script conforme STANDARD_NAMES.md et prompt.txt

echo "🗂️ === PRÉPARATION STRUCTURE LOGS EXTENSIONS ==="
SESSION=$(date +%Y%m%d_%H%M%S)

# Création structure logs selon STANDARD_NAMES.md
mkdir -p logs/{extensions,compilation,execution,validation,forensic}
mkdir -p logs/extensions/{persistance,wal,recovery,integration}

# Configuration session
echo "extensions_complete_${SESSION}" > logs/current_session.txt
echo "📋 Session créée: extensions_complete_${SESSION}"

# Templates logs forensiques
cat > logs/log_template_extension.txt << 'EOF'
=== EXTENSION TEST LOG - %TEST_NAME% ===
Date: %TIMESTAMP%
Session: %SESSION%
Module: %MODULE_NAME%
Status: %STATUS%
Metrics: %METRICS%
EOF

echo "✅ Structure logs préparée pour extensions"
echo "📁 Dossiers créés:"
ls -la logs/
ls -la logs/extensions/
