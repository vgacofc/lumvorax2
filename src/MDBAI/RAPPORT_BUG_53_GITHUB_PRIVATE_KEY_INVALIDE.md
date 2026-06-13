# 🚨 RAPPORT FORENSIQUE - BUG #53: GITHUB_PRIVATE_KEY Invalide

**Date**: 2026-06-06T16:20:00+02:00  
**Cycle**: C199.2  
**Sévérité**: 🔴 CRITIQUE - Bloque toute analyse de dépôts privés  
**Statut**: ⚠️ EN COURS - Nécessite action utilisateur

---

## 📋 Résumé Exécutif

L'analyse du dépôt `https://github.com/Vgactec/lvgacnatif` échoue systématiquement avec l'erreur:
```
remote: Invalid username or token. Password authentication is not supported for Git operations.
fatal: Authentication failed for 'https://github.com/Vgactec/lvgacnatif/'
```

**Cause racine**: `GITHUB_PRIVATE_KEY` dans `.env` contient le `CLIENT_SECRET` OAuth au lieu d'une clé privée RSA.

---

## 🔍 Analyse Forensique

### 1. Séquence d'Échec Observée

**Logs d'exécution** (`2026-06-06T14-12-41_execution.log`):

```json
16:13:24 - [WORKER] Progress 5% — Vérification dépôt + token GitHub App...
16:13:24 - [WORKER] Progress 10% — Clonage du dépôt...
16:13:24 - [GITHUB] Clone Vgactec/lvgacnatif → /tmp/mdbai-analysis/repo_5f80ffc0...
16:13:24 - [GITHUB] Clone branch=main échoué, tentative sans --branch
16:13:25 - [WORKER] Job échoué: Clone échoué: Invalid username or token
```

**3 tentatives de retry automatique** (BullMQ):
- 16:13:24 - Tentative #1 ❌
- 16:13:27 - Retry #1 ❌
- 16:13:32 - Retry #2 ❌
- **Puis arrêt** (pas de boucle infinie réelle)

### 2. Configuration Invalide Détectée

**Fichier**: `lumvorax2/src/MDBAI/.env` (ligne 42)

```bash
# ❌ INVALIDE - Ce n'est PAS une clé privée RSA
GITHUB_PRIVATE_KEY="f648cfbdc58dd1e040352374abd7f7276593943a"
```

**Problème**: Cette valeur est identique au `GITHUB_APP_CLIENT_SECRET` (ligne 8), qui est un token OAuth, pas une clé privée RSA.

### 3. Format Attendu

Une clé privée GitHub App doit être au format PEM RSA:

```
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA1234567890abcdef...
(plusieurs lignes de caractères base64)
...xyz987654321
-----END RSA PRIVATE KEY-----
```

### 4. Impact sur le Code

**Fichier**: `src/workers/analysis.worker.js` (lignes 76-88)

```javascript
// Tentative d'obtention Installation Token
if (!effectiveToken && parsed) {
  try {
    const tempGithub = new GitHubService(null);
    effectiveToken = await tempGithub.getInstallationToken(parsed.owner, parsed.repo);
    // ✅ Devrait obtenir token valide ici
  } catch (tokenErr) {
    // ❌ Échoue silencieusement car GITHUB_PRIVATE_KEY invalide
    log.warn('[WORKER] Installation token non disponible — clone public sans token');
  }
}
```

**Fichier**: `src/services/github.service.js` (lignes 286-309)

```javascript
async getInstallationToken(owner, repo) {
  const appId     = config.github.appId;
  const privateKey = config.github.privateKey; // ❌ Contient CLIENT_SECRET au lieu de RSA key
  
  if (!appId || !privateKey) {
    throw new MdbaiError(ERR_GITHUB_AUTH, 'GITHUB_APP_ID et GITHUB_PRIVATE_KEY requis');
  }
  
  // ❌ createAppJwt() échoue car privateKey n'est pas une clé RSA valide
  const appJwt = createAppJwt(appId, privateKey);
  // ...
}
```

