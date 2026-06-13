# 🚀 GUIDE DE CONFIGURATION POUR TEST TELEGRAM COMPLET

**Date** : 2026-06-03  
**Version** : 1.0.0  
**Objectif** : Configurer MDBAI pour tester l'authentification hybride via Telegram

---

## 📋 PRÉREQUIS

### 1. Redis (Installation en cours...)
```bash
# Installation Redis
sudo apt-get update
sudo apt-get install -y redis-server

# Démarrer Redis
sudo systemctl start redis-server
sudo systemctl enable redis-server

# Vérifier Redis
redis-cli ping
# Doit répondre: PONG
```

### 2. Bot Telegram (@masterdebugai_bot)
Le bot existe déjà : **@masterdebugai_bot**

**⚠️ IMPORTANT** : Vous devez obtenir un NOUVEAU token depuis @BotFather car l'ancien a été révoqué.

---

## 🔧 ÉTAPE 1 : OBTENIR UN NOUVEAU TOKEN TELEGRAM

### Sur Telegram (via votre téléphone ou application)

1. **Ouvrir @BotFather**
   - Rechercher `@BotFather` dans Telegram
   - Démarrer une conversation

2. **Révoquer l'ancien token** (sécurité)
   ```
   /mybots
   → Sélectionner "masterdebugai_bot"
   → API Token
   → Revoke current token
   → Yes, I'm sure
   ```

3. **Générer un nouveau token**
   ```
   /mybots
   → Sélectionner "masterdebugai_bot"
   → API Token
   → Generate new token
   ```

4. **Copier le token**
   - Format : `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
   - ⚠️ **NE JAMAIS partager ce token publiquement !**

---

## 🔧 ÉTAPE 2 : CRÉER LE FICHIER .env

### Créer le fichier
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
nano .env
```

### Contenu du fichier .env
```bash
# ── Application ───────────────────────────────────────────────────────────
NODE_ENV=development
API_PORT=3001
LOG_LEVEL=debug
BASE_URL=http://localhost:3001

# ── Session (générer avec: node -e "console.log(require('crypto').randomBytes(32).toString('hex'))")
SESSION_SECRET=a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2

# ── Telegram Bot (@masterdebugai_bot) ─────────────────────────────────────
# ⚠️ REMPLACER PAR VOTRE NOUVEAU TOKEN DEPUIS @BotFather
TELEGRAM_BOT_TOKEN=VOTRE_NOUVEAU_TOKEN_ICI

# ── Redis Local ───────────────────────────────────────────────────────────
REDIS_HOST=localhost
REDIS_PORT=6379
# Pas de mot de passe pour Redis local

# ── Email SMTP (optionnel pour ce test) ───────────────────────────────────
# Pour tester l'inscription par email, configurer un compte SMTP
# Exemple avec Gmail (nécessite App Password):
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_SECURE=false
SMTP_USER=votre-email@gmail.com
SMTP_PASSWORD=votre-app-password
EMAIL_FROM=noreply@mdbai.dev

# ── GitHub OAuth (optionnel pour ce test) ─────────────────────────────────
# Laisser vide si vous testez uniquement Telegram
GITHUB_APP_ID=
GITHUB_APP_CLIENT_ID=
GITHUB_APP_CLIENT_SECRET=
GITHUB_WEBHOOK_SECRET=
GITHUB_PRIVATE_KEY=
```

### Sauvegarder et quitter
- `Ctrl + O` (sauvegarder)
- `Entrée` (confirmer)
- `Ctrl + X` (quitter)

---

## 🔧 ÉTAPE 3 : GÉNÉRER UN SESSION_SECRET SÉCURISÉ

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
node -e "console.log(require('crypto').randomBytes(32).toString('hex'))"
```

**Copier le résultat** et remplacer la valeur de `SESSION_SECRET` dans `.env`

---

## 🚀 ÉTAPE 4 : DÉMARRER LE SERVEUR AVEC TOUTES LES FONCTIONNALITÉS

### Arrêter le serveur actuel
```bash
# Dans le terminal où npm start tourne
Ctrl + C
```

### Redémarrer avec la nouvelle configuration
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
npm start
```

