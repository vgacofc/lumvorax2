# PROTOCOLE MDBAI - MASTER DEBUG AI PLATFORM

**Date création**: 2026-05-27  
**Dernière mise à jour**: 2026-05-28  
**Version**: 1.0.0  
**Avancement global**: 🟡 **42%** — Infrastructure ✅ | Telegram 🟡 | Forensic 🟡 | Analysis 🔴 | GitHub PR 🔴 | Beta 🔴  
**Objectif**: Plateforme d'analyse forensique automatisée de dépôts GitHub avec technologie LumVorax  
**Principe**: MVP 100% gratuit utilisant GitHub Codespaces + forensic bit-level  
**Architecture**: Zero-budget, maximum viable, production-ready  

> **État 2026-05-28** : Serveur Express + Redis + BullMQ + Bot Telegram + Dashboard opérationnels.  
> Premier rapport généré : `reports/RAPPORT_MDBAI_2026-05-28T1618_mdbai-bb.md` (octocat/Hello-World, score 100/100).  
> Bugs actifs : détection langage retourne "unknown", métriques forensic à 0, PR GitHub non testée.

---

## 🎯 VISION DU PROJET

### Objectif Principal

Créer une plateforme d'analyse forensique automatisée qui:
1. **Connecte** n'importe quel dépôt GitHub via OAuth
2. **Clone** et **exécute** le code dans un environnement isolé
3. **Analyse** avec forensic bit-level (technologie LumVorax)
4. **Génère** rapport markdown détaillé
5. **Publie** automatiquement dans Pull Request

### Contrainte Absolue

**BUDGET: 0€** - Utilisation exclusive de services gratuits:
- GitHub Codespaces (60h/mois gratuit)
- GitHub Actions (2000 min/mois gratuit)
- Doppler (secrets management gratuit)
- Telegram Bot (gratuit)

---

## 📋 EXPERTISES MOBILISÉES

### Expertises Techniques Identifiées

| Domaine | Niveau | Application MDBAI |
|---------|--------|-------------------|
| **DevOps** | Expert | CI/CD, GitHub Actions, Codespaces |
| **Cloud Computing** | Avancé | Architecture distribuée, orchestration |
| **Cybersecurity** | Expert | Sandbox, isolation, analyse malware |
| **Forensic Analysis** | Expert | Bit-level logging, memory tracking |
| **Distributed Systems** | Avancé | Queue management, worker orchestration |
| **API Integration** | Expert | GitHub API, OAuth, Webhooks |
| **System Programming** | Expert | Bas niveau, instrumentation kernel |
| **AI/ML** | Intermédiaire | Analyse logs, pattern detection |

---

## 🏗️ ARCHITECTURE SYSTÈME

### Architecture MVP (100% Gratuit)

```
┌─────────────────────────────────────────────────────────────┐
│                    UTILISATEUR FINAL                         │
│              (Développeur avec dépôt GitHub)                 │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                  TELEGRAM BOT INTERFACE                      │
│  • Commandes: /analyze <repo_url>                           │
│  • Notifications temps réel                                  │
│  • Token: 8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8   │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│              GITHUB APP OAUTH (Authentification)             │
│  • Permissions: repo read, PR write, webhooks               │
│  • Scope: public_repo, repo:status                          │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                 ORCHESTRATOR (Replit/Node.js)                │
│  • API REST Express.js                                       │
│  • Queue management (BullMQ + Redis)                         │
│  • Job scheduling                                            │
│  • Webhook handler                                           │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│            GITHUB CODESPACES (Execution Environment)         │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  1. Clone Repository                                  │  │
│  │  2. Install Dependencies (npm/pip/cargo/etc)          │  │
│  │  3. Run Tests/Build                                   │  │
│  │  4. Capture stdout/stderr/logs                        │  │
│  │  5. LumVorax Forensic Instrumentation                 │  │
│  │     • Memory tracking                                 │  │
│  │     • Syscall tracing                                 │  │
│  │     • Bit-level logging                               │  │
│  └───────────────────────────────────────────────────────┘  │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│              ANALYSIS ENGINE (Local/Codespace)               │
│  • Log parsing (regex + heuristics)                          │
│  • Error detection                                           │
│  • Memory leak detection                                     │
│  • Security vulnerability scan                               │
│  • Performance analysis                                      │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│              REPORT GENERATOR (Markdown)                     │
│  • Structured report.md                                      │
│  • Code snippets                                             │
│  • Recommendations                                           │
│  • Forensic data visualization                               │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│           GITHUB PR AUTOMATION (Octokit.js)                  │
│  • Create branch: mdbai-analysis-<timestamp>                 │
│  • Commit report.md                                          │
│  • Open Pull Request                                         │
│  • Add labels: analysis, automated                           │
└─────────────────────────────────────────────────────────────┘
```

