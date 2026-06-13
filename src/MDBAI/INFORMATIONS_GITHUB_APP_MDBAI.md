# 🔐 INFORMATIONS GITHUB APP MDBAI - CONFIGURATION COMPLÈTE

**Date**: 2026-06-03T02:50Z  
**Statut**: ✅ **CONFIGURÉE** — Tous les secrets Doppler en place  
**Objectif**: Informations exactes pour installation et utilisation de la GitHub App MDBAI

---

## 📋 INFORMATIONS GITHUB APP

### Identifiants Publics

| Champ | Valeur |
|-------|--------|
| **App Name** | `mdbai-master-debug-ai` |
| **App ID** | `3888479` |
| **Client ID** | `Iv23liM06X4pQnng7oFm` |
| **Owner** | `@vgacofc` |
| **Public URL** | https://github.com/apps/mdbai-master-debug-ai |

### URLs de Configuration

| Type | URL |
|------|-----|
| **Installation URL** | https://github.com/apps/mdbai-master-debug-ai/installations/new |
| **Homepage URL** | `https://{VOTRE_DOMAINE}` (à configurer) |
| **OAuth Callback** | `https://{VOTRE_DOMAINE}/auth/github/callback` |
| **Webhook URL** | `https://{VOTRE_DOMAINE}/webhook/github` |

---

## 🔑 SECRETS DOPPLER CONFIGURÉS

### Secrets MDBAI (✅ Tous configurés)

```bash
# GitHub App
MDBAI_APP_ID=3888479
MDBAI_CLIENT_ID=Iv23liM06X4pQnng7oFm
MDBAI_CLIENT_SECRET=25a467c860016f63afedbbb... (40 chars)
MDBAI_PRIVATE_KEY=-----BEGIN RSA PRIVATE KEY----- ... (RSA 2048 bits)
MDBAI_WEBHOOK_SECRET=ed34c4d917c8a90924b0cb9... (40 chars)

# Telegram Bot
TELEGRAM_BOT_TOKEN=8820756284:AAEaeBUd2PIR... (45 chars)

# Redis Cloud
REDIS_URL=redis://default:lAOtAui... (URL complète)
REDIS_HOST=redis-17068.c327.europe-west1-1.gce.redns.redis-cloud.com
REDIS_PORT=17068
REDIS_USERNAME=default
REDIS_PASSWORD=lAOtAuizLK2LyIVlrSdEpPF... (32 chars)

# Session
SESSION_SECRET=5OJClYN0YEPYY36XXHDyiZj... (32 chars)

# API
API_PORT=3001
```

**Statut** : ✅ **12/12 secrets configurés** dans Doppler (project: `lumvorax`, config: `dev_lumvorax`)

---

## 🤖 BOT TELEGRAM

### Informations Bot

| Champ | Valeur |
|-------|--------|
| **Username** | `@masterdebugai_bot` |
| **Bot ID** | `8820756284` |
| **URL Telegram** | donne |
| **Token** | `8820756284:AAEaeBUd2PIR...` (configuré dans Doppler) |

### Commandes Disponibles

```
/start - Démarrer le bot et voir les instructions
/help - Afficher l'aide complète
/analyze <url> - Analyser un dépôt GitHub
/status <jobId> - Voir le statut d'une analyse
```

---

## 🔧 PERMISSIONS GITHUB APP

### Repository Permissions

| Permission | Niveau | Usage |
|-----------|--------|-------|
| **Contents** | Read & Write | Clone repo, commit rapport |
| **Pull Requests** | Read & Write | Créer PR avec rapport |
| **Issues** | Read & Write | Créer issues si bugs détectés |
| **Metadata** | Read-only | Infos repo (langage, taille) |
| **Workflows** | Read & Write | Analyser GitHub Actions |

### Account Permissions

| Permission | Niveau | Usage |
|-----------|--------|-------|
| **Email addresses** | Read-only | Identifier utilisateur |

### Webhooks Activés

