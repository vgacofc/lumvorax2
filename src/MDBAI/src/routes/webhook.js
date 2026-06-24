/**
 * MDBAI — Route Webhook GitHub
 * POST /webhook/github — Reçoit les événements push/PR GitHub
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 * BUG-040 FIX: Récupère le token GitHub de l'utilisateur propriétaire du dépôt
 * BUG-044 FIX: Déduplication pour éviter 300+ jobs en boucle
 */

import { Router } from 'express';
import crypto from 'crypto';
import { createJob } from '../models/job.model.js';
import { enqueueAnalysisJob } from '../services/redis.service.js';
import { parseGitHubUrl } from '../utils/validator.js';
import { findUserByGithub } from '../services/user.service.js';
import { DeduplicationService } from '../services/deduplication.service.js';
import logger from '../utils/logger.js';
import { config } from '../config.js';

const webhookRouter = Router();
const dedup = new DeduplicationService();

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

  // BUG-044 FIX: Vérifier déduplication (même repo + commit = 1 job max)
  const branch = body.ref?.replace('refs/heads/', '') || 'main';
  const commitSha = body.after || body.pull_request?.head?.sha || 'unknown';
  
  if (await dedup.isDuplicate(repoUrl, branch, commitSha)) {
    logger.info('[WEBHOOK] Job dupliqué IGNORÉ', { repoUrl, branch, commitSha: commitSha.substring(0, 8) });
    return res.json({
      ok: true,
      message: 'Job dupliqué ignoré (déjà en cours d\'analyse)',
      deduplication: true
    });
  }

  // BUG-040 FIX: Récupérer le token GitHub de l'utilisateur propriétaire du dépôt
  let githubToken = '';
  try {
    const owner = parsed.owner;
    const user = await findUserByGithub(owner);
    if (user && user.github_token) {
      githubToken = user.github_token;
      logger.info('[WEBHOOK] Token GitHub récupéré pour utilisateur', { owner, userId: user.id });
    } else {
      logger.warn('[WEBHOOK] Aucun token trouvé pour propriétaire', { owner });
    }
  } catch (e) {
    logger.warn('[WEBHOOK] Impossible de récupérer token utilisateur', { error: e.message });
  }

  // Marquer comme en cours AVANT de créer le job
  await dedup.markAsProcessing(repoUrl, branch, commitSha);
  
  const job = createJob(
    repoUrl, 'webhook', githubToken,
    branch
  );

  try {
    await enqueueAnalysisJob(job);
    logger.info('[WEBHOOK] Job enqueued via webhook', { jobId: job.id, repo: repoUrl, branch, commitSha: commitSha.substring(0, 8), hasToken: !!githubToken });
    return res.json({ ok: true, job_id: job.id, repo: repoUrl });
  } catch (e) {
    logger.error('[WEBHOOK] Enqueue échoué', { error: e.message });
    return res.status(500).json({ error: 'Impossible d\'enqueuer le job' });
  }
});

export default webhookRouter;
