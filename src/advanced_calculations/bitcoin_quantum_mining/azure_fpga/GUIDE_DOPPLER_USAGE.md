# GUIDE UTILISATION DOPPLER - SECRETS AZURE
## Accès aux informations Azure depuis n'importe quelle session

**Date** : 2026-05-11  
**Projet Doppler** : lumvorax  
**Config** : dev  
**Secrets Azure** : 20 variables disponibles

---

## ✅ SECRETS DISPONIBLES

### Configuration Azure CLI

```bash
AZURE_CLI_VERSION=""                    # Version Azure CLI installée
AZURE_CLI_INSTALLED="true"              # Status installation
AZURE_CLI_INSTALL_DATE="2026-05-11T15:35:38Z"  # Date installation
AZURE_PYTHON_PATH="/home/lvx/LVX/lumvorax2/emmaus/bin/python3"
AZURE_CONFIG_DIR="/home/lvx/.azure"
AZURE_EXTENSIONS_DIR="/home/lvx/.azure/cliextensions"
AZURE_VENV_PATH="/home/lvx/LVX/lumvorax2/emmaus"
AZURE_ACTIVATE_CMD="source /home/lvx/LVX/lumvorax2/emmaus/bin/activate"
```

### Projet FPGA

```bash
AZURE_FPGA_PROJECT_PATH="/home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/azure_fpga"
AZURE_FPGA_PROJECT_STATUS="azure_cli_installed_awaiting_login"
AZURE_FPGA_CURRENT_PHASE="15Y-F28-1"
AZURE_FPGA_PROGRESS="15"
AZURE_FPGA_RECOMMENDED_TYPE="Standard_NP10s"
AZURE_FPGA_RECOMMENDED_LOCATION="eastus"
AZURE_GUIDE_PATH="/home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/azure_fpga/GUIDE_CONNEXION_AZURE.md"
```

### Commandes Azure

```bash
AZURE_LOGIN_CMD="az login"
AZURE_GET_SUBSCRIPTION_CMD="az account show --query id -o tsv"
AZURE_GET_TENANT_CMD="az account show --query tenantId -o tsv"
AZURE_LIST_FPGA_CMD="az vm list-sizes --location eastus --output table | grep NP"
AZURE_CREATE_RG_CMD="az group create --name lumvorax-fpga-rg --location eastus"
```

---

## 🚀 UTILISATION LOCALE

### 1. Lister tous les secrets Azure

```bash
doppler secrets --project lumvorax --config dev | grep AZURE
```

### 2. Obtenir une variable spécifique

```bash
doppler secrets get AZURE_FPGA_PROJECT_PATH --project lumvorax --config dev --plain
```

### 3. Exécuter une commande avec les variables

```bash
doppler run --project lumvorax --config dev -- bash -c 'echo $AZURE_FPGA_PROJECT_PATH'
```

### 4. Lancer un shell avec toutes les variables

```bash
doppler run --project lumvorax --config dev -- bash
# Maintenant toutes les variables AZURE_* sont disponibles
echo $AZURE_LOGIN_CMD
echo $AZURE_FPGA_RECOMMENDED_TYPE
```

### 5. Exécuter le script de connexion Azure

```bash
doppler run --project lumvorax --config dev -- bash -c '
  eval $AZURE_ACTIVATE_CMD
  eval $AZURE_LOGIN_CMD
'
```

---

## 🔄 UTILISATION REPLIT

### Étape 1 : Installer Doppler CLI sur Replit

```bash
# Dans le terminal Replit
curl -Ls --tlsv1.2 --proto "=https" --retry 3 https://cli.doppler.com/install.sh | sudo sh
```

### Étape 2 : Se connecter à Doppler

```bash
doppler login
```

Une URL s'ouvrira pour l'authentification. Suivez les instructions.

### Étape 3 : Configurer le projet

```bash
doppler setup --project lumvorax --config dev
```

### Étape 4 : Utiliser les secrets

```bash
# Méthode 1 : Exécuter avec doppler run
doppler run -- python mon_script.py

# Méthode 2 : Exporter dans l'environnement
eval $(doppler secrets download --no-file --format env)
echo $AZURE_FPGA_PROJECT_PATH
```

### Étape 5 : Script Python avec Doppler

