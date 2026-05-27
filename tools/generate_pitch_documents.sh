#!/bin/bash
# Script de génération des documents pitch deck LUMVORAX
# Utilise les outils disponibles sur le système

set -e

DOCS_DIR="docs"
PITCH_DECK="$DOCS_DIR/LUMVORAX_PITCH_DECK.md"
PITCH_DECK_V2="$DOCS_DIR/LUMVORAX_PITCH_DECK_ULTRA_COMPLET_V2.md"
EXEC_SUMMARY="$DOCS_DIR/LUMVORAX_EXECUTIVE_SUMMARY.md"
OUTPUT_DIR="$DOCS_DIR/pitch_output"

echo "=========================================="
echo "GÉNÉRATION DOCUMENTS PITCH DECK LUMVORAX"
echo "=========================================="
echo ""

# Créer le dossier de sortie
mkdir -p "$OUTPUT_DIR"

# Vérifier que les fichiers source existent
if [ ! -f "$PITCH_DECK" ]; then
    echo "❌ Erreur: $PITCH_DECK introuvable"
    exit 1
fi

if [ ! -f "$EXEC_SUMMARY" ]; then
    echo "❌ Erreur: $EXEC_SUMMARY introuvable"
    exit 1
fi

if [ ! -f "$PITCH_DECK_V2" ]; then
    echo "❌ Erreur: $PITCH_DECK_V2 introuvable"
    exit 1
fi

echo "✅ Fichiers source trouvés (3 documents)"
echo ""

# Fonction pour vérifier si une commande existe
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Tentative de génération PDF avec pandoc
echo "📄 Tentative génération PDF..."
if command_exists pandoc; then
    echo "   Pandoc trouvé, génération en cours..."
    
    # Pitch deck complet
    if pandoc "$PITCH_DECK" -o "$OUTPUT_DIR/LUMVORAX_PITCH_DECK.pdf" \
        --pdf-engine=xelatex \
        -V geometry:margin=1in \
        -V fontsize=11pt 2>/dev/null; then
        echo "   ✅ PDF pitch deck généré: $OUTPUT_DIR/LUMVORAX_PITCH_DECK.pdf"
    else
        echo "   ⚠️  Échec génération PDF pitch deck (LaTeX manquant?)"
    fi
    
    # Executive summary
    if pandoc "$EXEC_SUMMARY" -o "$OUTPUT_DIR/LUMVORAX_EXECUTIVE_SUMMARY.pdf" \
        --pdf-engine=xelatex \
        -V geometry:margin=1in \
        -V fontsize=11pt 2>/dev/null; then
        echo "   ✅ PDF executive summary généré: $OUTPUT_DIR/LUMVORAX_EXECUTIVE_SUMMARY.pdf"
    else
        echo "   ⚠️  Échec génération PDF executive summary"
    fi
    
    # Pitch deck V2 ultra-complet
    if pandoc "$PITCH_DECK_V2" -o "$OUTPUT_DIR/LUMVORAX_PITCH_DECK_ULTRA_COMPLET_V2.pdf" \
        --pdf-engine=xelatex \
        -V geometry:margin=1in \
        -V fontsize=10pt 2>/dev/null; then
        echo "   ✅ PDF pitch deck V2 généré: $OUTPUT_DIR/LUMVORAX_PITCH_DECK_ULTRA_COMPLET_V2.pdf"
    else
        echo "   ⚠️  Échec génération PDF pitch deck V2"
    fi
else
    echo "   ⚠️  Pandoc non installé, PDF non généré"
    echo "   💡 Installez avec: sudo apt-get install pandoc texlive-xetex"
fi

echo ""

