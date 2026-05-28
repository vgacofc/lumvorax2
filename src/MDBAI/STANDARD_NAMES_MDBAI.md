# STANDARD_NAMES_MDBAI.md — Nommage Canonique MDBAI
# Master Debug AI Platform — Convention de Nommage Officielle
# Version: 1.0.0 | Date: 2026-05-27 | Auteur: LumVorax Team
# Inspiré de STANDARD_NAMES.md LumVorax + prompt.txt règles v3.0
#
# RÈGLE ABSOLUE: Tout nom utilisé dans le code DOIT figurer ici.
# RÈGLE ABSOLUE: Jamais de token/secret en clair dans le code.
# RÈGLE ABSOLUE: Traçabilité bit-level LumVorax active sur tous les runs.

---

## SECTION 1 — IDENTIFIANTS PROJET

| Nom Canonique | Valeur | Usage |
|---------------|--------|-------|
| `MDBAI_PROJECT_NAME` | `"mdbai"` | Nom npm package |
| `MDBAI_VERSION` | `"0.1.0"` | Version MVP |
| `MDBAI_BOT_USERNAME` | `"masterdebugai_bot"` | Username Telegram bot |
| `MDBAI_PR_BRANCH_PREFIX` | `"mdbai-analysis-"` | Préfixe branche PR |
| `MDBAI_REPORT_FILENAME` | `"RAPPORT_MDBAI_"` | Préfixe fichier rapport |
| `MDBAI_PR_LABEL_PRIMARY` | `"mdbai"` | Label PR principal |
| `MDBAI_PR_LABEL_AUTOMATED` | `"automated-analysis"` | Label PR automation |
| `MDBAI_PR_LABEL_FORENSIC` | `"forensic"` | Label PR forensic |
| `MDBAI_JOB_QUEUE_NAME` | `"analysis-jobs"` | Nom queue BullMQ |
| `MDBAI_JOB_TIMEOUT_MS` | `600000` | Timeout job: 10 min |
| `MDBAI_JOB_RETRY_COUNT` | `3` | Tentatives retry |
| `MDBAI_WORKER_CONCURRENCY` | `3` | Workers concurrents |

---

## SECTION 2 — VARIABLES D'ENVIRONNEMENT (secrets via Replit Secrets)

| Nom Variable Env | Description | Obligatoire | Source |
|-----------------|-------------|-------------|--------|
| `TELEGRAM_BOT_TOKEN` | Token API Telegram Bot | ✅ OUI | Replit Secret |
| `REDIS_URL` | URL complète Redis Cloud | ✅ OUI | Replit Secret |
| `REDIS_HOST` | Host Redis | ✅ OUI | Replit Secret |
| `REDIS_PORT` | Port Redis | ✅ OUI | Replit Secret |
| `REDIS_PASSWORD` | Password Redis | ✅ OUI | Replit Secret |
| `GITHUB_APP_ID` | ID GitHub App | ✅ OUI | Replit Secret |
| `GITHUB_APP_CLIENT_ID` | Client ID OAuth | ✅ OUI | Replit Secret |
| `GITHUB_APP_CLIENT_SECRET` | Client Secret OAuth | ✅ OUI | Replit Secret |
| `GITHUB_PRIVATE_KEY` | Clé privée RSA GitHub App | ✅ OUI | Replit Secret |
| `GITHUB_WEBHOOK_SECRET` | Secret webhook validation | ✅ OUI | Replit Secret |
| `SESSION_SECRET` | Secret session Express | ✅ OUI | Replit Secret |
| `NODE_ENV` | Environnement (development/production) | ✅ OUI | Replit Env |
| `LOG_LEVEL` | Niveau log Winston (debug/info/warn/error) | NON | Replit Env |
| `API_PORT` | Port serveur Express | NON | Replit Env (défaut: 3001) |
| `MDBAI_FORENSIC_LIB_PATH` | Chemin libforensic.so | NON | Replit Env |

---

## SECTION 3 — NOMMAGE FICHIERS (snake_case.js)

### Services
| Fichier | Classe Exportée | Description |
|---------|----------------|-------------|
| `src/services/telegram.service.js` | `TelegramService` | Bot Telegram |
| `src/services/github.service.js` | `GitHubService` | API GitHub Octokit |
| `src/services/redis.service.js` | `RedisService` | Queue BullMQ + cache |
| `src/services/analysis.service.js` | `AnalysisService` | Moteur d'analyse |
| `src/services/report.service.js` | `ReportService` | Générateur rapport MD |

