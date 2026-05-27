#!/bin/bash
# Script pour pousser les informations Azure sur Doppler
# Phase 15Y-F28 : Projet C - Azure FPGA
# Accessible sur toutes les sessions et Replit

set -e

echo "=== PUSH AZURE CREDENTIALS TO DOPPLER ==="
echo "Phase 15Y-F28 : Projet C - Azure FPGA"
echo ""

# Configuration Doppler
PROJECT="lumvorax"
CONFIG="dev"

echo "Projet Doppler : $PROJECT"
echo "Config : $CONFIG"
echo ""

# Vérifier si Doppler est installé
if ! command -v doppler &> /dev/null; then
    echo "✗ Doppler n'est pas installé"
    echo "Installation : curl -Ls --tlsv1.2 --proto \"=https\" --retry 3 https://cli.doppler.com/install.sh | sudo sh"
    exit 1
fi

echo "✓ Doppler v$(doppler --version | cut -d' ' -f2) installé"
echo ""

# Activer environnement emmaus
echo "Activation environnement emmaus..."
cd /home/lvx/LVX/lumvorax2
source emmaus/bin/activate

# Vérifier si Azure CLI est installé
if ! command -v az &> /dev/null; then
    echo "✗ Azure CLI n'est pas installé"
    echo "Exécuter d'abord : pip install azure-cli"
    exit 1
fi

echo "✓ Azure CLI v$(az --version | head -n1 | cut -d' ' -f2) installé"
echo ""

# Pousser les informations de configuration Azure
echo "=== PUSH AZURE CONFIGURATION ==="

# 1. Azure CLI version
echo "1. Azure CLI version..."
doppler secrets set AZURE_CLI_VERSION "$(az --version | head -n1 | cut -d' ' -f2)" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 2. Python location
echo "2. Python location..."
doppler secrets set AZURE_PYTHON_PATH "/home/lvx/LVX/lumvorax2/emmaus/bin/python3" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 3. Config directory
echo "3. Azure config directory..."
doppler secrets set AZURE_CONFIG_DIR "/home/lvx/.azure" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 4. Extensions directory
echo "4. Azure extensions directory..."
doppler secrets set AZURE_EXTENSIONS_DIR "/home/lvx/.azure/cliextensions" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 5. Environnement emmaus path
echo "5. Environnement emmaus path..."
doppler secrets set AZURE_VENV_PATH "/home/lvx/LVX/lumvorax2/emmaus" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 6. Activation script
echo "6. Activation script..."
doppler secrets set AZURE_ACTIVATE_CMD "source /home/lvx/LVX/lumvorax2/emmaus/bin/activate" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 7. Guide connexion path
echo "7. Guide connexion path..."
doppler secrets set AZURE_GUIDE_PATH "/home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/azure_fpga/GUIDE_CONNEXION_AZURE.md" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 8. Projet FPGA path
echo "8. Projet FPGA path..."
doppler secrets set AZURE_FPGA_PROJECT_PATH "/home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/azure_fpga" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 9. Status installation
echo "9. Status installation..."
doppler secrets set AZURE_CLI_INSTALLED "true" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 10. Date installation
echo "10. Date installation..."
doppler secrets set AZURE_CLI_INSTALL_DATE "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

echo ""
echo "=== PUSH AZURE FPGA PROJECT INFO ==="

# 11. Projet C status
echo "11. Projet C status..."
doppler secrets set AZURE_FPGA_PROJECT_STATUS "azure_cli_installed_awaiting_login" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 12. Phase actuelle
echo "12. Phase actuelle..."
doppler secrets set AZURE_FPGA_CURRENT_PHASE "15Y-F28-1" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 13. Progression
echo "13. Progression..."
doppler secrets set AZURE_FPGA_PROGRESS "15" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 14. FPGA type recommandé
echo "14. FPGA type recommandé..."
doppler secrets set AZURE_FPGA_RECOMMENDED_TYPE "Standard_NP10s" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 15. Location recommandée
echo "15. Location recommandée..."
doppler secrets set AZURE_FPGA_RECOMMENDED_LOCATION "eastus" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

echo ""
echo "=== PUSH AZURE LOGIN COMMANDS ==="

# 16. Commande login
echo "16. Commande login..."
doppler secrets set AZURE_LOGIN_CMD "az login" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 17. Commande get subscription ID
echo "17. Commande get subscription ID..."
doppler secrets set AZURE_GET_SUBSCRIPTION_CMD "az account show --query id -o tsv" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 18. Commande get tenant ID
echo "18. Commande get tenant ID..."
doppler secrets set AZURE_GET_TENANT_CMD "az account show --query tenantId -o tsv" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 19. Commande list FPGA
echo "19. Commande list FPGA..."
doppler secrets set AZURE_LIST_FPGA_CMD "az vm list-sizes --location eastus --output table | grep NP" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

# 20. Commande create resource group
echo "20. Commande create resource group..."
doppler secrets set AZURE_CREATE_RG_CMD "az group create --name lumvorax-fpga-rg --location eastus" \
    --project "$PROJECT" --config "$CONFIG" 2>&1 | grep -v "Warning"

echo ""
echo "=== VÉRIFICATION ==="

# Lister les secrets Azure
echo "Secrets Azure dans Doppler :"
doppler secrets get --project "$PROJECT" --config "$CONFIG" | grep AZURE || true

echo ""
echo "✅ PUSH DOPPLER COMPLÉTÉ"
echo ""
echo "Pour utiliser dans une autre session :"
echo "  doppler run --project lumvorax --config dev -- bash"
echo ""
echo "Pour utiliser dans Replit :"
echo "  1. Installer Doppler CLI"
echo "  2. doppler login"
echo "  3. doppler setup --project lumvorax --config dev"
echo "  4. doppler run -- python script.py"
echo ""
echo "Variables disponibles :"
echo "  \$AZURE_CLI_VERSION"
echo "  \$AZURE_PYTHON_PATH"
echo "  \$AZURE_CONFIG_DIR"
echo "  \$AZURE_VENV_PATH"
echo "  \$AZURE_ACTIVATE_CMD"
echo "  \$AZURE_GUIDE_PATH"
echo "  \$AZURE_FPGA_PROJECT_PATH"
echo "  \$AZURE_LOGIN_CMD"
echo "  \$AZURE_GET_SUBSCRIPTION_CMD"
echo "  \$AZURE_GET_TENANT_CMD"
echo "  Et plus..."

# Made with Bob
