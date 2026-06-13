# 🚀 DÉMARRAGE RAPIDE - TEST TELEGRAM

**Pour tester le système d'authentification hybride via Telegram en 5 minutes**

---

## ⚡ MÉTHODE AUTOMATIQUE (RECOMMANDÉE)

### 1. Exécuter le script de configuration

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./setup_and_start.sh
```

Le script va :
- ✅ Vérifier et démarrer Redis
- ✅ Installer les dépendances Node.js
- ✅ Générer un SESSION_SECRET sécurisé
- ✅ Vous demander votre token Telegram
- ✅ Créer le fichier `.env`
- ✅ Démarrer le serveur

### 2. Obtenir votre token Telegram

**Sur Telegram** :
1. Rechercher `@BotFather`
2. Envoyer `/mybots`
3. Sélectionner `masterdebugai_bot`
4. Cliquer `API Token` → `Revoke current token` → `Yes`
5. Cliquer `API Token` → `Generate new token`
6. **Copier le token** (format: `1234567890:ABCdef...`)

### 3. Tester via Telegram

1. **Ouvrir Telegram** et rechercher `@masterdebugai_bot`
2. **Envoyer** `/start`
3. **Copier votre Telegram ID** (le bot vous l'envoie)
4. **Inscription** :
   ```bash
   curl -X POST http://localhost:3001/auth/register/phone \
     -H "Content-Type: application/json" \
     -d '{"phone":"+33612345678","telegramId":"VOTRE_ID","name":"Test"}'
   ```
5. **Recevoir le code OTP** sur Telegram
6. **Vérifier** :
   ```bash
   curl -X POST http://localhost:3001/auth/verify-phone \
     -H "Content-Type: application/json" \
     -d '{"phone":"+33612345678","code":"123456"}'
   ```

---

## 🔧 MÉTHODE MANUELLE

### 1. Démarrer Redis
```bash
echo "emmaus" | sudo -S systemctl start redis-server
redis-cli ping  # Doit répondre: PONG
```

### 2. Créer le fichier .env
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
nano .env
```

**Contenu minimal** :
```bash
NODE_ENV=development
API_PORT=3001
LOG_LEVEL=debug
BASE_URL=http://localhost:3001
SESSION_SECRET=$(node -e "console.log(require('crypto').randomBytes(32).toString('hex'))")
TELEGRAM_BOT_TOKEN=VOTRE_TOKEN_ICI
REDIS_HOST=localhost
REDIS_PORT=6379
```

### 3. Démarrer le serveur
```bash
npm start
```

---

## 📋 COMMANDES DE TEST RAPIDES

### Health Check
```bash
curl http://localhost:3001/health
```

### Statut du système
```bash
curl http://localhost:3001/auth/status-hybrid
```

### Inscription Email (sans Telegram)
```bash
curl -X POST http://localhost:3001/auth/register/email \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","password":"Test123!","name":"Test User"}'
```

### Vérifier Redis
```bash
redis-cli KEYS 'mdbai:*'
```

---

## 📖 DOCUMENTATION COMPLÈTE

- **Guide détaillé** : [`GUIDE_CONFIGURATION_TEST_TELEGRAM.md`](./GUIDE_CONFIGURATION_TEST_TELEGRAM.md)
- **Architecture** : [`GUIDE_AUTHENTIFICATION_HYBRIDE.md`](./GUIDE_AUTHENTIFICATION_HYBRIDE.md)
- **Sécurité** : [`SECURITY.md`](./SECURITY.md)

---

## ✅ RÉSULTAT ATTENDU

Après le démarrage, vous devriez voir :
```
✅ [REDIS] Connexion établie
✅ [TELEGRAM] Bot initialisé: @masterdebugai_bot
✅ [MDBAI] Serveur Express démarré sur port 3001
```

**Le système est prêt pour vos tests manuels ! 🎯**