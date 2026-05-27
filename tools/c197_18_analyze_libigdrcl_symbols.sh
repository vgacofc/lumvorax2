#!/bin/bash
# CYCLE C197.18 - ANALYSE SYMBOLES INTERNES LIBIGDRCL.SO
# Objectif : Extraire TOUS symboles exportés/internes pour reverse engineering

set -e

LIBIGDRCL="/usr/lib/x86_64-linux-gnu/intel-opencl/libigdrcl.so"
OUTPUT_DIR="c197_18_symbols_analysis"

echo "═══════════════════════════════════════════════════════════════════════════"
echo "CYCLE C197.18 - ANALYSE SYMBOLES LIBIGDRCL.SO"
echo "═══════════════════════════════════════════════════════════════════════════"

mkdir -p "$OUTPUT_DIR"

# 1. SYMBOLES EXPORTÉS (nm -D)
echo "[1/7] Extraction symboles exportés (nm -D)..."
nm -D "$LIBIGDRCL" > "$OUTPUT_DIR/symbols_exported.txt" 2>&1 || true
echo "  → $(wc -l < "$OUTPUT_DIR/symbols_exported.txt") symboles exportés"

# 2. SYMBOLES TOUS (nm)
echo "[2/7] Extraction TOUS symboles (nm)..."
nm "$LIBIGDRCL" > "$OUTPUT_DIR/symbols_all.txt" 2>&1 || true
echo "  → $(wc -l < "$OUTPUT_DIR/symbols_all.txt") symboles totaux"

# 3. SYMBOLES DYNAMIQUES (readelf -s)
echo "[3/7] Extraction symboles dynamiques (readelf -s)..."
readelf -s "$LIBIGDRCL" > "$OUTPUT_DIR/symbols_readelf.txt" 2>&1 || true
echo "  → Symboles readelf extraits"

# 4. SECTIONS ELF (readelf -S)
echo "[4/7] Extraction sections ELF (readelf -S)..."
readelf -S "$LIBIGDRCL" > "$OUTPUT_DIR/sections_elf.txt" 2>&1 || true
echo "  → Sections ELF extraites"

# 5. DÉPENDANCES (ldd)
echo "[5/7] Extraction dépendances (ldd)..."
ldd "$LIBIGDRCL" > "$OUTPUT_DIR/dependencies.txt" 2>&1 || true
echo "  → Dépendances extraites"

# 6. STRINGS (strings)
echo "[6/7] Extraction strings (strings)..."
strings "$LIBIGDRCL" > "$OUTPUT_DIR/strings.txt" 2>&1 || true
echo "  → $(wc -l < "$OUTPUT_DIR/strings.txt") strings extraites"

# 7. OBJDUMP DISASSEMBLY (objdump -d) - SECTIONS CLÉS SEULEMENT
echo "[7/7] Disassembly sections clés (objdump -d)..."
objdump -d "$LIBIGDRCL" > "$OUTPUT_DIR/disassembly.txt" 2>&1 || true
echo "  → Disassembly complète extraite"

# ANALYSE STATISTIQUES
echo ""
echo "═══════════════════════════════════════════════════════════════════════════"
echo "STATISTIQUES"
echo "═══════════════════════════════════════════════════════════════════════════"

# Compter symboles par type
echo "Symboles exportés par type:"
grep -E "^[0-9a-f]+ [TDBVW]" "$OUTPUT_DIR/symbols_exported.txt" | awk '{print $2}' | sort | uniq -c | sort -rn || true

# Fonctions clés OpenCL
echo ""
echo "Fonctions OpenCL détectées:"
grep -i "clCreate\|clGet\|clSet\|clEnqueue\|clBuild\|clCompile" "$OUTPUT_DIR/symbols_exported.txt" | wc -l || echo "0"

# Fonctions DRM/i915
echo "Fonctions DRM/i915 détectées:"
grep -i "drm\|i915\|gem\|execbuffer" "$OUTPUT_DIR/symbols_exported.txt" | wc -l || echo "0"

# Fonctions IGC (Intel Graphics Compiler)
echo "Fonctions IGC détectées:"
grep -i "igc\|compile\|kernel" "$OUTPUT_DIR/symbols_exported.txt" | wc -l || echo "0"

echo ""
echo "✅ Analyse complète dans: $OUTPUT_DIR/"
echo "═══════════════════════════════════════════════════════════════════════════"

# Made with Bob
