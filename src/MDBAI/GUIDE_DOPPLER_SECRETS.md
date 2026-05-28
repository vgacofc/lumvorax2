# GUIDE CONFIGURATION DOPPLER - MDBAI

**Date**: 2026-05-27  
**Version**: 1.0.0  
**Objectif**: Configuration sécurisée des secrets via Doppler  
**URL Projet**: https://dashboard.doppler.com/workplace/b6deb943584e68011892/projects/lumvorax/configs/dev_debugai

---

## 🔐 SÉCURITÉ CRITIQUE

### ⚠️ TOKENS 

Les tokens suivants dans `TOKEN_CLE_PLATFORME.txt` sont  et doivent être :

| Service | Token Compromis | Action Requise |
|---------|-----------------|----------------|
| Prisma Postgres | `sk_3TBG35XoftnxSiHZsdzN4` | 
| Redis Cloud | `lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip` |
| Upstash Box | `box_fbc484766ea9bfdb8bc6ff3c4ca74bee4ad9b9605c90fa07ab11526aaea8a422` |  
| OVH Cloud | `09161e9fcb54bbc7a98fab5d6eaa8128` | 
| UpCloud | `ucat_01KSN6YG7A5CBB8ZA9YAS2K5JB` 
| Taskforce | `d7102033-d4bf-4b91-95df-2e24ab08d2da` |
### ✅ TOKEN VALIDE

| Service | Token | Statut |
|---------|-------|--------|
| Telegram Bot | `8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8` | ✅ VALIDE (à vérifier) |

---

## 📋 LISTE COMPLÈTE DES SECRETS

### Secrets à Configurer dans Doppler

**Projet**: `lumvorax`  
**Config**: `dev_debugai`

| Nom Variable | Service | Type | Obligatoire | Statut |
|--------------|---------|------|-------------|--------|
| `GITHUB_APP_ID` | GitHub OAuth | string | ✅ OUI | ⏳ À créer |
| `GITHUB_APP_SECRET` | GitHub OAuth | string | ✅ OUI | ⏳ À créer |
| `GITHUB_WEBHOOK_SECRET` | GitHub Webhooks | string | ✅ OUI | ⏳ À créer |
| `GITHUB_PRIVATE_KEY` | GitHub App | multiline | ✅ OUI | ⏳ À créer |
| `TELEGRAM_BOT_TOKEN` | Telegram | string | ✅ OUI | ✅ Existant |
| `REDIS_URL` | Redis Cloud | string | ✅ OUI | ⚠️ À régénérer |
| `REDIS_HOST` | Redis Cloud | string | ✅ OUI | ⚠️ À régénérer |
| `REDIS_PORT` | Redis Cloud | number | ✅ OUI | ✅ 17068 |
| `REDIS_PASSWORD` | Redis Cloud | string | ✅ OUI | ⚠️ À régénérer |
| `DOPPLER_TOKEN` | Doppler | string | ✅ OUI | ⏳ À créer |
| `NODE_ENV` | Application | string | ✅ OUI | ✅ development |
| `LOG_LEVEL` | Application | string | ❌ Non | ✅ debug |
| `API_PORT` | Application | number | ❌ Non | ✅ 3000 |
| `WEBHOOK_URL` | Application | string | ✅ OUI | ⏳ À définir |

---

## 🚀 PROCÉDURE CONFIGURATION

### Étape 1: Installation Doppler CLI

```bash
# macOS
brew install dopplerhq/cli/doppler

# Linux
curl -Ls https://cli.doppler.com/install.sh | sh

# Windows
scoop install doppler

# Vérification
doppler --version
```

### Étape 2: Authentification Doppler

```bash
# Login interactif
doppler login

# Ou avec token
doppler configure set token <DOPPLER_SERVICE_TOKEN>
```

### Étape 3: Configuration Projet

```bash
# Se placer dans le projet
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Configurer projet Doppler
doppler setup \
  --project lumvorax \
  --config dev_debugai

# Vérifier configuration
doppler configure
```

### Étape 4: Création GitHub App

#### 4.1 Créer GitHub App

1. Aller sur https://github.com/settings/apps/new
2. Remplir formulaire:
   - **App name**: `MDBAI-Production`
   - **Homepage URL**: `https://github.com/lumvorax/mdbai`
   - **Webhook URL**: `https://your-replit-url.repl.co/api/webhook`
   - **Webhook secret**: Générer avec `openssl rand -hex 32`

#### 4.2 Permissions Requises

**Repository permissions**:
- Contents: Read & Write
- Pull requests: Read & Write
- Metadata: Read
- Webhooks: Read

**Organization permissions**:
- Members: Read

**User permissions**:
- Email addresses: Read

#### 4.3 Subscribe to Events

- [x] Push
- [x] Pull request
- [x] Issues
- [x] Issue comment

