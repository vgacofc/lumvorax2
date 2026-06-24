#!/bin/bash
# Script pour pousser tous les secrets MDBAI depuis .env vers Doppler
# Usage: ./push_secrets_to_doppler.sh

set -e

echo "═══════════════════════════════════════════════════════"
echo "  Push Secrets MDBAI vers Doppler"
echo "  Projet: mdbai | Config: dev"
echo "═══════════════════════════════════════════════════════"

# Vérifier que le fichier .env existe
if [ ! -f .env ]; then
    echo "❌ Erreur: Fichier .env introuvable"
    exit 1
fi

echo "✅ Fichier .env trouvé"

# Extraire et pousser les secrets MDBAI
echo ""
echo "🔐 Push des secrets MDBAI..."

# MDBAI GitHub App
doppler secrets set MDBAI_APP_ID="3888479" --project mdbai --config dev --silent
echo "   ✅ MDBAI_APP_ID"

doppler secrets set MDBAI_CLIENT_ID="Iv23liM06X4pQnng7oFm" --project mdbai --config dev --silent
echo "   ✅ MDBAI_CLIENT_ID"

doppler secrets set MDBAI_CLIENT_SECRET="25a467c860016f63afedbbbaec6246fc2a85d51b" --project mdbai --config dev --silent
echo "   ✅ MDBAI_CLIENT_SECRET"

doppler secrets set MDBAI_WEBHOOK_SECRET="92b153c1a278ee8eddf9b501956c32ec9e7f5426aff28f24ced156bcf401c441" --project mdbai --config dev --silent
echo "   ✅ MDBAI_WEBHOOK_SECRET"

# MDBAI Private Key (multiline)
MDBAI_PRIVATE_KEY="-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEA6ofsLiN1KoCJw+9R06ZmHaPi811wXMfue71UdbbyczjkJESo
gMLnkKLI7dCEiz0OouplhzQtqq1cVWIW+f8XMJtEOPYhC4hDSeqktuO3GI5PJFZs
ZsAUuYU2LczPYsRefmS+mZzTu+3Z10MOX7qDe9elpCW466jwchhXm4OS59ArCFqt
TDtVuu3mRR9LaHn+nSWH5ZIZs88de73FyqIcwG9N/p0iWtNzj0lwKnfZBcs1NX7E
GyjOYpAN/2UIhFyU5f14fFVZ+isMZqDO4/WM6hciVX2PrGxYFWaN0W+U31STvUcb
YwPxoEYYiYSgpQHYnxxY+vbEyKmWC9O16hasmwIDAQABAoIBADYjGgmsdadR9Hse
Cy1oA7pj0keLEhzYqdw2YACsMlsWjtMwZhUS6JEigm8i6vWZKn/Ay57iFIiJa/7p
nZQnphq6PI1Jg2c+FQpC97Kf7evlLa+do3g0bpREg7UeyeBqo8VZTAX5hYaj6N0f
xXmjfCZiKQIzEvSTEq3DoLVtMcQAgEgdKheJogqqkD4onPl/Ui4B8Jln3PwknSsU
qPnKZizTYBZJLEeAMg6+nMNnWW9POczlFvcv7rF8063sAS7eQJ+JdGDxJXFjqzCf
ApaeXGPgp0cDk2wEJh8Rl7ckUjB6pvWE7+AP/BCaWU1mxJRhS6umspkQN5sZ4YGz
GmuHNQECgYEA+mbJlKaXp70P6DaxIdx7Vm5bob6+0bOvAT6oslIkDMAlrxq1bFID
SPTE2hazkb2/liBoljeemDVZ4QbxPNyh6kPQEKyfL+7IgdhVhSiBcgTigcl458C4
4ZKL239CMFXSISy1625QqYGOlRWhb1sMQRfge0GiIBA5T97050TQ2gsCgYEA78ZM
Jsg7rrsu5je0xtIMRJb1yP0e/B727uRg9FwkLAmaOQ7MdOf51iVE5i/bE1GlZGTX
1woBg/63pSPHVsZ4NFm3BM82Ql6fp5brysmm9NDy6lVpm1Zi4HUvmWYlHLSc/bsC
ivsQwhXxsZ7vv8ntd+bFTfTZcCM6C373gv4NobECgYAsghK4onf4b8mhC/+mNiuS
LGGLOX1vhtHHldykfac8wPaxzjyLOXSuBhvra/eVIzYnuDD+6nn9MhZ8wiAlIk6B
dnZIUHanLXlkYxCob4+6tZWRXj9oakAykDrd1BvuvYJxNSJM67nYt1ehtkuQoI1o
EJdtMThhHKEjSk0bwoahCwKBgCst0KTjRiwvNUDsIzfyt5bagKSTQLMyeFGI+xTG
SxVLHWTND3C2Hb28tG2njJIsD9bGekAVYzR7+GJa0RkFYLw8s8K/9gt5stKhURER
w2ku/6/Hdd8tbVzyfYHobSKAPA1NUiXpFJl/Dilob9Bf8s221y00vLt+/IL5eGU0
jwvhAoGBAKfj8CQAu7gxAa0OtqrZm31lUWyv7IvzNveKXo8ySIctSwUS9XdLuRbT
FA0wXB5CdlcCMXKO+nUTGM87KoSt6j7yZh7M2up/cNnlJ+RatMrvKv1/E70CmVNI
HlMed6rXDK1m99FpO0cIFsDVAf+Z15pDGJJAbSCft7s638e/9Rai
-----END RSA PRIVATE KEY-----"

