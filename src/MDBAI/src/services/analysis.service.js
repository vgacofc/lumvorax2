/**
 * MDBAI — Service d'analyse forensique (moteur principal)
 * Conforme STANDARD_NAMES_MDBAI.md Section 5 (detectLanguage, runTests...)
 * Conforme prompt.txt Règle #1 : traçabilité bit-level active à 100%
 */

import { execSync, execFileSync } from 'child_process';
import { existsSync, readdirSync, readFileSync } from 'fs';
import { join } from 'path';
import logger from '../utils/logger.js';
import { ForensicBridge } from '../utils/forensic.js';
import {
  LANG_NODEJS, LANG_PYTHON, LANG_RUST, LANG_GO, LANG_C_CPP, LANG_UNKNOWN,
  VULN_SEVERITY_LOW, VULN_SEVERITY_MEDIUM, VULN_SEVERITY_HIGH, VULN_SEVERITY_CRITICAL,
  createEmptyResult, calculateScore,
} from '../models/result.model.js';
import { MdbaiError, ERR_LANG_DETECT, ERR_DEPS_INSTALL, ERR_ANALYSIS_TIMEOUT } from '../utils/errors.js';

export class AnalysisService {
  constructor(jobId) {
    this.jobId = jobId;
    this.forensic = new ForensicBridge(jobId);
    this.log = logger.child ? logger.child({ jobId }) : logger;
  }

  /**
   * Analyse complète d'un dépôt cloné
   * Point d'entrée principal appelé par le worker
   * @param {string} repoDir - Répertoire du clone
   * @param {function} onProgress - Callback progression (0-100, message)
   * @returns {Promise<AnalysisResult>}
   */
  async analyze(repoDir, onProgress = () => {}) {
    const result = createEmptyResult();
    const startMs = Date.now();

    this.log.info('[ANALYSIS] Début analyse forensique', { repoDir });
    this.forensic.init();

    try {
      onProgress(5, 'Détection langage...');
      const lang = await this.detectLanguage(repoDir);
      result.repo.language = lang;
      this.log.info(`[ANALYSIS] Langage détecté: ${lang}`);

      onProgress(15, `Installation dépendances ${lang}...`);
      const depsResult = await this.installDependencies(repoDir, lang);
      this.log.info('[ANALYSIS] Dépendances installées', { exitCode: depsResult.exit_code });

      onProgress(30, 'Exécution tests avec instrumentation LumVorax...');
      const execCmd = this._getTestCommand(lang);
      const forensicData = this.forensic.runAnalysis(repoDir, execCmd);

      result.execution.stdout   = forensicData.stdout;
      result.execution.stderr   = forensicData.stderr;
      result.execution.exit_code = forensicData.exit_code;
      result.forensic.lum_snapshots = forensicData.lum_snapshots;
      result.forensic.memory_leaks  = forensicData.memory_leaks;

      onProgress(55, 'Détection erreurs et crashes...');
      result.analysis.errors = this.detectErrors(forensicData.stdout, forensicData.stderr);

      onProgress(65, 'Analyse fuites mémoire...');
      result.forensic.memory_leaks = [
        ...result.forensic.memory_leaks,
        ...this.detectMemoryLeaks(forensicData),
      ].filter((v, i, arr) => arr.findIndex(x => x.raw === v.raw) === i);

      onProgress(75, 'Scan vulnérabilités...');
      result.analysis.vulnerabilities = await this.scanVulnerabilities(repoDir, lang);

      onProgress(85, 'Analyse performance...');
      result.forensic.performance = this.analyzePerformance(forensicData);

      onProgress(92, 'Calcul score qualité...');
      result.analysis.score = calculateScore(result);

      result.execution.duration_ms = Date.now() - startMs;
      this.log.info('[ANALYSIS] Analyse terminée ✅', {
        score: result.analysis.score,
        errors: result.analysis.errors.length,
        vulns: result.analysis.vulnerabilities.length,
        leaks: result.forensic.memory_leaks.length,
        duration_ms: result.execution.duration_ms,
      });

    } catch (e) {
      result.analysis.errors.push({
        type: 'system', severity: 'critical',
        message: e.message, code: e.code || 'UNKNOWN',
      });
      this.log.error('[ANALYSIS] Erreur analyse', { error: e.message });
    } finally {
      this.forensic.destroy();
    }

    return result;
  }

