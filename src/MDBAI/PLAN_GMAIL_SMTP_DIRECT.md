# PLAN — ENVOI EMAIL DIRECT VIA GMAIL SMTP

**Date:** 2026-06-05 19:07 UTC+2  
**Solution:** Utiliser Gmail SMTP avec nodemailer (déjà installé)  
**Cycle:** C198.7

---

## 🎯 SOLUTION : GMAIL SMTP DIRECT

### Principe
Utiliser ton compte Gmail `vgac42@gmail.com` comme serveur SMTP pour envoyer les codes OTP aux utilisateurs.

### Avantages
- ✅ **Gratuit** (500 emails/jour avec Gmail)
- ✅ **Immédiat** (pas de vérification domaine)
- ✅ **Simple** (nodemailer déjà installé)
- ✅ **Fiable** (infrastructure Google)

### Flux Final
```
1. /register → Email
2. Email → Mot de passe
3. Mot de passe → EMAIL ENVOYÉ avec code OTP
   Bot: "📧 Code envoyé à votre email
         Vérifiez votre boîte de réception
         Entrez le code reçu pour activer"
4. User vérifie email → reçoit code 123456
5. User envoie: 123456
6. Bot: "✅ Compte activé!"
```

---

## 🔧 CONFIGURATION GMAIL

### Étape 1: Activer "App Password" Gmail

**Important:** Gmail nécessite un "App Password" (pas le mot de passe normal) pour SMTP.

**Procédure:**
1. Aller sur https://myaccount.google.com/security
2. Activer "2-Step Verification" (si pas déjà fait)
3. Aller sur https://myaccount.google.com/apppasswords
4. Créer un "App Password" pour "Mail"
5. Copier le mot de passe généré (16 caractères)

### Étape 2: Ajouter Variables d'Environnement

**Fichier:** `lumvorax2/src/MDBAI/.env`

```bash
# Gmail SMTP Configuration
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_SECURE=false
SMTP_USER=vgac42@gmail.com
SMTP_PASSWORD=xxxx xxxx xxxx xxxx  # App Password de l'étape 1
EMAIL_FROM=vgac42@gmail.com
```

---

## 📝 MODIFICATIONS CODE

### Fichier 1: `email.service.js`

#### Remplacer Resend par Nodemailer

