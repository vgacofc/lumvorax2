# GUIDE MISE À JOUR GITHUB APP MDBAI
**Version**: 1.0.0  
**Date**: 2026-05-30T23:17Z  
**Auteur**: Bob (Expert DevOps, GitHub Apps, Networking)  
**Contexte**: Migration architecture Telegram-first → GitHub App OAuth

---

## RÉSUMÉ EXÉCUTIF

Ce guide documente **3 options techniques** pour mettre à jour l'URL de callback OAuth de la GitHub App MDBAI après migration vers architecture Telegram-first. La GitHub App actuelle pointe vers une URL Replit obsolète qui doit être remplacée par une URL accessible publiquement.

### Problème Identifié
- **GitHub App actuelle**: URL callback Replit (non accessible)
- **Architecture cible**: Bot Telegram @masterdebugai_bot comme interface principale
- **Besoin**: URL publique HTTPS pour OAuth callback GitHub

### 3 Options Disponibles
1. **Option A**: Replit URL (simple, gratuit, limité)
2. **Option B**: Ngrok Tunnel (développement, temporaire)
3. **Option C**: GitHub Codespaces (production, recommandé)

---

## OPTION A: REPLIT URL (SIMPLE)

### Description
Utiliser l'URL publique fournie automatiquement par Replit pour héberger le serveur OAuth.

### Avantages
- ✅ **Gratuit** (plan Replit gratuit)
- ✅ **Simple** (URL automatique)
- ✅ **HTTPS natif** (certificat Replit)
- ✅ **Déploiement rapide** (< 5 minutes)

### Inconvénients
- ❌ **Limites ressources** (CPU/RAM partagés)
- ❌ **URL changeante** (si Repl supprimé/recréé)
- ❌ **Pas de contrôle DNS** (sous-domaine Replit)
- ❌ **Latence variable** (serveurs partagés)

### Procédure Détaillée

#### Étape 1: Créer Repl
```bash
# 1. Aller sur https://replit.com
# 2. Créer nouveau Repl: Node.js
# 3. Nom: "mdbai-oauth-server"
```

#### Étape 2: Code Serveur OAuth
```javascript
// index.js
const express = require('express');
const axios = require('axios');
const app = express();

const GITHUB_CLIENT_ID = process.env.GITHUB_CLIENT_ID;
const GITHUB_CLIENT_SECRET = process.env.GITHUB_CLIENT_SECRET;
const TELEGRAM_BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN;

// Route callback OAuth GitHub
app.get('/auth/github/callback', async (req, res) => {
  const { code, state } = req.query;
  
  try {
    // Échanger code contre access_token
    const tokenResponse = await axios.post(
      'https://github.com/login/oauth/access_token',
      {
        client_id: GITHUB_CLIENT_ID,
        client_secret: GITHUB_CLIENT_SECRET,
        code: code
      },
      { headers: { Accept: 'application/json' } }
    );
    
    const accessToken = tokenResponse.data.access_token;
    
    // Récupérer infos utilisateur GitHub
    const userResponse = await axios.get('https://api.github.com/user', {
      headers: { Authorization: `Bearer ${accessToken}` }
    });
    
    const githubUser = userResponse.data;
    
    // Notifier Bot Telegram (state = chat_id)
    await axios.post(
      `https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`,
      {
        chat_id: state,
        text: `✅ Authentification réussie!\n\nGitHub: @${githubUser.login}\nNom: ${githubUser.name}\n\nVous pouvez maintenant utiliser /analyze`
      }
    );
    
    // Stocker token (Redis/BDD)
    // await redis.set(`github_token:${state}`, accessToken);
    
    res.send(`
      <html>
        <body style="font-family: Arial; text-align: center; padding: 50px;">
          <h1>✅ Authentification réussie!</h1>
          <p>Vous pouvez fermer cette fenêtre et retourner sur Telegram.</p>
        </body>
      </html>
    `);
  } catch (error) {
    console.error('OAuth error:', error);
    res.status(500).send('Erreur authentification');
  }
});

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`OAuth server running on port ${PORT}`);
});
```

