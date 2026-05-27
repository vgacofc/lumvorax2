/**
 * MDBAI — Route Statut Jobs
 * GET /api/status/:jobId  — État d'un job avec progression %
 * GET /api/report/:jobId  — Rapport complet
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 */

import { Router } from 'express';
import { getJobStatus, getJobResult } from '../services/redis.service.js';
import logger from '../utils/logger.js';

const statusRouter = Router();

/**
 * GET /api/status/:jobId
 * Retourne l'état complet d'un job avec progression temps réel
 */
statusRouter.get('/:jobId', async (req, res) => {
  const { jobId } = req.params;

  if (!jobId) {
    return res.status(400).json({ error: 'jobId requis' });
  }

  try {
    const status = await getJobStatus(jobId);
    if (!status) {
      return res.status(404).json({
        error: 'Job non trouvé',
        jobId,
        hint: 'Le job a peut-être expiré (TTL 24h)',
      });
    }

    return res.json({
      ok: true,
      jobId: status.id,
      state: status.state,
      progress: status.progress || 0,
      progress_bar: buildProgressBar(status.progress || 0),
      timestamp: status.timestamp,
      failed_reason: status.failedReason || null,
      report_url: status.state === 'completed' ? `/api/report/${jobId}` : null,
    });
  } catch (e) {
    logger.error('[STATUS] Erreur récupération statut', { jobId, error: e.message });
    return res.status(500).json({ error: e.message, jobId });
  }
});

/**
 * GET /api/report/:jobId
 * Récupère le rapport Markdown complet
 */
statusRouter.get('/report/:jobId', async (req, res) => {
  const { jobId } = req.params;
  try {
    const result = await getJobResult(jobId);
    if (!result) {
      return res.status(404).json({ error: 'Rapport non trouvé', jobId });
    }
    return res.json({ ok: true, jobId, result });
  } catch (e) {
    logger.error('[STATUS] Erreur récupération rapport', { jobId, error: e.message });
    return res.status(500).json({ error: e.message });
  }
});

/**
 * GET /api/status
 * Liste des derniers jobs (dashboard)
 */
statusRouter.get('/', async (req, res) => {
  return res.json({
    ok: true,
    message: 'MDBAI Status API v0.1.0',
    endpoints: {
      job_status: 'GET /api/status/:jobId',
      job_report: 'GET /api/report/:jobId',
      start_analysis: 'POST /api/analyze',
    },
  });
});

function buildProgressBar(progress) {
  const filled = Math.floor(progress / 10);
  return '█'.repeat(filled) + '░'.repeat(10 - filled) + ` ${progress}%`;
}

export default statusRouter;