### Flux de Données

```
User → Telegram Bot → GitHub OAuth → Orchestrator
                                          ↓
                                    Queue (Redis)
                                          ↓
                                    GitHub Codespace
                                          ↓
                                    Clone + Execute
                                          ↓
                                    LumVorax Forensic
                                          ↓
                                    Logs Collection
                                          ↓
                                    Analysis Engine
                                          ↓
                                    Report Generation
                                          ↓
                                    GitHub PR
                                          ↓
                                    Notification User
```

---

## 🔐 GESTION SÉCURISÉE DES SECRETS

### Doppler Configuration

**Projet Doppler**: `lumvorax`  
**Config**: `dev_debugai`  
**URL**: https://dashboard.doppler.com/workplace/b6deb943584e68011892/projects/lumvorax/configs/dev_debugai

### Secrets à Configurer

| Nom Variable | Service | Usage | Statut |
|--------------|---------|-------|--------|
| `GITHUB_APP_ID` | GitHub | OAuth App ID | ⚠️ À créer |
| `GITHUB_APP_SECRET` | GitHub | OAuth App Secret | ⚠️ À créer |
| `GITHUB_WEBHOOK_SECRET` | GitHub | Webhook validation | ⚠️ À créer |
| `TELEGRAM_BOT_TOKEN` | Telegram | Bot API | ✅ Existant |
| `REDIS_URL` | Redis Cloud | Queue backend | ✅ Existant |
| `DATABASE_URL` | Prisma Postgres | Persistence | ⚠️ RÉVOQUÉ |
| `DOPPLER_TOKEN` | Doppler | Secrets sync | ⚠️ À créer |


---

## 📦 STACK TECHNIQUE

### Backend (Orchestrator)

```javascript
// package.json
{
  "name": "mdbai-orchestrator",
  "version": "1.0.0",
  "type": "module",
  "dependencies": {
    "express": "^4.18.2",
    "bullmq": "^4.0.0",
    "ioredis": "^5.3.2",
    "@octokit/rest": "^20.0.0",
    "node-telegram-bot-api": "^0.64.0",
    "dotenv": "^16.3.1",
    "@prisma/client": "^5.0.0",
    "winston": "^3.11.0"
  }
}
```

### Forensic Engine (LumVorax)

```c
// Modules à intégrer depuis src/
- src/logger/forensic_logger.c
- src/monitoring/memory_tracker.c
- src/debug/syscall_tracer.c
- src/metrics/performance_analyzer.c
```

### GitHub Actions Workflow

```yaml
# .github/workflows/mdbai-analysis.yml
name: MDBAI Analysis
on:
  workflow_dispatch:
    inputs:
      repo_url:
        description: 'Repository URL to analyze'
        required: true
jobs:
  analyze:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Run MDBAI Analysis
        run: |
          # Clone target repo
          # Install dependencies
          # Run forensic analysis
          # Generate report
```

---

## 🧪 PROTOCOLE TEST CUMULATIF

### Philosophie

Inspiré de `PROTOCOLE_TEST_CUMULATIF_DRM.md`:
1. **JAMAIS supprimer un test validé**
2. **Exécution cumulative** - Tous les tests à chaque cycle
3. **Validation stricte** - 0 erreur, 0 warning
4. **Documentation obligatoire**

### Phase 1: Infrastructure (Tests 001-010)

#### TEST_001: Telegram Bot Connection
**Objectif**: Vérifier connexion bot Telegram  
**Validation**:
```javascript
const bot = new TelegramBot(process.env.TELEGRAM_BOT_TOKEN);
const me = await bot.getMe();
assert(me.username === 'masterdebugai_bot');
```
**Statut**: ✅ FAIT — Bot @masterdebugai_bot actif, commandes enregistrées (2026-05-28)

#### TEST_002: GitHub OAuth Flow
**Objectif**: Authentification utilisateur GitHub  
**Validation**:
```javascript
const octokit = new Octokit({ auth: token });
const { data } = await octokit.users.getAuthenticated();
assert(data.login !== null);
```
**Statut**: ⚠️ PARTIEL — Routes /auth/github et /auth/github/callback existent, flux non testé en production