```python
#!/usr/bin/env python3
import os
import subprocess

# Les variables sont automatiquement disponibles si lancé avec doppler run
azure_project_path = os.getenv('AZURE_FPGA_PROJECT_PATH')
azure_login_cmd = os.getenv('AZURE_LOGIN_CMD')
azure_fpga_type = os.getenv('AZURE_FPGA_RECOMMENDED_TYPE')

print(f"Projet FPGA : {azure_project_path}")
print(f"Type FPGA recommandé : {azure_fpga_type}")

# Exécuter commande Azure
subprocess.run(azure_login_cmd, shell=True)
```

**Lancer avec** :
```bash
doppler run -- python mon_script.py
```

---

## 📦 UTILISATION DOCKER

### Dockerfile avec Doppler

```dockerfile
FROM python:3.12

# Installer Doppler
RUN apt-get update && apt-get install -y curl && \
    curl -Ls --tlsv1.2 --proto "=https" --retry 3 https://cli.doppler.com/install.sh | sh

# Copier le code
COPY . /app
WORKDIR /app

# Le token Doppler sera passé en variable d'environnement
CMD ["doppler", "run", "--", "python", "app.py"]
```

**Lancer** :
```bash
docker run -e DOPPLER_TOKEN="dp.st.xxxx" mon_image
```

---

## 🔐 SÉCURITÉ

### Obtenir un Service Token

Pour automatisation (CI/CD, Replit, Docker) :

```bash
# Créer un service token
doppler configs tokens create lumvorax-dev --project lumvorax --config dev
```

**Utiliser le token** :
```bash
export DOPPLER_TOKEN="dp.st.xxxx"
doppler secrets --plain
```

### Best Practices

1. ✅ **Ne jamais commit** le token Doppler
2. ✅ **Utiliser service tokens** pour automatisation
3. ✅ **Rotation régulière** des tokens
4. ✅ **Audit logs** activés
5. ✅ **Accès minimal** (principe du moindre privilège)

---

## 🔄 MISE À JOUR DES SECRETS

### Ajouter/Modifier un secret

```bash
doppler secrets set AZURE_NEW_SECRET "valeur" --project lumvorax --config dev
```

### Supprimer un secret

```bash
doppler secrets delete AZURE_OLD_SECRET --project lumvorax --config dev
```

### Bulk update depuis fichier

```bash
# Créer fichier .env
cat > azure_secrets.env << EOF
AZURE_NEW_VAR1=value1
AZURE_NEW_VAR2=value2
EOF

# Upload
doppler secrets upload azure_secrets.env --project lumvorax --config dev
```

---

## 📊 MONITORING

### Voir l'historique des changements

```bash
doppler activity --project lumvorax --config dev
```

### Voir qui a accès

```bash
doppler projects members --project lumvorax
```

---

## 🛠️ SCRIPTS UTILES

### Script 1 : Vérifier disponibilité secrets

```bash
#!/bin/bash
# check_azure_secrets.sh

REQUIRED_SECRETS=(
    "AZURE_CLI_VERSION"
    "AZURE_FPGA_PROJECT_PATH"
    "AZURE_LOGIN_CMD"
    "AZURE_FPGA_RECOMMENDED_TYPE"
)

echo "Vérification secrets Azure..."
for secret in "${REQUIRED_SECRETS[@]}"; do
    value=$(doppler secrets get "$secret" --project lumvorax --config dev --plain 2>/dev/null)
    if [ -n "$value" ]; then
        echo "✓ $secret : OK"
    else
        echo "✗ $secret : MANQUANT"
    fi
done
```

### Script 2 : Setup Azure depuis Doppler

```bash
#!/bin/bash
# setup_azure_from_doppler.sh

echo "Setup Azure depuis Doppler..."

# Charger les variables
eval $(doppler secrets download --no-file --format env --project lumvorax --config dev)

# Activer environnement
eval $AZURE_ACTIVATE_CMD

# Vérifier Azure CLI
if command -v az &> /dev/null; then
    echo "✓ Azure CLI installé : $(az --version | head -n1)"
else
    echo "✗ Azure CLI non installé"
    exit 1
fi

# Afficher infos projet
echo ""
echo "Projet FPGA : $AZURE_FPGA_PROJECT_PATH"
echo "Phase actuelle : $AZURE_FPGA_CURRENT_PHASE"
echo "Progression : $AZURE_FPGA_PROGRESS%"
echo "Type FPGA recommandé : $AZURE_FPGA_RECOMMENDED_TYPE"
echo "Location recommandée : $AZURE_FPGA_RECOMMENDED_LOCATION"
echo ""
echo "Commande login : $AZURE_LOGIN_CMD"
```