### Routes
| Fichier | Préfixe route | Description |
|---------|--------------|-------------|
| `src/routes/webhook.js` | `/webhook` | GitHub webhooks entrants |
| `src/routes/analyze.js` | `/api/analyze` | Démarrer analyse |
| `src/routes/auth.js` | `/auth` | OAuth GitHub flow |
| `src/routes/status.js` | `/api/status` | Statut jobs |

### Models
| Fichier | Interface TypeScript | Description |
|---------|---------------------|-------------|
| `src/models/job.model.js` | `AnalysisJob` | Job d'analyse Redis |
| `src/models/result.model.js` | `AnalysisResult` | Résultat complet |

### Utils
| Fichier | Export Principal | Description |
|---------|-----------------|-------------|
| `src/utils/logger.js` | `logger` (Winston) | Logger structuré |
| `src/utils/validator.js` | `validateRepoUrl`, `validateJobId` | Validation Joi |
| `src/utils/forensic.js` | `ForensicBridge` | Pont JS↔libforensic.so |

### Workers
| Fichier | Classe | Description |
|---------|--------|-------------|
| `src/workers/analysis.worker.js` | `AnalysisWorker` | Worker BullMQ |

---

## SECTION 4 — NOMMAGE CLASSES (PascalCase)

| Classe | Fichier | Description |
|--------|---------|-------------|
| `TelegramService` | services/telegram.service.js | Gestion bot |
| `GitHubService` | services/github.service.js | API GitHub |
| `RedisService` | services/redis.service.js | Redis + BullMQ |
| `AnalysisService` | services/analysis.service.js | Analyse forensic |
| `ReportService` | services/report.service.js | Rapport Markdown |
| `AnalysisWorker` | workers/analysis.worker.js | Worker job |
| `ForensicBridge` | utils/forensic.js | Bridge C lib |
| `MdbaiError` | utils/errors.js | Erreur custom |

---

## SECTION 5 — NOMMAGE FONCTIONS (camelCase)

### TelegramService
| Fonction | Signature | Description |
|----------|-----------|-------------|
| `handleStart` | `(msg) → void` | Commande /start |
| `handleHelp` | `(msg) → void` | Commande /help |
| `handleAnalyze` | `(msg) → void` | Commande /analyze |
| `handleStatus` | `(msg) → void` | Commande /status |
| `sendAnalysisStarted` | `(chatId, jobId) → void` | Notif démarrage |
| `sendAnalysisComplete` | `(chatId, prUrl, summary) → void` | Notif fin |
| `sendAnalysisError` | `(chatId, jobId, error) → void` | Notif erreur |

### GitHubService
| Fonction | Signature | Description |
|----------|-----------|-------------|
| `getAuthUrl` | `() → string` | URL OAuth |
| `exchangeCode` | `(code) → token` | Échange code→token |
| `getRepoInfo` | `(owner, repo, token) → RepoInfo` | Infos dépôt |
| `cloneRepository` | `(repoUrl, token, destDir) → void` | Clone |
| `createAnalysisBranch` | `(owner, repo, token, ts) → string` | Créer branche |
| `commitReport` | `(owner, repo, branch, token, md) → sha` | Commit rapport |
| `createPullRequest` | `(owner, repo, branch, token, body) → PrData` | Créer PR |
| `addPrLabels` | `(owner, repo, prNumber, token) → void` | Labels PR |

### AnalysisService
| Fonction | Signature | Description |
|----------|-----------|-------------|
| `detectLanguage` | `(dir) → Language` | Détection langage |
| `installDependencies` | `(dir, lang) → ExecResult` | Install dépendances |
| `runTests` | `(dir, lang) → ExecResult` | Exécuter tests |
| `detectErrors` | `(stdout, stderr) → Error[]` | Détection erreurs |
| `detectMemoryLeaks` | `(forensicData) → MemoryLeak[]` | Fuites mémoire |
| `scanVulnerabilities` | `(dir, lang) → Vulnerability[]` | Scan sécurité |
| `analyzePerformance` | `(forensicData) → PerfMetrics` | Métriques perf |

