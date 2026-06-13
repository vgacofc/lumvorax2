# 🧪 GUIDE TEST UTILISATEUR RÉEL - CYCLE C193

**Date**: 2026-06-03 02:38 UTC+2  
**Version**: 1.0.0  
**Objectif**: Tester MDBAI comme un utilisateur réel avec votre compte GitHub  
**Durée estimée**: 30 minutes

---

## 📋 PRÉ-REQUIS

### Comptes Nécessaires
- ✅ Compte GitHub existant (le vôtre)
- ✅ Compte Telegram (pour recevoir notifications)
- ✅ Dépôt GitHub test (public ou privé)

### Informations Système MDBAI
- **GitHub App**: https://github.com/apps/mdbai-master-debug-ai
- **App ID**: 3888479
- **Bot Telegram**: @masterdebugai_bot
- **Serveur**: Replit (URL dynamique)

---

## 🚀 ÉTAPE 1: DÉMARRER LE SERVEUR MDBAI

### 1.1 Vérifier l'Infrastructure

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Vérifier node_modules
if [ ! -d "node_modules" ]; then
    echo "⚠️ Installation dépendances..."
    npm install
fi

# Vérifier libmdbai_hooks_forensic.so
if [ ! -f "forensic/libmdbai_hooks_forensic.so" ]; then
    echo "⚠️ Compilation hooks forensiques..."
    cd forensic
    make clean && make
    cd ..
fi

# Vérifier Doppler
doppler --version || echo "⚠️ Doppler non installé"
```

### 1.2 Démarrer le Serveur

```bash
# Option A: Avec Doppler (recommandé)
doppler run --token $DOPPLER_TOKEN -- npm start

# Option B: Sans Doppler (variables env manuelles)
export MDBAI_APP_ID=3888479
export MDBAI_CLIENT_ID=Iv23liM06X4pQnng7oFm
# ... autres variables
npm start
```

**Résultat attendu**:
```
🚀 MDBAI Server started on port 3000
✅ Redis connected: redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068
✅ Telegram Bot connected: @masterdebugai_bot
✅ GitHub App configured: ID 3888479
```

### 1.3 Noter l'URL du Serveur

Le serveur Replit génère une URL dynamique :
```
https://{random-id}.replit.dev
```

**⚠️ IMPORTANT**: Notez cette URL, vous en aurez besoin pour configurer le webhook GitHub.

---

## 🔗 ÉTAPE 2: INSTALLER LA GITHUB APP SUR VOTRE COMPTE

### 2.1 Accéder à la Page d'Installation

Ouvrez dans votre navigateur :
```
https://github.com/apps/mdbai-master-debug-ai/installations/new
```

### 2.2 Sélectionner les Dépôts

**Option A - Tous les dépôts** (recommandé pour test):
- ✅ Cocher "All repositories"

**Option B - Dépôts spécifiques**:
- ✅ Cocher "Only select repositories"
- ✅ Sélectionner votre dépôt de test

### 2.3 Autoriser les Permissions

La GitHub App demande les permissions suivantes :
- ✅ **Contents**: Read & Write (pour créer PR)
- ✅ **Pull requests**: Read & Write (pour publier rapports)
- ✅ **Metadata**: Read (pour infos dépôt)
- ✅ **Webhooks**: Read (pour événements push)

**Cliquer sur "Install"**

### 2.4 Vérifier l'Installation

Après installation, vous serez redirigé vers :
```
https://{random-id}.replit.dev/auth/callback?installation_id={ID}&setup_action=install
```

**Résultat attendu**:
```
✅ GitHub App installed successfully!
Installation ID: {votre_installation_id}
```

---

## 📱 ÉTAPE 3: CONNECTER TELEGRAM

### 3.1 Démarrer le Bot

1. Ouvrir Telegram
2. Rechercher: `@masterdebugai_bot`
3. Cliquer sur "Start" ou envoyer `/start`

**Résultat attendu**:
```
🤖 Bienvenue sur MDBAI - Master Debug AI!

Je suis votre assistant d'analyse forensique automatisée.

📋 Commandes disponibles:
/start - Afficher ce message
/help - Aide détaillée
/analyze <url> - Analyser un dépôt GitHub
/status - Statut des analyses en cours
/logs - Voir les logs récents

🔗 Pour commencer:
1. Installez la GitHub App: https://github.com/apps/mdbai-master-debug-ai
2. Utilisez /analyze https://github.com/user/repo
```

### 3.2 Lier votre Compte GitHub (Optionnel)

```
/link
```

Le bot vous donnera un lien OAuth pour lier votre compte GitHub.

---

## 🧪 ÉTAPE 4: TESTER L'ANALYSE D'UN DÉPÔT

### 4.1 Préparer un Dépôt de Test

**Option A - Utiliser un dépôt existant**:
- Votre propre dépôt GitHub
- Exemple: `https://github.com/votre-username/votre-repo`

