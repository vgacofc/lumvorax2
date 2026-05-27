/**
 * MDBAI — Route Webhook GitHub
 * POST /webhook/github — Reçoit les événements push/PR GitHub
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 */

import { Router } from 'express';
import crypto from 'crypto';
import { createJob } from '../models/job.model.js';
import { enqueueAnalysisJob } from '../services/redis.service.js';
import { parseGitHubUrl } from '../utils/validator.js';
import logger from '../utils/logger.js';
import { config } from '../config.js';

const webhookRouter = Router();

function verifyGitHubSignature(payload, signature) {
  if (!config.github.webhookSecret) return true;
  const expected = `sha256=${crypto
    .createHmac('sha256', config.github.webhookSecret)
    .update(payload)
    .digest('hex')}`;
  try {
    return crypto.timingSafeEqual(Buffer.from(signature || ''), Buffer.from(expected));
  } catch { return false; }
}

webhookRouter.post('/github', async (req, res) => {
  const event   = req.headers['x-github-event'];
  const sig     = req.headers['x-hub-signature-256'];
  const rawBody = req.rawBody || JSON.stringify(req.body);

  logger.info('[WEBHOOK] Événement GitHub reçu', { event });

  if (!verifyGitHubSignature(rawBody, sig)) {
    logger.warn('[WEBHOOK] Signature invalide — rejetée');
    return res.status(401).json({ error: 'Signature invalide' });
  }

  if (event === 'ping') {
    return res.json({ ok: true, message: 'MDBAI webhook actif ✅' });
  }

  if (!['push', 'pull_request'].includes(event)) {
    return res.json({ ok: true, message: `Événement ${event} ignoré` });
  }

  const body    = req.body;
  const repoUrl = body.repository?.clone_url?.replace(/\.git$/, '');
  const parsed  = repoUrl ? parseGitHubUrl(repoUrl) : null;

  if (!parsed) {
    return res.status(400).json({ error: 'Dépôt non identifiable' });
  }

  const job = createJob(
    repoUrl, 'webhook', '',
    body.ref?.replace('refs/heads/', '') || 'main'
  );

  try {
    await enqueueAnalysisJob(job);
    logger.info('[WEBHOOK] Job enqueued via webhook', { jobId: job.id, repo: repoUrl });
    return res.json({ ok: true, job_id: job.id, repo: repoUrl });
  } catch (e) {
    logger.error('[WEBHOOK] Enqueue échoué', { error: e.message });
    return res.status(500).json({ error: 'Impossible d\'enqueuer le job' });
  }
});

export default webhookRouter;