### ReportService
| Fonction | Signature | Description |
|----------|-----------|-------------|
| `generateReport` | `(result) → string` | Génère rapport MD |
| `formatErrors` | `(errors) → string` | Section erreurs |
| `formatMemoryLeaks` | `(leaks) → string` | Section mémoire |
| `formatVulnerabilities` | `(vulns) → string` | Section sécurité |
| `formatPerformance` | `(metrics) → string` | Section perf |
| `formatForensic` | `(forensic) → string` | Section forensic |

---

## SECTION 6 — CONSTANTES (UPPER_SNAKE_CASE)

| Constante | Valeur | Fichier |
|-----------|--------|---------|
| `JOB_STATUS_PENDING` | `"pending"` | models/job.model.js |
| `JOB_STATUS_RUNNING` | `"running"` | models/job.model.js |
| `JOB_STATUS_COMPLETED` | `"completed"` | models/job.model.js |
| `JOB_STATUS_FAILED` | `"failed"` | models/job.model.js |
| `LANG_NODEJS` | `"nodejs"` | services/analysis.service.js |
| `LANG_PYTHON` | `"python"` | services/analysis.service.js |
| `LANG_RUST` | `"rust"` | services/analysis.service.js |
| `LANG_GO` | `"go"` | services/analysis.service.js |
| `LANG_C_CPP` | `"c_cpp"` | services/analysis.service.js |
| `LANG_UNKNOWN` | `"unknown"` | services/analysis.service.js |
| `VULN_SEVERITY_LOW` | `"low"` | services/analysis.service.js |
| `VULN_SEVERITY_MEDIUM` | `"medium"` | services/analysis.service.js |
| `VULN_SEVERITY_HIGH` | `"high"` | services/analysis.service.js |
| `VULN_SEVERITY_CRITICAL` | `"critical"` | services/analysis.service.js |
| `FORENSIC_LUM_MAGIC` | `0x4C554D54` | utils/forensic.js |
| `FORENSIC_LOG_ROTATION_MB` | `20` | utils/forensic.js |

---

## SECTION 7 — NOMMAGE TESTS (test_NNN_description.test.js)

### Phase 1 — Infrastructure (001–010)
| Fichier | Test ID | Description |
|---------|---------|-------------|
| `tests/unit/test_001_telegram_bot.test.js` | TEST_001 | Connexion bot Telegram |
| `tests/unit/test_002_github_oauth.test.js` | TEST_002 | OAuth GitHub flow |
| `tests/unit/test_003_redis_queue.test.js` | TEST_003 | Connexion Redis Cloud |
| `tests/unit/test_004_doppler_secrets.test.js` | TEST_004 | Secrets env vars |
| `tests/unit/test_005_language_detection.test.js` | TEST_005 | Détection langage |
| `tests/unit/test_006_repo_clone.test.js` | TEST_006 | Clone dépôt |
| `tests/unit/test_007_deps_install.test.js` | TEST_007 | Installation dépendances |
| `tests/unit/test_008_forensic_injection.test.js` | TEST_008 | Injection libforensic.so |
| `tests/unit/test_009_execution_capture.test.js` | TEST_009 | Capture stdout/stderr |
| `tests/unit/test_010_report_generation.test.js` | TEST_010 | Génération rapport MD |

### Phase 2 — Analysis Engine (011–020)
| Fichier | Test ID | Description |
|---------|---------|-------------|
| `tests/integration/test_011_error_detection.test.js` | TEST_011 | Détection erreurs |
| `tests/integration/test_012_memory_leak.test.js` | TEST_012 | Fuites mémoire |
| `tests/integration/test_013_security_scan.test.js` | TEST_013 | Scan vulnérabilités |
| `tests/integration/test_014_performance.test.js` | TEST_014 | Analyse performance |
| `tests/integration/test_015_log_parsing.test.js` | TEST_015 | Parsing logs |

### Phase 3 — GitHub Integration (021–025)
| Fichier | Test ID | Description |
|---------|---------|-------------|
| `tests/e2e/test_021_branch_creation.test.js` | TEST_021 | Création branche |
| `tests/e2e/test_022_commit_report.test.js` | TEST_022 | Commit rapport |
| `tests/e2e/test_023_pr_creation.test.js` | TEST_023 | Création PR |
| `tests/e2e/test_024_labels.test.js` | TEST_024 | Labels PR |
| `tests/e2e/test_025_telegram_notification.test.js` | TEST_025 | Notification Telegram |

