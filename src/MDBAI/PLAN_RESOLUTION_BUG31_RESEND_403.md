# PLAN DE RÉSOLUTION — BUG #31 — ERREUR RESEND 403

**Date:** 2026-06-05 18:32 UTC+2  
**Priorité:** CRITIQUE  
**Cycle:** C198.7

---

## 📋 ANALYSE DU PROBLÈME

### Erreur Actuelle
```
Resend API error: 403 - {
  "statusCode":403,
  "name":"validation_error",
  "message":"You can only send testing emails to your own email address (vgac42@gmail.com). 
  To send emails to other recipients, please verify a domain at resend.com/domains, 
  and change the `from` address to an email using this domain."
}
```

### Cause Racine
**Resend en mode TEST** ne permet d'envoyer des emails QU'À l'adresse email du compte (vgac42@gmail.com). Pour envoyer à d'autres utilisateurs, il faut :
1. Vérifier un domaine sur resend.com/domains
2. Utiliser une adresse `from` avec ce domaine vérifié

### Impact
- ❌ Impossible d'envoyer code OTP aux nouveaux utilisateurs
- ❌ Flux inscription bloqué à l'étape 2/3
- ❌ Utilisateurs ne peuvent pas activer leur compte

---

## 🎯 SOLUTIONS POSSIBLES

### Option A : Vérifier un Domaine (PRODUCTION)
**Avantages:**
- ✅ Solution permanente
- ✅ Emails professionnels (no-reply@tondomaine.com)
- ✅ Pas de limite d'envoi

**Inconvénients:**
- ❌ Nécessite un domaine (coût ~10€/an)
- ❌ Configuration DNS (TXT, MX records)
- ❌ Délai de vérification (quelques heures)

**Étapes:**
1. Acheter domaine (ex: mdbai.dev)
2. Ajouter domaine sur resend.com/domains
3. Configurer DNS (TXT records pour SPF/DKIM)
4. Attendre vérification
5. Modifier `from: 'no-reply@mdbai.dev'`

---

### Option B : Mode DEV - Affichage Code dans Telegram (TEMPORAIRE) ⭐
**Avantages:**
- ✅ Solution immédiate (0€)
- ✅ Pas de configuration externe
- ✅ Fonctionne pour tests MVP
- ✅ UX acceptable pour early adopters

**Inconvénients:**
- ⚠️ Pas d'email de confirmation
- ⚠️ Moins professionnel
- ⚠️ Nécessite domaine pour production

**Flux Modifié:**
```
1. Utilisateur: /register
2. Bot: "Entrez votre email"
3. Utilisateur: email@example.com
4. Bot: "Entrez votre mot de passe"
5. Utilisateur: MotDePasse123
6. Bot: "Votre code de vérification: 123456
       Copiez ce code et envoyez-le pour valider votre compte.
       ⏱ Expire dans 10 minutes"
7. Utilisateur: 123456
8. Bot: "✅ Compte activé!"
```

---

## 🚀 SOLUTION RETENUE : OPTION B (MODE DEV)

### Justification
- **Budget 0€** : Pas de domaine nécessaire pour MVP
- **Rapidité** : Implémentation immédiate
- **Tests** : Permet de valider le flux complet
- **Migration** : Facile de passer à Option A plus tard

### Architecture Technique

#### 1. Génération Code OTP
```javascript
// user.model.js - Fonction existante
const code = Math.floor(100000 + Math.random() * 900000).toString();
const expires = Date.now() + 10 * 60 * 1000; // 10 minutes
```

#### 2. Stockage Redis
```javascript
// Clés Redis
mdbai:user:{userId}                    // Données utilisateur
mdbai:user:telegram:{telegramId}       // Index telegram
mdbai:user:email:{email}               // Index email
mdbai:otp:{email}                      // Code OTP (hash)
mdbai:otp:attempts:{email}             // Compteur tentatives
```

#### 3. Flux Telegram Modifié

