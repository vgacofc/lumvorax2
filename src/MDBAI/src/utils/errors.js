/**
 * MDBAI — Erreurs custom
 * Conforme STANDARD_NAMES_MDBAI.md Section 12
 */

export class MdbaiError extends Error {
  constructor(code, message, details = null) {
    super(message);
    this.name = 'MdbaiError';
    this.code = code;
    this.details = details;
    this.timestamp = new Date().toISOString();
  }
  toJSON() {
    return { name: this.name, code: this.code, message: this.message,
      details: this.details, timestamp: this.timestamp };
  }
}

export const ERR_TELEGRAM_CONNECT  = 'MDBAI_ERR_001';
export const ERR_GITHUB_AUTH       = 'MDBAI_ERR_002';
export const ERR_REDIS_CONNECT     = 'MDBAI_ERR_003';
export const ERR_REPO_CLONE        = 'MDBAI_ERR_004';
export const ERR_LANG_DETECT       = 'MDBAI_ERR_005';
export const ERR_DEPS_INSTALL      = 'MDBAI_ERR_006';
export const ERR_FORENSIC_INIT     = 'MDBAI_ERR_007';
export const ERR_ANALYSIS_TIMEOUT  = 'MDBAI_ERR_008';
export const ERR_REPORT_GEN        = 'MDBAI_ERR_009';
export const ERR_PR_CREATE         = 'MDBAI_ERR_010';

export function isMdbaiError(err) { return err instanceof MdbaiError; }

export default MdbaiError;