### Vérifier les logs
Vous devriez voir :
```
✅ [REDIS] Connexion établie
✅ [TELEGRAM] Bot initialisé: @masterdebugai_bot
✅ [MDBAI] Serveur Express démarré sur port 3001
```

---

## 📱 ÉTAPE 5 : TESTER VIA TELEGRAM

### 5.1 Inscription par Téléphone

1. **Ouvrir Telegram** sur votre téléphone

2. **Rechercher le bot** : `@masterdebugai_bot`

3. **Démarrer une conversation** : `/start`

4. **Obtenir votre Telegram ID**
   - Le bot vous enverra votre ID automatiquement
   - Format : `123456789`
   - **Copier cet ID** (vous en aurez besoin)

5. **Inscription via API REST**
   ```bash
   curl -X POST http://localhost:3001/auth/register/phone \
     -H "Content-Type: application/json" \
     -d '{
       "phone": "+33612345678",
       "telegramId": "VOTRE_TELEGRAM_ID_ICI",
       "name": "Test User"
     }'
   ```

6. **Recevoir le code OTP**
   - Le bot vous enverra un code à 6 chiffres
   - Exemple : `Code OTP: 123456`
   - **Copier ce code**

7. **Vérifier le téléphone**
   ```bash
   curl -X POST http://localhost:3001/auth/verify-phone \
     -H "Content-Type: application/json" \
     -d '{
       "phone": "+33612345678",
       "code": "123456"
     }'
   ```

8. **Connexion**
   ```bash
   curl -X POST http://localhost:3001/auth/login/phone \
     -H "Content-Type: application/json" \
     -d '{
       "phone": "+33612345678"
     }'
   ```

9. **Recevoir le nouveau code OTP** et vérifier
   ```bash
   curl -X POST http://localhost:3001/auth/verify-phone \
     -H "Content-Type: application/json" \
     -d '{
       "phone": "+33612345678",
       "code": "NOUVEAU_CODE"
     }'
   ```

---

## 📧 ÉTAPE 6 : TESTER L'INSCRIPTION PAR EMAIL (OPTIONNEL)

### 6.1 Configuration Gmail (recommandé pour test)

1. **Activer l'authentification à 2 facteurs** sur votre compte Gmail

2. **Générer un App Password**
   - Aller sur : https://myaccount.google.com/apppasswords
   - Sélectionner "Mail" et "Autre (nom personnalisé)"
   - Nom : "MDBAI Test"
   - Copier le mot de passe généré (16 caractères)

3. **Mettre à jour .env**
   ```bash
   SMTP_USER=votre-email@gmail.com
   SMTP_PASSWORD=le-mot-de-passe-app-16-chars
   ```

4. **Redémarrer le serveur**

### 6.2 Test d'inscription

1. **Inscription**
   ```bash
   curl -X POST http://localhost:3001/auth/register/email \
     -H "Content-Type: application/json" \
     -d '{
       "email": "votre-email@gmail.com",
       "password": "MotDePasse123!",
       "name": "Test User Email"
     }'
   ```

2. **Vérifier votre boîte email**
   - Vous recevrez un email de vérification
   - Cliquer sur le lien ou copier le token

3. **Vérifier l'email**
   ```bash
   curl "http://localhost:3001/auth/verify-email?token=VOTRE_TOKEN"
   ```

4. **Connexion**
   ```bash
   curl -X POST http://localhost:3001/auth/login/email \
     -H "Content-Type: application/json" \
     -d '{
       "email": "votre-email@gmail.com",
       "password": "MotDePasse123!"
     }'
   ```

---

## 🔍 ÉTAPE 7 : VÉRIFIER LES DONNÉES DANS REDIS

