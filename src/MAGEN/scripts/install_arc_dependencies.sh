#!/bin/bash
# Installation des dépendances ARC-AGI-3 pour MAGEN
# Exécution: bash scripts/install_arc_dependencies.sh

set -e  # Arrêt sur erreur

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAGEN_ROOT="$(dirname "$SCRIPT_DIR")"
VENV_DIR="$MAGEN_ROOT/venv"
WHEELS_DIR="$MAGEN_ROOT/data/arc_agi_3/arc_agi_3_wheels"

echo "🔧 Installation dépendances ARC-AGI-3 pour MAGEN"
echo "================================================"
echo "📁 Racine MAGEN: $MAGEN_ROOT"
echo "🐍 Venv: $VENV_DIR"
echo "📦 Wheels: $WHEELS_DIR"
echo ""

# Vérifier que le venv existe
if [ ! -d "$VENV_DIR" ]; then
    echo "❌ Environnement virtuel introuvable: $VENV_DIR"
    echo "Création du venv..."
    python3 -m venv "$VENV_DIR"
    echo "✅ Venv créé"
fi

# Activer le venv
source "$VENV_DIR/bin/activate"

echo "📦 Installation des dépendances de base..."
pip install --upgrade pip setuptools wheel

# Installer requirements.txt si existe
if [ -f "$MAGEN_ROOT/requirements.txt" ]; then
    echo "📦 Installation depuis requirements.txt..."
    pip install -r "$MAGEN_ROOT/requirements.txt"
fi

# Vérifier que les wheels existent
if [ ! -d "$WHEELS_DIR" ]; then
    echo "❌ Répertoire wheels introuvable: $WHEELS_DIR"
    echo "Téléchargez d'abord le dataset ARC-AGI-3:"
    echo "  python scripts/download_arc_dataset.py"
    exit 1
fi

# Compter les wheels
WHEEL_COUNT=$(ls -1 "$WHEELS_DIR"/*.whl 2>/dev/null | wc -l)
if [ "$WHEEL_COUNT" -eq 0 ]; then
    echo "❌ Aucun wheel trouvé dans: $WHEELS_DIR"
    exit 1
fi

echo "📦 $WHEEL_COUNT wheels trouvés"
echo ""

# Installer les wheels ARC-AGI-3
echo "📦 Installation des wheels ARC-AGI-3..."
pip install "$WHEELS_DIR"/*.whl

echo ""
echo "✅ Installation terminée!"
echo ""
echo "🧪 Test de l'installation:"
python3 -c "
import sys
try:
    from arc_agi import EnvironmentWrapper
    from arcengine import GameAction, GameState, FrameData
    print('✅ arc_agi importé avec succès')
    print('✅ arcengine importé avec succès')
    sys.exit(0)
except ImportError as e:
    print(f'❌ Erreur import: {e}')
    sys.exit(1)
"

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 Installation réussie!"
    echo ""
    echo "📝 Prochaines étapes:"
    echo "  1. Tester l'adaptateur:"
    echo "     python adapters/arc_agi_adapter.py"
    echo ""
    echo "  2. Exécuter les tests MAGEN:"
    echo "     python tests/test_magen_real_arc_games.py --games 3"
    echo ""
    echo "  3. Mode forensique complet:"
    echo "     python tests/test_magen_real_arc_games.py --games 5 --forensic-full"
else
    echo ""
    echo "❌ Problème d'installation détecté"
    echo "Vérifiez les logs ci-dessus"
    exit 1
fi

# Made with Bob
