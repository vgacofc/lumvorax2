# 📧 TUTORIEL: Générer Mot de Passe Application Gmail

## 🎯 CE QUE VOUS DEVEZ FAIRE

Vous êtes actuellement sur la page **"Security & sign-in"** de votre compte Google.

**PARFAIT!** Vous avez déjà activé la "2-Step Verification" (validation en 2 étapes).

---

## 📍 OÙ VOUS ÊTES MAINTENANT

Vous voyez cette page:
```
Security & sign-in
├── 2-Step Verification (On since 12:29 AM) ✅
├── Passkeys and security keys
├── Password
├── Google prompt
└── 2-Step Verification phones
```

---

## 🔍 CE QU'IL FAUT CHERCHER

Sur cette MÊME page "Security & sign-in", **descendez vers le bas** jusqu'à trouver:

**"App passwords"** ou **"Mots de passe d'application"**

### Où exactement?

La section "App passwords" se trouve **APRÈS** ces sections:
1. ✅ 2-Step Verification (vous y êtes)
2. ✅ Passkeys and security keys
3. ✅ Password
4. ✅ Google prompt
5. ✅ 2-Step Verification phones
6. ✅ Recovery phone
7. ✅ Recovery email
8. 👉 **App passwords** ← CHERCHEZ ICI

---

## 📋 ÉTAPES DÉTAILLÉES

### ÉTAPE 1: Trouver "App passwords"

**Sur la page actuelle** (Security & sign-in), **scrollez vers le bas**.

Vous devriez voir une section qui ressemble à:
```
App passwords
Generate passwords for apps that don't support 2-Step Verification
```

**Si vous ne voyez PAS cette section**:
- C'est normal! Elle n'apparaît que si la 2-Step Verification est activée
- Vous l'avez activée à 12:29 AM ✅
- Rafraîchissez la page (F5) et scrollez à nouveau

### ÉTAPE 2: Cliquer sur "App passwords"

Une fois trouvé, **cliquez** sur "App passwords" ou "Mots de passe d'application".

Vous serez redirigé vers une nouvelle page.

### ÉTAPE 3: Créer un nouveau mot de passe

Sur la nouvelle page, vous verrez:
```
App passwords
Select the app and device you want to generate the app password for.
```

**Actions**:
1. Dans le menu déroulant "Select app", choisissez **"Other (Custom name)"** ou **"Autre (nom personnalisé)"**
2. Entrez le nom: **MDBAI Production**
3. Cliquez sur **"Generate"** ou **"Générer"**

### ÉTAPE 4: Copier le mot de passe

Google va afficher un mot de passe de **16 caractères** dans une boîte jaune:
```
┌─────────────────────────────────────┐
│ Your app password for MDBAI         │
│                                     │
│  abcd efgh ijkl mnop               │
│                                     │
│  Copy this password and paste it    │
│  into the app you're trying to use │
└─────────────────────────────────────┘
```

**IMPORTANT**:
- Le mot de passe est affiché avec des **espaces** (abcd efgh ijkl mnop)
- Vous devez le copier **SANS les espaces**: `abcdefghijklmnop`
- Cliquez sur le bouton **"Copy"** pour copier automatiquement
- **Sauvegardez-le** quelque part (vous ne pourrez plus le revoir)

---

## 🚀 APRÈS AVOIR COPIÉ LE MOT DE PASSE

### Option 1: Script Automatique (Recommandé)

Ouvrez un terminal et exécutez:
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./configure_email_smtp.sh
```

Le script vous demandera:
```
Entrez le mot de passe application Gmail (16 caractères):
```

**Collez** le mot de passe copié (sans espaces): `abcdefghijklmnop`

Le script configurera automatiquement les 6 variables SMTP dans Doppler.

### Option 2: Configuration Manuelle

Si vous préférez configurer manuellement:
```bash
# Remplacez <MOT_DE_PASSE> par votre mot de passe copié
doppler secrets set SMTP_HOST="smtp.gmail.com" --project mdbai --config prd
doppler secrets set SMTP_PORT="587" --project mdbai --config prd
doppler secrets set SMTP_SECURE="false" --project mdbai --config prd
doppler secrets set SMTP_USER="vgacofficiel@gmail.com" --project mdbai --config prd
doppler secrets set SMTP_PASS="abcdefghijklmnop" --project mdbai --config prd
doppler secrets set EMAIL_FROM="MDBAI <vgacofficiel@gmail.com>" --project mdbai --config prd
```

---

## ❓ SI VOUS NE TROUVEZ PAS "App passwords"

### Solution 1: Lien Direct

Essayez ce lien direct:
**https://myaccount.google.com/apppasswords**

### Solution 2: Navigation Manuelle

1. Allez sur: https://myaccount.google.com/
2. Cliquez sur **"Security"** dans le menu de gauche
3. Scrollez jusqu'à la section **"How you sign in to Google"**
4. Cherchez **"App passwords"** ou **"Mots de passe d'application"**
5. Cliquez dessus

### Solution 3: Vérifier 2-Step Verification

Si "App passwords" n'apparaît toujours pas:
1. Vérifiez que "2-Step Verification" est bien **ON** (activé)
2. Attendez 5 minutes (propagation Google)
3. Rafraîchissez la page (F5)
4. Scrollez à nouveau

---

## 🎯 RÉSUMÉ VISUEL

```
Vous êtes ici:
┌─────────────────────────────────────────┐
│ Security & sign-in                      │
├─────────────────────────────────────────┤
│ ✅ 2-Step Verification (ON)             │
│ ✅ Passkeys and security keys           │
│ ✅ Password                              │
│ ✅ Google prompt                         │
│ ✅ 2-Step Verification phones           │
│ ✅ Recovery phone                        │
│ ✅ Recovery email                        │
│                                         │
│ 👇 SCROLLEZ ICI 👇                      │
│                                         │
│ 🎯 App passwords ← CLIQUEZ ICI          │
│    Generate passwords for apps...       │
└─────────────────────────────────────────┘
```

---

## ✅ CHECKLIST

- [ ] Je suis sur la page "Security & sign-in"
- [ ] J'ai scrollé vers le bas
- [ ] J'ai trouvé "App passwords"
- [ ] J'ai cliqué dessus
- [ ] J'ai créé un mot de passe nommé "MDBAI Production"
- [ ] J'ai copié le mot de passe (16 caractères sans espaces)
- [ ] J'ai exécuté `./configure_email_smtp.sh`
- [ ] J'ai collé le mot de passe dans le script
- [ ] Le script a confirmé la configuration

---

## 🆘 BESOIN D'AIDE?

Si vous ne trouvez toujours pas "App passwords":
1. Envoyez-moi une capture d'écran de votre page "Security & sign-in"
2. Ou essayez le lien direct: https://myaccount.google.com/apppasswords

**Une fois le mot de passe copié, revenez ici et exécutez le script!**