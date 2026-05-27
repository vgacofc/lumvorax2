#!/bin/bash
# C68-SSH-PERSIST : Clé SSH persistante depuis Doppler ou secrets Replit
# Au lieu de générer une nouvelle clé à chaque session (qui casse GitHub),
# cherche la clé existante dans l'ordre :
#   1. Variable SSH_PRIVATE_KEY (Doppler/Replit secret)
#   2. doppler secrets get SSH_PRIVATE_KEY (si doppler CLI disponible)
#   3. Variable SSH_REPLIT (fallback clé publique uniquement)
#   4. Génère une nouvelle paire UNIQUEMENT si rien n'est trouvé

set -e

mkdir -p ~/.ssh
chmod 700 ~/.ssh

KEY_PATH="$HOME/.ssh/id_ed25519"
FOUND=0

# ─── Étape 1 : Clé privée depuis variable d'environnement ────────────────────
if [ -n "$SSH_PRIVATE_KEY" ] && echo "$SSH_PRIVATE_KEY" | grep -q "BEGIN"; then
    printf '%s\n' "$SSH_PRIVATE_KEY" > "$KEY_PATH"
    chmod 600 "$KEY_PATH"
    ssh-keygen -y -f "$KEY_PATH" > "${KEY_PATH}.pub" 2>/dev/null || true
    echo "[SSH] ✅ Étape 1 : clé privée chargée depuis SSH_PRIVATE_KEY (env)"
    FOUND=1
fi

# ─── Étape 2 : Doppler CLI ────────────────────────────────────────────────────
if [ "$FOUND" -eq 0 ] && command -v doppler &>/dev/null; then
    PRIV=$(doppler secrets get SSH_PRIVATE_KEY --plain 2>/dev/null || true)
    if [ -n "$PRIV" ] && echo "$PRIV" | grep -q "BEGIN"; then
        printf '%s\n' "$PRIV" > "$KEY_PATH"
        chmod 600 "$KEY_PATH"
        ssh-keygen -y -f "$KEY_PATH" > "${KEY_PATH}.pub" 2>/dev/null || true
        echo "[SSH] ✅ Étape 2 : clé privée chargée depuis Doppler (SSH_PRIVATE_KEY)"
        FOUND=1
    fi
fi

# ─── Étape 3 : SSH_REPLIT (clé publique seulement) ────────────────────────────
if [ "$FOUND" -eq 0 ] && [ -n "$SSH_REPLIT" ]; then
    echo "$SSH_REPLIT" > "${KEY_PATH}.pub"
    echo "[SSH] ⚠️  Étape 3 : seulement la clé PUBLIQUE trouvée dans SSH_REPLIT"
    echo "     → git clone OK, git push IMPOSSIBLE sans clé privée"
    echo "     → Lancez : doppler run -- bash a/generate_ssh_key.sh"
    FOUND=1
fi

# ─── Étape 4 : Génération d'urgence ──────────────────────────────────────────
if [ "$FOUND" -eq 0 ]; then
    echo "[SSH] ⚠️  Aucune clé persistante trouvée — génération d'urgence"
    ssh-keygen -t ed25519 -C "lumvorax-replit-$(date +%Y%m%d)" \
               -f "$KEY_PATH" -N ""
    echo "[SSH] 📋 Clé publique à ajouter dans GitHub Settings → SSH keys :"
    echo "──────────────────────────────────────────────────────────────────"
    cat "${KEY_PATH}.pub"
    echo "──────────────────────────────────────────────────────────────────"
    echo "[SSH] Pour rendre cette clé persistante sur Doppler :"
    echo "   doppler secrets set SSH_PRIVATE_KEY=\"\$(cat $KEY_PATH)\""
fi

# ─── Résumé final ─────────────────────────────────────────────────────────────
if [ -f "${KEY_PATH}.pub" ]; then
    PUB=$(cat "${KEY_PATH}.pub")
    echo "[SSH] 📋 Clé publique active : $PUB"
    cp "${KEY_PATH}.pub" ./github_ssh_key.txt
    echo "[SSH] Sauvée → github_ssh_key.txt"
fi

# ─── Config SSH pour GitHub (évite les timeout Replit) ───────────────────────
cat > ~/.ssh/config << 'SSHCFG'
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519
    StrictHostKeyChecking no
    ServerAliveInterval 60
    ServerAliveCountMax 30
    BatchMode yes
SSHCFG
chmod 600 ~/.ssh/config

echo "[SSH] ✅ Config SSH GitHub configurée (ServerAliveInterval=60s → stable)"
echo "[SSH] Test connexion GitHub..."
ssh -T git@github.com 2>&1 | grep -E "Hi|denied|host" || true