**Étape 1 - Email:**
```
Bot: "Inscription MDBAI\n\nEtape 1/3: Entrez votre adresse email"
User: "email@example.com"
```

**Étape 2 - Mot de passe:**
```
Bot: "Email enregistré: email@example.com\n\nEtape 2/3: Entrez votre mot de passe\n(minimum 8 caractères)"
User: "MotDePasse123"
```

**Étape 3 - Code OTP (NOUVEAU):**
```
Bot: "✅ Compte créé!\n\n🔐 Votre code de vérification:\n\n[ 123456 ]\n\n📋 Instructions:\n1. Copiez ce code\n2. Envoyez-le dans ce chat\n3. Votre compte sera activé\n\n⏱ Ce code expire dans 10 minutes"
User: "123456"
Bot: "✅ Compte activé avec succès!\n\nVous pouvez maintenant utiliser:\n/github — Connecter GitHub\n/analyze — Lancer une analyse"
```

---

## 📝 MODIFICATIONS À APPORTER

### Fichier 1: `telegram.service.js`

#### Modification `_handleRegistrationStep()` - Ligne ~350
```javascript
// AVANT (étape password)
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
  
  // ANCIEN: Envoyer email
  await sendVerificationCodeEmail(registration.email, newUser.email_verification_code);
  
  // ANCIEN: Message
  await this.bot.sendMessage(chatId,
    `Compte cree!\n\nUn code de verification a ete envoye a ${registration.email}.\n\nCopiez le code et envoyez-le ici pour activer votre compte.`);
}

// APRÈS (étape password)
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
  
  // NOUVEAU: Afficher code directement dans Telegram
  await this.bot.sendMessage(chatId,
    `Compte cree!\n\n` +
    `Code de verification:\n\n` +
    `[ ${newUser.email_verification_code} ]\n\n` +
    `Instructions:\n` +
    `1. Copiez ce code\n` +
    `2. Envoyez-le dans ce chat\n` +
    `3. Votre compte sera active\n\n` +
    `Ce code expire dans 10 minutes`);
  
  // Mettre à jour état inscription
  registration.step = 'waiting_code';
  registration.userId = newUser.id;
  this.pendingRegistrations.set(chatId, registration);
  
  logger.info(`[TELEGRAM] Code OTP affiché pour ${registration.email}`);
}
```

#### Ajout Nouvelle Étape - Validation Code
```javascript
// NOUVEAU: Étape validation code OTP
if (registration.step === 'waiting_code') {
  const code = text.trim();
  
  // Valider format code (6 chiffres)
  if (!/^\d{6}$/.test(code)) {
    await this.bot.sendMessage(chatId,
      `Code invalide. Le code doit contenir 6 chiffres.\n\nReessayez ou tapez /register pour recommencer.`);
    return;
  }
  
  // Récupérer utilisateur
  const user = await findUserById(registration.userId);
  
  if (!user) {
    await this.bot.sendMessage(chatId,
      `Erreur: Utilisateur introuvable. Reessayez avec /register`);
    this.pendingRegistrations.delete(chatId);
    return;
  }
  
  // Vérifier code
  if (user.email_verification_code !== code) {
    await this.bot.sendMessage(chatId,
      `Code incorrect. Reessayez.`);
    return;
  }
  
  // Vérifier expiration
  if (Date.now() > user.email_verification_code_expires) {
    await this.bot.sendMessage(chatId,
      `Code expire. Tapez /register pour recommencer.`);
    this.pendingRegistrations.delete(chatId);
    return;
  }
  
  // Activer compte
  await updateUser(user.id, {
    email_verified: true,
    status: 'active',
    email_verification_code: null,
    email_verification_code_expires: null
  });
  
  // Supprimer état inscription
  this.pendingRegistrations.delete(chatId);
  
  // Message succès
  await this.bot.sendMessage(chatId,
    `Compte active avec succes!\n\n` +
    `Vous pouvez maintenant utiliser:\n` +
    `/github — Connecter votre compte GitHub\n` +
    `/analyze — Lancer une analyse forensique`);
  
  logger.info(`[TELEGRAM] Compte ${user.email} activé avec succès`);
}
```

