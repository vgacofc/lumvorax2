# 📧 GUIDE CONFIGURATION EMAIL SMTP - MDBAI

## 🔍 ANALYSE: Où sont passées les informations SMTP locales?

### Résultat de l'Audit Forensique

**CONSTAT**: Les variables SMTP n'ont **JAMAIS été configurées** en local.

**Preuve**:
```bash
# Recherche dans tous les fichiers .env
grep -r "SMTP" /home/lvx/LVX/lumvorax2/src/MDBAI/.env*

# Résultat: Seulement dans .env.example (template)
/home/lvx/LVX/lumvorax2/src/MDBAI/.env.example:SMTP_HOST=smtp.gmail.com
/home/lvx/LVX/lumvorax2/src/MDBAI/.env.example:SMTP_USER=votre-email@gmail.com
/home/lvx/LVX/lumvorax2/src/MDBAI/.env.example:SMTP_PASSWORD=votre-app-password
```

**Fichier .env actuel**: Aucune variable SMTP présente (vérifié ligne 1-114)

### Pourquoi ça "fonctionnait" avant?

**Réponse**: Ça ne fonctionnait PAS vraiment!

Le code utilise un **mode fallback Ethereal** quand SMTP n'est pas configuré:

```javascript
// lumvorax2/src/MDBAI/src/services/email.service.js:33-43
if (!process.env.SMTP_USER || !process.env.SMTP_PASS) {
  logger.warn('Configuration SMTP manquante, mode test');
  const testAccount = await nodemailer.createTestAccount();
  config.host = 'smtp.ethereal.email';  // ← Email de TEST
  config.port = 587;
  config.auth = {
    user: testAccount.user,
    pass: testAccount.pass
  };
}
```

**Ethereal Email** = Service de test qui capture les emails mais ne les envoie PAS vraiment.

Les emails étaient "envoyés" vers Ethereal (visible dans les logs) mais **jamais reçus** par les utilisateurs.

---

## 🎯 SOLUTION: Configuration Gmail SMTP

### Étape 1: Générer Mot de Passe Application Gmail

**LIEN DIRECT**: https://myaccount.google.com/apppasswords

**Procédure complète**:

1. **Aller sur**: https://myaccount.google.com/security

2. **Activer "Validation en 2 étapes"**:
   - Cliquer sur "Validation en 2 étapessmtp.gmail.com"
   - Suivre les instructions (SMS ou application)
   - Confirmer l'activation

3. **Générer mot de passe application**:
   - Retourner sur https://myaccount.google.com/security
   - Chercher "Mots de passe d'application" (en bas de page)
   - Cliquer dessus
   - Sélectionner "Autre (nom personnalisé)"
   - Entrer: **MDBAI Production**
   - Cliquer "Générer"
   - **COPIER** le mot de passe (16 caractères sans espaces)
   - Exemple: `abcd efgh ijkl mnop` → Copier: `abcdefghijklmnop`

4. **Sauvegarder le mot de passe** (vous ne pourrez plus le revoir)

### Étape 2: Configurer Doppler

**Option A: Script Automatique** (Recommandé)
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./configure_email_smtp.sh
# Suivre les instructions interactives
```

**Option B: Commandes Manuelles**
```bash
# Remplacer <MOT_DE_PASSE_16_CHARS> par votre mot de passe Gmail
doppler secrets set SMTP_HOST="smtp.gmail.com" --project mdbai --config prd
doppler secrets set SMTP_PORT="587" --project mdbai --config prd
doppler secrets set SMTP_SECURE="false" --project mdbai --config prd
doppler secrets set SMTP_USER="vgacofficiel@gmail.com" --project mdbai --config prd
doppler secrets set SMTP_PASS="<MOT_DE_PASSE_16_CHARS>" --project mdbai --config prd
doppler secrets set EMAIL_FROM="MDBAI <vgacofficiel@gmail.com>" --project mdbai --config prd
```

### Étape 3: Vérifier Configuration

```bash
# Lister les secrets SMTP dans Doppler
doppler secrets --project mdbai --config prd | grep SMTP

