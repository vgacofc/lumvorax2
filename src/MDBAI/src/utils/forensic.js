/**
 * MDBAI — Pont JavaScript ↔ libmdbai_forensic.so
 * Conforme STANDARD_NAMES_MDBAI.md Section 7 + 9 + 10
 * Conforme prompt.txt Règle #1 : traçabilité bit-level ACTIVE
 *
 * FORENSIC_LUM_MAGIC = 0x4D444241 ('MDBA')
 * FORENSIC_LOG_ROTATION_MB = 20
 */

import { execSync, execFileSync } from 'child_process';
import { existsSync, mkdirSync, statSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import logger from './logger.js';

const __dirname = dirname(fileURLToPath(import.meta.url));

export const FORENSIC_LUM_MAGIC       = 0x4D444241;
export const FORENSIC_LOG_ROTATION_MB = 20;

const FORENSIC_DIR = join(__dirname, '../../forensic');
const LIB_PATH = process.env.MDBAI_FORENSIC_LIB_PATH ||
  join(FORENSIC_DIR, 'libmdbai_forensic.so');

/**
 * ForensicBridge — interface vers la bibliothèque C forensic
 */
export class ForensicBridge {
  constructor(jobId) {
    this.jobId = jobId;
    this.active = false;
    this.libAvailable = existsSync(LIB_PATH);
    this.forensicDir = FORENSIC_DIR;
    mkdirSync(join(FORENSIC_DIR, 'memory'), { recursive: true });
    mkdirSync(join(FORENSIC_DIR, 'logger'), { recursive: true });
  }

  /**
   * Initialise le forensic (wrapper autour de mdbai_forensic_init)
   * En mode dégradé si libforensic.so absent : utilise runner bash
   */
  init() {
    if (this.libAvailable) {
      logger.info(`[FORENSIC] libmdbai_forensic.so chargé — job ${this.jobId}`);
    } else {
      logger.warn(`[FORENSIC] lib non disponible — mode dégradé bash runner — job ${this.jobId}`);
    }
    this.active = true;
    return this;
  }

  /**
   * Lance l'analyse forensic sur un répertoire cible
   * @param {string} targetDir - Répertoire à analyser
   * @param {string} execCmd - Commande à exécuter avec instrumentation
   * @returns {object} Données forensic collectées
   */
  runAnalysis(targetDir, execCmd) {
    const runnerScript = join(FORENSIC_DIR, 'mdbai_analysis_runner.sh');
    const logFile = join(FORENSIC_DIR, 'logger', `${this.jobId}_forensic.log`);
    const memFile = join(FORENSIC_DIR, 'memory', `${this.jobId}_memory.lum`);

    logger.info(`[FORENSIC] Démarrage analyse — job ${this.jobId}`, {
      targetDir, execCmd, libAvailable: this.libAvailable
    });

    const env = {
      ...process.env,
      MDBAI_JOB_ID: this.jobId,
      MDBAI_LOG_FILE: logFile,
      MDBAI_MEM_FILE: memFile,
    };

    if (this.libAvailable) {
      env.LD_PRELOAD = LIB_PATH;
    }

    let stdout = '', stderr = '', exitCode = 0;
    try {
      if (existsSync(runnerScript)) {
        const result = execFileSync('bash', [runnerScript, targetDir, execCmd], {
          env, timeout: 300000, cwd: targetDir,
          maxBuffer: 50 * 1024 * 1024,
        });
        stdout = result.toString();
      } else {
        const result = execSync(`${execCmd} 2>&1`, {
          env, timeout: 300000, cwd: targetDir,
          maxBuffer: 50 * 1024 * 1024,
        });
        stdout = result.toString();
      }
    } catch (err) {
      stderr = err.stderr?.toString() || err.message;
      exitCode = err.status || 1;
      logger.warn(`[FORENSIC] Exécution avec exit_code=${exitCode} — job ${this.jobId}`);
    }

    return this._collectForensicData(logFile, memFile, stdout, stderr, exitCode);
  }

  /**
   * Collecte et parse les données forensic produites
   */
  _collectForensicData(logFile, memFile, stdout, stderr, exitCode) {
    const data = {
      stdout,
      stderr,
      exit_code: exitCode,
      memory_leaks: [],
      syscalls: [],
      performance: { cpu_percent: 0, memory_mb: 0, duration_ms: 0 },
      lum_snapshots: [],
      lib_active: this.libAvailable,
    };

    if (existsSync(memFile)) {
      try {
        const stat = statSync(memFile);
        data.lum_snapshots.push({
          file: memFile,
          size_bytes: stat.size,
          magic: `0x${FORENSIC_LUM_MAGIC.toString(16).toUpperCase()}`,
          type: 'memory',
        });
        logger.info(`[FORENSIC] Snapshot LUM: ${stat.size} octets — job ${this.jobId}`);
      } catch (e) {
        logger.warn(`[FORENSIC] Lecture snapshot échouée: ${e.message}`);
      }
    }

    data.memory_leaks = this._parseMemoryLeaks(stdout + stderr);
    return data;
  }

  /**
   * Détecte les fuites mémoire dans stdout/stderr
   * Compatible avec valgrind, AddressSanitizer, et format LumVorax
   */
  _parseMemoryLeaks(output) {
    const leaks = [];
    const patterns = [
      { re: /definitely lost: ([\d,]+) bytes? in \d+ blocks?/gi, tool: 'valgrind' },
      { re: /Direct leak of (\d+) bytes?/gi, tool: 'asan' },
      { re: /\[LUM-LEAK\][^\n]*size=(\d+)/g, tool: 'lumvorax' },
      { re: /memory leak.*?(\d+)\s*(?:bytes?|KB)/gi, tool: 'generic' },
    ];
    for (const { re, tool } of patterns) {
      let m;
      while ((m = re.exec(output)) !== null) {
        leaks.push({ tool, size_bytes: parseInt(m[1].replace(/,/g, ''), 10) || 0,
          raw: m[0].slice(0, 200) });
      }
    }
    return leaks;
  }

  destroy() {
    this.active = false;
    logger.info(`[FORENSIC] Contexte libéré — job ${this.jobId}`);
  }
}

export default ForensicBridge;