#### 4.4 Récupérer Credentials

Après création:
1. **App ID**: Copier depuis page app
2. **Client ID**: Copier depuis page app
3. **Client Secret**: Générer et copier
4. **Private Key**: Télécharger `.pem` file
5. **Webhook Secret**: Celui généré à l'étape 4.1

### Étape 5: Ajouter Secrets à Doppler

```bash
# GitHub App ID
doppler secrets set GITHUB_APP_ID="123456"

# GitHub App Secret
doppler secrets set GITHUB_APP_SECRET="ghp_xxxxxxxxxxxxxxxxxxxx"

# GitHub Webhook Secret
doppler secrets set GITHUB_WEBHOOK_SECRET="your_webhook_secret_here"

# GitHub Private Key (multiline)
doppler secrets set GITHUB_PRIVATE_KEY="$(cat path/to/private-key.pem)"

# Telegram Bot Token
doppler secrets set TELEGRAM_BOT_TOKEN="8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8"

# Vérifier
doppler secrets
```

### Étape 6: Régénérer Redis Credentials

#### 6.1 Aller sur Redis Cloud

URL: https://app.redislabs.com/#/databases

#### 6.2 Régénérer Password

1. Sélectionner database `database-MNTBGRVB`
2. Aller dans "Security"
3. Cliquer "Regenerate password"
4. Copier nouveau password

#### 6.3 Ajouter à Doppler

```bash
# Redis URL complète
doppler secrets set REDIS_URL="redis://default:NEW_PASSWORD@redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068"

# Redis Host
doppler secrets set REDIS_HOST="redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com"

# Redis Port
doppler secrets set REDIS_PORT="17068"

# Redis Password
doppler secrets set REDIS_PASSWORD="NEW_PASSWORD"
```

### Étape 7: Configuration Application

```bash
# Node environment
doppler secrets set NODE_ENV="development"

# Log level
doppler secrets set LOG_LEVEL="debug"

# API Port
doppler secrets set API_PORT="3000"

# Webhook URL (Replit)
doppler secrets set WEBHOOK_URL="https://mdbai.your-username.repl.co/api/webhook"
```

### Étape 8: Générer Doppler Service Token

```bash
# Créer service token pour CI/CD
doppler configs tokens create \
  --project lumvorax \
  --config dev_debugai \
  --name "MDBAI Production" \
  --max-age 0

# Copier token généré
# Format: dp.st.xxxxxxxxxxxxxxxxxxxx
```

### Étape 9: Tester Configuration

```bash
# Lister tous les secrets
doppler secrets

# Télécharger secrets en .env
doppler secrets download --no-file --format env > .env

# Vérifier .env
cat .env

# Exécuter application avec secrets
doppler run -- npm start
```

---

## 🔧 UTILISATION DANS CODE

### Node.js (Recommandé)

```javascript
// Ne PAS utiliser dotenv
// Doppler injecte directement dans process.env

// server.js
const express = require('express');
const app = express();

// Secrets disponibles directement
const config = {
  github: {
    appId: process.env.GITHUB_APP_ID,
    appSecret: process.env.GITHUB_APP_SECRET,
    webhookSecret: process.env.GITHUB_WEBHOOK_SECRET,
    privateKey: process.env.GITHUB_PRIVATE_KEY
  },
  telegram: {
    botToken: process.env.TELEGRAM_BOT_TOKEN
  },
  redis: {
    url: process.env.REDIS_URL,
    host: process.env.REDIS_HOST,
    port: parseInt(process.env.REDIS_PORT),
    password: process.env.REDIS_PASSWORD
  },
  app: {
    env: process.env.NODE_ENV,
    port: parseInt(process.env.API_PORT),
    logLevel: process.env.LOG_LEVEL,
    webhookUrl: process.env.WEBHOOK_URL
  }
};

// Validation
if (!config.github.appId) {
  throw new Error('GITHUB_APP_ID not set');
}

app.listen(config.app.port, () => {
  console.log(`Server running on port ${config.app.port}`);
});
```

### Docker

```dockerfile
# Dockerfile
FROM node:20-alpine

WORKDIR /app

# Installer Doppler CLI
RUN wget -q -t3 'https://packages.doppler.com/public/cli/rsa.8004D9FF50437357.key' -O /etc/apk/keys/cli@doppler-8004D9FF50437357.rsa.pub && \
    echo 'https://packages.doppler.com/public/cli/alpine/any-version/main' | tee -a /etc/apk/repositories && \
    apk add doppler

COPY package*.json ./
RUN npm ci --only=production

COPY . .

# Utiliser Doppler comme entrypoint
ENTRYPOINT ["doppler", "run", "--"]
CMD ["node", "server.js"]
```

### GitHub Actions