#### Étape 3: Configuration Replit
```bash
# .replit
run = "node index.js"

[env]
GITHUB_CLIENT_ID = "your_client_id"
GITHUB_CLIENT_SECRET = "your_client_secret"
TELEGRAM_BOT_TOKEN = "your_bot_token"
```

#### Étape 4: Déployer et Obtenir URL
```bash
# 1. Cliquer "Run" dans Replit
# 2. Copier URL publique (ex: https://mdbai-oauth-server.username.repl.co)
# 3. Tester: curl https://mdbai-oauth-server.username.repl.co/health
```

#### Étape 5: Mettre à Jour GitHub App
```bash
# 1. Aller sur https://github.com/settings/apps/your-app
# 2. Section "Identifying and authorizing users"
# 3. Callback URL: https://mdbai-oauth-server.username.repl.co/auth/github/callback
# 4. Sauvegarder
```

### Coût
- **Gratuit** (plan Replit gratuit)

### Temps Mise en Place
- **5-10 minutes**

---

## OPTION B: NGROK TUNNEL (DÉVELOPPEMENT)

### Description
Créer un tunnel HTTPS temporaire vers serveur local avec Ngrok.

### Avantages
- ✅ **Développement rapide** (tunnel instantané)
- ✅ **Contrôle total** (serveur local)
- ✅ **HTTPS gratuit** (certificat Ngrok)
- ✅ **Logs détaillés** (dashboard Ngrok)

### Inconvénients
- ❌ **URL temporaire** (change à chaque redémarrage)
- ❌ **Pas pour production** (tunnel instable)
- ❌ **Limite gratuite** (40 connexions/min)
- ❌ **Nécessite serveur actif** (local ou VPS)

### Procédure Détaillée

#### Étape 1: Installer Ngrok
```bash
# Linux/Mac
curl -s https://ngrok-agent.s3.amazonaws.com/ngrok.asc | \
  sudo tee /etc/apt/trusted.gpg.d/ngrok.asc >/dev/null && \
  echo "deb https://ngrok-agent.s3.amazonaws.com buster main" | \
  sudo tee /etc/apt/sources.list.d/ngrok.list && \
  sudo apt update && sudo apt install ngrok

# Ou télécharger: https://ngrok.com/download
```

#### Étape 2: Configurer Ngrok
```bash
# S'inscrire sur https://dashboard.ngrok.com/signup
# Copier authtoken

ngrok config add-authtoken YOUR_AUTHTOKEN
```

#### Étape 3: Créer Serveur Local
```javascript
// server.js (même code que Option A)
const express = require('express');
const app = express();

// ... (code OAuth identique) ...

app.listen(3000, () => {
  console.log('OAuth server running on http://localhost:3000');
});
```

#### Étape 4: Lancer Tunnel Ngrok
```bash
# Terminal 1: Lancer serveur
node server.js

# Terminal 2: Lancer tunnel
ngrok http 3000

# Output:
# Forwarding: https://abc123.ngrok.io -> http://localhost:3000
```

#### Étape 5: Mettre à Jour GitHub App
```bash
# 1. Copier URL Ngrok (ex: https://abc123.ngrok.io)
# 2. GitHub App Settings → Callback URL
# 3. URL: https://abc123.ngrok.io/auth/github/callback
# 4. Sauvegarder

# ⚠️ ATTENTION: URL change à chaque redémarrage Ngrok!
```

### Coût
- **Gratuit** (plan Ngrok gratuit)
- **$8/mois** (plan Pro, URL fixe)

### Temps Mise en Place
- **10-15 minutes**

---

## OPTION C: GITHUB CODESPACES (PRODUCTION) ⭐ RECOMMANDÉ

### Description
Utiliser GitHub Codespaces pour héberger serveur OAuth avec URL publique stable.

### Avantages
- ✅ **Production-ready** (infrastructure GitHub)
- ✅ **URL stable** (ne change pas)
- ✅ **HTTPS natif** (certificat GitHub)
- ✅ **Intégration GitHub** (même écosystème)
- ✅ **Ressources garanties** (CPU/RAM dédiés)
- ✅ **Logs centralisés** (GitHub Actions)

### Inconvénients
- ❌ **Coût** ($0.18/heure, ~$130/mois si 24/7)
- ❌ **Configuration initiale** (plus complexe)
- ❌ **Nécessite repo GitHub** (code serveur)

