#!/bin/bash
# generate_tests.sh - Génère squelettes tests pour module Firecracker
# Usage: ./generate_tests.sh <module_name>

set -euo pipefail

MODULE=$1
FIRECRACKER_DIR="/tmp/firecracker_c164"
OUTPUT_DIR="$FIRECRACKER_DIR/tests/generated"

if [ -z "$MODULE" ]; then
    echo "❌ Usage: $0 <module_name>"
    echo "Exemples: virtio_net, snapshot, mmds, vsock, seccomp"
    exit 1
fi

echo "🔍 Génération tests pour module: $MODULE"
echo "📁 Répertoire Firecracker: $FIRECRACKER_DIR"
echo "📁 Répertoire sortie: $OUTPUT_DIR"

# Créer répertoire sortie
mkdir -p "$OUTPUT_DIR"

# Fichier de sortie
OUTPUT_FILE="$OUTPUT_DIR/${MODULE}_tests.rs"
echo "// Tests générés automatiquement pour module: $MODULE" > "$OUTPUT_FILE"
echo "// Date: $(date -u +%Y-%m-%d\ %H:%M:%S\ UTC)" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"
echo "#[cfg(test)]" >> "$OUTPUT_FILE"
echo "mod ${MODULE}_tests {" >> "$OUTPUT_FILE"
echo "    use super::*;" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Compteurs
FUNC_COUNT=0
TEST_COUNT=0

# Rechercher fichiers source du module
echo "🔎 Recherche fichiers source..."

case "$MODULE" in
    virtio_net)
        SEARCH_PATHS=(
            "$FIRECRACKER_DIR/src/vmm/src/devices/virtio/net"
        )
        ;;
    snapshot)
        SEARCH_PATHS=(
            "$FIRECRACKER_DIR/src/vmm/src/snapshot"
        )
        ;;
    mmds)
        SEARCH_PATHS=(
            "$FIRECRACKER_DIR/src/vmm/src/mmds"
        )
        ;;
    vsock)
        SEARCH_PATHS=(
            "$FIRECRACKER_DIR/src/vmm/src/devices/virtio/vsock"
        )
        ;;
    seccomp)
        SEARCH_PATHS=(
            "$FIRECRACKER_DIR/src/seccompiler/src"
        )
        ;;
    *)
        echo "❌ Module inconnu: $MODULE"
        exit 1
        ;;
esac

# Analyser chaque fichier source
for search_path in "${SEARCH_PATHS[@]}"; do
    if [ ! -d "$search_path" ]; then
        echo "⚠️  Chemin non trouvé: $search_path"
        continue
    fi
    
    echo "📂 Analyse: $search_path"
    
    # Trouver tous les fichiers .rs
    find "$search_path" -name "*.rs" -type f | while read -r file; do
        echo "  📄 Fichier: $(basename "$file")"
        
        # Extraire fonctions publiques
        grep -E "^\s*pub\s+fn\s+(\w+)" "$file" | \
            sed -E 's/.*pub\s+fn\s+(\w+).*/\1/' | \
            while read -r func; do
                ((FUNC_COUNT++))
                ((TEST_COUNT++))
                
                echo "    ✓ Fonction: $func"
                
                # Générer test
                cat >> "$OUTPUT_FILE" <<EOF
    #[test]
    fn test_${func}() {
        // TODO: Implémenter test pour fonction: $func
        // Fichier source: $(basename "$file")
        // 
        // Étapes suggérées:
        // 1. Créer contexte de test (mock objects, test data)
        // 2. Appeler la fonction avec paramètres valides
        // 3. Vérifier résultat attendu avec assertions
        // 4. Tester cas d'erreur si applicable
        // 5. Vérifier effets de bord (état modifié, etc.)
        
        todo!("Test à implémenter pour $func");
    }

EOF
            done
        
        # Extraire structures publiques
        grep -E "^\s*pub\s+struct\s+(\w+)" "$file" | \
            sed -E 's/.*pub\s+struct\s+(\w+).*/\1/' | \
            while read -r struct; do
                ((TEST_COUNT++))
                
                echo "    ✓ Structure: $struct"
                
                # Générer test création
                cat >> "$OUTPUT_FILE" <<EOF
    #[test]
    fn test_${struct}_creation() {
        // TODO: Implémenter test création pour structure: $struct
        // Fichier source: $(basename "$file")
        //
        // Étapes suggérées:
        // 1. Créer instance de $struct avec valeurs par défaut
        // 2. Vérifier que tous les champs sont initialisés correctement
        // 3. Tester constructeurs alternatifs si disponibles
        
        todo!("Test création à implémenter pour $struct");
    }

EOF
            done
    done
done

# Fermer module
echo "}" >> "$OUTPUT_FILE"

echo ""
echo "✅ Génération terminée!"
echo "📊 Statistiques:"
echo "   - Fonctions trouvées: $FUNC_COUNT"
echo "   - Tests générés: $TEST_COUNT"
echo "   - Fichier: $OUTPUT_FILE"
echo ""
echo "🚀 Prochaines étapes:"
echo "   1. Ouvrir: $OUTPUT_FILE"
echo "   2. Implémenter les tests marqués TODO"
echo "   3. Exécuter: cd $FIRECRACKER_DIR && cargo test --test $(basename "$OUTPUT_FILE" .rs)"
echo "   4. Mesurer couverture: ./measure_coverage.sh"

# Made with Bob