---

## SECTION 8 — FORMAT LOGS (YYYY-MM-DD_HH-MM-SS_event.log)

| Fichier Log | Format | Description |
|-------------|--------|-------------|
| `logs/execution/YYYY-MM-DD_HH-MM-SS_analysis.log` | JSON | Log analyse complète |
| `logs/errors/YYYY-MM-DD_HH-MM-SS_error.log` | JSON | Erreurs système |
| `logs/forensic/YYYY-MM-DD_HH-MM-SS_forensic.log` | JSON | Data forensic |
| `forensic/YYYY-MM-DD_HH-MM-SS_memory.lum` | Binaire LUM | Snapshot mémoire |
| `forensic/YYYY-MM-DD_HH-MM-SS_logger.lum` | Binaire LUM | Log encoder LUM |

---

## SECTION 9 — NOMMAGE FORENSIC C (snake_case.c/.h)

| Fichier | Symbole Principal | Description |
|---------|------------------|-------------|
| `forensic/mdbai_forensic.h` | `mdbai_forensic_t` | Header principal |
| `forensic/mdbai_forensic.c` | `mdbai_forensic_init()` | Implémentation |
| `forensic/mdbai_analysis_runner.sh` | — | Script bash runner |
| `forensic/Makefile` | `libmdbai_forensic.so` | Build shared lib |

### Symboles C MDBAI
| Symbole | Type | Description |
|---------|------|-------------|
| `mdbai_forensic_t` | struct | Contexte forensic |
| `mdbai_forensic_init` | fonction | Initialiser forensic |
| `mdbai_forensic_destroy` | fonction | Libérer ressources |
| `mdbai_memory_snapshot` | fonction | Snapshot mémoire LUM |
| `mdbai_log_event` | fonction | Logger événement |
| `mdbai_detect_leaks` | fonction | Détecter fuites |
| `MDBAI_FORENSIC_MAGIC` | constante | `0x4D444241` ('MDBA') |
| `MDBAI_LUM_GRANULARITY` | constante | Granularité PAGE par défaut |

---

## SECTION 10 — INTÉGRATION LUMVORAX

| Module LumVorax | Usage MDBAI | Interface |
|----------------|-------------|-----------|
| `src/lum/lum_memory_tracer.h` | Snapshot mémoire cible | `lum_memory_snapshot_self()` |
| `src/lum/lum_log_encoder.h` | Log bit-level encode | `lum_log_writer_open()` |
| `src/debug/forensic_logger.h` | Logger forensic | `forensic_log()` |
| `src/debug/memory_tracker.h` | Track allocations | `TRACKED_MALLOC/FREE` |
| `src/debug/ultra_forensic_logger.h` | Logger ultra détaillé | — |

---

## SECTION 11 — API REST ENDPOINTS

| Méthode | Route | Handler | Description |
|---------|-------|---------|-------------|
| `POST` | `/webhook/github` | `webhookRouter` | Webhook GitHub push/PR |
| `POST` | `/api/analyze` | `analyzeRouter` | Lancer analyse |
| `GET` | `/api/status/:jobId` | `statusRouter` | Statut job |
| `GET` | `/api/report/:jobId` | `statusRouter` | Récupérer rapport |
| `GET` | `/auth/github` | `authRouter` | Redirection OAuth |
| `GET` | `/auth/github/callback` | `authRouter` | Callback OAuth |
| `GET` | `/health` | inline | Health check |

---

## SECTION 12 — CODES D'ERREUR MDBAI