- ✅ **Push** — Analyse automatique sur push
- ✅ **Pull Request** — Analyse PR avant merge
- ✅ **Create** — Nouvelle branche/tag
- ✅ **Repository** — Changements repo

---

## 📦 INSTALLATION GITHUB APP

### Étape 1 : Installer sur Votre Compte

1. **Aller sur** : https://github.com/apps/mdbai-master-debug-ai/installations/new

2. **Sélectionner** :
   - [ ] All repositories (tous les dépôts)
   - [x] Only select repositories (dépôts spécifiques) — **RECOMMANDÉ**

3. **Choisir dépôts** :
   - Sélectionner 1 ou plusieurs dépôts à analyser
   - Exemple : `votre-username/test-repo`

4. **Cliquer** : "Install"

5. **Autoriser** : Accepter les permissions demandées

### Étape 2 : Vérifier Installation

Après installation, vous serez redirigé vers :
```
https://{VOTRE_DOMAINE}/auth/github/callback?code=...&installation_id=...
```

**Note** : Si le serveur MDBAI n'est pas démarré, vous verrez une erreur. C'est normal, l'installation est quand même enregistrée sur GitHub.

### Étape 3 : Vérifier dans GitHub

1. Aller sur : https://github.com/settings/installations
2. Vous devriez voir : **mdbai-master-debug-ai** installée
3. Cliquer dessus pour voir les dépôts autorisés

---

## 🚀 DÉMARRAGE SERVEUR MDBAI

### Option 1 : Script Automatisé (RECOMMANDÉ)

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./start_test_utilisateur.sh
```

**Le script vérifie automatiquement** :
- ✅ node_modules présent (sinon `npm install`)
- ✅ libmdbai_hooks_forensic.so compilé (sinon `make`)
- ✅ Doppler installé et configuré
- ✅ Secrets Doppler complets (12/12)
- ✅ Port 3001 disponible

### Option 2 : Démarrage Manuel

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Vérifier node_modules
if [ ! -d "node_modules" ]; then
    npm install
fi

# Démarrer avec Doppler
doppler run --project lumvorax --config dev_lumvorax -- npm start
```

### Vérification Serveur Démarré

```bash
# Health check
curl http://localhost:3001/health

# Devrait retourner :
# {"status":"ok","redis":"connected","worker":"active"}
```

---

## 🧪 TESTER LE SYSTÈME

### Test 1 : Bot Telegram

1. **Ouvrir Telegram**
2. **Chercher** : `@masterdebugai_bot`
3. **Envoyer** : `/start`
4. **Résultat attendu** :
   ```
   🤖 Bienvenue sur MDBAI - Master Debug AI!
   
   Je peux analyser vos dépôts GitHub avec forensic bit-level.
   
   Commandes disponibles :
   /analyze <url> - Analyser un dépôt
   /status <jobId> - Voir statut analyse
   /help - Aide complète
   ```

### Test 2 : Analyse Dépôt

1. **Dans Telegram, envoyer** :
   ```
   /analyze https://github.com/octocat/Hello-World
   ```

2. **Résultat attendu** :
   ```
   ✅ Analyse démarrée pour octocat/Hello-World
   📊 Job ID: mdbai-xxxxx
   ⏱️ Temps estimé: 3-5 minutes
   
   Je vous notifierai quand l'analyse sera terminée.
   ```

3. **Attendre notification** :
   ```
   ✅ Analyse terminée!
   
   📄 Rapport: https://github.com/octocat/Hello-World/pull/XXX
   
   🔍 Résumé:
   • Score qualité: 85/100
   • Erreurs: 0
   • Warnings: 2
   • Vulnérabilités: 0
   ```

### Test 3 : Vérifier Pull Request

1. **Aller sur GitHub** : https://github.com/octocat/Hello-World/pulls
2. **Voir PR créée** : "🤖 MDBAI Analysis Report"
3. **Ouvrir PR** : Voir le rapport complet `RAPPORT_MDBAI_*.md`
4. **Télécharger fichiers forensiques** : Voir les snapshots `.mdba`