# Tentative de génération DOCX avec pandoc
echo "📝 Tentative génération DOCX..."
if command_exists pandoc; then
    # Pitch deck complet
    if pandoc "$PITCH_DECK" -o "$OUTPUT_DIR/LUMVORAX_PITCH_DECK.docx" 2>/dev/null; then
        echo "   ✅ DOCX pitch deck généré: $OUTPUT_DIR/LUMVORAX_PITCH_DECK.docx"
    else
        echo "   ⚠️  Échec génération DOCX pitch deck"
    fi
    
    # Executive summary
    if pandoc "$EXEC_SUMMARY" -o "$OUTPUT_DIR/LUMVORAX_EXECUTIVE_SUMMARY.docx" 2>/dev/null; then
        echo "   ✅ DOCX executive summary généré: $OUTPUT_DIR/LUMVORAX_EXECUTIVE_SUMMARY.docx"
    else
        echo "   ⚠️  Échec génération DOCX executive summary"
    fi
    
    # Pitch deck V2 ultra-complet
    if pandoc "$PITCH_DECK_V2" -o "$OUTPUT_DIR/LUMVORAX_PITCH_DECK_ULTRA_COMPLET_V2.docx" 2>/dev/null; then
        echo "   ✅ DOCX pitch deck V2 généré: $OUTPUT_DIR/LUMVORAX_PITCH_DECK_ULTRA_COMPLET_V2.docx"
    else
        echo "   ⚠️  Échec génération DOCX pitch deck V2"
    fi
else
    echo "   ⚠️  Pandoc non installé, DOCX non généré"
fi

echo ""

# Génération HTML (toujours possible)
echo "🌐 Génération HTML..."
if command_exists pandoc; then
    # Pitch deck complet
    if pandoc "$PITCH_DECK" -o "$OUTPUT_DIR/LUMVORAX_PITCH_DECK.html" \
        --standalone \
        --css=style.css 2>/dev/null; then
        echo "   ✅ HTML pitch deck généré: $OUTPUT_DIR/LUMVORAX_PITCH_DECK.html"
    else
        echo "   ⚠️  Échec génération HTML pitch deck"
    fi
    
    # Executive summary
    if pandoc "$EXEC_SUMMARY" -o "$OUTPUT_DIR/LUMVORAX_EXECUTIVE_SUMMARY.html" \
        --standalone \
        --css=style.css 2>/dev/null; then
        echo "   ✅ HTML executive summary généré: $OUTPUT_DIR/LUMVORAX_EXECUTIVE_SUMMARY.html"
    else
        echo "   ⚠️  Échec génération HTML executive summary"
    fi
    
    # Pitch deck V2 ultra-complet
    if pandoc "$PITCH_DECK_V2" -o "$OUTPUT_DIR/LUMVORAX_PITCH_DECK_ULTRA_COMPLET_V2.html" \
        --standalone \
        --css=style.css 2>/dev/null; then
        echo "   ✅ HTML pitch deck V2 généré: $OUTPUT_DIR/LUMVORAX_PITCH_DECK_ULTRA_COMPLET_V2.html"
    else
        echo "   ⚠️  Échec génération HTML pitch deck V2"
    fi
else
    echo "   ⚠️  Pandoc non installé, HTML non généré"
fi

echo ""

# Copier les fichiers Markdown dans le dossier de sortie
echo "📋 Copie fichiers Markdown..."
cp "$PITCH_DECK" "$OUTPUT_DIR/"
cp "$PITCH_DECK_V2" "$OUTPUT_DIR/"
cp "$EXEC_SUMMARY" "$OUTPUT_DIR/"
cp "$DOCS_DIR/PITCH_DECK_README.md" "$OUTPUT_DIR/" 2>/dev/null || true
echo "   ✅ Fichiers Markdown copiés (4 fichiers)"

echo ""
echo "=========================================="
echo "RÉSUMÉ"
echo "=========================================="
echo ""
echo "📁 Dossier de sortie: $OUTPUT_DIR"
echo ""
echo "Fichiers disponibles:"
ls -lh "$OUTPUT_DIR" | tail -n +2 | awk '{print "   " $9 " (" $5 ")"}'

echo ""
echo "=========================================="
echo "INSTRUCTIONS"
echo "=========================================="
echo ""
echo "Pour générer PDF/DOCX si pandoc n'est pas installé:"
echo "1. Installez pandoc: sudo apt-get install pandoc texlive-xetex"
echo "2. Relancez ce script: bash $0"
echo ""
echo "Ou utilisez les services en ligne:"
echo "- PDF: https://www.markdowntopdf.com/"
echo "- DOCX: https://products.aspose.app/words/conversion/md-to-docx"
echo ""
echo "Pour les slides PNG, utilisez:"
echo "- Google Slides (import Markdown)"
echo "- Canva (templates présentation)"
echo "- Beautiful.ai (AI-powered slides)"
echo ""
echo "✅ Génération terminée!"

# Made with Bob