### Fichier 2: `email.service.js`

#### Modification `sendVerificationCodeEmail()` - Ligne 118
```javascript
// AVANT
export async function sendVerificationCodeEmail(email, code) {
  const emailData = {
    from: 'MDBAI Platform <onboarding@resend.dev>',
    to: [email],
    // ... reste du code ...
  };
  
  // Envoi email
  const response = await fetch(RESEND_API_URL, { /* ... */ });
}

// APRÈS - Mode DEV (ne pas envoyer email)
export async function sendVerificationCodeEmail(email, code) {
  // MODE DEV: Ne pas envoyer email (Resend 403)
  // Le code est affiché directement dans Telegram
  logger.info('[EMAIL] Mode DEV - Code OTP non envoyé par email', {
    email,
    code: code.substring(0, 2) + '****' // Log partiel pour sécurité
  });
  
  // TODO: Activer envoi email après vérification domaine
  // const emailData = { ... };
  // const response = await fetch(RESEND_API_URL, { ... });
  
  return; // Pas d'envoi en mode DEV
}
```

---

## ✅ CHECKLIST IMPLÉMENTATION

### Phase 1: Modifications Code
- [ ] Modifier `telegram.service.js` - Étape password (afficher code)
- [ ] Ajouter `telegram.service.js` - Étape validation code
- [ ] Modifier `email.service.js` - Désactiver envoi email (mode DEV)
- [ ] Tester localement avec utilisateur test

### Phase 2: Tests
- [ ] Test flux complet: /register → email → password → code → activation
- [ ] Test code invalide (format incorrect)
- [ ] Test code expiré (après 10 minutes)
- [ ] Test code incorrect (mauvais chiffres)
- [ ] Test utilisateur déjà inscrit

### Phase 3: Documentation
- [ ] Créer `RAPPORT_RESOLUTION_BUG31_C198.7.md`
- [ ] Mettre à jour `LEÇONS_APPRISES_MDBAI.md`
- [ ] Documenter limitation Resend dans README
- [ ] Ajouter TODO pour vérification domaine

---

## 📊 PLAN MIGRATION PRODUCTION

### Quand Vérifier un Domaine ?
- **Trigger:** Passage de 10+ utilisateurs actifs
- **Budget:** ~10€/an pour domaine
- **Délai:** 24-48h pour vérification DNS

### Étapes Migration
1. Acheter domaine (ex: mdbai.dev)
2. Ajouter sur resend.com/domains
3. Configurer DNS (TXT records)
4. Attendre vérification
5. Modifier `from: 'no-reply@mdbai.dev'`
6. Réactiver `sendVerificationCodeEmail()`
7. Supprimer affichage code dans Telegram

---

## 🔐 SÉCURITÉ

### Mesures Actuelles
- ✅ Code 6 chiffres aléatoire
- ✅ Expiration 10 minutes
- ✅ Stockage hash dans Redis
- ✅ Validation format strict

### Améliorations Futures
- [ ] Limite tentatives (3 max)
- [ ] Cooldown entre codes (1 minute)
- [ ] Rate limiting par IP
- [ ] Logs forensiques tentatives

---

## 📝 LEÇONS APPRISES

### LEÇON-180: Resend Mode Test Limite Destinataires
**Contexte:** Resend en mode test ne permet d'envoyer qu'à l'email du compte.

**Solution:** Afficher code OTP directement dans Telegram pour MVP.

**Production:** Vérifier domaine pour envoi emails illimité.

### LEÇON-181: MVP Peut Fonctionner Sans Email
**Contexte:** Flux inscription peut être 100% Telegram pour tests.

**Avantage:** Pas de dépendance externe, budget 0€.

**Limitation:** Moins professionnel, nécessite domaine pour scale.

---

**Prochaine Action:** Implémenter modifications dans `telegram.service.js` et `email.service.js`