/**
 * MDBAI — Validation des entrées avec Joi
 * Conforme STANDARD_NAMES_MDBAI.md Section 5 + Section 12 codes erreur
 */

import Joi from 'joi';

const GITHUB_URL_PATTERN = /^https:\/\/github\.com\/[\w.-]+\/[\w.-]+(\.git)?$/;

export function validateRepoUrl(url) {
  if (!url || typeof url !== 'string') {
    return { valid: false, error: 'ERR_REPO_CLONE: URL manquante' };
  }
  if (!GITHUB_URL_PATTERN.test(url.trim())) {
    return { valid: false, error: 'ERR_REPO_CLONE: URL GitHub invalide. Format: https://github.com/owner/repo' };
  }
  return { valid: true, value: url.trim().replace(/\.git$/, '') };
}

export function validateJobId(jobId) {
  const schema = Joi.string().uuid({ version: 'uuidv4' }).required();
  const { error } = schema.validate(jobId);
  if (error) return { valid: false, error: 'ERR_JOB_ID: jobId doit être un UUID v4 valide' };
  return { valid: true, value: jobId };
}

export function parseGitHubUrl(url) {
  const match = url.match(/github\.com\/([^/]+)\/([^/.\s]+)/);
  if (!match) return null;
  return { owner: match[1], repo: match[2].replace(/\.git$/, '') };
}

export const analyzeRequestSchema = Joi.object({
  repo_url: Joi.string().pattern(GITHUB_URL_PATTERN).required(),
  user_id: Joi.string().required(),
  github_token: Joi.string().optional(),
  branch: Joi.string().default('main'),
});

export function validateAnalyzeRequest(body) {
  const { error, value } = analyzeRequestSchema.validate(body, { abortEarly: false });
  if (error) {
    return { valid: false, errors: error.details.map(d => d.message) };
  }
  return { valid: true, value };
}
