/**
 * MDBAI — Worker BullMQ Analysis
 * Conforme STANDARD_NAMES_MDBAI.md Section 4 (AnalysisWorker)
 * Exécuté dans un processus séparé — concurrence max: 3
 *
 * Pipeline:
 *   1. Clone dépôt (GitHubService)
 *   2. Détection langage + install dépendances (AnalysisService)
 *   3. Exécution tests avec forensic LumVorax (AnalysisService)
 *   4. Génération rapport Markdown (ReportService)
 *   5. Commit + PR GitHub (GitHubService)
 *   6. Notification Telegram (TelegramService)
 *   7. Stockage résultat Redis (RedisService)
 */

import { Worker } from 'bullmq';
import { mkdirSync, rmSync } from 'fs';
import { join } from 'path';
import { randomUUID } from 'crypto';
import logger from '../utils/logger.js';
import { config } from '../config.js';
import { getRedisClient, storeJobResult } from '../services/redis.service.js';
import { GitHubService } from '../services/github.service.js';
import { AnalysisService } from '../services/analysis.service.js';
import { ReportService } from '../services/report.service.js';
import { TelegramService } from '../services/telegram.service.js';
import { parseGitHubUrl } from '../utils/validator.js';
import { updateJobStatus, JOB_STATUS_RUNNING, JOB_STATUS_COMPLETED, JOB_STATUS_FAILED } from '../models/job.model.js';

const TMP_BASE = config.analysis.tmpDir;
mkdirSync(TMP_BASE, { recursive: true });

let telegramService = null;
function getTelegram() {
  if (!telegramService) {
    telegramService = new TelegramService();
    if (config.telegram.token) telegramService.init(null);
  }
  return telegramService;
}

/**
 * Traitement principal d'un job d'analyse
 */