### Procédure Détaillée

#### Étape 1: Créer Repository GitHub
```bash
# 1. Créer repo: https://github.com/new
# 2. Nom: "mdbai-oauth-server"
# 3. Visibilité: Private
# 4. Initialiser avec README
```

#### Étape 2: Structure Projet
```bash
mdbai-oauth-server/
├── .devcontainer/
│   └── devcontainer.json
├── src/
│   └── server.js
├── package.json
├── .env.example
└── README.md
```

#### Étape 3: Configuration Codespace
```json
// .devcontainer/devcontainer.json
{
  "name": "MDBAI OAuth Server",
  "image": "mcr.microsoft.com/devcontainers/javascript-node:18",
  "forwardPorts": [3000],
  "portsAttributes": {
    "3000": {
      "label": "OAuth Server",
      "onAutoForward": "notify",
      "visibility": "public"
    }
  },
  "postCreateCommand": "npm install",
  "customizations": {
    "vscode": {
      "extensions": [
        "dbaeumer.vscode-eslint"
      ]
    }
  }
}
```

#### Étape 4: Code Serveur Production
```javascript
// src/server.js
const express = require('express');
const axios = require('axios');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const app = express();

// Sécurité
app.use(helmet());
app.use(express.json());

// Rate limiting
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100 // 100 requêtes max
});
app.use('/auth/', limiter);

// Configuration
const config = {
  github: {
    clientId: process.env.GITHUB_CLIENT_ID,
    clientSecret: process.env.GITHUB_CLIENT_SECRET
  },
  telegram: {
    botToken: process.env.TELEGRAM_BOT_TOKEN
  },
  redis: {
    url: process.env.REDIS_URL || 'redis://localhost:6379'
  }
};

// Route OAuth callback
app.get('/auth/github/callback', async (req, res) => {
  const { code, state } = req.query;
  
  if (!code || !state) {
    return res.status(400).send('Missing code or state');
  }
  
  try {
    // Échanger code contre token
    const tokenResponse = await axios.post(
      'https://github.com/login/oauth/access_token',
      {
        client_id: config.github.clientId,
        client_secret: config.github.clientSecret,
        code: code
      },
      { headers: { Accept: 'application/json' } }
    );
    
    const accessToken = tokenResponse.data.access_token;
    
    if (!accessToken) {
      throw new Error('No access token received');
    }
    
    // Récupérer infos utilisateur
    const userResponse = await axios.get('https://api.github.com/user', {
      headers: { Authorization: `Bearer ${accessToken}` }
    });
    
    const githubUser = userResponse.data;
    
    // Stocker token (Redis)
    // await redis.setex(`github_token:${state}`, 3600, accessToken);
    
    // Notifier Telegram
    await axios.post(
      `https://api.telegram.org/bot${config.telegram.botToken}/sendMessage`,
      {
        chat_id: state,
        text: `✅ *Authentification GitHub réussie!*\n\n` +
              `👤 Utilisateur: @${githubUser.login}\n` +
              `📧 Email: ${githubUser.email || 'Non public'}\n` +
              `🏢 Entreprise: ${githubUser.company || 'N/A'}\n\n` +
              `Vous pouvez maintenant utiliser /analyze pour analyser vos repos.`,
        parse_mode: 'Markdown'
      }
    );
    
    // Page succès
    res.send(`
      <!DOCTYPE html>
      <html>
        <head>
          <title>MDBAI - Authentification Réussie</title>
          <style>
            body {
              font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
              display: flex;
              justify-content: center;
              align-items: center;
              height: 100vh;
              margin: 0;
              background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            }
            .container {
              background: white;
              padding: 40px;
              border-radius: 10px;
              box-shadow: 0 10px 40px rgba(0,0,0,0.2);
              text-align: center;
              max-width: 400px;
            }
            h1 { color: #333; margin-bottom: 20px; }
            p { color: #666; line-height: 1.6; }
            .success-icon {
              font-size: 64px;
              margin-bottom: 20px;
            }
          </style>
        </head>
        <body>
          <div class="container">
            <div class="success-icon">✅</div>
            <h1>Authentification Réussie!</h1>
            <p>Votre compte GitHub a été lié avec succès.</p>
            <p>Vous pouvez fermer cette fenêtre et retourner sur Telegram.</p>
          </div>
        </body>
      </html>
    `);
    
  } catch (error) {
    console.error('OAuth error:', error);
    
    // Notifier erreur sur Telegram
    await axios.post(
      `https://api.telegram.org/bot${config.telegram.botToken}/sendMessage`,
      {
        chat_id: state,
        text: `❌ Erreur lors de l'authentification GitHub.\n\nVeuillez réessayer avec /auth`
      }
    ).catch(console.error);
    
    res.status(500).send(`
      <!DOCTYPE html>
      <html>
        <head>
          <title>MDBAI - Erreur</title>
          <style>
            body {
              font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
              display: flex;
              justify-content: center;
              align-items: center;
              height: 100vh;
              margin: 0;
              background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            }
            .container {
              background: white;
              padding: 40px;
              border-radius: 10px;
              box-shadow: 0 10px 40px rgba(0,0,0,0.2);
              text-align: center;
              max-width: 400px;
            }
            h1 { color: #333; margin-bottom: 20px; }
            p { color: #666; line-height: 1.6; }
            .error-icon {
              font-size: 64px;
              margin-bottom: 20px;
            }
          </style>
        </head>
        <body>
          <div class="container">
            <div class="error-icon">❌</div>
            <h1>Erreur d'Authentification</h1>
            <p>Une erreur s'est produite lors de l'authentification.</p>
            <p>Veuillez retourner sur Telegram et réessayer avec /auth</p>
          </div>
        </body>
      </html>
    `);
  }
});

