# 🌐 URLS NGROK POUR GITHUB APP MDBAI - CONFIGURATION LOCALE

**Date**: 2026-06-03T03:16Z  
**Statut**: ✅ **NGROK ACTIF**  
**URL Publique**: https://headboard-romp-crust.ngrok-free.dev

---

## 🎉 NGROK DÉMARRÉ AVEC SUCCÈS !

### Informations Tunnel

| Paramètre | Valeur |
|-----------|--------|
| **URL Publique HTTPS** | `https://headboard-romp-crust.ngrok-free.dev` |
| **Port Local** | `3001` |
| **Dashboard ngrok** | http://localhost:4040 |
| **Statut** | ✅ Actif |

### Vérification

```bash
# Tester l'URL publique
curl https://headboard-romp-crust.ngrok-free.dev/health

# Devrait retourner :
# {"status":"healthy","redis":"connected",...}
```

---

## 📋 VALEURS EXACTES POUR GITHUB APP

### À remplir dans : https://github.com/settings/apps/mdbai-master-debug-ai

#### 1. Basic Information

**GitHub App name**
```
MDBAI - Master Debug AI
```

**Homepage URL**
```
https://headboard-romp-crust.ngrok-free.dev
```

---

#### 2. Identifying and Authorizing Users

**Callback URL**
```
https://headboard-romp-crust.ngrok-free.dev/auth/github/callback
```

**Request user authorization (OAuth) during installation**
```
☑ COCHER CETTE CASE
```

**Enable Device Flow**
```
☐ NE PAS COCHER
```

---

#### 3. Post Installation

**Setup URL (optional)**
```
LAISSER VIDE
```

**Redirect on update**
```
☐ NE PAS COCHER
```

---

#### 4. Webhook

**Active**
```
☑ COCHER CETTE CASE
```

**Webhook URL**
```
https://headboard-romp-crust.ngrok-free.dev/webhook/github
```

**Webhook Secret**
```
✅ DÉJÀ CONFIGURÉ (dans Doppler: MDBAI_WEBHOOK_SECRET)
Valeur: ed34c4d917c8a90924b0cb9...
```

**SSL verification**
```
☑ Enable SSL verification
```

---

#### 5. Permissions

**Repository Permissions** (à configurer) :

| Permission | Niveau |
|-----------|--------|
| Contents | `Read and write` |
| Pull requests | `Read and write` |
| Issues | `Read and write` |
| Metadata | `Read-only` |
| Workflows | `Read and write` |

**Account Permissions** :

| Permission | Niveau |
|-----------|--------|
| Email addresses | `Read-only` |

---

#### 6. Subscribe to Events

Cocher les événements suivants :
```
☑ Push
☑ Pull request
☑ Create
☑ Repository
```

---

## ✅ APRÈS CONFIGURATION

### 1. Sauvegarder
Cliquer sur **"Save changes"** en bas de la page

### 2. Installer l'App
```
URL : https://github.com/apps/mdbai-master-debug-ai/installations/new
```

### 3. Tester Webhook
```bash
# GitHub enverra un webhook "ping" automatiquement
# Vérifier dans les logs :
tail -f /home/lvx/LVX/lumvorax2/src/MDBAI/server_output.log

# Vous devriez voir :
# [WEBHOOK] Ping reçu de GitHub ✅
```

---

## 🧪 TESTS DE VALIDATION

### Test 1 : Health Check Public

```bash
curl https://headboard-romp-crust.ngrok-free.dev/health
```

**Résultat attendu** :
```json
{
  "status": "healthy",
  "redis": "connected",
  "telegram": true,
  "worker": true
}
```

### Test 2 : Dashboard ngrok

Ouvrir dans navigateur : http://localhost:4040

Vous verrez :
- Toutes les requêtes HTTP en temps réel
- Webhooks GitHub reçus
- Détails des requêtes/réponses

### Test 3 : Bot Telegram

```
1. Ouvrir Telegram
2. Chercher : @masterdebugai_bot
3. Envoyer : /start
4. Envoyer : /analyze https://github.com/octocat/Hello-World
```

### Test 4 : Webhook GitHub