doppler secrets set MDBAI_PRIVATE_KEY="$MDBAI_PRIVATE_KEY" --project mdbai --config dev --silent
echo "   ✅ MDBAI_PRIVATE_KEY"

# Telegram
doppler secrets set TELEGRAM_BOT_TOKEN="8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8" --project mdbai --config dev --silent
echo "   ✅ TELEGRAM_BOT_TOKEN"

# Redis
doppler secrets set REDIS_HOST="redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com" --project mdbai --config dev --silent
echo "   ✅ REDIS_HOST"

doppler secrets set REDIS_PORT="17068" --project mdbai --config dev --silent
echo "   ✅ REDIS_PORT"

doppler secrets set REDIS_USERNAME="default" --project mdbai --config dev --silent
echo "   ✅ REDIS_USERNAME"

doppler secrets set REDIS_PASSWORD="lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip" --project mdbai --config dev --silent
echo "   ✅ REDIS_PASSWORD"

doppler secrets set REDIS_URL="redis://default:lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip@redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068" --project mdbai --config dev --silent
echo "   ✅ REDIS_URL"

# Application
doppler secrets set NODE_ENV="production" --project mdbai --config dev --silent
echo "   ✅ NODE_ENV"

doppler secrets set API_PORT="3000" --project mdbai --config dev --silent
echo "   ✅ API_PORT"

doppler secrets set LOG_LEVEL="info" --project mdbai --config dev --silent
echo "   ✅ LOG_LEVEL"

doppler secrets set API_HOST="0.0.0.0" --project mdbai --config dev --silent
echo "   ✅ API_HOST"

doppler secrets set PUBLIC_URL="http://51.255.22.253" --project mdbai --config dev --silent
echo "   ✅ PUBLIC_URL"

# Session (générer nouveau secret)
SESSION_SECRET=$(openssl rand -hex 32)
doppler secrets set SESSION_SECRET="$SESSION_SECRET" --project mdbai --config dev --silent
echo "   ✅ SESSION_SECRET (généré)"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  ✅ Push Terminé !"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "📊 Résumé:"
echo "   - Secrets MDBAI GitHub: 5"
echo "   - Secrets Telegram: 1"
echo "   - Secrets Redis: 5"
echo "   - Secrets Application: 6"
echo "   - Total: 17 secrets"
echo ""
echo "🔍 Vérification:"
echo "   doppler secrets --project mdbai --config dev"
echo ""

# Made with Bob