async function processAnalysisJob(bullJob) {
  const jobData = bullJob.data;
  const jobId   = jobData.id || bullJob.id;
  const log     = logger.child ? logger.child({ jobId }) : logger;

  log.info('[WORKER] Démarrage traitement job', {
    repo: jobData.repo_url, user: jobData.user_id,
  });

  const startMs = Date.now();
  const repoDir = join(TMP_BASE, `repo_${randomUUID()}`);
  const github  = new GitHubService(jobData.github_token || null);
  const parsed  = parseGitHubUrl(jobData.repo_url || '');

  const progress = async (pct, msg) => {
    await bullJob.updateProgress(pct);
    log.info(`[WORKER] Progress ${pct}% — ${msg}`);
    if (jobData.user_id && jobData.user_id !== 'webhook') {
      try {
        await getTelegram().sendProgress(jobData.user_id, jobId, pct, msg);
      } catch {}
    }
  };

  // Token GitHub : OAuth utilisateur en priorité, sinon Installation Token GitHub App
  let effectiveToken = jobData.github_token || '';

  try {
    await progress(5, '🔍 Vérification dépôt + token GitHub App...');

    // Auto-obtention Installation Token si pas de token OAuth
    if (!effectiveToken && parsed) {
      try {
        const tempGithub = new GitHubService(null);
        effectiveToken = await tempGithub.getInstallationToken(parsed.owner, parsed.repo);
        log.info('[WORKER] Installation token obtenu via GitHub App ✅', {
          owner: parsed.owner, repo: parsed.repo,
        });
      } catch (tokenErr) {
        log.warn('[WORKER] Installation token non disponible — clone public sans token', {
          error: tokenErr.message,
        });
      }
    }

    await progress(10, '📥 Clonage du dépôt...');
    await github.cloneRepository(
      jobData.repo_url, effectiveToken,
      repoDir, jobData.branch || 'main'
    );

    if (parsed) {
      try {
        const infoGithub = effectiveToken ? new GitHubService(effectiveToken) : github;
        const info = await infoGithub.getRepoInfo(parsed.owner, parsed.repo);
        jobData.repo_info = info;
      } catch {}
    }

    await progress(25, '🔬 Analyse forensique LumVorax...');
    const analysisService = new AnalysisService(jobId);
    const result = await analysisService.analyze(repoDir, progress);
    result.repo.url   = jobData.repo_url;
    result.repo.owner = parsed?.owner || '';
    result.repo.name  = parsed?.repo  || '';
    result.execution.duration_ms = Date.now() - startMs;

    await progress(90, '📝 Génération rapport...');
    const reportService = new ReportService();
    const markdown = reportService.generateReport(result, {
      jobId, repoUrl: jobData.repo_url,
      repoOwner: parsed?.owner || '', repoName: parsed?.repo || '',
      startedAt: jobData.started_at || new Date().toISOString(),
      duration_ms: result.execution.duration_ms,
    });
    result.report.markdown = markdown;
    reportService.saveReport(markdown, jobId);

    // Utilise le token effectif (OAuth ou Installation) pour créer la PR
    if (effectiveToken && parsed) {
      await progress(93, '🌿 Création branche GitHub...');
      try {
        const ts     = Date.now();
        const prGithub = new GitHubService(effectiveToken);
        const branch = await prGithub.createAnalysisBranch(parsed.owner, parsed.repo, effectiveToken, ts);
        const sha    = await prGithub.commitReport(parsed.owner, parsed.repo, branch, effectiveToken, markdown);
        const pr     = await prGithub.createPullRequest(parsed.owner, parsed.repo, branch, effectiveToken, markdown, result);
        await prGithub.addPrLabels(parsed.owner, parsed.repo, pr.number, effectiveToken);
        result.report.pr_url     = pr.url;
        result.report.pr_number  = pr.number;
        result.report.branch     = branch;
        result.report.commit_sha = sha;
        log.info('[WORKER] PR créée', { prUrl: pr.url, prNumber: pr.number });
      } catch (prErr) {
        log.warn('[WORKER] PR non créée (token insuffisant?)', { error: prErr.message });
      }
    }

    await progress(97, '💾 Sauvegarde résultat...');
    await storeJobResult(jobId, result);

    if (jobData.user_id && jobData.user_id !== 'webhook') {
      try {
        await getTelegram().sendAnalysisComplete(jobData.user_id, result.report.pr_url, {
          score: result.analysis.score,
          errors: result.analysis.errors,
          vulnerabilities: result.analysis.vulnerabilities,
          memory_leaks: result.forensic.memory_leaks,
          duration_ms: result.execution.duration_ms,
        });
      } catch {}
    }

    await progress(100, '✅ Analyse terminée!');
    log.info('[WORKER] Job terminé ✅', {
      score: result.analysis.score,
      duration_ms: result.execution.duration_ms,
      prUrl: result.report.pr_url,
    });

    return { ok: true, jobId, score: result.analysis.score, pr_url: result.report.pr_url };

  } catch (e) {
    log.error('[WORKER] Job échoué', { error: e.message, stack: e.stack });
    if (jobData.user_id && jobData.user_id !== 'webhook') {
      try { await getTelegram().sendAnalysisError(jobData.user_id, jobId, e.message); } catch {}
    }
    throw e;
  } finally {
    try { rmSync(repoDir, { recursive: true, force: true }); } catch {}
  }
}

/**
 * Démarre le worker BullMQ
 * @returns {Worker}
 */
export function startAnalysisWorker() {
  const connection = getRedisClient();
  const worker = new Worker(
    config.jobs.queueName,
    processAnalysisJob,
    {
      connection,
      concurrency: config.jobs.workerConcurrency,
      limiter: { max: 10, duration: 60000 },
    }
  );

  worker.on('active',    (job) => logger.info(`[WORKER] Actif job=${job.id}`));
  worker.on('completed', (job, ret) => logger.info(`[WORKER] Terminé job=${job.id}`, ret));
  worker.on('failed',    (job, err) => logger.error(`[WORKER] Échoué job=${job?.id}`, { error: err.message }));
  worker.on('error',     (err) => logger.error('[WORKER] Erreur interne', { error: err.message }));
  worker.on('stalled',   (jobId) => logger.warn(`[WORKER] Stalled job=${jobId}`));

  logger.info(`[WORKER] Worker démarré — concurrence=${config.jobs.workerConcurrency} queue="${config.jobs.queueName}"`);
  return worker;
}

export default { startAnalysisWorker };
