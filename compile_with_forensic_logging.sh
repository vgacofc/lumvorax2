
#!/bin/bash

source prepare_forensic_logs.sh
SESSION=$(cat logs/current_session_forensic.txt)

echo "=== COMPILATION AVEC LOGGING FORENSIQUE COMPLET ==="
echo "Session: $SESSION"
echo "Timestamp: $(date -u)"

# Métadonnées système
COMPILER_VERSION=$(clang --version | head -1)
OS_INFO=$(uname -a)
ARCH=$(uname -m)

# Génération checksums sources AVANT compilation
echo "📊 Génération checksums sources..."
find src/ -name "*.c" -o -name "*.h" | sort | xargs sha256sum > "logs/forensic/validation/source_checksums_${SESSION}.txt"

# Compilation avec logs détaillés
echo "🔨 Compilation avec logs forensiques..."
TIMESTAMP_START=$(date -u +%Y%m%d_%H%M%S)

make clean 2>&1 | tee "logs/forensic/compilation/clean_${SESSION}.log"
make all 2>&1 | tee "logs/forensic/compilation/build_${SESSION}.log"

COMPILE_STATUS=$?
TIMESTAMP_END=$(date -u +%Y%m%d_%H%M%S)

# Génération checksum binaire si compilation réussie
if [ $COMPILE_STATUS -eq 0 ] && [ -f bin/lum_vorax ]; then
    BINARY_CHECKSUM=$(sha256sum bin/lum_vorax | cut -d' ' -f1)
    BINARY_SIZE=$(ls -la bin/lum_vorax | awk '{print $5}')
    echo "✅ Binaire généré: $BINARY_SIZE bytes, checksum: $BINARY_CHECKSUM"
else
    BINARY_CHECKSUM="COMPILATION_FAILED"
    echo "❌ Compilation échouée"
fi

# Génération métadonnées complètes
SOURCE_CHECKSUM=$(cat "logs/forensic/validation/source_checksums_${SESSION}.txt" | sha256sum | cut -d' ' -f1)

sed "s/%SESSION_ID%/$SESSION/g; \
     s/%TIMESTAMP_START%/$TIMESTAMP_START/g; \
     s/%TIMESTAMP_END%/$TIMESTAMP_END/g; \
     s/%OS_INFO%/$OS_INFO/g; \
     s/%COMPILER_VERSION%/$COMPILER_VERSION/g; \
     s/%ARCH%/$ARCH/g; \
     s/%SOURCE_CHECKSUM%/$SOURCE_CHECKSUM/g; \
     s/%BINARY_CHECKSUM%/$BINARY_CHECKSUM/g" \
     logs/forensic/metadata_template.json > "logs/forensic/compilation/metadata_${SESSION}.json"

echo "📋 Métadonnées forensiques sauvegardées: logs/forensic/compilation/metadata_${SESSION}.json"
echo "🔍 Logs compilation: logs/forensic/compilation/build_${SESSION}.log"

exit $COMPILE_STATUS
