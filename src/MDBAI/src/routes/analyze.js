/**
 * MDBAI — Route API Analyse
 * POST /api/analyze — Lance une analyse forensique
 * GET  /api/analyze/:jobId — Récupère l'état
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 */

import { Router } from 'express';
import { createJob } from '../models/job.model.js';
import { enqueueAnalysisJob, getJobStatus, getJobResult } from '../services/redis.service.js';
import { validateAnalyzeRequest, parseGitHubUrl } from '../utils/validator.js';
import { DeduplicationService } from '../services/deduplication.service.js';
import logger from '../utils/logger.js';

const analyzeRouter = Router();
const dedup = new DeduplicationService();

/**
 * POST /api/analyze
 * Lance une analyse forensique sur un dépôt GitHub
 * Body: { repo_url, user_id, github_token?, branch? }
 */
analyzeRouter.post('/', async (req, res) => {
  const validation = validateAnalyzeRequest(req.body);
  if (!validation.valid) {
    return res.status(400).json({
      error: 'Données invalides',
      details: validation.errors,
    });
  }

  const { repo_url, user_id, github_token, branch } = validation.value;
  
  // BUG #60 FIX: Vérifier déduplication (même repo + branch)
  const parsed = parseGitHubUrl(repo_url);
  if (parsed) {
    const branchName = branch || 'main';
    const commitSha = 'api-request'; // Pour API, on utilise un identifiant générique
    
    if (await dedup.isDuplicate(repo_url, branchName, commitSha)) {
      logger.info('[ANALYZE] Job dupliqué IGNORÉ via API', { repo: repo_url, branch: branchName, user: user_id });
      return res.status(409).json({
        error: 'Analyse déjà en cours',
        message: 'Une analyse de ce dépôt est déjà en cours. Veuillez attendre qu\'elle se termine.',
        deduplication: true
      });
    }
  }
  
  const job = createJob(repo_url, user_id, github_token || '', branch || 'main');

  try {
    await enqueueAnalysisJob(job);
    logger.info('[ANALYZE] Job enqueued via API', {
      jobId: job.id, repo: repo_url, user: user_id,
    });

    return res.status(202).json({
      ok: true,
      job_id: job.id,
      status: job.status,
      message: 'Analyse démarrée. Utilisez /api/status/:jobId pour suivre la progression.',
      status_url: `/api/status/${job.id}`,
    });
  } catch (e) {
    logger.error('[ANALYZE] Enqueue échoué', { error: e.message });
    return res.status(500).json({ error: 'Impossible de démarrer l\'analyse', detail: e.message });
  }
});

/**
 * GET /api/analyze/:jobId/report
 * Récupère le rapport complet d'un job terminé
 */
analyzeRouter.get('/:jobId/report', async (req, res) => {
  const { jobId } = req.params;
  try {
    const result = await getJobResult(jobId);
    if (!result) {
      return res.status(404).json({ error: 'Rapport non trouvé', jobId });
    }
    const accept = req.headers.accept || '';
    if (accept.includes('text/plain') || accept.includes('text/markdown')) {
      return res.type('text/markdown').send(result.report?.markdown || '');
    }
    return res.json({ ok: true, jobId, report: result });
  } catch (e) {
    logger.error('[ANALYZE] Récupération rapport échouée', { jobId, error: e.message });
    return res.status(500).json({ error: e.message });
  }
});

export default analyzeRouter;