Après installation de l'App :
1. Faire un push sur un dépôt autorisé
2. GitHub enverra webhook à : `https://headboard-romp-crust.ngrok-free.dev/webhook/github`
3. MDBAI lancera analyse automatiquement
4. Notification Telegram reçue

---

## 🔄 SI NGROK REDÉMARRE

### Problème
ngrok génère une **nouvelle URL aléatoire** à chaque redémarrage (version gratuite)

### Solution 1 : Compte ngrok (RECOMMANDÉ)

```bash
# 1. Créer compte gratuit : https://dashboard.ngrok.com/signup
# 2. Récupérer authtoken
# 3. Configurer :
ngrok config add-authtoken VOTRE_TOKEN

# 4. Utiliser domaine fixe (plan payant) ou URL stable (plan gratuit)
```

### Solution 2 : Mettre à jour GitHub App

Si ngrok redémarre avec nouvelle URL :

1. **Récupérer nouvelle URL** :
   ```bash
   curl -s http://localhost:4040/api/tunnels | jq -r '.tunnels[] | select(.proto=="https") | .public_url'
   ```

2. **Mettre à jour GitHub App** :
   - Homepage URL : `https://NOUVELLE-URL.ngrok-free.dev`
   - Callback URL : `https://NOUVELLE-URL.ngrok-free.dev/auth/github/callback`
   - Webhook URL : `https://NOUVELLE-URL.ngrok-free.dev/webhook/github`

3. **Sauvegarder changements**

---

## 📊 MONITORING

### Logs Serveur MDBAI
```bash
tail -f /home/lvx/LVX/lumvorax2/src/MDBAI/server_output.log
```

### Logs ngrok
```bash
tail -f /home/lvx/LVX/lumvorax2/src/MDBAI/ngrok_output.log
```

### Dashboard ngrok (Web UI)
```
http://localhost:4040
```

### Processus actifs
```bash
# Serveur MDBAI
ps aux | grep "node src/server.js"

# ngrok
ps aux | grep ngrok
```

---

## 🛑 ARRÊTER LES SERVICES

### Arrêter Serveur MDBAI
```bash
pkill -f "node src/server.js"
```

### Arrêter ngrok
```bash
pkill ngrok
```

### Arrêter tout
```bash
pkill -f "node src/server.js"
pkill ngrok
```

---

## 🚀 REDÉMARRER TOUT

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# 1. Démarrer serveur MDBAI
doppler run --project lumvorax --config dev_lumvorax -- npm start > server_output.log 2>&1 &

# 2. Attendre 3 secondes
sleep 3

# 3. Démarrer ngrok
ngrok http 3001 > ngrok_output.log 2>&1 &

# 4. Attendre 3 secondes
sleep 3

# 5. Récupérer URL ngrok
curl -s http://localhost:4040/api/tunnels | jq -r '.tunnels[] | select(.proto=="https") | .public_url'
```

---

## 📝 CHECKLIST FINALE

### Infrastructure
- [x] Serveur MDBAI démarré (port 3001)
- [x] ngrok actif (https://headboard-romp-crust.ngrok-free.dev)
- [x] Redis connecté
- [x] Bot Telegram actif
- [x] Worker BullMQ actif

### GitHub App
- [ ] Homepage URL mise à jour
- [ ] Callback URL mise à jour
- [ ] Webhook URL mise à jour
- [ ] Permissions configurées
- [ ] Events configurés
- [ ] Changements sauvegardés

### Tests
- [ ] Health check public fonctionne
- [ ] Dashboard ngrok accessible
- [ ] Bot Telegram répond
- [ ] Webhook GitHub reçu
- [ ] Analyse lancée avec succès

---

## 🎯 PROCHAINES ÉTAPES

1. **Mettre à jour GitHub App** avec les URLs ngrok ci-dessus
2. **Sauvegarder** les changements
3. **Installer l'App** sur votre compte
4. **Tester** avec Bot Telegram : `/analyze https://github.com/octocat/Hello-World`
5. **Vérifier** Pull Request créée automatiquement

---

**Auteur** : Bob (Expert Software Engineer)  
**Date** : 2026-06-03T03:16Z  
**URL ngrok** : https://headboard-romp-crust.ngrok-free.dev  
**Statut** : ✅ PRÊT — Serveur + ngrok actifs, webhooks débloqués !