  /**
   * Détecte le langage principal du dépôt
   * @param {string} dir
   * @returns {Promise<string>}
   */
  async detectLanguage(dir) {
    const files = this._listFiles(dir, 2);

    if (files.includes('package.json'))  return LANG_NODEJS;
    if (files.includes('requirements.txt') || files.includes('setup.py') ||
        files.includes('pyproject.toml')) return LANG_PYTHON;
    if (files.includes('Cargo.toml'))    return LANG_RUST;
    if (files.includes('go.mod'))        return LANG_GO;
    if (files.includes('Makefile') ||
        files.some(f => f.endsWith('.c') || f.endsWith('.cpp') || f.endsWith('.cc')))
      return LANG_C_CPP;

    const ext = this._countExtensions(dir);
    if (ext['.js'] || ext['.ts'])  return LANG_NODEJS;
    if (ext['.py'])                return LANG_PYTHON;
    if (ext['.rs'])                return LANG_RUST;
    if (ext['.go'])                return LANG_GO;
    if (ext['.c'] || ext['.cpp']) return LANG_C_CPP;

    return LANG_UNKNOWN;
  }

  /**
   * Installe les dépendances du projet
   * CF-005: dépendances installées
   */
  async installDependencies(dir, lang) {
    const cmds = {
      [LANG_NODEJS]:  ['npm', ['install', '--no-audit', '--no-fund', '--prefer-offline']],
      [LANG_PYTHON]:  ['pip', ['install', '-r', 'requirements.txt', '-q']],
      [LANG_RUST]:    ['cargo', ['fetch']],
      [LANG_GO]:      ['go', ['mod', 'download']],
      [LANG_C_CPP]:   ['make', ['--dry-run']],
    };
    const cmd = cmds[lang];
    if (!cmd) return { exit_code: 0, stdout: '', stderr: '' };

    const [bin, args] = cmd;
    let stdout = '', stderr = '', exit_code = 0;
    try {
      stdout = execFileSync(bin, args, {
        cwd: dir, timeout: 120000, stdio: 'pipe',
      }).toString();
    } catch (e) {
      stderr = e.stderr?.toString() || e.message;
      exit_code = e.status || 1;
      this.log.warn(`[ANALYSIS] Install dépendances exit=${exit_code}`);
    }
    return { exit_code, stdout, stderr };
  }

  /**
   * Retourne la commande de test selon le langage
   */
  _getTestCommand(lang) {
    const cmds = {
      [LANG_NODEJS]: 'npm test 2>&1 || true',
      [LANG_PYTHON]: 'python -m pytest -v 2>&1 || python -m unittest discover 2>&1 || true',
      [LANG_RUST]:   'cargo test 2>&1 || true',
      [LANG_GO]:     'go test ./... 2>&1 || true',
      [LANG_C_CPP]:  'make test 2>&1 || true',
      [LANG_UNKNOWN]: 'ls -la 2>&1 || true',
    };
    return cmds[lang] || cmds[LANG_UNKNOWN];
  }

  /**
   * Détecte les erreurs dans stdout/stderr
   * CF-009: > 0 erreurs trouvées
   */
  detectErrors(stdout = '', stderr = '') {
    const combined = `${stdout}\n${stderr}`;
    const errors = [];

    const patterns = [
      { re: /SIGSEGV|Segmentation fault/gi, severity: 'critical', type: 'segfault' },
      { re: /SIGABRT|Aborted/gi, severity: 'critical', type: 'abort' },
      { re: /\bthread '.*?' panicked at (.+)/gi, severity: 'critical', type: 'rust_panic' },
      { re: /\bpanic:\s+(.+)/gi, severity: 'critical', type: 'panic' },
      { re: /FATAL:\s+(.+)/gi, severity: 'critical', type: 'fatal' },
      { re: /SyntaxError:\s+(.+)/gi, severity: 'critical', type: 'syntax_error' },
      { re: /TypeError:\s+(.+)/gi, severity: 'high', type: 'type_error' },
      { re: /ReferenceError:\s+(.+)/gi, severity: 'high', type: 'reference_error' },
      { re: /(?<!Type)(?<!Reference)(?<!Syntax)(?<!Range)(?<!URI)Error:\s+(.+)/gi, severity: 'high', type: 'runtime_error' },
      { re: /AssertionError:\s+(.+)/gi, severity: 'medium', type: 'assertion' },
      { re: /FAIL\s+(.+)\n.*●/gm, severity: 'medium', type: 'test_failure' },
      { re: /FAILED\s+([\w./]+\.py::\w+)/g, severity: 'medium', type: 'test_failure' },
      { re: /warning\[(.+)\]:\s+(.+)/gi, severity: 'low', type: 'compiler_warning' },
    ];

    for (const { re, severity, type } of patterns) {
      let m;
      while ((m = re.exec(combined)) !== null) {
        const msg = (m[1] || m[0]).trim().slice(0, 300);
        if (msg && !errors.find(e => e.message === msg)) {
          errors.push({ type, severity, message: msg, raw: m[0].slice(0, 500) });
        }
        if (errors.length >= 50) break;
      }
    }

    return errors;
  }