**Option B - Créer un dépôt de test**:
```bash
# Sur GitHub, créer un nouveau dépôt "mdbai-test"
# Puis localement:
mkdir mdbai-test
cd mdbai-test
git init
echo "# MDBAI Test" > README.md
echo "console.log('Hello MDBAI');" > index.js
echo '{"name": "mdbai-test", "version": "1.0.0"}' > package.json
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/votre-username/mdbai-test.git
git push -u origin main
```

### 4.2 Lancer l'Analyse via Telegram

Dans Telegram, envoyer :
```
/analyze https://github.com/votre-username/votre-repo
```

**Résultat attendu**:
```
🔍 Analyse démarrée!

📊 Job ID: job_1234567890
📦 Dépôt: votre-username/votre-repo
⏱️ Temps estimé: 2-5 minutes

Je vous notifierai dès que l'analyse sera terminée.
```

### 4.3 Suivre la Progression

Le bot enverra des mises à jour en temps réel :
```
⏳ [20%] Clonage du dépôt...
⏳ [40%] Installation des dépendances...
⏳ [60%] Exécution des tests...
⏳ [80%] Analyse forensique...
⏳ [100%] Génération du rapport...
```

### 4.4 Recevoir le Rapport

Après 2-5 minutes :
```
✅ Analyse terminée!

📊 Résultats:
- Score global: 85/100
- Erreurs détectées: 3
- Warnings: 12
- Fuites mémoire: 0
- Vulnérabilités: 1 (faible)

📄 Rapport complet:
https://github.com/votre-username/votre-repo/pull/1

📁 Fichiers forensiques:
- forensic_trace.jsonl (45 MB, 313,174 événements)
- 17 snapshots .mdba (88 KB)
```

---

## 🔍 ÉTAPE 5: VÉRIFIER LE RAPPORT GITHUB

### 5.1 Accéder à la Pull Request

1. Aller sur votre dépôt GitHub
2. Cliquer sur "Pull requests"
3. Ouvrir la PR "MDBAI Analysis - {timestamp}"

### 5.2 Contenu du Rapport

Le rapport contient :