// Health check
app.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    timestamp: new Date().toISOString(),
    uptime: process.uptime()
  });
});

// Metrics (optionnel)
app.get('/metrics', (req, res) => {
  res.json({
    memory: process.memoryUsage(),
    uptime: process.uptime(),
    version: process.version
  });
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`✅ MDBAI OAuth Server running on port ${PORT}`);
  console.log(`📍 Health check: http://localhost:${PORT}/health`);
});
```

#### Étape 5: Package.json
```json
{
  "name": "mdbai-oauth-server",
  "version": "1.0.0",
  "description": "OAuth server for MDBAI GitHub App",
  "main": "src/server.js",
  "scripts": {
    "start": "node src/server.js",
    "dev": "nodemon src/server.js"
  },
  "dependencies": {
    "express": "^4.18.2",
    "axios": "^1.6.0",
    "helmet": "^7.1.0",
    "express-rate-limit": "^7.1.0"
  },
  "devDependencies": {
    "nodemon": "^3.0.0"
  }
}
```

#### Étape 6: Créer Codespace
```bash
# 1. Aller sur repo GitHub
# 2. Cliquer "Code" → "Codespaces" → "Create codespace on main"
# 3. Attendre création (~2 minutes)
# 4. Codespace s'ouvre dans VS Code web
```

#### Étape 7: Configurer Variables d'Environnement
```bash
# Dans terminal Codespace:
echo "GITHUB_CLIENT_ID=your_client_id" >> .env
echo "GITHUB_CLIENT_SECRET=your_client_secret" >> .env
echo "TELEGRAM_BOT_TOKEN=your_bot_token" >> .env

# Installer dépendances
npm install

