/**
 * MDBAI — Serveur Express Principal
 * Port: API_PORT (défaut 3001) — séparé du port Flask 5000
 * Conforme STANDARD_NAMES_MDBAI.md + prompt.txt Règle #1 (traçabilité bit-level)
 * Conforme guidelines Flask: port 5000 réservé au projet principal
 */

import express from 'express';
import session from 'express-session';
import cors from 'cors';
import helmet from 'helmet';
import { config, validateConfig } from './config.js';
import logger from './utils/logger.js';
import webhookRouter from './routes/webhook.js';
import analyzeRouter from './routes/analyze.js';
import authRouter from './routes/auth.js';
import statusRouter from './routes/status.js';
import dashboardRouter from './routes/dashboard.js';
import { TelegramService } from './services/telegram.service.js';
import { getAnalysisQueue, pingRedis, closeRedis, getJobResult } from './services/redis.service.js';
import { startAnalysisWorker } from './workers/analysis.worker.js';
import { createJob } from './models/job.model.js';
import { enqueueAnalysisJob } from './services/redis.service.js';

const app = express();
let worker = null;
let telegramService = null;

/**
 * Middlewares de sécurité
 */
app.use(helmet({ contentSecurityPolicy: false }));
app.use(cors({ origin: true, credentials: true }));

/**
 * Capture rawBody pour validation signature webhook HMAC-SHA256
 * Méthode officielle Express: option `verify` de express.json()
 * Évite le double-consume du stream (bug "stream is not readable")
 */
app.use(express.json({
  limit: '10mb',
  verify: (req, _res, buf) => { req.rawBody = buf.toString('utf8'); },
}));
app.use(express.urlencoded({ extended: true }));
app.use(session({
  secret: config.session.secret,
  resave: false,
  saveUninitialized: false,
  cookie: { secure: config.app.env === 'production', maxAge: 24 * 60 * 60 * 1000 },
}));

/**
 * Logging des requêtes entrantes
 */
app.use((req, res, next) => {
  logger.debug(`[HTTP] ${req.method} ${req.path}`, { ip: req.ip });
  next();
});

/**
 * Routes API
 */
app.use('/webhook', webhookRouter);
app.use('/api/analyze', analyzeRouter);
app.use('/api/status', statusRouter);
app.use('/auth', authRouter);
app.use('/dashboard', dashboardRouter);

/**
 * GET /api/report/:jobId — Rapport complet séparé de /api/status
 * CF-010: Rapport généré — RAPPORT_MDBAI_*.md
 */
app.get('/api/report/:jobId', async (req, res) => {
  const { jobId } = req.params;
  try {
    const result = await getJobResult(jobId);
    if (!result) return res.status(404).json({ error: 'Rapport non trouvé', jobId,
      hint: 'Le rapport a peut-être expiré (TTL 24h)' });
    return res.json({ ok: true, jobId, result });
  } catch (e) {
    logger.error('[REPORT] Erreur récupération rapport', { jobId, error: e.message });
    return res.status(500).json({ error: e.message });
  }
});

/**
 * GET /health — Health check endpoint
 */
app.get('/health', async (req, res) => {
  const redisOk = await pingRedis().catch(() => false);
  const status  = redisOk ? 'healthy' : 'degraded';
  res.status(redisOk ? 200 : 503).json({
    status,
    version: '0.1.0',
    service: 'mdbai',
    redis: redisOk ? 'connected' : 'disconnected',
    telegram: !!(telegramService?.initialized),
    worker: !!(worker),
    timestamp: new Date().toISOString(),
    forensic_lib: config.forensic.libPath,
  });
});

/**
 * GET / — Redirige vers /dashboard
 */
app.get('/', (req, res) => {
  res.redirect('/dashboard');
});

/**
 * 404 handler
 */
app.use((req, res) => {
  res.status(404).json({ error: `Route non trouvée: ${req.method} ${req.path}` });
});

/**
 * Gestionnaire d'erreurs global
 */
app.use((err, req, res, next) => {
  logger.error('[SERVER] Erreur non gérée', { error: err.message, stack: err.stack });
  res.status(err.status || 500).json({
    error: err.code || 'INTERNAL_ERROR',
    message: err.message,
    ...(config.app.env !== 'production' ? { stack: err.stack } : {}),
  });
});

/**
 * Initialise et démarre tous les composants MDBAI
 */
async function startMdbai() {
  logger.info('═══════════════════════════════════════════════════════');
  logger.info('  MDBAI — Master Debug AI v0.1.0 — Démarrage');
  logger.info('  Forensic Engine: LumVorax C111 (bit-level tracing)');
  logger.info('  Budget: 0€ — Tier gratuit uniquement');
  logger.info('═══════════════════════════════════════════════════════');

  const configOk = validateConfig();
  if (!configOk) {
    logger.warn('[MDBAI] Mode dégradé — certains secrets manquants');
  }

  logger.info('[MDBAI] Connexion Redis...');
  const redisOk = await pingRedis().catch(() => false);
  logger.info(`[MDBAI] Redis: ${redisOk ? '✅ connecté' : '⚠️ non disponible (mode dégradé)'}`);

  if (redisOk) {
    logger.info('[MDBAI] Initialisation queue BullMQ...');
    getAnalysisQueue();

    logger.info('[MDBAI] Démarrage worker d\'analyse...');
    worker = startAnalysisWorker();
  }

  logger.info('[MDBAI] Initialisation bot Telegram...');
  telegramService = new TelegramService();
  telegramService.init(async (chatId, repoUrl, userId) => {
    const job = createJob(repoUrl, String(chatId), '', 'main');
    try {
      if (redisOk) {
        await enqueueAnalysisJob(job);
        await telegramService.sendAnalysisStarted(chatId, job.id, repoUrl);
      } else {
        await telegramService.bot?.sendMessage(chatId,
          '⚠️ Redis non disponible — analyses désactivées temporairement');
      }
    } catch (e) {
      logger.error('[MDBAI] Enqueue Telegram échoué', { error: e.message });
      await telegramService.sendAnalysisError(chatId, job.id, e.message);
    }
  });

  const port = config.app.port;
  app.listen(port, '0.0.0.0', () => {
    logger.info(`[MDBAI] Serveur Express démarré sur port ${port} ✅`);
    logger.info(`[MDBAI] Health: http://0.0.0.0:${port}/health`);
    logger.info(`[MDBAI] Bot Telegram: @${config.telegram.botUsername}`);
  });

  process.on('SIGTERM', gracefulShutdown);
  process.on('SIGINT',  gracefulShutdown);
}

async function gracefulShutdown(signal) {
  logger.info(`[MDBAI] Signal ${signal} reçu — arrêt propre...`);
  if (worker)          await worker.close().catch(() => {});
  if (telegramService) await telegramService.stop().catch(() => {});
  await closeRedis().catch(() => {});
  logger.info('[MDBAI] Arrêt propre effectué ✅');
  process.exit(0);
}

startMdbai().catch(e => {
  logger.error('[MDBAI] Erreur démarrage fatale', { error: e.message, stack: e.stack });
  process.exit(1);
});

export default app;
