/**
 * MDBAI — Modèle Résultat d'analyse
 * Conforme STANDARD_NAMES_MDBAI.md Section 4
 * Interface: AnalysisResult
 */

export const VULN_SEVERITY_LOW      = 'low';
export const VULN_SEVERITY_MEDIUM   = 'medium';
export const VULN_SEVERITY_HIGH     = 'high';
export const VULN_SEVERITY_CRITICAL = 'critical';

export const LANG_NODEJS  = 'nodejs';
export const LANG_PYTHON  = 'python';
export const LANG_RUST    = 'rust';
export const LANG_GO      = 'go';
export const LANG_C_CPP   = 'c_cpp';
export const LANG_UNKNOWN = 'unknown';

/**
 * Crée un résultat vide (squelette)
 * @returns {AnalysisResult}
 */
export function createEmptyResult() {
  return {
    repo: {
      name: '',
      owner: '',
      url: '',
      language: LANG_UNKNOWN,
      framework: null,
      branch: 'main',
      commit: null,
    },
    execution: {
      duration_ms: 0,
      exit_code: -1,
      stdout: '',
      stderr: '',
      timed_out: false,
    },
    forensic: {
      memory_leaks: [],
      syscalls: [],
      performance: {
        cpu_percent: 0,
        memory_mb: 0,
        io_read_mb: 0,
        io_write_mb: 0,
        duration_ms: 0,
      },
      lum_snapshots: [],
    },
    analysis: {
      errors: [],
      warnings: [],
      vulnerabilities: [],
      suggestions: [],
      score: 100,
    },
    report: {
      markdown: '',
      pr_url: null,
      pr_number: null,
      branch: null,
      commit_sha: null,
    },
  };
}

/**
 * Calcule le score qualité global (0–100)
 * @param {AnalysisResult} result
 * @returns {number}
 */
export function calculateScore(result) {
  let score = 100;
  const { errors = [], warnings = [], vulnerabilities = [] } = result.analysis;
  score -= errors.filter(e => e.severity === 'critical').length * 20;
  score -= errors.filter(e => e.severity === 'high').length * 10;
  score -= errors.filter(e => e.severity === 'medium').length * 5;
  score -= warnings.length * 2;
  score -= vulnerabilities.filter(v => v.severity === VULN_SEVERITY_CRITICAL).length * 25;
  score -= vulnerabilities.filter(v => v.severity === VULN_SEVERITY_HIGH).length * 15;
  score -= vulnerabilities.filter(v => v.severity === VULN_SEVERITY_MEDIUM).length * 8;
  score -= (result.forensic.memory_leaks || []).length * 5;
  return Math.max(0, Math.min(100, score));
}

export default {
  createEmptyResult, calculateScore,
  VULN_SEVERITY_LOW, VULN_SEVERITY_MEDIUM, VULN_SEVERITY_HIGH, VULN_SEVERITY_CRITICAL,
  LANG_NODEJS, LANG_PYTHON, LANG_RUST, LANG_GO, LANG_C_CPP, LANG_UNKNOWN,
};
