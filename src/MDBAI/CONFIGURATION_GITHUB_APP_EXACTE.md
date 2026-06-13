# 🔧 CONFIGURATION GITHUB APP MDBAI - VALEURS EXACTES À REMPLIR

**Date**: 2026-06-03T03:07Z  
**GitHub App**: MDBAI - Master Debug AI  
**Owner**: @vgacofc  
**App ID**: 3888479

---

## 📋 INFORMATIONS ACTUELLES (DÉJÀ CONFIGURÉES)

### About
- **Owned by**: `@vgacofc`
- **App ID**: `3888479`
- **Client ID**: `Iv23liM06X4pQnng7oFm`

### Client Secrets (2 secrets configurés)
1. **Client secret 1**: `*****2a85d51b` (Added 5 days ago, Never used)
2. **Client secret 2**: `*****36aea71a` (Added 5 days ago, Never used)

### Private Keys (2 clés configurées)
1. **Private key 1**: `SHA256:nzOjVUIQH9OMeQdELO15gyMWyN+iAJVGhRX68Ww5eqM=` (Added 5 days ago)
2. **Private key 2**: `SHA256:Apyd2CHiSOs6yaAuW1T+kO8j5HysUDW0jO3Ci8nIzY8=` (Added 5 days ago)

---

## ✏️ VALEURS À REMPLIR/METTRE À JOUR

### 1. Basic Information

#### GitHub App name
```
MDBAI - Master Debug AI
```

#### Homepage URL
```
http://localhost:3001
```
**OU si vous avez un domaine public** :
```
https://votre-domaine.com
```

---

### 2. Identifying and Authorizing Users

#### Callback URL
```
http://localhost:3001/auth/github/callback
```
**OU si vous avez un domaine public** :
```
https://votre-domaine.com/auth/github/callback
```

#### Request user authorization (OAuth) during installation
```
☑ Cocher cette case
```
**Raison** : Permet d'obtenir le token OAuth de l'utilisateur lors de l'installation

#### Enable Device Flow
```
☐ Ne PAS cocher
```
**Raison** : Non nécessaire pour MDBAI (utilise OAuth web flow)

---

### 3. Post Installation

#### Setup URL (optional)
```
LAISSER VIDE
```
**Raison** : Pas de setup additionnel requis

#### Redirect on update
```
☐ Ne PAS cocher
```
**Raison** : Pas de redirection nécessaire lors des mises à jour

---

### 4. Webhook

#### Active
```
☑ Cocher cette case
```

#### Webhook URL
```
http://localhost:3001/webhook/github
```
**OU si vous avez un domaine public** :
```
https://votre-domaine.com/webhook/github
```

**⚠️ IMPORTANT** : 
- Pour localhost, GitHub ne pourra PAS envoyer de webhooks (normal en développement)
- Pour production, utilisez un domaine public accessible (ex: ngrok, Replit, etc.)

#### Webhook Secret
```
DÉJÀ CONFIGURÉ ✅
```
**Valeur actuelle** : `ed34c4d917c8a90924b0cb9...` (dans Doppler: MDBAI_WEBHOOK_SECRET)

**Si vous devez le changer** :
1. Générer nouveau secret : `openssl rand -hex 20`
2. Mettre à jour dans GitHub App settings
3. Mettre à jour dans Doppler : `doppler secrets set MDBAI_WEBHOOK_SECRET=nouveau_secret`

#### SSL verification
```
☑ Enable SSL verification (recommandé)
```
**Note** : Pour localhost, désactiver temporairement si nécessaire

---

### 5. Permissions & Events

#### Repository Permissions

| Permission | Niveau | Raison |
|-----------|--------|--------|
| **Contents** | `Read and write` | Clone repo + commit rapport |
| **Pull requests** | `Read and write` | Créer PR avec rapport |
| **Issues** | `Read and write` | Créer issues si bugs |
| **Metadata** | `Read-only` | Infos repo (langage, taille) |
| **Workflows** | `Read and write` | Analyser GitHub Actions |

#### Account Permissions

| Permission | Niveau | Raison |
|-----------|--------|--------|
| **Email addresses** | `Read-only` | Identifier utilisateur |

#### Subscribe to Events (Webhooks)

Cocher les événements suivants :
```
☑ Push
☑ Pull request
☑ Create
☑ Repository
```

**Raison** : Permet l'analyse automatique lors de ces événements

---

## 🔐 SECRETS DOPPLER (DÉJÀ CONFIGURÉS ✅)

### Vérification des secrets actuels

```bash
# Vérifier tous les secrets MDBAI
doppler secrets --project lumvorax --config dev_lumvorax | grep MDBAI
```

### Secrets configurés