# Résultat attendu:
# SMTP_HOST        smtp.gmail.com
# SMTP_PORT        587
# SMTP_SECURE      false
# SMTP_USER        vgacofficiel@gmail.com
# SMTP_PASS        ****************
# EMAIL_FROM       MDBAI <vgacofficiel@gmail.com>
```

### Étape 4: Redémarrer Conteneur

```bash
ssh ubuntu@51.255.22.253 "cd ~/mdbai/src/MDBAI && docker compose -f docker-compose.production.yml restart mdbai-api-1"
```

### Étape 5: Tester Inscription

```bash
# Sur Telegram
1. /start
2. Cliquer "S'inscrire"
3. Entrer email: test@example.com
4. Entrer mot de passe: Test1234
5. ✅ Vérifier réception email avec code 6 chiffres
6. Entrer code
7. ✅ Vérifier message "Inscription réussie!"
```

---

## 🔧 ALTERNATIVES SI GMAIL NE FONCTIONNE PAS

### Option 1: SendGrid (Gratuit 100 emails/jour)

**Inscription**: https://signup.sendgrid.com/

**Configuration**:
```bash
doppler secrets set SMTP_HOST="smtp.sendgrid.net" --project mdbai --config prd
doppler secrets set SMTP_PORT="587" --project mdbai --config prd
doppler secrets set SMTP_SECURE="false" --project mdbai --config prd
doppler secrets set SMTP_USER="apikey" --project mdbai --config prd
doppler secrets set SMTP_PASS="<CLÉ_API_SENDGRID>" --project mdbai --config prd
doppler secrets set EMAIL_FROM="MDBAI <noreply@mdbai.com>" --project mdbai --config prd
```

### Option 2: Mailgun (Gratuit 100 emails/jour)

**Inscription**: https://signup.mailgun.com/

**Configuration**:
```bash
doppler secrets set SMTP_HOST="smtp.mailgun.org" --project mdbai --config prd
doppler secrets set SMTP_PORT="587" --project mdbai --config prd
doppler secrets set SMTP_SECURE="false" --project mdbai --config prd
doppler secrets set SMTP_USER="<MAILGUN_USER>" --project mdbai --config prd
doppler secrets set SMTP_PASS="<MAILGUN_PASSWORD>" --project mdbai --config prd
doppler secrets set EMAIL_FROM="MDBAI <noreply@mdbai.com>" --project mdbai --config prd
```

### Option 3: Mailtrap (Test uniquement)

**Inscription**: https://mailtrap.io/

**Configuration**:
```bash
doppler secrets set SMTP_HOST="smtp.mailtrap.io" --project mdbai --config prd
doppler secrets set SMTP_PORT="2525" --project mdbai --config prd
doppler secrets set SMTP_SECURE="false" --project mdbai --config prd
doppler secrets set SMTP_USER="<MAILTRAP_USER>" --project mdbai --config prd
doppler secrets set SMTP_PASS="<MAILTRAP_PASSWORD>" --project mdbai --config prd
doppler secrets set EMAIL_FROM="MDBAI <test@mdbai.com>" --project mdbai --config prd
```

---

## 🐛 DÉPANNAGE

### Problème: "Connection timeout"

**Cause**: Firewall bloque port 587

**Solution**:
```bash
# Vérifier connectivité SMTP
ssh ubuntu@51.255.22.253 "telnet smtp.gmail.com 587"

# Si timeout, essayer port 465 (SSL)
doppler secrets set SMTP_PORT="465" --project mdbai --config prd
doppler secrets set SMTP_SECURE="true" --project mdbai --config prd
```

### Problème: "Invalid login"

**Cause**: Mot de passe application incorrect

**Solution**:
1. Régénérer nouveau mot de passe sur https://myaccount.google.com/apppasswords
2. Mettre à jour Doppler:
```bash
doppler secrets set SMTP_PASS="<NOUVEAU_MOT_DE_PASSE>" --project mdbai --config prd
```

### Problème: "Service email non initialisé"

**Cause**: Variables SMTP manquantes ou incorrectes

**Solution**:
```bash
# Vérifier toutes les variables
doppler secrets --project mdbai --config prd | grep -E "SMTP|EMAIL"

# Vérifier logs conteneur
ssh ubuntu@51.255.22.253 "docker logs mdbai-api-1 | grep -i email"
```

---

## 📊 RÉCAPITULATIF

### Ce qui était en local
- ❌ Aucune configuration SMTP réelle
- ⚠️ Mode fallback Ethereal (emails de test)
- ❌ Emails jamais reçus par utilisateurs

### Ce qui est nécessaire maintenant
- ✅ Configuration SMTP Gmail complète
- ✅ Mot de passe application (16 caractères)
- ✅ 6 variables dans Doppler
- ✅ Redémarrage conteneur

### Liens Importants
- **Gmail App Passwords**: https://myaccount.google.com/apppasswords
- **Gmail Security**: https://myaccount.google.com/security
- **SendGrid Signup**: https://signup.sendgrid.com/
- **Mailgun Signup**: https://signup.mailgun.com/

---

**Temps estimé configuration complète**: 10-15 minutes

**Une fois configuré, le système sera 100% fonctionnel pour les beta testeurs.**