# Lancer serveur
npm start
```

#### Étape 8: Obtenir URL Publique
```bash
# 1. VS Code affiche notification "Your application running on port 3000 is available"
# 2. Cliquer "Open in Browser"
# 3. Copier URL (ex: https://abc123-3000.app.github.dev)
# 4. Tester: curl https://abc123-3000.app.github.dev/health
```

#### Étape 9: Mettre à Jour GitHub App
```bash
# 1. Aller sur https://github.com/settings/apps/your-app
# 2. Callback URL: https://abc123-3000.app.github.dev/auth/github/callback
# 3. Sauvegarder
```

#### Étape 10: Garder Codespace Actif
```bash
# Option 1: Garder onglet ouvert (gratuit 60h/mois)
# Option 2: Configurer auto-start (GitHub Actions)
# Option 3: Utiliser service externe (cron job ping)
```

### Coût
- **Gratuit**: 60 heures/mois (plan GitHub Free)
- **$0.18/heure**: Au-delà de 60h (~$130/mois si 24/7)
- **Optimisation**: Arrêter Codespace quand non utilisé

### Temps Mise en Place
- **30-45 minutes** (première fois)
- **5 minutes** (redéploiement)

---

## COMPARAISON DES OPTIONS

| Critère | Option A: Replit | Option B: Ngrok | Option C: Codespaces ⭐ |
|---------|------------------|-----------------|------------------------|
| **Coût** | Gratuit | Gratuit | $0-130/mois |
| **Stabilité URL** | Moyenne | Faible | Haute |
| **Production** | Non | Non | Oui |
| **HTTPS** | Oui | Oui | Oui |
| **Setup** | 5 min | 10 min | 30 min |
| **Contrôle** | Faible | Moyen | Total |
| **Ressources** | Limitées | Locales | Garanties |
| **Logs** | Basiques | Détaillés | Centralisés |
| **Recommandé** | Prototype | Dev | Production |

---

## RECOMMANDATION FINALE

### Pour Développement/Test
**Option B: Ngrok** (développement rapide, logs détaillés)

### Pour Production
**Option C: GitHub Codespaces** (stabilité, intégration GitHub, production-ready)

### Pour Prototype Rapide
**Option A: Replit** (déploiement instantané, gratuit)

---

## CHECKLIST MISE À JOUR GITHUB APP

Quelle que soit l'option choisie:

- [ ] Obtenir URL publique HTTPS
- [ ] Tester endpoint `/health` accessible
- [ ] Configurer variables d'environnement (CLIENT_ID, CLIENT_SECRET, BOT_TOKEN)
- [ ] Mettre à jour GitHub App Settings → Callback URL
- [ ] Tester flow OAuth complet:
  - [ ] Générer URL OAuth avec state
  - [ ] Cliquer lien, autoriser app
  - [ ] Vérifier redirection vers callback
  - [ ] Vérifier notification Telegram reçue
- [ ] Documenter URL dans CAHIER_DES_CHARGES_MDBAI.md
- [ ] Mettre à jour thinking6.json avec décision

---

## SÉCURITÉ

### Bonnes Pratiques
1. **Variables d'environnement**: Ne jamais commit secrets
2. **HTTPS obligatoire**: GitHub refuse HTTP
3. **Rate limiting**: Protéger contre abus
4. **Validation state**: Vérifier state OAuth (CSRF)
5. **Expiration tokens**: Limiter durée vie tokens
6. **Logs sécurisés**: Ne pas logger tokens

### Exemple .env
```bash
# .env (NE JAMAIS COMMIT)
GITHUB_CLIENT_ID=Iv1.abc123def456
GITHUB_CLIENT_SECRET=abc123def456ghi789jkl012mno345pqr678stu
TELEGRAM_BOT_TOKEN=123456789:ABCdefGHIjklMNOpqrSTUvwxYZ123456789
REDIS_URL=redis://localhost:6379
PORT=3000
```

### Exemple .gitignore
```bash
# .gitignore
.env
.env.local
.env.*.local
node_modules/
*.log
```

---

## PROCHAINES ÉTAPES

Après mise à jour GitHub App:

1. **Tester flow OAuth complet** (commande `/auth` Telegram)
2. **Implémenter stockage tokens** (Redis/PostgreSQL)
3. **Créer commande `/analyze`** (analyse repo GitHub)
4. **Intégrer LumVorax C111** (forensique bit-level)
5. **Configurer webhooks GitHub** (événements repo)
6. **Déployer workers BullMQ** (traitement asynchrone)

---

## SUPPORT

### Ressources
- **GitHub Apps**: https://docs.github.com/en/apps
- **OAuth Flow**: https://docs.github.com/en/apps/oauth-apps/building-oauth-apps
- **Codespaces**: https://docs.github.com/en/codespaces
- **Ngrok**: https://ngrok.com/docs
- **Replit**: https://docs.replit.com

### Contact
- **Telegram**: @masterdebugai_bot
- **GitHub Issues**: https://github.com/your-org/mdbai/issues

---

**FIN DU GUIDE**