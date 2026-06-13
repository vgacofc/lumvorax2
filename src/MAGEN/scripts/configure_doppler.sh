#!/bin/bash
# Configuration Doppler pour secrets MAGEN
# Stockage sécurisé: KAGGLE_API_TOKEN, LUMVORAX_SECRET_KEY, etc.

set -e

echo "🔐 Configuration Doppler pour MAGEN"
echo "===================================="
echo ""

# Vérifier que Doppler est installé
if ! command -v doppler &> /dev/null; then
    echo "❌ Doppler non installé"
    echo ""
    echo "Installation:"
    echo "  # Debian/Ubuntu"
    echo "  curl -sLf --retry 3 --tlsv1.2 --proto '=https' 'https://packages.doppler.com/public/cli/gpg.DE2A7741A397C129.key' | sudo gpg --dearmor -o /usr/share/keyrings/doppler-archive-keyring.gpg"
    echo "  echo 'deb [signed-by=/usr/share/keyrings/doppler-archive-keyring.gpg] https://packages.doppler.com/public/cli/deb/debian any-version main' | sudo tee /etc/apt/sources.list.d/doppler-cli.list"
    echo "  sudo apt-get update && sudo apt-get install doppler"
    echo ""
    echo "  # Ou via script:"
    echo "  curl -Ls --tlsv1.2 --proto '=https' --retry 3 https://cli.doppler.com/install.sh | sudo sh"
    exit 1
fi

echo "✅ Doppler installé: $(doppler --version)"
echo ""

# Vérifier authentification
if ! doppler me &> /dev/null; then
    echo "🔑 Authentification Doppler requise"
    echo "Exécutez: doppler login"
    exit 1
fi

echo "✅ Authentifié: $(doppler me --json | jq -r '.workplace.name' 2>/dev/null || echo 'OK')"
echo ""

# Créer projet MAGEN
PROJECT="magen"
echo "📦 Création projet: $PROJECT"

if doppler projects get "$PROJECT" &> /dev/null; then
    echo "✅ Projet $PROJECT existe déjà"
else
    doppler projects create "$PROJECT" --description "MAGEN - Mémoire Artificielle pour ARC-AGI-3"
    echo "✅ Projet $PROJECT créé"
fi

echo ""

# Créer environnements
CONFIGS=("local" "kaggle" "production")

for CONFIG in "${CONFIGS[@]}"; do
    echo "🔧 Configuration: $CONFIG"
    
    if doppler configs get "$CONFIG" --project "$PROJECT" &> /dev/null; then
        echo "  ✅ Config $CONFIG existe"
    else
        doppler configs create "$CONFIG" --project "$PROJECT"
        echo "  ✅ Config $CONFIG créée"
    fi
done

echo ""
echo "🔑 Configuration des secrets"
echo "=============================="
echo ""

# Fonction pour définir un secret
set_secret() {
    local config=$1
    local key=$2
    local prompt=$3
    local default=$4
    
    echo "📝 $key ($config)"
    
    # Vérifier si le secret existe déjà
    if doppler secrets get "$key" --project "$PROJECT" --config "$config" &> /dev/null; then
        read -p "  Secret existe. Mettre à jour? (y/N): " update
        if [[ ! "$update" =~ ^[Yy]$ ]]; then
            echo "  ⏭️  Ignoré"
            return
        fi
    fi
    
    # Demander la valeur
    if [ -n "$default" ]; then
        read -p "  $prompt [$default]: " value
        value=${value:-$default}
    else
        read -sp "  $prompt: " value
        echo ""
    fi
    
    if [ -z "$value" ]; then
        echo "  ⚠️  Valeur vide - ignoré"
        return
    fi
    
    # Définir le secret
    echo "$value" | doppler secrets set "$key" --project "$PROJECT" --config "$config"
    echo "  ✅ Secret $key défini"
}

# Secrets pour environnement LOCAL
echo "🏠 Environnement: local"
echo "----------------------"

# KAGGLE_API_TOKEN
if [ -f "$HOME/.kaggle/access_token" ]; then
    KAGGLE_TOKEN=$(cat "$HOME/.kaggle/access_token")
    echo "$KAGGLE_TOKEN" | doppler secrets set "KAGGLE_API_TOKEN" --project "$PROJECT" --config "local"
    echo "✅ KAGGLE_API_TOKEN importé depuis ~/.kaggle/access_token"
else
    set_secret "local" "KAGGLE_API_TOKEN" "Token Kaggle (KGAT_...)" ""
fi

# LUMVORAX_SECRET_KEY
set_secret "local" "LUMVORAX_SECRET_KEY" "Clé secrète LumVorax" "magen_lumvorax_$(date +%s)"

# MAGEN_ENCRYPTION_KEY
set_secret "local" "MAGEN_ENCRYPTION_KEY" "Clé chiffrement MAGEN" "magen_encrypt_$(date +%s)"

echo ""
echo "☁️  Environnement: kaggle"
echo "------------------------"

# Copier les secrets de local vers kaggle
doppler secrets download --project "$PROJECT" --config "local" --format json | \
    doppler secrets upload --project "$PROJECT" --config "kaggle"
echo "✅ Secrets copiés de local vers kaggle"

echo ""
echo "🚀 Environnement: production"
echo "---------------------------"

# Copier les secrets de local vers production
doppler secrets download --project "$PROJECT" --config "local" --format json | \
    doppler secrets upload --project "$PROJECT" --config "production"
echo "✅ Secrets copiés de local vers production"

echo ""
echo "✅ Configuration Doppler terminée!"
echo ""
echo "📋 Résumé:"
doppler secrets --project "$PROJECT" --config "local" --only-names

echo ""
echo "🔧 Utilisation:"
echo "  # Exécuter avec secrets local"
echo "  doppler run --project $PROJECT --config local -- python tests/test_magen_real_arc_games.py"
echo ""
echo "  # Exécuter avec secrets kaggle"
echo "  doppler run --project $PROJECT --config kaggle -- python submit_to_kaggle.py"
echo ""
echo "  # Exporter secrets dans .env"
echo "  doppler secrets download --project $PROJECT --config local --format env > .env.local"
echo ""
echo "  # Voir tous les secrets"
echo "  doppler secrets --project $PROJECT --config local"

# Made with Bob