---

## 🔍 VALIDATION MAGIC NUMBER

### Vérifier Format Binaire .mdba

```bash
# Télécharger un snapshot depuis la PR
# Exemple : snapshot_001.mdba

# Vérifier magic number (doit être 0x4D444241 = "MDBA")
xxd -l 80 snapshot_001.mdba | head -5

# Résultat attendu :
# 00000000: 4d44 4241 0100 0000 ...  MDBA........
#           ^^^^ ^^^^
#           Magic number validé ✅
```

---

## 📊 DASHBOARD WEB (OPTIONNEL)

### Accès Dashboard

```bash
# URL : http://localhost:3001/dashboard

# Affiche :
# - Nombre d'analyses en cours
# - Historique analyses
# - Métriques BullMQ
# - Statut Redis
```

---

## 🐛 DÉPANNAGE

### Problème 1 : Serveur ne démarre pas

**Symptôme** : `Error: Cannot find module 'express'`

**Solution** :
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
npm install
```

### Problème 2 : Bot Telegram ne répond pas

**Symptôme** : Pas de réponse à `/start`

**Vérifications** :
```bash
# 1. Vérifier serveur démarré
curl http://localhost:3001/health

# 2. Vérifier token Telegram
doppler secrets get TELEGRAM_BOT_TOKEN --project lumvorax --config dev_lumvorax --plain

# 3. Vérifier logs serveur
tail -f /home/lvx/LVX/lumvorax2/src/MDBAI/logs/mdbai.log
```

### Problème 3 : GitHub App non autorisée

**Symptôme** : Erreur "GitHub App not installed"

**Solution** :
1. Réinstaller : https://github.com/apps/mdbai-master-debug-ai/installations/new
2. Vérifier permissions accordées
3. Redémarrer serveur MDBAI

### Problème 4 : Redis non connecté

**Symptôme** : `Error: Redis connection failed`

**Vérifications** :
```bash
# Tester connexion Redis
redis-cli -h redis-17068.c327.europe-west1-1.gce.redns.redis-cloud.com \
          -p 17068 \
          -a lAOtAuizLK2LyIVlrSdEpPF... \
          PING

# Devrait retourner : PONG
```

---

## 📝 CHECKLIST VALIDATION COMPLÈTE

### Infrastructure

- [x] Secrets Doppler configurés (12/12)
- [x] libmdbai_forensic.so compilé (369 KB)
- [x] libmdbai_hooks_forensic.so compilé (22 KB)
- [x] node_modules installé (92 packages)
- [ ] Serveur MDBAI démarré (port 3001)

### GitHub App

- [x] App créée (ID: 3888479)
- [x] Permissions configurées
- [x] Webhooks activés
- [ ] Installée sur compte utilisateur
- [ ] Testée sur dépôt réel

### Bot Telegram

- [x] Bot créé (@masterdebugai_bot)
- [x] Token configuré dans Doppler
- [ ] Commande /start testée
- [ ] Commande /analyze testée
- [ ] Notifications reçues

### Tests End-to-End

- [ ] Analyse dépôt lancée
- [ ] Pull Request créée
- [ ] Rapport généré
- [ ] Fichiers .mdba téléchargeables
- [ ] Magic number 0x4D444241 validé

---

## 🎯 PROCHAINES ÉTAPES

1. **Démarrer serveur** : `./start_test_utilisateur.sh`
2. **Installer GitHub App** : https://github.com/apps/mdbai-master-debug-ai/installations/new
3. **Connecter Bot Telegram** : @masterdebugai_bot
4. **Lancer analyse** : `/analyze https://github.com/votre-username/votre-repo`
5. **Vérifier résultats** : Pull Request + rapport + fichiers .mdba

---

**Auteur** : Bob (Expert Software Engineer)  
**Date** : 2026-06-03T02:50Z  
**Version** : 1.0.0  
**Statut** : ✅ PRÊT — Tous les secrets configurés, infrastructure validée