| Variable | Valeur (partielle) | Statut |
|----------|-------------------|--------|
| `MDBAI_APP_ID` | `3888479` | ✅ |
| `MDBAI_CLIENT_ID` | `Iv23liM06X4pQnng7oFm` | ✅ |
| `MDBAI_CLIENT_SECRET` | `25a467c860016f63afedbbb...` | ✅ |
| `MDBAI_PRIVATE_KEY` | `-----BEGIN RSA PRIVATE KEY-----...` | ✅ |
| `MDBAI_WEBHOOK_SECRET` | `ed34c4d917c8a90924b0cb9...` | ✅ |

---

## 🚀 APRÈS CONFIGURATION

### 1. Sauvegarder les changements
Cliquer sur **"Save changes"** en bas de la page GitHub App settings

### 2. Installer l'App sur votre compte
```
URL : https://github.com/apps/mdbai-master-debug-ai/installations/new
```

### 3. Tester la configuration

#### Test 1 : Health Check
```bash
curl http://localhost:3001/health
```
**Résultat attendu** : `{"status":"healthy",...}`

#### Test 2 : Bot Telegram
```
1. Ouvrir Telegram
2. Chercher : @masterdebugai_bot
3. Envoyer : /start
```

#### Test 3 : Analyse dépôt
```
/analyze https://github.com/octocat/Hello-World
```

---

## 📝 CHECKLIST CONFIGURATION

### GitHub App Settings
- [x] App ID: 3888479
- [x] Client ID: Iv23liM06X4pQnng7oFm
- [x] Client secrets configurés (2)
- [x] Private keys configurées (2)
- [ ] Homepage URL: `http://localhost:3001` (à remplir)
- [ ] Callback URL: `http://localhost:3001/auth/github/callback` (à remplir)
- [ ] Webhook URL: `http://localhost:3001/webhook/github` (à remplir)
- [ ] OAuth during installation: ☑ (à cocher)
- [ ] Permissions configurées (Contents, PRs, Issues, Metadata, Workflows)
- [ ] Events configurés (Push, PR, Create, Repository)

### Doppler Secrets
- [x] MDBAI_APP_ID
- [x] MDBAI_CLIENT_ID
- [x] MDBAI_CLIENT_SECRET
- [x] MDBAI_PRIVATE_KEY
- [x] MDBAI_WEBHOOK_SECRET
- [x] TELEGRAM_BOT_TOKEN
- [x] REDIS_URL
- [x] SESSION_SECRET

### Serveur MDBAI
- [x] Serveur démarré (port 3001)
- [x] Redis connecté
- [x] Bot Telegram actif
- [x] Worker BullMQ actif
- [x] Forensic lib chargée

---

## 🔄 SI VOUS CHANGEZ DE DOMAINE

### Passer de localhost à domaine public

1. **Mettre à jour GitHub App** :
   - Homepage URL: `https://nouveau-domaine.com`
   - Callback URL: `https://nouveau-domaine.com/auth/github/callback`
   - Webhook URL: `https://nouveau-domaine.com/webhook/github`

2. **Redémarrer serveur MDBAI** :
   ```bash
   # Arrêter serveur actuel
   pkill -f "node src/server.js"
   
   # Redémarrer
   cd /home/lvx/LVX/lumvorax2/src/MDBAI
   doppler run --project lumvorax --config dev_lumvorax -- npm start
   ```

3. **Tester nouvelle configuration** :
   ```bash
   curl https://nouveau-domaine.com/health
   ```

---

## 🐛 DÉPANNAGE

### Problème : Webhook ne fonctionne pas

**Symptôme** : Pas de notification lors des push GitHub

**Causes possibles** :
1. Webhook URL pointe vers localhost (GitHub ne peut pas y accéder)
2. SSL verification activée mais certificat invalide
3. Webhook secret incorrect

**Solutions** :
1. Utiliser ngrok ou domaine public pour webhooks
2. Désactiver SSL verification temporairement
3. Vérifier MDBAI_WEBHOOK_SECRET dans Doppler

### Problème : OAuth callback échoue

**Symptôme** : Erreur lors de l'installation de l'App

**Causes possibles** :
1. Callback URL incorrecte
2. Serveur MDBAI non démarré
3. Client secret incorrect

**Solutions** :
1. Vérifier Callback URL: `http://localhost:3001/auth/github/callback`
2. Démarrer serveur: `./start_test_utilisateur.sh`
3. Vérifier MDBAI_CLIENT_SECRET dans Doppler

---

## 📞 SUPPORT

### Logs serveur
```bash
tail -f /home/lvx/LVX/lumvorax2/src/MDBAI/server_output.log
```

### Vérifier secrets
```bash
doppler secrets --project lumvorax --config dev_lumvorax | grep MDBAI
```

### Tester connexion Redis
```bash
curl http://localhost:3001/health | jq .redis
```

### Tester Bot Telegram
```bash
curl http://localhost:3001/health | jq .telegram
```

---

**Auteur** : Bob (Expert Software Engineer)  
**Date** : 2026-06-03T03:07Z  
**Version** : 1.0.0  
**Statut** : ✅ PRÊT — Serveur démarré, secrets configurés