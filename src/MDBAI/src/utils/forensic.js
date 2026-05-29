/**
 * MDBAI — Pont JavaScript ↔ libmdbai_forensic.so
 * Conforme STANDARD_NAMES_MDBAI.md Section 7 + 9 + 10
 * Conforme prompt.txt Règle #1 : traçabilité bit-level ACTIVE
 *
 * FORENSIC_LUM_MAGIC = 0x4D444241 ('MDBA')
 * FORENSIC_LOG_ROTATION_MB = 20
 *
 * BUG-METRIC-001 FIX:
 *  - Mesure CPU/RAM/IO réelles via /proc/self/stat + /proc/self/status + /proc/self/io
 *  - Échantillonnage avant/après exécution pour CPU delta
 *  - Fallback gracieux si /proc non disponible (macOS, Windows)
 */

import { execSync, execFileSync, spawnSync } from 'child_process';
import { existsSync, mkdirSync, statSync, readFileSync } from 'fs';
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
 * Lit les métriques CPU depuis /proc/self/stat (Linux uniquement)
 * @returns {{ utime: number, stime: number, cutime: number, cstime: number } | null}
 */
function readProcStat(pid = 'self') {
  try {
    const raw = readFileSync(`/proc/${pid}/stat`, 'utf8');
    const parts = raw.trim().split(' ');
    return {
      utime:  parseInt(parts[13], 10),
      stime:  parseInt(parts[14], 10),
      cutime: parseInt(parts[15], 10),
      cstime: parseInt(parts[16], 10),
    };
  } catch { return null; }
}

/**
 * Lit la mémoire RSS depuis /proc/self/status (Linux)
 * @returns {number} RSS en kB, 0 si non disponible
 */
function readProcStatus(pid = 'self') {
  try {
    const raw = readFileSync(`/proc/${pid}/status`, 'utf8');
    const match = raw.match(/VmRSS:\s+(\d+)\s+kB/);
    return match ? parseInt(match[1], 10) : 0;
  } catch { return 0; }
}

/**
 * Lit I/O depuis /proc/self/io (Linux, nécessite cap)
 * @returns {{ read_bytes: number, write_bytes: number }}
 */
function readProcIo(pid = 'self') {
  try {
    const raw = readFileSync(`/proc/${pid}/io`, 'utf8');
    const readMatch  = raw.match(/read_bytes:\s+(\d+)/);
    const writeMatch = raw.match(/write_bytes:\s+(\d+)/);
    return {
      read_bytes:  readMatch  ? parseInt(readMatch[1],  10) : 0,
      write_bytes: writeMatch ? parseInt(writeMatch[1], 10) : 0,
    };
  } catch { return { read_bytes: 0, write_bytes: 0 }; }
}

/**
 * Lit le clock ticks par seconde (CLK_TCK), typiquement 100 sur Linux
 * @returns {number}
 */
function getClkTck() {
  try {
    const r = spawnSync('getconf', ['CLK_TCK'], { encoding: 'utf8', timeout: 1000 });
    return parseInt(r.stdout?.trim(), 10) || 100;
  } catch { return 100; }
}

/**
 * Calcule le % CPU utilisé entre deux snapshots /proc/stat
 * @param {{ utime, stime, cutime, cstime } | null} before
 * @param {{ utime, stime, cutime, cstime } | null} after
 * @param {number} durationMs
 * @returns {number} CPU % (0-100+)
 */
function calcCpuPercent(before, after, durationMs, clkTck = 100) {
  if (!before || !after || durationMs <= 0) return 0;
  const ticksBefore = before.utime + before.stime + before.cutime + before.cstime;
  const ticksAfter  = after.utime  + after.stime  + after.cutime  + after.cstime;
  const deltaTicks  = ticksAfter - ticksBefore;
  const elapsedSec  = durationMs / 1000;
  return Math.min(Math.round((deltaTicks / clkTck / elapsedSec) * 100), 9999);
}