---

## 🎯 Solution Requise

### Étape 1: Obtenir la Vraie Clé Privée RSA

1. **Accéder aux paramètres de la GitHub App**:
   ```
   https://github.com/settings/apps/mdbai-master-debug-ai
   ```

2. **Section "Private keys"**:
   - Cliquer sur "Generate a private key"
   - Télécharger le fichier `.pem` (ex: `mdbai-master-debug-ai.2026-06-06.private-key.pem`)

3. **Copier le contenu du fichier**:
   ```bash
   cat mdbai-master-debug-ai.2026-06-06.private-key.pem
   ```

### Étape 2: Mettre à Jour `.env`

Remplacer la ligne 42 par le contenu complet de la clé privée:

```bash
# GitHub App Private Key (pour Installation Token)
GITHUB_PRIVATE_KEY="-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA...
(contenu complet de la clé)
...
-----END RSA PRIVATE KEY-----"
```

**⚠️ IMPORTANT**: 
- Garder les guillemets doubles
- Inclure les lignes `-----BEGIN RSA PRIVATE KEY-----` et `-----END RSA PRIVATE KEY-----`
- Ne pas ajouter d'espaces ou de retours à la ligne supplémentaires

### Étape 3: Redémarrer le Serveur

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
bash start-mdbai.sh
```

### Étape 4: Tester l'Analyse

```
/analyze https://github.com/Vgactec/lvgacnatif
```

---

## 📊 Vérification Post-Fix

### Logs Attendus (Succès)

```json
[WORKER] Progress 5% — Vérification dépôt + token GitHub App...
[GITHUB] Obtention installation token pour Vgactec/lvgacnatif
[GITHUB] Installation ID: 12345678 pour Vgactec/lvgacnatif
[GITHUB] Token d'installation obtenu ✅ — expire: 2026-06-06T17:20:00Z
[WORKER] Installation token obtenu via GitHub App ✅
[WORKER] Progress 10% — Clonage du dépôt...
[GITHUB] Clone Vgactec/lvgacnatif → /tmp/mdbai-analysis/repo_...
[GITHUB] Clone réussi ✅
```

### Commande de Vérification

```bash
# Vérifier que la clé privée est bien au format RSA
grep -c "BEGIN RSA PRIVATE KEY" /home/lvx/LVX/lumvorax2/src/MDBAI/.env
# Devrait retourner: 1
```

---

## 🔐 Sécurité

### ⚠️ Rappels Importants

1. **Ne JAMAIS commiter `.env` dans Git**
   - Déjà dans `.gitignore` ✅
   
2. **Clé privée = Accès complet à la GitHub App**
   - Équivalent à un mot de passe root
   - Rotation recommandée tous les 90 jours

3. **Permissions GitHub App**
   - Actuellement: Read/Write sur repositories
   - Principe du moindre privilège appliqué ✅

---

## 📚 Leçons Apprises

### LEÇON-189: Distinction CLIENT_SECRET vs PRIVATE_KEY

**Contexte**: Confusion entre deux types de credentials GitHub App

**Problème**: 
- `CLIENT_SECRET` = Token OAuth pour authentification utilisateur (flow web)
- `PRIVATE_KEY` = Clé RSA pour générer JWT et obtenir Installation Tokens

**Solution**:
- Utiliser `CLIENT_SECRET` uniquement pour OAuth callback (`/auth/github/callback`)
- Utiliser `PRIVATE_KEY` pour accès programmatique aux repos via Installation Tokens

**Impact**: CRITIQUE - Sans clé RSA valide, impossible d'accéder aux dépôts privés

---

### LEÇON-190: Format PEM Multi-lignes dans Variables d'Environnement

**Contexte**: Stockage de clés RSA dans fichiers `.env`

**Problème**: 
- Clés RSA contiennent des retours à la ligne
- Guillemets doubles requis pour préserver le format

**Solution**:
```bash
# ✅ CORRECT
GITHUB_PRIVATE_KEY="-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA...
-----END RSA PRIVATE KEY-----"