```bash
# Se connecter à Redis
redis-cli

# Lister toutes les clés
KEYS mdbai:*

# Voir un utilisateur spécifique
GET mdbai:user:user-XXXXX

# Voir l'index email
GET mdbai:user:email:votre-email@gmail.com

# Voir l'index téléphone
GET mdbai:user:phone:+33612345678

# Voir l'index Telegram
GET mdbai:user:telegram:123456789

# Quitter Redis
EXIT
```

---

## 📊 ÉTAPE 8 : VÉRIFIER LE STATUT DU SYSTÈME

```bash
# Health check
curl http://localhost:3001/health

# Statut authentification
curl http://localhost:3001/auth/status-hybrid
```

**Réponse attendue** :
```json
{
  "status": "operational",
  "methods": {
    "github": false,
    "email": true,
    "phone": true
  },
  "services": {
    "redis": true,
    "telegram": true,
    "smtp": true
  }
}
```

---

## 🐛 DÉPANNAGE

### Redis ne démarre pas
```bash
sudo systemctl status redis-server
sudo journalctl -u redis-server -n 50
```

### Bot Telegram ne répond pas
- Vérifier que le token est correct dans `.env`
- Vérifier les logs du serveur
- Tester le token : `curl https://api.telegram.org/bot<TOKEN>/getMe`

### Emails non reçus
- Vérifier le dossier spam
- Vérifier les logs du serveur
- Tester la connexion SMTP avec un outil comme `telnet smtp.gmail.com 587`

### Erreur "Connection is closed" (Redis)
```bash
# Redémarrer Redis
sudo systemctl restart redis-server

# Vérifier la connexion
redis-cli ping
```

---

## 📝 COMMANDES UTILES

### Logs en temps réel
```bash
# Logs du serveur (dans le terminal npm start)
# Les logs s'affichent automatiquement

# Logs Redis
sudo journalctl -u redis-server -f

# Tester le bot Telegram
curl https://api.telegram.org/bot<VOTRE_TOKEN>/getMe
```

### Réinitialiser Redis (effacer toutes les données)
```bash
redis-cli FLUSHALL
```

### Arrêter proprement
```bash
# Arrêter le serveur Node.js
Ctrl + C (dans le terminal npm start)

# Arrêter Redis
sudo systemctl stop redis-server
```

---

## ✅ CHECKLIST DE TEST COMPLET

- [ ] Redis installé et démarré
- [ ] Nouveau token Telegram obtenu depuis @BotFather
- [ ] Fichier `.env` créé avec le token
- [ ] SESSION_SECRET généré et configuré
- [ ] Serveur démarré sans erreurs
- [ ] Bot Telegram répond au `/start`
- [ ] Telegram ID obtenu
- [ ] Inscription par téléphone réussie
- [ ] Code OTP reçu sur Telegram
- [ ] Vérification téléphone réussie
- [ ] Connexion par téléphone réussie
- [ ] (Optionnel) Inscription par email réussie
- [ ] (Optionnel) Email de vérification reçu
- [ ] (Optionnel) Vérification email réussie
- [ ] (Optionnel) Connexion par email réussie
- [ ] Données visibles dans Redis
- [ ] Health check retourne "operational"

---

## 🎯 RÉSULTAT ATTENDU

À la fin de ce guide, vous aurez :

1. ✅ **Serveur MDBAI opérationnel** avec Redis et Telegram
2. ✅ **Bot Telegram fonctionnel** envoyant des codes OTP
3. ✅ **Système d'authentification hybride** testé manuellement
4. ✅ **Données persistées** dans Redis
5. ✅ **Validation complète** du cycle inscription → vérification → connexion

---

## 📞 SUPPORT

Si vous rencontrez des problèmes :

1. **Vérifier les logs** du serveur (terminal npm start)
2. **Vérifier Redis** : `redis-cli ping`
3. **Vérifier le bot** : `curl https://api.telegram.org/bot<TOKEN>/getMe`
4. **Consulter** les rapports forensiques dans `lumvorax2/src/MDBAI/`

---

**Bon test ! 🚀**