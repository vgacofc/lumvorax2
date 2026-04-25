#!/usr/bin/env bash
# generate_ssh_key.sh — C96 : génération clé SSH ed25519 pour push GitHub auto.
#
# Stratégie :
#   1. Si SSH_REPLIT contient déjà la clé privée → l'écrire dans ~/.ssh/id_ed25519
#   2. Sinon, générer une nouvelle paire ed25519 et afficher la pub à coller
#      dans GitHub > Settings > SSH and GPG keys
#   3. Configurer SSH pour github.com (StrictHostKeyChecking accept-new)
#   4. Tester la connexion et basculer le remote git origin sur SSH si HTTPS
#
# Usage :
#   bash generate_ssh_key.sh             # génère si absent + setup
#   bash generate_ssh_key.sh --rotate    # force régénération
#   bash generate_ssh_key.sh --doppler   # secondaire : push vers Doppler aussi
#
# Variables environnement attendues (Replit Secrets ou Doppler) :
#   SSH_REPLIT       (optionnel, contenu OpenSSH PRIVATE KEY)
#   GITHUB_USER      (optionnel, défaut = $REPLIT_USER ou $USER)
#   GITHUB_REPO      (optionnel, défaut = lumvorax2)
#   DOPPLER_TOKEN    (pour push secondaire de la pub key vers Doppler)

set -uo pipefail

SSH_DIR="${HOME}/.ssh"
KEY_PATH="${SSH_DIR}/id_ed25519"
KEY_PATH_PUB="${KEY_PATH}.pub"
GITHUB_USER="${GITHUB_USER:-${REPLIT_USER:-${USER:-lumvorax}}}"
GITHUB_REPO="${GITHUB_REPO:-lumvorax2}"
ROTATE=0
DOPPLER_PUSH=0

for arg in "$@"; do
  case "$arg" in
    --rotate)  ROTATE=1 ;;
    --doppler) DOPPLER_PUSH=1 ;;
    --help|-h)
      sed -n '2,18p' "$0"
      exit 0
      ;;
  esac
done

mkdir -p "$SSH_DIR"
chmod 700 "$SSH_DIR"

log() { printf '[ssh-keygen-c96] %s\n' "$*" >&2; }

# 1. Récupération depuis SSH_REPLIT si présent et pas de rotation forcée
if [ -n "${SSH_REPLIT:-}" ] && [ "$ROTATE" -eq 0 ] && [ ! -f "$KEY_PATH" ]; then
  log "Restauration clé depuis SSH_REPLIT (Replit Secret)"
  printf '%s' "$SSH_REPLIT" > "$KEY_PATH"
  chmod 600 "$KEY_PATH"
  ssh-keygen -y -f "$KEY_PATH" > "$KEY_PATH_PUB" 2>/dev/null || {
    log "ERR: SSH_REPLIT ne contient pas une clé OpenSSH valide"
    rm -f "$KEY_PATH"
  }
fi

# 2. Génération si absente ou rotation
if [ ! -f "$KEY_PATH" ] || [ "$ROTATE" -eq 1 ]; then
  log "Génération nouvelle paire ed25519"
  rm -f "$KEY_PATH" "$KEY_PATH_PUB"
  ssh-keygen -t ed25519 -N '' \
    -C "lumvorax-c96-${GITHUB_USER}@$(hostname 2>/dev/null || echo replit)" \
    -f "$KEY_PATH" >/dev/null
fi

chmod 600 "$KEY_PATH"
chmod 644 "$KEY_PATH_PUB"

# 3. Config SSH pour github.com
CONF="${SSH_DIR}/config"
touch "$CONF"
chmod 600 "$CONF"
if ! grep -q "Host github.com" "$CONF" 2>/dev/null; then
  cat >> "$CONF" <<'EOF'

Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519
    IdentitiesOnly yes
    StrictHostKeyChecking accept-new
    ServerAliveInterval 30
EOF
  log "SSH config github.com ajouté"
fi

# 4. Affichage pub key (à coller dans GitHub)
log "Clé publique (à ajouter sur https://github.com/settings/ssh/new) :"
cat "$KEY_PATH_PUB"

# 5. Test connexion (succès = exit 1 avec message "successfully authenticated")
log "Test SSH GitHub :"
ssh -o BatchMode=yes -o ConnectTimeout=8 -T git@github.com 2>&1 | head -3 || true

# 6. Bascule remote origin sur SSH si HTTPS
if git rev-parse --git-dir >/dev/null 2>&1; then
  CUR=$(git remote get-url origin 2>/dev/null || echo "")
  if echo "$CUR" | grep -q '^https://github.com/'; then
    NEW=$(echo "$CUR" | sed -E 's#https://github.com/#git@github.com:#; s#\.git$##')".git"
    log "Bascule origin: $CUR → $NEW"
    git remote set-url origin "$NEW"
  fi
fi

# 7. Push secondaire pub key vers Doppler (option de secours)
if [ "$DOPPLER_PUSH" -eq 1 ] && [ -n "${DOPPLER_TOKEN:-}" ]; then
  log "Push pub key → Doppler (secret SSH_PUBLIC_KEY_C96)"
  PUB_B64=$(base64 -w0 < "$KEY_PATH_PUB")
  curl -s -X POST "https://api.doppler.com/v3/configs/config/secrets" \
    -H "Authorization: Bearer ${DOPPLER_TOKEN}" \
    -H "Content-Type: application/json" \
    -d "{\"secrets\":{\"SSH_PUBLIC_KEY_C96\":{\"value\":\"$(cat "$KEY_PATH_PUB")\"},\"SSH_PUBLIC_KEY_C96_B64\":{\"value\":\"${PUB_B64}\"}}}" \
    | head -c 400
  echo
fi

log "OK — clé SSH prête : $KEY_PATH (pub: $KEY_PATH_PUB)"
log "Pour activer push auto depuis Ubuntu : exporter SSH_REPLIT=\$(cat $KEY_PATH | base64 -w0)"
