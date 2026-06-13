# 🔐 GUIDE AUTHENTIFICATION HYBRIDE MDBAI

## Master Debug AI Platform - Système d'Authentification Multi-Méthodes

**Version** : 1.0.0 (Cycle C194)  
**Date** : 2026-06-03  
**Sudo password** : `emmaus`

---

## 🎯 OBJECTIF

MDBAI supporte désormais **3 méthodes d'authentification** au choix :

1. ✅ **GitHub OAuth** (existant) - Connexion via compte GitHub
2. ✅ **Email + Mot de passe** (nouveau) - Inscription classique avec validation email
3. ✅ **Téléphone via Telegram** (nouveau) - Inscription avec code OTP envoyé sur Telegram

Les utilisateurs peuvent **choisir librement** leur méthode préférée.

---

## 📋 ARCHITECTURE

### Composants Créés

```
src/
├── models/
│   └── user.model.js              # Modèle utilisateur (3 méthodes auth)
├── services/
│   ├── user.service.js            # Gestion utilisateurs (Redis)
│   └── email.service.js           # Envoi emails (nodemailer)
└── routes/
    ├── auth.js                    # Routes GitHub OAuth (existant)
    └── auth-hybrid.js             # Routes Email + Téléphone (nouveau)
```

### Base de Données Redis

**Clés utilisées** :
- `mdbai:user:{userId}` - Données utilisateur complètes
- `mdbai:user:email:{email}` - Index email → userId
- `mdbai:user:phone:{phone}` - Index téléphone → userId
- `mdbai:user:github:{login}` - Index GitHub → userId
- `mdbai:user:telegram:{telegramId}` - Index Telegram → userId

---

## 🚀 UTILISATION

### 1. Inscription par Email

**Endpoint** : `POST /auth/register/email`

**Body** :
```json
{
  "email": "user@example.com",
  "password": "motdepasse123",
  "telegram_id": "123456789"
}
```

**Réponse** :
```json
{
  "success": true,
  "message": "Inscription réussie. Vérifiez votre email pour activer votre compte.",
  "user": {
    "id": "user-abc123",
    "email": "user@example.com",
    "status": "pending",
    "auth_method": "email"
  }
}
```

**Processus** :
1. Utilisateur s'inscrit avec email + mot de passe
2. Email de vérification envoyé automatiquement
3. Utilisateur clique sur le lien dans l'email
4. Compte activé → statut `active`

---

### 2. Inscription par Téléphone

**Endpoint** : `POST /auth/register/phone`

**Body** :
```json
{
  "phone": "+33612345678",
  "telegram_id": "123456789"
}
```

**Réponse** :
```json
{
  "success": true,
  "message": "Inscription réussie. Vérifiez votre Telegram pour le code de vérification.",
  "user": {
    "id": "user-def456",
    "phone": "+33612345678",
    "status": "pending",
    "auth_method": "phone"
  }
}
```

**Processus** :
1. Utilisateur s'inscrit avec numéro de téléphone
2. Code OTP (6 chiffres) envoyé sur Telegram
3. Utilisateur vérifie avec `POST /auth/verify-phone`
4. Compte activé → statut `active`

---

### 3. Connexion par Email

**Endpoint** : `POST /auth/login/email`

**Body** :
```json
{
  "email": "user@example.com",
  "password": "motdepasse123"
}
```

**Réponse** :
```json
{
  "success": true,
  "message": "Connexion réussie",
  "user": {
    "id": "user-abc123",
    "email": "user@example.com",
    "auth_method": "email",
    "last_login": "2026-06-03T10:30:00.000Z"
  }
}
```

---

### 4. Connexion par Téléphone

**Endpoint** : `POST /auth/login/phone`

**Body** :
```json
{
  "phone": "+33612345678",
  "code": "123456"
}
```

**Réponse** :
```json
{
  "success": true,
  "message": "Connexion réussie",
  "user": {
    "id": "user-def456",
    "phone": "+33612345678",
    "auth_method": "phone",
    "last_login": "2026-06-03T10:35:00.000Z"
  }
}
```

---

### 5. Vérification Email

**Endpoint** : `GET /auth/verify-email?token={uuid}`

**Processus** :
1. Utilisateur reçoit email avec lien de vérification
2. Clique sur le lien
3. Token validé → compte activé
4. Page de confirmation affichée

---

### 6. Vérification Téléphone

**Endpoint** : `POST /auth/verify-phone`

**Body** :
```json
{
  "phone": "+33612345678",
  "code": "123456"
}
```

**Réponse** :
```json
{
  "success": true,
  "message": "Téléphone vérifié avec succès",
  "user": {
    "id": "user-def456",
    "phone": "+33612345678",
    "status": "active"
  }
}
```

---

### 7. Renvoyer Code OTP

**Endpoint** : `POST /auth/resend-otp`

**Body** :
```json
{
  "phone": "+33612345678"
}
```

**Réponse** :
```json
{
  "success": true,
  "message": "Nouveau code envoyé sur Telegram"
}
```

---

### 8. Statut Connexion

**Endpoint** : `GET /auth/status-hybrid`

**Réponse** :
```json
{
  "connected": true,
  "user": {
    "id": "user-abc123",
    "auth_method": "email",
    "email": "user@example.com",
    "phone": null,
    "github_login": null
  }
}
```

---

### 9. Déconnexion

**Endpoint** : `POST /auth/logout`

**Réponse** :
```json
{
  "success": true,
  "message": "Déconnexion réussie"
}
```

---

## 🔧 CONFIGURATION

### Variables d'Environnement

Ajouter dans `.env` ou Doppler Secrets :