#### TEST_003: Redis Queue Connection
**Objectif**: Connexion Redis Cloud  
**Validation**:
```javascript
const redis = new Redis(process.env.REDIS_URL);
await redis.set('test', 'ok');
const value = await redis.get('test');
assert(value === 'ok');
```
**Statut**: ✅ FAIT — Redis Cloud connecté, BullMQ "analysis-jobs" opérationnel, 3 workers actifs (2026-05-28)

#### TEST_004: Doppler Secrets Sync
**Objectif**: Récupération secrets Doppler  
**Validation**:
```bash
doppler secrets download --no-file --format env
assert $GITHUB_APP_ID != ""
```
**Statut**: ✅ FAIT — 14/14 secrets chargés via DOPPLER_MDBAI_TOKEN (dev_debugai, 2026-05-28)

#### TEST_005: GitHub Codespace Creation
**Objectif**: Créer Codespace programmatiquement  
**Validation**:
```javascript
const codespace = await octokit.codespaces.createForAuthenticatedUser({
  repository_id: repo.id,
  machine: 'basicLinux32gb'
});
assert(codespace.data.state === 'Available');
```
**Statut**: ❌ À FAIRE — Codespace API non intégrée (MVP utilise exécution locale)

#### TEST_006: Repository Clone
**Objectif**: Cloner dépôt dans Codespace  
**Validation**:
```bash
git clone https://github.com/user/repo.git
cd repo
assert -d .git
```
**Statut**: ⚠️ PARTIEL — Clone implémenté mais detectLanguage() retourne "unknown" (bug actif)

#### TEST_007: Dependency Installation
**Objectif**: Installer dépendances automatiquement  
**Validation**:
```bash
# Détection automatique package manager
if [ -f package.json ]; then npm install; fi
if [ -f requirements.txt ]; then pip install -r requirements.txt; fi
if [ -f Cargo.toml ]; then cargo build; fi
```
**Statut**: ❌ À FAIRE — Gestionnaire de paquets non exécuté réellement

#### TEST_008: LumVorax Forensic Injection
**Objectif**: Injecter instrumentation forensic  
**Validation**:
```c
forensic_logger_init();
memory_tracker_start();
syscall_tracer_attach();
assert(forensic_active() == true);
```
**Statut**: ⚠️ PARTIEL — libmdbai_forensic.so compilée 17KB (Magic 0x4D444241), LD_PRELOAD non injecté dans processus cible

#### TEST_009: Execution + Log Capture
**Objectif**: Exécuter code et capturer logs  
**Validation**:
```bash
npm test 2>&1 | tee execution.log
assert -f execution.log
assert $(wc -l < execution.log) -gt 0
```
**Statut**: ⚠️ PARTIEL — Capture stdout/stderr en place, exécution réelle non effectuée (1s vs 45s attendu)

#### TEST_010: Report Generation
**Objectif**: Générer rapport markdown  
**Validation**:
```javascript
const report = await generateReport(logs, forensicData);
assert(report.includes('# MDBAI Analysis Report'));
assert(report.includes('## Forensic Data'));
```
**Statut**: ✅ FAIT — Rapport généré pour octocat/Hello-World (score 100/100, format Markdown complet, 2026-05-28T16:18Z)

### Phase 2: Analysis Engine (Tests 011-020)

#### TEST_011: Error Detection
**Objectif**: Détecter erreurs compilation/runtime  
**Validation**:
```javascript
const errors = detectErrors(logs);
assert(errors.length >= 0);
assert(errors[0].type in ['syntax', 'runtime', 'logic']);
```
**Statut**: ⚠️ PARTIEL — Regex basiques implémentées, parsing profond non réalisé

#### TEST_012: Memory Leak Detection
**Objectif**: Identifier fuites mémoire  
**Validation**:
```c
memory_leak_t* leaks = analyze_memory_leaks(forensic_data);
assert(leaks != NULL);
```
**Statut**: ❌ À FAIRE — Nécessite LD_PRELOAD opérationnel (Sprint 3)

#### TEST_013: Security Vulnerability Scan
**Objectif**: Scanner vulnérabilités sécurité  
**Validation**:
```javascript
const vulns = scanVulnerabilities(code);
assert(vulns.severity in ['low', 'medium', 'high', 'critical']);
```
**Statut**: ❌ À FAIRE — npm audit / pip-audit non intégrés