```javascript
/**
 * MDBAI — Service d'envoi d'emails
 * Utilise Gmail SMTP via nodemailer
 */

import nodemailer from 'nodemailer';
import { config } from '../config.js';
import logger from '../utils/logger.js';

// Configuration transporteur Gmail SMTP
const transporter = nodemailer.createTransport({
  host: config.email.smtpHost,        // smtp.gmail.com
  port: config.email.smtpPort,        // 587
  secure: config.email.smtpSecure,    // false (STARTTLS)
  auth: {
    user: config.email.smtpUser,      // vgac42@gmail.com
    pass: config.email.smtpPassword   // App Password
  }
});

/**
 * Envoie un code de vérification à 6 chiffres par email
 * @param {string} email - Email destinataire
 * @param {string} code - Code de vérification à 6 chiffres
 * @returns {Promise<void>}
 */
export async function sendVerificationCodeEmail(email, code) {
  const mailOptions = {
    from: `"MDBAI Platform" <${config.email.fromAddress}>`,
    to: email,
    subject: '🔐 Votre code de vérification - MDBAI',
    html: `
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="utf-8">
        <style>
          body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; }
          .container { max-width: 600px; margin: 0 auto; padding: 20px; }
          .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; text-align: center; border-radius: 10px 10px 0 0; }
          .content { background: #f9f9f9; padding: 30px; border-radius: 0 0 10px 10px; text-align: center; }
          .code-box { background: #fff; padding: 30px; border: 3px solid #667eea; border-radius: 10px; margin: 30px 0; }
          .code { font-size: 48px; font-weight: bold; letter-spacing: 12px; color: #667eea; font-family: 'Courier New', monospace; }
          .footer { text-align: center; margin-top: 30px; color: #666; font-size: 12px; }
        </style>
      </head>
      <body>
        <div class="container">
          <div class="header">
            <h1>🔐 MDBAI Platform</h1>
            <p>Code de Vérification Telegram</p>
          </div>
          <div class="content">
            <h2>Votre code de vérification</h2>
            <p>📱 Copiez ce code et collez-le dans le bot Telegram <strong>@masterdebugai_bot</strong> :</p>
            
            <div class="code-box">
              <div class="code">${code}</div>
            </div>
            
            <p><strong>⏱ Ce code expire dans 10 minutes.</strong></p>
            
            <hr style="margin: 30px 0; border: none; border-top: 1px solid #ddd;">
            
            <h3>📋 Instructions</h3>
            <ol style="text-align: left; max-width: 400px; margin: 20px auto;">
              <li>Ouvrez Telegram</li>
              <li>Allez sur le bot <strong>@masterdebugai_bot</strong></li>
              <li>Collez le code ci-dessus</li>
              <li>Votre compte sera activé immédiatement</li>
            </ol>
            
            <p style="margin-top: 30px; font-size: 14px; color: #666;">Si vous n'avez pas demandé ce code, ignorez cet email.</p>
          </div>
          <div class="footer">
            <p>© 2026 MDBAI Platform - Master Debug AI</p>
            <p>Analyse forensique automatisée pour GitHub</p>
          </div>
        </div>
      </body>
      </html>
    `
  };
  
  try {
    const info = await transporter.sendMail(mailOptions);
    
    logger.info('[EMAIL] Code de vérification envoyé via Gmail SMTP', {
      email,
      messageId: info.messageId
    });
  } catch (error) {
    logger.error('[EMAIL] Erreur envoi code vérification Gmail', {
      email,
      error: error.message
    });
    throw error;
  }
}

// Exporter aussi pour autres fonctions
export { transporter };
```

### Fichier 2: `config.js`

#### Vérifier Configuration Email

```javascript
email: {
  smtpHost: process.env.SMTP_HOST || 'smtp.gmail.com',
  smtpPort: parseInt(process.env.SMTP_PORT || '587', 10),
  smtpSecure: process.env.SMTP_SECURE === 'true',
  smtpUser: process.env.SMTP_USER || '',
  smtpPassword: process.env.SMTP_PASSWORD || '',
  fromAddress: process.env.EMAIL_FROM || 'vgac42@gmail.com',
},
```

### Fichier 3: `telegram.service.js`

#### Message Après Création Compte

```javascript
// Ligne ~370 - Après création utilisateur
if (registration.step === 'waiting_password') {
  // ... validation password ...
  
  // Créer utilisateur
  const newUser = await registerUser({
    email: registration.email,
    password: text,
    telegram_id: telegramId,
    username: registration.username,
    auth_method: 'email'
  });
  
  // Envoyer code par email (Gmail SMTP)
  await sendVerificationCodeEmail(registration.email, newUser.email_verification_code);
  
  // Message utilisateur
  await this.bot.sendMessage(chatId,
    `Compte cree!\n\n` +
    `Un code de verification a ete envoye a:\n${registration.email}\n\n` +
    `Verifiez votre boite de reception (et spam).\n` +
    `Entrez le code recu pour activer votre compte.\n\n` +
    `Le code expire dans 10 minutes.`);
  
  // Mettre à jour état inscription
  registration.step = 'waiting_code';
  registration.userId = newUser.id;
  this.pendingRegistrations.set(chatId, registration);
  
  logger.info(`[TELEGRAM] Code OTP envoyé par email à ${registration.email}`);
}
```

---

## 🔐 SÉCURITÉ GMAIL

### Limites Gmail Gratuites
- **500 emails/jour** (largement suffisant pour MVP)
- **100 destinataires/email**
- Pas de limite sur nombre d'utilisateurs

### Bonnes Pratiques
- ✅ Utiliser App Password (pas mot de passe principal)
- ✅ Activer 2FA sur compte Gmail
- ✅ Surveiller activité compte Gmail
- ✅ Ne jamais commit App Password dans Git

### Variables Sensibles
```bash
# .env (NE PAS COMMIT)
SMTP_PASSWORD=xxxx xxxx xxxx xxxx  # App Password Gmail

# .gitignore
.env
.env.local
```

---

## ✅ CHECKLIST IMPLÉMENTATION

### Phase 1: Configuration Gmail
- [ ] Activer 2-Step Verification sur vgac42@gmail.com
- [ ] Créer App Password Gmail
- [ ] Ajouter variables SMTP dans `.env`
- [ ] Vérifier nodemailer installé (`npm list nodemailer`)

### Phase 2: Modifications Code
- [ ] Modifier `email.service.js` - Remplacer Resend par nodemailer
- [ ] Vérifier `config.js` - Configuration email
- [ ] Modifier `telegram.service.js` - Message "code envoyé par email"
- [ ] Ajouter étape validation code (déjà planifiée)

### Phase 3: Tests
- [ ] Test envoi email local
- [ ] Test flux complet: /register → email → password → code email → validation
- [ ] Test code invalide
- [ ] Test code expiré
- [ ] Vérifier emails arrivent (inbox + spam)

---

## 📊 COMPARAISON SOLUTIONS

| Critère | Resend (domaine) | Gmail SMTP | Affichage Telegram |
|---------|------------------|------------|-------------------|
| **Coût** | 10€/an | Gratuit | Gratuit |
| **Setup** | 24-48h | 5 minutes | Immédiat |
| **Limite** | Illimité | 500/jour | Aucune |
| **Pro** | Professionnel | Fiable | Simple |
| **Cons** | Coût + délai | Limite quotidienne | Pas d'email |

**Recommandation:** Gmail SMTP pour MVP, migration Resend si >500 users/jour.

---

## 🚀 MIGRATION FUTURE (Optionnelle)

### Quand Migrer vers Resend ?
- **Trigger:** >500 inscriptions/jour
- **Ou:** Besoin emails transactionnels (notifications, rapports)
- **Ou:** Image professionnelle (no-reply@mdbai.dev)

### Étapes Migration
1. Vérifier domaine sur Resend
2. Modifier `email.service.js` - Remplacer nodemailer par fetch Resend
3. Tester en parallèle
4. Basculer progressivement

---

## 📝 LEÇONS APPRISES

### LEÇON-182: Gmail SMTP Alternative Gratuite à Resend
**Contexte:** Resend mode test limite destinataires.

**Solution:** Gmail SMTP avec nodemailer (500 emails/jour gratuit).

**Avantage:** Setup 5 minutes, pas de coût, fiable.

### LEÇON-183: App Password Gmail Obligatoire pour SMTP
**Contexte:** Gmail bloque connexions SMTP avec mot de passe normal.

**Solution:** Créer App Password dédié (16 caractères).

**Sécurité:** 2FA obligatoire, App Password révocable.

---

**Prochaine Action:** Créer App Password Gmail et implémenter nodemailer dans `email.service.js`