```markdown
# 🔬 MDBAI Forensic Analysis Report

**Date**: 2026-06-03 02:45 UTC+2
**Repository**: votre-username/votre-repo
**Commit**: abc123def456
**Duration**: 3m 24s

## 📊 Executive Summary

- **Global Score**: 85/100 ⭐⭐⭐⭐
- **Status**: ✅ PASS
- **Errors**: 3 found
- **Warnings**: 12 found
- **Memory Leaks**: 0 detected
- **Vulnerabilities**: 1 low severity

## 🔍 Detailed Analysis

### Errors Detected (3)

1. **TypeError in index.js:42**
   - Severity: HIGH
   - Message: Cannot read property 'foo' of undefined
   - Fix: Add null check before accessing property

2. **ReferenceError in utils.js:15**
   - Severity: MEDIUM
   - Message: 'bar' is not defined
   - Fix: Import or declare 'bar' variable

3. **SyntaxError in config.js:8**
   - Severity: LOW
   - Message: Unexpected token '}'
   - Fix: Remove extra closing brace

### Forensic Data

- **Events traced**: 313,174
- **Snapshots created**: 17 files (.mdba format)
- **Memory allocations**: 45,892
- **File I/O operations**: 8,234
- **Thread operations**: 156

### Recommendations

1. ✅ Fix TypeError in index.js (HIGH priority)
2. ✅ Update dependency 'lodash' to 4.17.21 (security)
3. ⚠️ Consider adding error handling in async functions
4. ℹ️ Code coverage: 67% (target: 80%)

## 📁 Forensic Files

Forensic data available in branch `mdbai-forensic-{timestamp}`:
- `forensic_trace.jsonl` - Complete event log
- `snapshots/*.mdba` - Binary snapshots (17 files)
- `analysis_report.json` - Machine-readable results
```

### 5.3 Télécharger les Fichiers Forensiques

```bash
# Cloner la branche forensique
git fetch origin mdbai-forensic-{timestamp}
git checkout mdbai-forensic-{timestamp}

# Vérifier les fichiers
ls -lh forensic_trace.jsonl
ls -lh snapshots/*.mdba

# Analyser les logs JSON
jq . forensic_trace.jsonl | head -20

# Valider magic number snapshots
xxd -l 80 snapshots/snapshot_*.mdba | head -1
# Doit afficher: 4142 444d (ABDM = 0x4D444241)
```

---

## 🎯 ÉTAPE 6: TESTER LES FONCTIONNALITÉS AVANCÉES

### 6.1 Analyser un Dépôt Privé

```
/analyze https://github.com/votre-username/repo-prive
```

**Note**: La GitHub App doit avoir accès au dépôt privé.

### 6.2 Comparer Deux Commits

```
/compare https://github.com/user/repo abc123 def456
```

### 6.3 Voir les Logs en Temps Réel

```
/logs
```

### 6.4 Vérifier le Statut

```
/status
```

**Résultat attendu**:
```
📊 Statut MDBAI

🔄 Analyses en cours: 2
✅ Analyses terminées: 15
❌ Analyses échouées: 1

📈 Statistiques:
- Temps moyen: 3m 42s
- Score moyen: 82/100
- Événements tracés: 4,234,567
- Snapshots créés: 234 fichiers
```

---

## ✅ ÉTAPE 7: VALIDATION COMPLÈTE

### 7.1 Checklist de Validation

- [ ] Serveur MDBAI démarré sans erreur
- [ ] GitHub App installée sur votre compte
- [ ] Bot Telegram répond à `/start`
- [ ] Analyse lancée via `/analyze`
- [ ] Notifications Telegram reçues en temps réel
- [ ] Pull Request créée sur GitHub
- [ ] Rapport markdown généré
- [ ] Fichiers forensiques téléchargeables
- [ ] Magic number 0x4D444241 validé dans snapshots
- [ ] Logs JSON parsables avec `jq`

### 7.2 Métriques Attendues

| Métrique | Valeur Attendue | Votre Résultat |
|----------|-----------------|----------------|
| Temps analyse | 2-5 minutes | _________ |
| Événements tracés | >100,000 | _________ |
| Snapshots créés | >10 | _________ |
| Score rapport | 70-100/100 | _________ |
| Taille logs JSON | >10 MB | _________ |
| Pull Request créée | Oui | _________ |

### 7.3 Captures d'Écran Recommandées

1. **Telegram**: Message de bienvenue du bot
2. **Telegram**: Notification analyse terminée
3. **GitHub**: Pull Request avec rapport
4. **GitHub**: Fichiers forensiques dans branche
5. **Terminal**: Validation magic number snapshots

---

## 🐛 DÉPANNAGE

### Problème 1: Serveur ne démarre pas

**Symptôme**: `Error: Cannot find module 'express'`

**Solution**:
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
rm -rf node_modules package-lock.json
npm install
```

### Problème 2: Bot Telegram ne répond pas

**Symptôme**: Pas de réponse à `/start`

**Solution**:
```bash
# Vérifier le token Telegram
doppler secrets get TELEGRAM_BOT_TOKEN

# Redémarrer le serveur
pkill -f "node.*server.js"
doppler run -- npm start
```

### Problème 3: GitHub App non installée

**Symptôme**: `Error: GitHub App not installed`

**Solution**:
1. Aller sur https://github.com/apps/mdbai-master-debug-ai
2. Cliquer sur "Configure"
3. Sélectionner votre compte
4. Installer sur les dépôts souhaités

### Problème 4: Analyse échoue

**Symptôme**: `❌ Analysis failed: Git clone error`

**Solution**:
```bash
# Vérifier les permissions GitHub App
# Vérifier que le dépôt est accessible
# Vérifier les logs serveur
tail -f /home/lvx/LVX/lumvorax2/src/MDBAI/logs/*.log
```

### Problème 5: Pas de fichiers forensiques

**Symptôme**: Branche forensique vide

**Solution**:
```bash
# Vérifier que libmdbai_hooks_forensic.so existe
ls -lh /home/lvx/LVX/lumvorax2/src/MDBAI/forensic/libmdbai_hooks_forensic.so

# Recompiler si nécessaire
cd /home/lvx/LVX/lumvorax2/src/MDBAI/forensic
make clean && make
```

---

## 📝 RAPPORT DE TEST

### Informations Test

- **Date**: _______________
- **Testeur**: _______________
- **Dépôt testé**: _______________
- **Durée totale**: _______________

### Résultats

**Fonctionnalités testées**:
- [ ] Installation GitHub App
- [ ] Connexion Bot Telegram
- [ ] Analyse dépôt public
- [ ] Analyse dépôt privé
- [ ] Génération rapport
- [ ] Création Pull Request
- [ ] Fichiers forensiques
- [ ] Validation magic numbers

**Bugs identifiés**:
1. _______________
2. _______________
3. _______________

**Améliorations suggérées**:
1. _______________
2. _______________
3. _______________

**Score global**: _____ / 10

**Commentaires**:
_______________________________________________
_______________________________________________
_______________________________________________

---

## 🎓 PROCHAINES ÉTAPES

Après validation complète :

1. **Documenter les résultats** dans `RAPPORT_TEST_UTILISATEUR_C193.md`
2. **Identifier les bugs** et créer issues GitHub
3. **Proposer améliorations** UX/UI
4. **Tester sur 10 dépôts variés** (Node.js, Python, Rust, Go, C++)
5. **Mesurer performance** (temps, mémoire, CPU)
6. **Valider scalabilité** (analyses simultanées)

---

**Fin du Guide** - Version 1.0.0 - 2026-06-03 02:38 UTC+2