#### TEST_014: Performance Analysis
**Objectif**: Analyser performance  
**Validation**:
```c
perf_metrics_t metrics = analyze_performance(forensic_data);
assert(metrics.cpu_usage >= 0.0 && metrics.cpu_usage <= 100.0);
```
**Statut**: ⚠️ PARTIEL — Structure de métriques en place (CPU=0%, RAM=0MB car aucune exécution réelle)

#### TEST_015: Code Quality Metrics
**Objectif**: Calculer métriques qualité  
**Validation**:
```javascript
const quality = calculateQuality(code);
assert(quality.score >= 0 && quality.score <= 100);
```
**Statut**: ⚠️ PARTIEL — Score 0-100 calculé (retourne 100/100 quand aucune erreur trouvée)

### Phase 3: GitHub Integration (Tests 021-030)

#### TEST_021: Branch Creation
**Objectif**: Créer branche analyse  
**Validation**:
```javascript
const branch = await octokit.git.createRef({
  owner, repo,
  ref: 'refs/heads/mdbai-analysis-' + Date.now(),
  sha: baseSha
});
assert(branch.data.ref.startsWith('refs/heads/mdbai-'));
```
**Statut**: ⚠️ PARTIEL — Code Octokit.js en place (src/services/github.service.js), non testé sur vrai dépôt

#### TEST_022: Commit Report
**Objectif**: Commit rapport dans branche  
**Validation**:
```javascript
const commit = await octokit.repos.createOrUpdateFileContents({
  owner, repo,
  path: 'MDBAI_REPORT.md',
  message: 'Add MDBAI analysis report',
  content: Buffer.from(report).toString('base64'),
  branch: 'mdbai-analysis-' + timestamp
});
assert(commit.data.commit.sha !== null);
```
**Statut**: ⚠️ PARTIEL — Code en place, non testé sur vrai dépôt

#### TEST_023: Pull Request Creation
**Objectif**: Créer PR avec rapport  
**Validation**:
```javascript
const pr = await octokit.pulls.create({
  owner, repo,
  title: '🤖 MDBAI Analysis Report',
  head: 'mdbai-analysis-' + timestamp,
  base: 'main',
  body: '# Automated Analysis\n\nSee MDBAI_REPORT.md for details.'
});
assert(pr.data.number > 0);
```
**Statut**: ⚠️ PARTIEL — Code en place, non testé sur vrai dépôt

#### TEST_024: PR Labels
**Objectif**: Ajouter labels au PR  
**Validation**:
```javascript
await octokit.issues.addLabels({
  owner, repo,
  issue_number: pr.data.number,
  labels: ['mdbai', 'automated-analysis', 'forensic']
});
```
**Statut**: ⚠️ PARTIEL — Labels définis (mdbai, automated-analysis, forensic), non appliqués sur vrai PR

#### TEST_025: Telegram Notification
**Objectif**: Notifier utilisateur via Telegram  
**Validation**:
```javascript
await bot.sendMessage(chatId, 
  `✅ Analysis complete!\n\nPR: ${pr.data.html_url}`
);
```
**Statut**: ⚠️ PARTIEL — Notification implémentée, non validée end-to-end

---

## 🚀 ROADMAP DÉVELOPPEMENT

### Sprint 1: Infrastructure (Semaine 1)
- [ ] Configuration Doppler
- [ ] Création GitHub App
- [ ] Setup Telegram Bot
- [ ] Connection Redis
- [ ] Tests 001-005

### Sprint 2: Execution Engine (Semaine 2)
- [ ] GitHub Codespace integration
- [ ] Repository cloning
- [ ] Dependency detection
- [ ] Execution wrapper
- [ ] Tests 006-010

### Sprint 3: Forensic Integration (Semaine 3)
- [ ] Port LumVorax modules
- [ ] Memory tracking
- [ ] Syscall tracing
- [ ] Log aggregation
- [ ] Tests 011-015

### Sprint 4: Analysis Engine (Semaine 4)
- [ ] Error detection
- [ ] Memory leak analysis
- [ ] Security scanning
- [ ] Performance metrics
- [ ] Tests 016-020

### Sprint 5: GitHub Integration (Semaine 5)
- [ ] Branch management
- [ ] Commit automation
- [ ] PR creation
- [ ] Notification system
- [ ] Tests 021-025

### Sprint 6: MVP Release (Semaine 6)
- [ ] End-to-end testing
- [ ] Documentation
- [ ] Demo video
- [ ] Public beta

---

## 📊 MÉTRIQUES DE SUCCÈS

### KPIs Techniques