```yaml
# .github/workflows/deploy.yml
name: Deploy MDBAI

on:
  push:
    branches: [main]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install Doppler CLI
        uses: dopplerhq/cli-action@v3
      
      - name: Deploy with secrets
        env:
          DOPPLER_TOKEN: ${{ secrets.DOPPLER_TOKEN }}
        run: |
          doppler run -- npm run deploy
```

### Replit

```bash
# .replit
run = "doppler run -- npm start"

[env]
DOPPLER_TOKEN = "dp.st.xxxxxxxxxxxxxxxxxxxx"
```

---

## 🔍 VÉRIFICATION SÉCURITÉ

### Checklist Sécurité

- [ ] Tous tokens compromis révoqués
- [ ] Nouveaux tokens générés
- [ ] Secrets ajoutés à Doppler
- [ ] `.env` dans `.gitignore`
- [ ] Pas de secrets en clair dans code
- [ ] Pas de secrets dans logs
- [ ] Doppler service token créé
- [ ] Permissions GitHub App minimales
- [ ] Webhook secret fort (32+ chars)
- [ ] Redis password régénéré

### Commandes Audit

```bash
# Vérifier secrets Doppler
doppler secrets

# Vérifier pas de secrets en clair
git grep -i "password\|secret\|token\|key" | grep -v ".md"

# Vérifier .gitignore
cat .gitignore | grep -E "\.env|secrets|credentials"

# Tester connexion Redis
redis-cli -u $REDIS_URL PING

# Tester GitHub App
curl -H "Authorization: Bearer $GITHUB_APP_SECRET" \
  https://api.github.com/app
```

---

## 

### 

1. ****:
   ```bash
   # GitHub
   # Aller sur https://github.com/settings/apps
   # Révoquer client secret
   
   # Redis
   # Aller sur Redis Cloud dashboard
   # Régénérer password
   
   # Telegram
   # Contacter @BotFather
   # /revoke
   ```

2. **Régénérer nouveaux tokens**

3. **Mettre à jour Doppler**:
   ```bash
   doppler secrets set GITHUB_APP_SECRET="NEW_SECRET"
   doppler secrets set REDIS_PASSWORD="NEW_PASSWORD"
   ```

4. **Redéployer application**:
   ```bash
   doppler run -- npm run deploy
   ```

5. **Vérifier logs**:
   ```bash
   doppler run -- npm run logs
   ```

6. **Notifier équipe**

---

## 📚 RESSOURCES

### Documentation Officielle

- [Doppler CLI](https://docs.doppler.com/docs/cli)
- [GitHub Apps](https://docs.github.com/en/apps)
- [Redis Cloud](https://docs.redis.com/latest/rc/)
- [Telegram Bots](https://core.telegram.org/bots)

### Commandes Utiles

```bash
# Doppler
doppler --help
doppler secrets --help
doppler run --help

# Lister projets
doppler projects

# Lister configs
doppler configs

# Exporter secrets
doppler secrets download --no-file --format env
doppler secrets download --no-file --format json
doppler secrets download --no-file --format yaml

# Comparer configs
doppler secrets diff dev_debugai prd

# Rollback secret
doppler secrets rollback GITHUB_APP_SECRET

# Logs audit
doppler activity
```

---

## 🎓 FORMATION AGENT REPLIT

### Prérequis

L'agent Replit doit:
1. Installer Doppler CLI
2. S'authentifier avec service token
3. Configurer projet/config
4. Utiliser `doppler run --` pour toutes commandes

### Exemple Workflow

```bash
# 1. Installation
curl -Ls https://cli.doppler.com/install.sh | sh

# 2. Configuration
doppler configure set token $DOPPLER_TOKEN
doppler setup --project lumvorax --config dev_debugai

# 3. Développement
doppler run -- npm run dev

# 4. Tests
doppler run -- npm test

# 5. Build
doppler run -- npm run build

# 6. Déploiement
doppler run -- npm run deploy
```

### Bonnes Pratiques

✅ **À FAIRE**:
- Toujours utiliser `doppler run --`
- Valider secrets au démarrage
- Logger sans exposer secrets
- Utiliser variables d'environnement
- Tester avec secrets de dev

❌ **À NE PAS FAIRE**:
- Hardcoder secrets dans code
- Committer `.env`
- Logger secrets
- Partager tokens
- Utiliser secrets de prod en dev

---

## 📞 SUPPORT

**Problème Doppler**:
- Documentation: https://docs.doppler.com
- Support: support@doppler.com
- Community: https://community.doppler.com

**Problème GitHub App**:
- Documentation: https://docs.github.com/en/apps
- Support: https://support.github.com

**Problème Redis**:
- Documentation: https://docs.redis.com
- Support: https://redis.com/company/support/

---

**Version**: 1.0.0  
**Date**: 2026-05-27  
**Auteur**: LumVorax Team  
**Statut**: PRODUCTION READY
