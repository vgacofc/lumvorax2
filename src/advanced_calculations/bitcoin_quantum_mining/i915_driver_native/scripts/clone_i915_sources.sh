#!/bin/bash
# Script de clonage sources driver i915 (kernel Linux)
# Usage: ./clone_i915_sources.sh

set -e

echo "=== CLONAGE SOURCES DRIVER I915 ==="
echo "Date: $(date)"
echo ""

# Vérifier version kernel
KERNEL_VERSION=$(uname -r | cut -d'-' -f1)
echo "📊 Kernel actuel: $KERNEL_VERSION"
echo ""

# Créer répertoire
WORK_DIR="$HOME/i915_driver_workspace"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

echo "📁 Workspace: $WORK_DIR"
echo ""

# Cloner sources kernel (shallow clone pour économiser espace)
if [ ! -d "linux" ]; then
    echo "🚀 Clonage kernel Linux v$KERNEL_VERSION..."
    echo "   (shallow clone, ~500 MB, 5-10 min)"
    
    git clone --depth 1 --branch v$KERNEL_VERSION \
        https://github.com/torvalds/linux.git linux || {
        echo "⚠️  Version exacte introuvable, clonage dernière stable..."
        git clone --depth 1 --branch master \
            https://github.com/torvalds/linux.git linux
    }
    
    echo "✅ Clonage terminé"
else
    echo "✅ Sources déjà clonées"
fi

echo ""
echo "📊 Statistiques:"
echo "   - Taille: $(du -sh linux 2>/dev/null | cut -f1)"
echo "   - Driver i915: $(find linux/drivers/gpu/drm/i915 -name '*.c' 2>/dev/null | wc -l) fichiers .c"
echo ""

# Créer structure projet
echo "📁 Création structure projet..."
mkdir -p patches tools vm tests

# Copier driver i915 dans workspace
if [ ! -d "i915_original" ]; then
    echo "📋 Copie driver i915 original..."
    cp -r linux/drivers/gpu/drm/i915 i915_original
    echo "✅ Copie terminée"
fi

# Créer copie de travail
if [ ! -d "i915_modified" ]; then
    echo "📋 Création copie de travail..."
    cp -r i915_original i915_modified
    echo "✅ Copie créée"
fi

echo ""
echo "✅ ENVIRONNEMENT PRÊT"
echo ""
echo "📁 Structure:"
echo "   $WORK_DIR/"
echo "   ├── linux/                    (sources kernel complètes)"
echo "   ├── i915_original/            (driver i915 original)"
echo "   ├── i915_modified/            (driver i915 modifié)"
echo "   ├── patches/                  (nos patches)"
echo "   ├── tools/                    (outils traçage)"
echo "   ├── vm/                       (config VM)"
echo "   └── tests/                    (tests)"
echo ""
echo "🔍 Fichiers clés i915:"
ls -lh i915_original/*.c 2>/dev/null | head -10 | awk '{print "   "$9" ("$5")"}'
echo ""
echo "📝 Prochaines étapes:"
echo "   1. Analyser i915_gem_execbuffer.c"
echo "   2. Ajouter traçage batch buffers"
echo "   3. Compiler module externe"
echo "   4. Tester en VM"

# Made with Bob