# ❌ INCORRECT (sans guillemets)
GITHUB_PRIVATE_KEY=-----BEGIN RSA PRIVATE KEY-----...

# ❌ INCORRECT (échappement \n)
GITHUB_PRIVATE_KEY="-----BEGIN RSA PRIVATE KEY-----\nMIIE..."
```

**Impact**: MAJEUR - Format invalide = JWT signing échoue silencieusement

---

### LEÇON-191: Retry Logic BullMQ vs Boucle Infinie

**Contexte**: Distinction entre retry automatique et boucle infinie

**Observation**:
- BullMQ retry par défaut: 3 tentatives avec backoff exponentiel
- Logs montrent 3 tentatives puis arrêt (16:13:24, 16:13:27, 16:13:32)
- **Pas de boucle infinie réelle** - juste retry normal

**Confusion utilisateur**:
- Telegram affiche "5% → 10% → 5% → 10%" pendant les retries
- Donne l'impression d'une boucle infinie
- En réalité: 3 tentatives distinctes avec même séquence de progression

**Solution**:
- Améliorer messages Telegram pour indiquer "Tentative X/3"
- Ajouter timeout global (ex: 5 min max par job)

**Impact**: MINEUR - Confusion UX mais comportement système correct

---

## 🎯 Actions Requises

### Immédiat (Utilisateur)

- [ ] **Obtenir clé privée RSA depuis GitHub App settings**
  - URL: https://github.com/settings/apps/mdbai-master-debug-ai
  - Section: "Private keys" → "Generate a private key"
  
- [ ] **Mettre à jour `.env` ligne 42 avec vraie clé RSA**
  - Format: `GITHUB_PRIVATE_KEY="-----BEGIN RSA PRIVATE KEY-----\n..."`
  
- [ ] **Redémarrer serveur MDBAI**
  - Commande: `cd /home/lvx/LVX/lumvorax2/src/MDBAI && bash start-mdbai.sh`
  
- [ ] **Tester analyse dépôt privé**
  - Commande Telegram: `/analyze https://github.com/Vgactec/lvgacnatif`

### Court Terme (Développement)

- [ ] **Améliorer validation configuration au démarrage**
  - Vérifier format RSA de `GITHUB_PRIVATE_KEY`
  - Échouer rapidement si invalide (fail-fast)
  
- [ ] **Améliorer messages Telegram pendant retries**
  - Afficher "Tentative 1/3", "Tentative 2/3", etc.
  - Éviter confusion avec boucle infinie

- [ ] **Ajouter timeout global par job**
  - Max 5 minutes par analyse
  - Éviter jobs bloqués indéfiniment

---

## 📈 Métriques

- **Temps d'analyse**: 9 secondes (3 tentatives × ~3s chacune)
- **Retries BullMQ**: 3 tentatives (comportement normal)
- **Impact utilisateur**: Analyse bloquée pour tous dépôts privés
- **Temps de résolution estimé**: 5 minutes (obtenir clé + redémarrer)

---

## ✅ Critères de Validation

### Test de Non-Régression

```bash
# 1. Vérifier format clé privée
grep "BEGIN RSA PRIVATE KEY" .env | wc -l
# Attendu: 1

# 2. Démarrer serveur
bash start-mdbai.sh

# 3. Vérifier logs au démarrage
tail -f logs/execution/*.log | grep "GITHUB_PRIVATE_KEY"
# Attendu: Aucune erreur de parsing

# 4. Tester analyse dépôt privé
# Via Telegram: /analyze https://github.com/Vgactec/lvgacnatif
# Attendu: "Installation token obtenu via GitHub App ✅"
```

---

**Rapport généré par**: MDBAI Forensic Engine v0.1.0  
**Cycle**: C199.2  
**Timestamp**: 2026-06-06T16:20:00+02:00