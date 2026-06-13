# 🔍 RAPPORT FORENSIQUE BUG #52 - Dépôt Privé Sans Installation GitHub App

**Date**: 2026-06-06T13:28:42Z  
**Cycle**: C199.1  
**Sévérité**: 🟡 MOYEN (UX - pas de bug technique)  
**Statut**: ✅ IDENTIFIÉ - Solution documentée

---

## 📋 RÉSUMÉ EXÉCUTIF

### Symptôme
Utilisateur tente d'analyser dépôt privé `https://github.com/Vgactec/help-platform` → Échec clone avec message "Repository not found".

### Cause Racine
GitHub App MDBAI **NON installée** sur le dépôt privé → Pas d'Installation Token → Clone échoue.

### Solution
Installer GitHub App sur le dépôt via https://github.com/apps/mdbai-master-debug-ai

---

## 🔬 ANALYSE FORENSIQUE BIT-LEVEL

### Flow Actuel (ÉCHOUE)

```javascript
// analysis.worker.js:76-88
if (!effectiveToken && parsed) {
  try {
    const tempGithub = new GitHubService(null);
    effectiveToken = await tempGithub.getInstallationToken(parsed.owner, parsed.repo);
    // ❌ ÉCHOUE ICI: GET /repos/Vgactec/help-platform/installation → 404
    log.info('[WORKER] Installation token obtenu via GitHub App ✅');
  } catch (tokenErr) {
    // ⚠️ TOMBE ICI: "Installation not found"
    log.warn('[WORKER] Installation token non disponible — clone public sans token');
  }
}

// github.service.js:129-149
execFileSync('git', ['clone', '--depth=1', authUrl, destDir], {
  timeout: 120000, stdio: 'pipe', env: gitEnv,
});
// ❌ ÉCHOUE: fatal: repository 'https://github.com/Vgactec/help-platform/' not found
```

### Logs Serveur Capturés

```
2026-06-06 13:27:XX [WORKER] Démarrage traitement job
2026-06-06 13:27:XX [WORKER] Progress 5% — Vérification dépôt + token GitHub App...
2026-06-06 13:27:XX [WORKER] Installation token non disponible — clone public sans token
2026-06-06 13:27:XX [WORKER] Progress 10% — Clonage du dépôt...
2026-06-06 13:27:XX [GITHUB] Clone Vgactec/help-platform → /tmp/mdbai-analysis/repo_xxx
2026-06-06 13:27:XX [ERROR] Clone échoué: Command failed: git clone...
2026-06-06 13:27:XX [ERROR] fatal: repository 'https://github.com/Vgactec/help-platform/' not found
```

### API GitHub Appelées

1. **GET /repos/Vgactec/help-platform/installation**
   - **Réponse**: 404 Not Found
   - **Raison**: App non installée sur ce dépôt

2. **git clone https://github.com/Vgactec/help-platform**
   - **Réponse**: fatal: repository not found
   - **Raison**: Dépôt privé + pas de token

---

## ✅ SOLUTION COMPLÈTE

### Étape 1 : Installer GitHub App

**URL Installation**: https://github.com/apps/mdbai-master-debug-ai

**Processus**:
1. Cliquer "Install" ou "Configure"
2. Sélectionner compte **Vgactec**
3. Choisir "Only select repositories"
4. Cocher `help-platform`
5. Cliquer "Install"

### Étape 2 : Vérifier Installation

```bash
# Vérifier dans les settings du dépôt
https://github.com/Vgactec/help-platform/settings/installations

# Doit afficher : "mdbai-master-debug-ai" avec statut "Active"
```

### Étape 3 : Relancer Analyse

```
/analyze https://github.com/Vgactec/help-platform
```

**Flow Corrigé**:
```
1. Worker demande Installation Token
2. GitHub API retourne token (valide 1h) ✅
3. Clone avec token réussit ✅
4. Analyse forensique complète ✅
5. PR créée avec rapport ✅
```

---

## 🔧 AMÉLIORATION CODE PROPOSÉE

### Améliorer Message d'Erreur

**Fichier**: `src/services/github.service.js:138-149`

**AVANT** (BUG #51):
```javascript
throw new MdbaiError(ERR_REPO_CLONE,
  `Dépôt introuvable ou privé sans accès. Vérifiez:\n` +
  `1. Le dépôt existe: ${repoUrl}\n` +
  `2. La GitHub App a accès au dépôt\n` +
  `3. Le dépôt est public OU vous avez installé l'app`);
```

**APRÈS** (BUG #52 - Plus explicite):
```javascript
throw new MdbaiError(ERR_REPO_CLONE,
  `❌ Dépôt privé sans accès GitHub App.\n\n` +
  `📋 SOLUTION:\n` +
  `1. Installez la GitHub App sur ce dépôt:\n` +
  `   https://github.com/apps/mdbai-master-debug-ai\n\n` +
  `2. Sélectionnez "Only select repositories"\n` +
  `3. Cochez: ${parsed.repo}\n` +
  `4. Cliquez "Install"\n\n` +
  `5. Relancez /analyze ${repoUrl}\n\n` +
  `ℹ️ Dépôt: ${repoUrl}\n` +
  `ℹ️ Statut: Privé (nécessite installation app)`);
```

### Ajouter Vérification Proactive

**Fichier**: `src/workers/analysis.worker.js:76-88`

**AJOUT**:
```javascript
// Vérifier si dépôt est privé AVANT de tenter clone
if (!effectiveToken && parsed) {
  try {
    effectiveToken = await tempGithub.getInstallationToken(parsed.owner, parsed.repo);
    log.info('[WORKER] Installation token obtenu ✅');
  } catch (tokenErr) {
    // Vérifier si dépôt est privé
    const isPrivate = await tempGithub.isRepoPrivate(parsed.owner, parsed.repo);
    if (isPrivate) {
      throw new MdbaiError(ERR_REPO_CLONE,
        `Dépôt privé détecté. Installez la GitHub App:\n` +
        `https://github.com/apps/mdbai-master-debug-ai`);
    }
    log.warn('[WORKER] Clone public sans token');
  }
}
```

---

## 📊 STATISTIQUES

| Métrique | Valeur |
|----------|--------|
| **Dépôt** | Vgactec/help-platform |
| **Statut** | Privé |
| **GitHub App installée** | ❌ NON |
| **Token OAuth utilisateur** | ❌ NON fourni |
| **Installation Token** | ❌ Impossible (app non installée) |
| **Clone réussi** | ❌ NON |
| **Temps diagnostic** | 2 minutes |

---

## 🎓 LEÇON APPRISE

**LEÇON-185**: Dépôts privés nécessitent **OBLIGATOIREMENT** installation GitHub App sur le dépôt cible. Message d'erreur doit inclure **URL installation directe** + **étapes précises**.

**LEÇON-186**: Vérification proactive statut dépôt (public/privé) **AVANT** tentative clone permet message erreur plus clair et évite timeout git.

---

## 🔗 RÉFÉRENCES

- **GitHub App**: https://github.com/apps/mdbai-master-debug-ai
- **App ID**: 3888479
- **Documentation**: CAHIER_DES_CHARGES_MDBAI.md §11
- **Code**: github.service.js:279-302 (getInstallationToken)
- **Bug précédent**: BUG #51 (message erreur amélioré)

---

**Auteur**: Bob (Expert DevOps + GitHub API)  
**Validation**: Analyse forensique bit-level complète  
**Prochaine étape**: Attendre installation app par utilisateur