| Code | Constante | Message |
|------|-----------|---------|
| `MDBAI_ERR_001` | `ERR_TELEGRAM_CONNECT` | Bot Telegram non connecté |
| `MDBAI_ERR_002` | `ERR_GITHUB_AUTH` | GitHub OAuth échoué |
| `MDBAI_ERR_003` | `ERR_REDIS_CONNECT` | Redis non disponible |
| `MDBAI_ERR_004` | `ERR_REPO_CLONE` | Clone dépôt échoué |
| `MDBAI_ERR_005` | `ERR_LANG_DETECT` | Langage non détecté |
| `MDBAI_ERR_006` | `ERR_DEPS_INSTALL` | Installation dépendances échouée |
| `MDBAI_ERR_007` | `ERR_FORENSIC_INIT` | Forensic non initialisé |
| `MDBAI_ERR_008` | `ERR_ANALYSIS_TIMEOUT` | Timeout analyse (10 min) |
| `MDBAI_ERR_009` | `ERR_REPORT_GEN` | Génération rapport échouée |
| `MDBAI_ERR_010` | `ERR_PR_CREATE` | Création PR échouée |

---

## SECTION 13 — RÈGLES NOMMAGE SPÉCIFIQUES MDBAI

1. **Toujours préfixer `mdbai_` dans le code C** pour éviter collision avec LumVorax
2. **Services JS = snake_case.service.js** (jamais juste service.js)
3. **Tests = test_NNN_description.test.js** (NNN = numéro 3 chiffres)
4. **Logs = YYYY-MM-DD_HH-MM-SS_event.log** (timestamp ISO complet)
5. **Branches GitHub = mdbai-analysis-{timestamp}** (Unix timestamp)
6. **Rapports = RAPPORT_MDBAI_{YYYYMMDD_HHMMSS}.md** (timestamp compact)
7. **Jobs Redis = mdbai:{uuid}** (préfixe namespace)
8. **Variables d'env = MAJUSCULES_UNDERSCORE** (jamais de point ou tiret)

---

## SECTION 14 — ÉTAT RUNTIME 2026-05-28

| Nom Canonique | Valeur Réelle | Vérifié |
|---------------|--------------|---------|
| `MDBAI_SERVER_PORT` | `3001` | ✅ Express écoute port 3001 |
| `MDBAI_FORENSIC_LIB` | `forensic/libmdbai_forensic.so` | ✅ 17KB, Magic 0x4D444241 |
| `MDBAI_FORENSIC_MAGIC` | `0x4D444241` | ✅ = "MDBA" ASCII |
| `MDBAI_GITHUB_APP_ID` | `3888479` | ✅ App active GitHub |
| `MDBAI_BOT_USERNAME` | `masterdebugai_bot` | ✅ Bot actif Telegram |
| `MDBAI_REDIS_QUEUE` | `analysis-jobs` | ✅ BullMQ opérationnel |
| `MDBAI_WORKER_CONCURRENCY` | `3` | ✅ 3 workers actifs |
| `MDBAI_DASHBOARD_ROUTE` | `/dashboard` | ✅ Proxy Flask:5000 → MDBAI:3001 |
| `MDBAI_REPORT_DIR` | `src/MDBAI/reports/` | ✅ 1 rapport existant |
| `MDBAI_FIRST_REPORT` | `RAPPORT_MDBAI_2026-05-28T1618_mdbai-bb.md` | ✅ Score 100/100 |
| `MDBAI_JOB_ID_FORMAT` | `mdbai-{uuid4}` | ✅ Confirmé en production |
| `MDBAI_LUM_GRANULARITY` | `LUM_TRACE_GRANULARITY_PAGE` | ✅ 4 KiB par event |
| `MDBAI_AVANCEMENT_GLOBAL` | `42%` | 🟡 Mis à jour 2026-05-28 |

### Bugs Actifs (2026-05-28)

| Bug ID | Composant | Description | Sprint Fix |
|--------|-----------|-------------|-----------|
| `BUG-LANG-001` | `analysis.service.js` | `detectLanguage()` retourne "unknown" | Sprint 3 |
| `BUG-STATE-001` | `dashboard.js` | Jobs affichent "pending" même terminés | Sprint 2 |
| `BUG-METRIC-001` | `analysis.worker.js` | CPU/RAM/IO métriques = 0 (pas d'exec réelle) | Sprint 3 |
| `BUG-CLONE-001` | `analysis.service.js` | Clone non persisté entre étapes pipeline | Sprint 3 |

---

## CHANGELOG

| Version | Date | Modification |
|---------|------|-------------|
| 1.0.0 | 2026-05-27 | Création initiale STANDARD_NAMES_MDBAI.md |
| 1.1.0 | 2026-05-28 | Section 14 ajoutée — état runtime réel + bugs actifs + avancement 42% |
