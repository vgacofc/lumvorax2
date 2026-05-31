#!/bin/bash
# MDBAI C164 - Surveillance build kernel et lancement automatique validation complète
# Vérifie toutes les 30s si kernel est prêt, puis lance run_c164_complete_validation.sh

FIRECRACKER_DIR="/tmp/firecracker_c164"
KERNEL_LOG="/tmp/firecracker_kernel_build_c164.log"
VALIDATION_SCRIPT="src/MDBAI/run_c164_complete_validation.sh"

echo "=== MDBAI C164 - SURVEILLANCE BUILD KERNEL ==="
echo "Timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "Vérification toutes les 30 secondes..."
echo ""

ITERATION=0
while true; do
    ((ITERATION++))
    TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ)
    
    # Vérifier si kernel est prêt
    if [ -f "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1" ] || [ -f "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1.bin" ]; then
        echo ""
        echo "=========================================="
        echo "✅ KERNEL 6.1 DÉTECTÉ !"
        echo "Timestamp: $TIMESTAMP"
        echo "=========================================="
        echo ""
        
        ls -lh "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1"* || true
        
        echo ""
        echo "🚀 LANCEMENT VALIDATION COMPLÈTE C164..."
        echo ""
        
        # Rendre le script exécutable et le lancer
        chmod +x "$VALIDATION_SCRIPT"
        bash "$VALIDATION_SCRIPT"
        
        echo ""
        echo "=== VALIDATION C164 TERMINÉE ==="
        exit 0
    fi
    
    # Afficher progression
    if [ -f "$KERNEL_LOG" ]; then
        LINES=$(wc -l < "$KERNEL_LOG")
        LAST_LINE=$(tail -1 "$KERNEL_LOG" | cut -c1-100)
        echo "[$ITERATION] $TIMESTAMP - Log: $LINES lignes - Dernière: $LAST_LINE"
    else
        echo "[$ITERATION] $TIMESTAMP - En attente log kernel..."
    fi
    
    # Vérifier si processus devtool est toujours actif
    if ! pgrep -f "devtool build_ci_artifacts" > /dev/null; then
        echo ""
        echo "⚠️  ATTENTION: Processus devtool non détecté !"
        echo "Vérification si kernel est quand même présent..."
        
        if [ -f "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1" ] || [ -f "$FIRECRACKER_DIR/resources/x86_64/vmlinux-6.1.bin" ]; then
            echo "✅ Kernel trouvé malgré processus arrêté"
            continue
        else
            echo "❌ Kernel non trouvé et processus arrêté"
            echo "Dernières lignes du log:"
            tail -20 "$KERNEL_LOG"
            exit 1
        fi
    fi
    
    # Attendre 30 secondes
    sleep 30
done

# Made with Bob
