#!/bin/bash
set -e  # Arrêter si erreur
set -x  # Debug mode

# BUG #73 FIX: Unset npm_config_prefix pour compatibilité NVM
unset npm_config_prefix

# Charger NVM
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"

# Utiliser Node.js v22
nvm use v22.22.3

# Charger la clé API Bob
source /home/lvx/LVX/lumvorax2/src/MDBAI/.env.bob

# Vérifier que la clé est chargée
if [ -z "$BOBSHELL_API_KEY" ]; then
  echo "ERREUR: BOBSHELL_API_KEY non chargée"
  exit 1
fi

# BUG #77 FIX: Passer le prompt via stdin (Bob CLI ne peut pas lire de fichiers)
# Le prompt contient déjà le chemin du fichier source-code-complete.txt
# mais Bob CLI ne peut pas le lire, donc on inclut le code source dans le prompt
cat "/home/lvx/LVX/lumvorax2/src/MDBAI/forensic/bob-tasks/mdbai-cadc0773-9e96-4932-bc36-25c1c8538f0b/bob-prompt.md" | bob -y --chat-mode code --output-format json