  /**
   * Détecte les fuites mémoire depuis les données forensic
   */
  detectMemoryLeaks(forensicData = {}) {
    const leaks = [...(forensicData.memory_leaks || [])];
    const combined = `${forensicData.stdout || ''}\n${forensicData.stderr || ''}`;

    const patterns = [
      /(\d+) bytes? (?:in|of) (\d+) block.*(?:definitely lost|indirectly lost)/gi,
      /LEAK SUMMARY.*definitely lost: ([\d,]+) bytes?/gi,
      /\[MDBAI-LEAK\] addr=0x[\da-f]+ size=(\d+)/gi,
    ];

    for (const re of patterns) {
      let m;
      while ((m = re.exec(combined)) !== null) {
        leaks.push({
          tool: 'auto-detected', size_bytes: parseInt(m[1]?.replace(/,/g, '') || '0', 10),
          raw: m[0].slice(0, 200),
        });
      }
    }
    return leaks;
  }

  /**
   * Scan vulnérabilités (npm audit, pip-audit, cargo audit)
   */
  async scanVulnerabilities(dir, lang) {
    const vulns = [];
    let output = '';
    try {
      if (lang === LANG_NODEJS && existsSync(join(dir, 'package.json'))) {
        output = execSync('npm audit --json 2>/dev/null || true', {
          cwd: dir, timeout: 60000, maxBuffer: 5 * 1024 * 1024,
        }).toString();
        const audit = JSON.parse(output);
        const vulnsMap = audit.vulnerabilities || {};
        for (const [name, data] of Object.entries(vulnsMap)) {
          const sev = data.severity || 'low';
          vulns.push({
            package: name, version: data.range || 'unknown',
            severity: sev === 'critical' ? VULN_SEVERITY_CRITICAL :
                     sev === 'high' ? VULN_SEVERITY_HIGH :
                     sev === 'moderate' ? VULN_SEVERITY_MEDIUM : VULN_SEVERITY_LOW,
            title: data.via?.[0]?.title || name,
            cve: data.via?.[0]?.url || null,
          });
        }
      }
      if (lang === LANG_PYTHON) {
        try {
          output = execSync('pip-audit --format=json 2>/dev/null || true', {
            cwd: dir, timeout: 60000,
          }).toString();
          const pAudit = JSON.parse(output);
          for (const item of (pAudit.dependencies || [])) {
            for (const vuln of (item.vulns || [])) {
              vulns.push({
                package: item.name, version: item.version,
                severity: VULN_SEVERITY_MEDIUM, title: vuln.id, cve: vuln.id,
              });
            }
          }
        } catch {}
      }
    } catch (e) {
      this.log.warn('[ANALYSIS] Scan vulnérabilités non disponible', { error: e.message });
    }
    return vulns;
  }

  /**
   * Analyse les métriques de performance depuis les données forensic
   */
  analyzePerformance(forensicData = {}) {
    return {
      cpu_percent: forensicData.cpu_percent || 0,
      memory_mb: Math.round((forensicData.memory_bytes || 0) / 1024 / 1024),
      io_read_mb: 0,
      io_write_mb: 0,
      duration_ms: forensicData.duration_ms || 0,
      lum_snapshots_count: (forensicData.lum_snapshots || []).length,
    };
  }

  _listFiles(dir, depth = 1) {
    try {
      const entries = [];
      const scan = (d, lvl) => {
        if (lvl > depth) return;
        for (const f of readdirSync(d)) {
          if (f.startsWith('.') || f === 'node_modules') continue;
          entries.push(f);
          try { scan(join(d, f), lvl + 1); } catch {}
        }
      };
      scan(dir, 0);
      return entries;
    } catch { return []; }
  }

  _countExtensions(dir) {
    const counts = {};
    const scan = (d, lvl) => {
      if (lvl > 2) return;
      try {
        for (const f of readdirSync(d)) {
          if (f.startsWith('.') || f === 'node_modules') continue;
          const ext = f.includes('.') ? `.${f.split('.').pop()}` : '';
          if (ext) counts[ext] = (counts[ext] || 0) + 1;
          try { scan(join(d, f), lvl + 1); } catch {}
        }
      } catch {}
    };
    scan(dir, 0);
    return counts;
  }
}

export default AnalysisService;