### Script 3 : Export pour CI/CD

```bash
#!/bin/bash
# export_azure_for_ci.sh

# Export toutes les variables Azure pour CI/CD
doppler secrets download --no-file --format env --project lumvorax --config dev | grep AZURE > azure_ci.env

echo "Variables exportées dans azure_ci.env"
echo "Utiliser avec : source azure_ci.env"
```

---

## 🌐 INTÉGRATION CI/CD

### GitHub Actions

```yaml
name: Azure FPGA Deploy

on: [push]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install Doppler CLI
        uses: dopplerhq/cli-action@v1
      
      - name: Load secrets
        run: doppler secrets download --no-file --format env
        env:
          DOPPLER_TOKEN: ${{ secrets.DOPPLER_TOKEN }}
      
      - name: Deploy to Azure
        run: |
          eval $AZURE_ACTIVATE_CMD
          eval $AZURE_LOGIN_CMD
          # Vos commandes de déploiement
```

### GitLab CI

```yaml
deploy:
  image: python:3.12
  before_script:
    - curl -Ls https://cli.doppler.com/install.sh | sh
    - doppler secrets download --no-file --format env > .env
    - source .env
  script:
    - eval $AZURE_ACTIVATE_CMD
    - eval $AZURE_LOGIN_CMD
    # Vos commandes de déploiement
  variables:
    DOPPLER_TOKEN: $DOPPLER_TOKEN
```

---

## 📱 ACCÈS MOBILE

### Doppler Dashboard

1. Aller sur https://dashboard.doppler.com
2. Se connecter
3. Projet : lumvorax
4. Config : dev
5. Voir/Modifier secrets Azure

---

## 🆘 DÉPANNAGE

### Erreur "Doppler not found"

```bash
# Réinstaller
curl -Ls --tlsv1.2 --proto "=https" --retry 3 https://cli.doppler.com/install.sh | sudo sh
```

### Erreur "Unauthorized"

```bash
# Se reconnecter
doppler logout
doppler login
```

### Erreur "Project not found"

```bash
# Vérifier projets disponibles
doppler projects

# Setup correct
doppler setup --project lumvorax --config dev
```

### Variables non chargées

```bash
# Vérifier que doppler run est utilisé
doppler run -- env | grep AZURE

# Ou export manuel
eval $(doppler secrets download --no-file --format env)
```

---

## 📚 RESSOURCES

- **Doppler Docs** : https://docs.doppler.com
- **Doppler CLI** : https://docs.doppler.com/docs/cli
- **Guide Azure** : [`GUIDE_CONNEXION_AZURE.md`](./GUIDE_CONNEXION_AZURE.md)
- **Rapport Phase F28** : [`RAPPORT_C198_PHASE_F28_AZURE_CLI_SETUP.md`](./CHAT/RAPPORT_C198_PHASE_F28_AZURE_CLI_SETUP.md)

---

## ✅ CHECKLIST UTILISATION

### Première utilisation

- [ ] Doppler CLI installé
- [ ] `doppler login` effectué
- [ ] `doppler setup --project lumvorax --config dev` effectué
- [ ] Test : `doppler secrets | grep AZURE` affiche 20 variables
- [ ] Test : `doppler run -- env | grep AZURE` affiche les valeurs

### Utilisation quotidienne

- [ ] Lancer shell : `doppler run --project lumvorax --config dev -- bash`
- [ ] Vérifier variables : `echo $AZURE_FPGA_PROJECT_PATH`
- [ ] Activer Azure : `eval $AZURE_ACTIVATE_CMD`
- [ ] Login Azure : `eval $AZURE_LOGIN_CMD`

---

**TOUS LES SECRETS AZURE SONT MAINTENANT ACCESSIBLES PARTOUT !**

- ✅ Sessions locales
- ✅ Replit
- ✅ Docker
- ✅ CI/CD
- ✅ Mobile (Dashboard)