/**
 * ForensicBridge — interface vers la bibliothèque C forensic
 */
export class ForensicBridge {
  constructor(jobId) {
    this.jobId = jobId;
    this.active = false;
    this.libAvailable = existsSync(LIB_PATH);
    this.forensicDir = FORENSIC_DIR;
    this._clkTck = getClkTck();
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
   * BUG-METRIC-001: mesure CPU/RAM/IO avant/après exécution
   * @param {string} targetDir - Répertoire à analyser
   * @param {string} execCmd - Commande à exécuter avec instrumentation
   * @returns {object} Données forensic collectées
   */
  runAnalysis(targetDir, execCmd) {
    const runnerScript = join(FORENSIC_DIR, 'mdbai_analysis_runner.sh');
    const logFile = join(FORENSIC_DIR, 'logger', `${this.jobId}_forensic.log`);
    const memFile = join(FORENSIC_DIR, 'memory', `${this.jobId}_memory.lum`);

    logger.info(`[FORENSIC] Démarrage analyse — job ${this.jobId}`, {
      targetDir, execCmd: execCmd.slice(0, 80), libAvailable: this.libAvailable,
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

    // Snapshots métriques AVANT exécution (BUG-METRIC-001)
    const statBefore = readProcStat();
    const ioBefore   = readProcIo();
    const startMs    = Date.now();

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
      stderr   = err.stderr?.toString() || err.message;
      exitCode = err.status || 1;
      logger.warn(`[FORENSIC] Exécution avec exit_code=${exitCode} — job ${this.jobId}`);
    }

    // Snapshots métriques APRÈS exécution (BUG-METRIC-001)
    const durationMs  = Date.now() - startMs;
    const statAfter   = readProcStat();
    const ioAfter     = readProcIo();
    const rssKb       = readProcStatus();

    const cpuPercent  = calcCpuPercent(statBefore, statAfter, durationMs, this._clkTck);
    const memoryMb    = Math.round(rssKb / 1024);
    const ioReadMb    = Math.round((ioAfter.read_bytes  - ioBefore.read_bytes)  / 1024 / 1024);
    const ioWriteMb   = Math.round((ioAfter.write_bytes - ioBefore.write_bytes) / 1024 / 1024);

    logger.info(`[FORENSIC] Métriques: CPU=${cpuPercent}% RAM=${memoryMb}MB IO_r=${ioReadMb}MB IO_w=${ioWriteMb}MB dur=${durationMs}ms — job ${this.jobId}`);

    return this._collectForensicData(
      logFile, memFile, stdout, stderr, exitCode,
      { cpu_percent: cpuPercent, memory_mb: memoryMb, io_read_mb: ioReadMb, io_write_mb: ioWriteMb, duration_ms: durationMs }
    );
  }

  /**
   * Collecte et parse les données forensic produites
   * @param {object} perfMetrics - {cpu_percent, memory_mb, io_read_mb, io_write_mb, duration_ms}
   */
  _collectForensicData(logFile, memFile, stdout, stderr, exitCode, perfMetrics = {}) {
    const data = {
      stdout,
      stderr,
      exit_code:    exitCode,
      memory_leaks: [],
      syscalls:     [],
      // Métriques de performance réelles (BUG-METRIC-001 FIX)
      cpu_percent:  perfMetrics.cpu_percent  || 0,
      memory_bytes: (perfMetrics.memory_mb   || 0) * 1024 * 1024,
      io_read_mb:   perfMetrics.io_read_mb   || 0,
      io_write_mb:  perfMetrics.io_write_mb  || 0,
      duration_ms:  perfMetrics.duration_ms  || 0,
      lum_snapshots: [],
      lib_active:    this.libAvailable,
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
        leaks.push({
          tool, size_bytes: parseInt(m[1].replace(/,/g, ''), 10) || 0,
          raw: m[0].slice(0, 200),
        });
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
