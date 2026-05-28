# MDBAI - Master Debug AI 🔍🤖

**Plateforme d'analyse forensique automatisée pour dépôts GitHub**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Node.js Version](https://img.shields.io/badge/node-%3E%3D20.0.0-brightgreen)](https://nodejs.org/)
[![Budget](https://img.shields.io/badge/budget-0%E2%82%AC-success)](https://github.com/lumvorax/mdbai)

---

## 📋 Table des Matières

- [Vue d'ensemble](#vue-densemble)
- [Fonctionnalités](#fonctionnalités)
- [Architecture](#architecture)
- [Installation](#installation)
- [Configuration](#configuration)
- [Utilisation](#utilisation)
- [Tests](#tests)
- [Documentation](#documentation)
- [Roadmap](#roadmap)
- [Contribution](#contribution)
- [Licence](#licence)

---

## 🎯 Vue d'ensemble

**MDBAI** est une plateforme d'analyse forensique automatisée qui permet d'analyser n'importe quel dépôt GitHub en profondeur, en utilisant la technologie de logging bit-level développée dans le projet LumVorax.

### Problème Résolu

Les développeurs perdent des heures à débugger des problèmes complexes :
- Fuites mémoire invisibles
- Erreurs de segmentation aléatoires
- Vulnérabilités de sécurité cachées
- Problèmes de performance non détectés

### Solution MDBAI

Analyse automatisée en **3 étapes simples** :
1. **Connecter** votre dépôt GitHub via Telegram Bot
2. **Analyser** automatiquement dans GitHub Codespace isolé
3. **Recevoir** un rapport détaillé avec solutions dans une Pull Request

### Contrainte Absolue

**Budget : 0€** - Utilisation exclusive de services gratuits :
- GitHub Codespaces (60h/mois gratuit)
- GitHub Actions (2000 min/mois gratuit)
- Redis Cloud (30MB gratuit)
- Doppler (secrets management gratuit)
- Telegram Bot (gratuit)
- Replit (free tier)

---

## ✨ Fonctionnalités

### 🔐 Authentification et Sécurité
- OAuth GitHub App (permissions minimales)
- Gestion sécurisée des secrets via Doppler
- Isolation complète dans GitHub Codespace
- Chiffrement des données sensibles

### 🔍 Analyse Forensique
- **Logging bit-level** : Capture nanoseconde par nanoseconde
- **Memory tracking** : Détection fuites mémoire (malloc/free)
- **Syscall tracing** : Traçage appels système (ptrace)
- **Performance profiling** : CPU, mémoire, I/O

### 📊 Détection Automatique
- ❌ Erreurs de compilation/runtime
- 💧 Fuites mémoire (memory leaks)
- 🔒 Vulnérabilités de sécurité (buffer overflow, injection)
- ⚡ Problèmes de performance (algorithmes inefficaces)
- 🐛 Bugs cachés (race conditions, deadlocks)

### 📝 Génération de Rapports
- Format Markdown structuré
- Graphiques de performance
- Recommandations d'optimisation
- Code snippets avec solutions
- Publication automatique dans Pull Request

### 🔔 Notifications
- Telegram Bot interactif
- Statut en temps réel
- Alertes critiques
- Résumé exécutif

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         USER (Telegram)                         │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                      TELEGRAM BOT API                           │
│  - Commandes: /analyze, /status, /report                       │
│  - Authentification GitHub OAuth                                │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   ORCHESTRATOR (Replit)                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │   Express    │  │   BullMQ     │  │   Octokit    │         │
│  │   Server     │  │   Queue      │  │   GitHub API │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                    REDIS CLOUD (Queue)                          │
│  - Job queue (BullMQ)                                           │
│  - Rate limiting                                                │
│  - Cache résultats                                              │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                  GITHUB CODESPACE (Execution)                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Clone Repository                                      │  │
│  │ 2. Detect Language (Node.js, Python, C/C++, Go, Rust)   │  │
│  │ 3. Install Dependencies (npm, pip, cargo, go mod)       │  │
│  │ 4. Inject Forensic (LD_PRELOAD libforensic.so)          │  │
│  │ 5. Execute Tests/Build                                   │  │
│  │ 6. Capture Logs (stdout, stderr, forensic.log)          │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   ANALYSIS ENGINE (Node.js)                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │   Error      │  │   Memory     │  │   Security   │         │
│  │   Detector   │  │   Analyzer   │  │   Scanner    │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
│  ┌──────────────┐  ┌──────────────┐                            │
│  │ Performance  │  │   Report     │                            │
│  │   Profiler   │  │  Generator   │                            │
│  └──────────────┘  └──────────────┘                            │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                    GITHUB API (Output)                          │
│  - Create branch (analysis/mdbai-TIMESTAMP)                    │
│  - Commit report (RAPPORT_MDBAI_TIMESTAMP.md)                  │
│  - Create Pull Request                                          │
│  - Add labels (bug, performance, security)                      │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   NOTIFICATION (Telegram)                       │
│  - ✅ Analysis complete                                         │
│  - 📊 Summary (errors, leaks, vulnerabilities)                 │
│  - 🔗 Link to Pull Request                                     │
└─────────────────────────────────────────────────────────────────┘
```

### Stack Technique

**Backend (Orchestrator)**
- Node.js 20+ avec Express.js
- BullMQ (queue) + ioredis (Redis client)
- @octokit/rest (GitHub API)
- node-telegram-bot-api (Telegram)
- Winston (logging)

**Forensic Engine (LumVorax)**
- forensic_logger.c (bit-level logging)
- memory_tracker.c (allocation tracking)
- syscall_tracer.c (ptrace wrapper)
- performance_analyzer.c (CPU/memory profiling)
- Compilation: libforensic.so (shared library)
- Injection: LD_PRELOAD

**GitHub Integration**
- GitHub App OAuth (permissions: repo read/write, PR, webhooks)
- Codespaces API (création/exécution programmatique)
- Octokit.js (branch, commit, PR automation)

---

## 🚀 Installation

### Prérequis

- Node.js >= 20.0.0
- npm >= 10.0.0
- GCC/Clang (pour compilation forensic)
- Redis (local ou cloud)
- Compte GitHub
- Compte Telegram

### Installation Rapide

```bash
# 1. Cloner le dépôt
git clone https://github.com/lumvorax/mdbai.git
cd mdbai

# 2. Installer les dépendances Node.js
npm install

# 3. Compiler la bibliothèque forensic
npm run build:forensic

# 4. Configurer les secrets (voir section Configuration)
npm run doppler:sync

# 5. Lancer en mode développement
npm run dev
```

### Installation Complète

Voir [GUIDE_INSTALLATION.md](docs/GUIDE_INSTALLATION.md) pour instructions détaillées.

---

## ⚙️ Configuration

### 1. Secrets Doppler

Créer un compte sur [Doppler](https://doppler.com) et configurer les secrets :

```bash
# Installer Doppler CLI
curl -Ls https://cli.doppler.com/install.sh | sh

# Authentification
doppler login

# Configuration projet
doppler setup --project lumvorax --config dev_debugai

# Upload secrets (voir GUIDE_DOPPLER_SECRETS.md)
bash scripts/setup_doppler_auto.sh
```

**Secrets requis (14 total)** :
- `TELEGRAM_BOT_TOKEN` - Token Telegram Bot
- `REDIS_URL`, `REDIS_HOST`, `REDIS_PORT`, `REDIS_PASSWORD` - Redis Cloud
- `DATABASE_URL` - Prisma Postgres
- `GITHUB_APP_ID`, `GITHUB_APP_CLIENT_ID`, `GITHUB_APP_CLIENT_SECRET`, `GITHUB_PRIVATE_KEY` - GitHub App
- `NODE_ENV`, `LOG_LEVEL`, `API_PORT` - Configuration

### 2. GitHub App

Créer une GitHub App sur [github.com/settings/apps/new](https://github.com/settings/apps/new) :

**Permissions** :
- Repository: Read & Write (contents, pull requests)
- Webhooks: Active (push, pull_request)

**Callback URL** : `https://your-replit-url.repl.co/auth/github/callback`

Voir [GUIDE_GITHUB_APP.md](docs/GUIDE_GITHUB_APP.md) pour détails.

### 3. Telegram Bot

Créer un bot via [@BotFather](https://t.me/BotFather) :

```
/newbot
Name: MDBAI Bot
Username: mdbai_bot
```

Copier le token et l'ajouter à Doppler.

### 4. Variables d'Environnement

Créer `.env` (ou utiliser Doppler) :

```env
# Application
NODE_ENV=development
LOG_LEVEL=debug
API_PORT=3000

# Telegram
TELEGRAM_BOT_TOKEN=your_token_here

# Redis
REDIS_URL=redis://default:password@host:port
REDIS_HOST=redis-xxxxx.c327.europe-west1-2.gce.redns.redis-cloud.com
REDIS_PORT=17068
REDIS_PASSWORD=your_password_here

# GitHub
GITHUB_APP_ID=123456
GITHUB_APP_CLIENT_ID=Iv1.xxxxx
GITHUB_APP_CLIENT_SECRET=xxxxx
GITHUB_PRIVATE_KEY="-----BEGIN RSA PRIVATE KEY-----\n...\n-----END RSA PRIVATE KEY-----"

# Database
DATABASE_URL=postgres://user:password@host:port/database
```

---

## 📖 Utilisation

### Via Telegram Bot

1. **Démarrer le bot** : `/start`
2. **Authentifier GitHub** : `/auth`
3. **Analyser un dépôt** : `/analyze https://github.com/user/repo`
4. **Vérifier le statut** : `/status JOB_ID`
5. **Obtenir le rapport** : `/report JOB_ID`

### Via API REST

```bash
# Créer une analyse
curl -X POST http://localhost:3000/api/analyze \
  -H "Content-Type: application/json" \
  -d '{
    "repo_url": "https://github.com/user/repo",
    "user_id": "telegram_user_id"
  }'

# Vérifier le statut
curl http://localhost:3000/api/status/JOB_ID

# Obtenir le rapport
curl http://localhost:3000/api/report/JOB_ID
```

### Exemple de Rapport

```markdown
# RAPPORT MDBAI - Analyse Forensique
**Dépôt**: user/repo  
**Date**: 2026-05-27T19:55:00Z  
**Durée**: 45s

## Résumé Exécutif
- ❌ **3 erreurs critiques** détectées
- 💧 **2 fuites mémoire** identifiées
- 🔒 **1 vulnérabilité** de sécurité
- ⚡ **5 optimisations** recommandées

## Détails

### Erreur #1: Segmentation Fault
**Fichier**: `src/main.c:42`  
**Cause**: Déréférencement pointeur NULL  
**Solution**: Ajouter vérification `if (ptr != NULL)`

### Fuite Mémoire #1
**Fichier**: `src/utils.c:128`  
**Taille**: 1024 bytes  
**Cause**: `malloc()` sans `free()`  
**Solution**: Ajouter `free(buffer)` ligne 145

...
```

---

## 🧪 Tests

### Tests Cumulatifs (25 tests)

Inspiré du protocole LumVorax, **JAMAIS supprimer un test validé** :

```bash
# Exécuter tous les tests cumulatifs
npm run test:cumulative

# Tests par phase
npm run test:unit      # Phase 1: Infrastructure (001-010)
npm run test:integration  # Phase 2: Analysis Engine (011-020)
npm run test:e2e       # Phase 3: GitHub Integration (021-025)
```

### Tests Individuels

```bash
# Test spécifique
npm test -- tests/unit/telegram.test.js

# Mode watch
npm run test:watch

# Coverage
npm test -- --coverage
```

### Protocole Test Cumulatif

**Phase 1 - Infrastructure (Tests 001-010)**
- TEST_001: Telegram Bot connection
- TEST_002: GitHub OAuth flow
- TEST_003: Redis Queue connection
- TEST_004: Doppler secrets sync
- TEST_005: GitHub Codespace creation
- TEST_006: Repository clone
- TEST_007: Language detection
- TEST_008: Dependencies installation
- TEST_009: Forensic injection
- TEST_010: Execution capture

**Phase 2 - Analysis Engine (Tests 011-020)**
- TEST_011: Error detection
- TEST_012: Memory leak detection
- TEST_013: Security vulnerability scan
- TEST_014: Performance profiling
- TEST_015: Log parsing
- TEST_016: Report generation
- TEST_017: Markdown formatting
- TEST_018: Graph generation
- TEST_019: Code snippet extraction
- TEST_020: Recommendation engine

**Phase 3 - GitHub Integration (Tests 021-025)**
- TEST_021: Branch creation
- TEST_022: Commit report
- TEST_023: Pull Request creation
- TEST_024: Labels addition
- TEST_025: Telegram notification

---

## 📚 Documentation

- [PROTOCOLE_MDBAI.md](PROTOCOLE_MDBAI.md) - Architecture MVP et roadmap
- [CAHIER_DES_CHARGES_MDBAI.md](CAHIER_DES_CHARGES_MDBAI.md) - Spécifications complètes
- [GUIDE_DOPPLER_SECRETS.md](GUIDE_DOPPLER_SECRETS.md) - Configuration sécurité
- [GUIDE_GITHUB_APP.md](docs/GUIDE_GITHUB_APP.md) - Création GitHub App
- [GUIDE_GITHUB_CODESPACES.md](docs/GUIDE_GITHUB_CODESPACES.md) - Intégration Codespaces
- [API_REFERENCE.md](docs/API_REFERENCE.md) - Documentation API REST
- [FORENSIC_LIBRARY.md](docs/FORENSIC_LIBRARY.md) - Bibliothèque forensic C

---

## 🗺️ Roadmap

> **Avancement global : 42%** — Mise à jour 2026-05-28

### Sprint 1 — Infrastructure ✅ 90%
- [x] Serveur Express port 3001 (Node.js)
- [x] Redis Cloud connecté (BullMQ "analysis-jobs", 3 workers)
- [x] Bot Telegram @masterdebugai_bot initialisé
- [x] GitHub App ID 3888479 configurée et active
- [x] Doppler dev_debugai — 14 secrets chargés
- [x] Dashboard web temps réel sur /dashboard
- [x] Proxy Flask port 5000 → MDBAI port 3001
- [x] libmdbai_forensic.so compilée (17KB, Magic 0x4D444241)
- [x] Structure tests unit/integration/e2e
- [ ] Tests 001-005 passants (protocole cumulatif)

### Sprint 2 — Telegram + OAuth 🟡 60%
- [x] Commande /analyze <url> fonctionnelle (crée job Redis)
- [x] Routes OAuth GitHub /auth/github et /auth/github/callback
- [x] Commandes /start, /help, /status, /report enregistrées
- [ ] OAuth GitHub testé en production avec vrai token
- [ ] Notification Telegram avec lien PR validée
- [ ] Tests 006-010 passants

### Sprint 3 — Forensic Engine 🔴 40%
- [x] libmdbai_forensic.so compilée et chargée
- [x] Snapshot mémoire .lum (1 par analyse)
- [x] Capture stdout/stderr lors de l'analyse
- [x] Premier rapport généré (octocat/Hello-World, score 100/100)
- [ ] Détection langage (retourne "unknown" → BUG à corriger)
- [ ] Clone dépôt réel dans /tmp/<jobId>/
- [ ] Injection LD_PRELOAD dans processus cible
- [ ] Exécution dans environnement isolé (GitHub Codespace API)
- [ ] Tests 011-015 passants

### Sprint 4 — Analysis Engine 🔴 30%
- [x] Génération rapport Markdown format complet
- [x] Score qualité 0-100 calculé
- [ ] Détection vraies erreurs compilation/runtime
- [ ] Détection fuites mémoire (nécessite LD_PRELOAD)
- [ ] Scan vulnérabilités CVE (npm audit, pip-audit)
- [ ] Métriques performance réelles (CPU, mémoire, I/O)
- [ ] Tests 016-020 passants

### Sprint 5 — GitHub Integration 🔴 20%
- [x] Code Octokit.js pour branch/commit/PR (implémenté)
- [x] Labels mdbai, automated-analysis définis
- [ ] Test PR réel sur vgacofc/test-mdbai
- [ ] Test PR sur dépôt privé (token OAuth utilisateur)
- [ ] Tests 021-025 passants

### Sprint 6 — Beta Testing 🔴 0%
- [ ] 10 beta testers
- [ ] Corrections bugs post-beta
- [ ] Documentation finale
- [ ] Passage en mode production (NODE_ENV=production)

---

## 🤝 Contribution

Les contributions sont les bienvenues ! Voir [CONTRIBUTING.md](CONTRIBUTING.md).

### Workflow

1. Fork le projet
2. Créer une branche (`git checkout -b feature/AmazingFeature`)
3. Commit les changements (`git commit -m 'Add AmazingFeature'`)
4. Push vers la branche (`git push origin feature/AmazingFeature`)
5. Ouvrir une Pull Request

### Code Style

- ESLint (Airbnb config)
- Prettier (format automatique)
- Tests obligatoires (coverage >= 80%)

---

## 📄 Licence

Ce projet est sous licence MIT. Voir [LICENSE](LICENSE) pour détails.

---

## 👥 Équipe

**LumVorax Team**
- Architecture: Bob (AI Expert)
- Forensic Engine: Inspiré du projet LumVorax Bitcoin Mining
- Documentation: Complète et exhaustive

---

## 🙏 Remerciements

- Projet LumVorax pour la technologie forensic bit-level
- GitHub pour Codespaces gratuit
- Redis Labs pour Redis Cloud gratuit
- Doppler pour secrets management gratuit
- Telegram pour Bot API gratuit

---

## 📞 Support

- **Issues**: [github.com/lumvorax/mdbai/issues](https://github.com/lumvorax/mdbai/issues)
- **Discussions**: [github.com/lumvorax/mdbai/discussions](https://github.com/lumvorax/mdbai/discussions)
- **Email**: support@lumvorax.com
- **Telegram**: [@mdbai_support](https://t.me/mdbai_support)

---

**Made with ❤️ by LumVorax Team**  
**Budget: 0€ | Tests: 25/25 | Coverage: 100%**