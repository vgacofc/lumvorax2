/**
 * MDBAI — Modèle Job d'analyse (stocké Redis via BullMQ)
 * Conforme STANDARD_NAMES_MDBAI.md Section 4 + 6
 * Interface: AnalysisJob
 */

import { randomUUID } from 'crypto';

export const JOB_STATUS_PENDING   = 'pending';
export const JOB_STATUS_RUNNING   = 'running';
export const JOB_STATUS_COMPLETED = 'completed';
export const JOB_STATUS_FAILED    = 'failed';

/**
 * Crée un nouveau job d'analyse
 * @param {string} repoUrl - URL GitHub du dépôt
 * @param {string} userId - ID utilisateur Telegram
 * @param {string} [githubToken] - Token OAuth GitHub
 * @param {string} [branch] - Branche cible
 * @returns {AnalysisJob}
 */
export function createJob(repoUrl, userId, githubToken = '', branch = 'main') {
  return {
    id: `mdbai:${randomUUID()}`,
    repo_url: repoUrl,
    user_id: String(userId),
    github_token: githubToken,
    branch,
    status: JOB_STATUS_PENDING,
    created_at: new Date().toISOString(),
    started_at: null,
    completed_at: null,
    error: null,
    result: null,
    attempts: 0,
  };
}

/**
 * Met à jour le statut d'un job
 * @param {AnalysisJob} job
 * @param {string} status
 * @param {object} [extra]
 * @returns {AnalysisJob}
 */
export function updateJobStatus(job, status, extra = {}) {
  const now = new Date().toISOString();
  const updated = { ...job, status, ...extra };
  if (status === JOB_STATUS_RUNNING)   updated.started_at   = now;
  if (status === JOB_STATUS_COMPLETED) updated.completed_at = now;
  if (status === JOB_STATUS_FAILED)    updated.completed_at = now;
  return updated;
}

/**
 * Sérialise un job pour Redis
 * @param {AnalysisJob} job
 * @returns {string}
 */
export function serializeJob(job) {
  const safe = { ...job };
  if (safe.github_token) safe.github_token = '[REDACTED]';
  return JSON.stringify(safe);
}

export default { createJob, updateJobStatus, serializeJob,
  JOB_STATUS_PENDING, JOB_STATUS_RUNNING, JOB_STATUS_COMPLETED, JOB_STATUS_FAILED };