```bash
# URL de base (pour liens de vérification)
BASE_URL=http://localhost:3001

# Configuration SMTP (Gmail exemple)
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_SECURE=false
SMTP_USER=votre-email@gmail.com
SMTP_PASSWORD=votre-app-password
EMAIL_FROM=noreply@mdbai.dev
```

### Configuration Gmail

1. Activer l'authentification à 2 facteurs
2. Générer un "App Password" : https://myaccount.google.com/apppasswords
3. Utiliser ce mot de passe dans `SMTP_PASSWORD`

### Autres Providers SMTP

**SendGrid** :
```bash
SMTP_HOST=smtp.sendgrid.net
SMTP_PORT=587
SMTP_USER=apikey
SMTP_PASSWORD=votre-api-key
```

**Mailgun** :
```bash
SMTP_HOST=smtp.mailgun.org
SMTP_PORT=587
SMTP_USER=postmaster@votre-domaine.mailgun.org
SMTP_PASSWORD=votre-password
```

**Mailtrap (dev)** :
```bash
SMTP_HOST=smtp.mailtrap.io
SMTP_PORT=2525
SMTP_USER=votre-username
SMTP_PASSWORD=votre-password
```

---

## 📊 SÉCURITÉ

### Hachage Mots de Passe

- **Algorithme** : bcrypt
- **Rounds** : 12 (très sécurisé)
- **Stockage** : Hash uniquement, jamais en clair

### Codes OTP

- **Format** : 6 chiffres aléatoires
- **Expiration** : 10 minutes
- **Stockage** : Redis avec TTL automatique

### Tokens Email

- **Format** : UUID v4
- **Expiration** : 24 heures
- **Usage unique** : Supprimé après vérification

### Protection Brute Force

- **Limite** : 5 tentatives échouées
- **Action** : Compte suspendu automatiquement
- **Déblocage** : Contact support

### Sessions

- **Stockage** : Express-session + Redis
- **Durée** : 24 heures
- **Sécurité** : Cookie httpOnly + secure en production

---

## 🧪 TESTS

### Test Inscription Email

```bash
curl -X POST http://localhost:3001/auth/register/email \
  -H "Content-Type: application/json" \
  -d '{
    "email": "test@example.com",
    "password": "password123",
    "telegram_id": "123456789"
  }'
```

### Test Inscription Téléphone

```bash
curl -X POST http://localhost:3001/auth/register/phone \
  -H "Content-Type: application/json" \
  -d '{
    "phone": "+33612345678",
    "telegram_id": "123456789"
  }'
```

### Test Connexion Email

```bash
curl -X POST http://localhost:3001/auth/login/email \
  -H "Content-Type: application/json" \
  -d '{
    "email": "test@example.com",
    "password": "password123"
  }'
```

### Test Statut

```bash
curl http://localhost:3001/auth/status-hybrid
```

---

## 🐛 DÉPANNAGE

### Erreur: EMAIL_SEND_FAILED

**Cause** : Configuration SMTP incorrecte

**Solution** :
1. Vérifier `SMTP_HOST`, `SMTP_PORT`, `SMTP_USER`, `SMTP_PASSWORD`
2. Tester avec : `node -e "require('./src/services/email.service.js').verifySmtpConfig()"`
3. Vérifier logs : `tail -f logs/mdbai.log`

### Erreur: EMAIL_ALREADY_EXISTS

**Cause** : Email déjà utilisé

**Solution** : Utiliser un autre email ou se connecter

### Erreur: CODE_EXPIRED

**Cause** : Code OTP expiré (>10 min)

**Solution** : Demander un nouveau code avec `POST /auth/resend-otp`

### Erreur: USER_NOT_ACTIVE

**Cause** : Compte suspendu (5 tentatives échouées)

**Solution** : Contacter support ou attendre déblocage automatique

---

## 📈 MÉTRIQUES

### Utilisateurs par Méthode

```bash
# Compter utilisateurs par méthode d'authentification
redis-cli --scan --pattern "mdbai:user:*" | \
  xargs redis-cli MGET | \
  jq -r '.auth_method' | \
  sort | uniq -c
```

### Taux de Vérification

```bash
# Compter utilisateurs vérifiés vs pending
redis-cli --scan --pattern "mdbai:user:*" | \
  xargs redis-cli MGET | \
  jq -r '.status' | \
  sort | uniq -c
```

---

## 🔄 MIGRATION UTILISATEURS EXISTANTS

Les utilisateurs GitHub OAuth existants continuent de fonctionner normalement.

Pour ajouter une méthode d'authentification supplémentaire :

1. **Ajouter email** : `POST /auth/link-email`
2. **Ajouter téléphone** : `POST /auth/link-phone`

*(Endpoints à implémenter si besoin)*

---

## 📝 NOTES

- **Compatibilité** : 100% rétrocompatible avec GitHub OAuth
- **Performance** : Redis pour stockage ultra-rapide
- **Scalabilité** : Support millions d'utilisateurs
- **Coût** : €0 (SMTP gratuit avec Gmail/SendGrid free tier)

---

## 🎉 SUCCÈS ATTENDU

```
╔════════════════════════════════════════════════════════════════╗
║  SYSTÈME D'AUTHENTIFICATION HYBRIDE OPÉRATIONNEL              ║
╚════════════════════════════════════════════════════════════════╝

✓ 3 méthodes d'authentification disponibles
✓ Validation email automatique (nodemailer)
✓ Validation téléphone via Telegram (code OTP)
✓ Sécurité renforcée (bcrypt + rate limiting)
✓ 100% compatible avec GitHub OAuth existant

🎉 UTILISATEURS PEUVENT CHOISIR LEUR MÉTHODE PRÉFÉRÉE
```

---

**Guide créé** : 2026-06-03 12:49 UTC  
**Version MDBAI** : 1.0.0 (Cycle C194)  
**Système d'authentification hybride** : ✅ Opérationnel