| Métrique | Objectif | Mesure |
|----------|----------|--------|
| **Temps analyse** | < 5 min | Moyenne temps exécution |
| **Taux succès** | > 95% | Analyses réussies / total |
| **Précision détection** | > 90% | Vrais positifs / total détections |
| **Disponibilité** | > 99% | Uptime service |
| **Coût** | 0€ | Budget mensuel |

### KPIs Utilisateur

| Métrique | Objectif | Mesure |
|----------|----------|--------|
| **Adoption** | 100 users | Utilisateurs actifs |
| **Satisfaction** | > 4/5 | Note moyenne |
| **Rétention** | > 80% | Users actifs mois N / mois N-1 |

---

## 🔧 COMMANDES UTILES

### Développement Local

```bash
# Installation dépendances
npm install

# Configuration secrets
doppler setup

# Lancement dev
npm run dev

# Tests
npm test

# Linting
npm run lint
```

### Déploiement

```bash
# Build production
npm run build

# Deploy Replit
git push replit main

# Vérifier logs
doppler run -- npm run logs
```

### Debugging

```bash
# Logs Telegram Bot
tail -f logs/telegram.log

# Logs Queue
redis-cli -u $REDIS_URL MONITOR

# Logs Codespace
gh codespace logs
```

---

## 📚 DOCUMENTATION RÉFÉRENCE

### APIs Utilisées

- [GitHub REST API](https://docs.github.com/en/rest)
- [GitHub OAuth Apps](https://docs.github.com/en/apps/oauth-apps)
- [GitHub Codespaces API](https://docs.github.com/en/rest/codespaces)
- [Telegram Bot API](https://core.telegram.org/bots/api)
- [BullMQ Documentation](https://docs.bullmq.io/)
- [Doppler CLI](https://docs.doppler.com/docs/cli)

### Ressources LumVorax

- `src/logger/` - Forensic logging
- `src/monitoring/` - Memory tracking
- `src/debug/` - Syscall tracing
- `src/metrics/` - Performance analysis
- `PROTOCOLE_TEST_CUMULATIF_DRM.md` - Méthodologie tests

---

## ⚠️ LIMITATIONS CONNUES

### Limitations GitHub Codespaces (Free Tier)

- **60 heures/mois** - Limite temps exécution
- **2 cores** - CPU limité
- **4 GB RAM** - Mémoire limitée
- **15 GB storage** - Stockage limité
- **Pas de GPU** - Pas d'accès GPU

### Limitations GitHub Actions (Free Tier)

- **2000 minutes/mois** - Limite CI/CD
- **6 heures max** - Timeout job
- **Pas de persistence** - VM détruite après job

### Workarounds

1. **Optimiser temps exécution** - Caching, parallelisation
2. **Rotation utilisateurs** - Plusieurs comptes GitHub
3. **Analyse incrémentale** - Analyser seulement changements
4. **Compression logs** - Réduire taille données

---

## 🎓 FORMATION AGENT REPLIT

### Prérequis Techniques

L'agent Replit doit maîtriser:
- Node.js / JavaScript ES6+
- Express.js REST API
- BullMQ / Redis
- GitHub API / Octokit
- Telegram Bot API
- Docker basics
- Git workflows
- Markdown

### Checklist Développement

- [ ] Lire intégralement ce protocole
- [ ] Étudier ORIGINE_IDÉ_MDBAI.md
- [ ] Analyser architecture LumVorax existante
- [ ] Configurer environnement Doppler
- [ ] Créer GitHub App
- [ ] Implémenter tests Phase 1
- [ ] Valider chaque test avant continuer
- [ ] Documenter chaque décision
- [ ] Commit fréquents avec messages clairs
- [ ] Respecter convention nommage

### Convention Nommage

```
Fichiers: snake_case.js
Classes: PascalCase
Fonctions: camelCase
Constantes: UPPER_SNAKE_CASE
Tests: test_XXX_description.js
Logs: YYYY-MM-DD_HH-MM-SS_event.log
```

---

## 📝 CHANGELOG

### Version 1.0.0 (2026-05-27)
- ✅ Création protocole initial
- ✅ Définition architecture MVP
- ✅ Identification stack technique
- ✅ Définition 25 tests cumulatifs
- ✅ Roadmap 6 sprints
- ✅ Documentation complète

---

**Auteur**: LumVorax Team  
**Contact**: Telegram @masterdebugai_bot  
**License**: MIT  
**Repository**: https://github.com/lumvorax/mdbai
