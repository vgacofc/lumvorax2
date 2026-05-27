
#!/bin/bash
# C197.43 - Script de capture forensique ultra-détaillée
# Compile et exécute la capture complète de toutes les dépendances OpenCL→GPU

set -e

echo "=== C197.43 - ULTRA FORENSIC CAPTURE ==="
echo ""

# Compilation de la bibliothèque forensique
echo "📦 Compilation bibliothèque forensique..."
gcc -shared -fPIC -o /tmp/c197_43_ultra_forensic.so \
