#!/bin/bash

set -e

# dossier ssh
mkdir -p ~/.ssh
chmod 700 ~/.ssh

# génération de la clé
ssh-keygen -t ed25519 -C "vgac4224@outlook.com" -f ~/.ssh/id_ed25519 -N ""

# copie de la clé publique dans un fichier du projet
cp ~/.ssh/id_ed25519.pub ./github_ssh_key.txt

echo "SSH key generated."
echo "Public key saved in: github_ssh_key.txt"