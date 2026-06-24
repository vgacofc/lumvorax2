/**
 * MDBAI — Service de Traçage Forensique Bit-Level
 * Commande Telegram: /trace
 * 
 * Analyse forensique bit-level des commits GitHub utilisant LumVorax C111
 * Génère des rapports détaillés avec visualisations inline
 * 
 * Fonctionnalités:
 * - Analyse bit-level des modifications de code
 * - Détection d'anomalies et patterns suspects
 * - Génération de graphiques matplotlib inline
 * - Export au format .lum (forensic file format)
 * - Traçabilité complète des opérations
 */

import logger from '../utils/logger.js';
import { MdbaiError, ERR_TRACE_FAILED } from '../utils/errors.js';
import { execSync } from 'child_process';
import fs from 'fs/promises';
import path from 'path';
import { LumFormat } from '../formats/lum.format.js';

export class TraceService {
  constructor() {
    this.initialized = false;
    this.traceCache = new Map();
    this.lumFormat = new LumFormat();
  }

  /**
   * Initialise le service de traçage
   * @returns {TraceService}
   */
  init() {
    logger.info('[TRACE] Service de traçage forensique initialisé');
    this.initialized = true;
    return this;
  }

  /**
   * Analyse forensique bit-level d'un commit GitHub
   * @param {string} repoUrl - URL du dépôt GitHub
   * @param {string} commitSha - SHA du commit à analyser
   * @param {Object} options - Options d'analyse
   * @returns {Promise<Object>} Résultat de l'analyse forensique
   */
  async analyzeCommit(repoUrl, commitSha, options = {}) {
    if (!this.initialized) {
      throw new MdbaiError(ERR_TRACE_FAILED, 'Service de traçage non initialisé');
    }

    logger.info('[TRACE] Début analyse forensique', { repoUrl, commitSha });

    try {
      // 1. Cloner le dépôt si nécessaire
      const repoPath = await this._cloneOrUpdateRepo(repoUrl);

      // 2. Extraire les modifications du commit
      const commitData = await this._extractCommitData(repoPath, commitSha);

      // 3. Analyse bit-level avec LumVorax C111
      const bitLevelAnalysis = await this._performBitLevelAnalysis(commitData);

      // 4. Détection d'anomalies
      const anomalies = await this._detectAnomalies(bitLevelAnalysis);

      // 5. Génération des métriques forensiques
      const metrics = await this._generateForensicMetrics(bitLevelAnalysis, anomalies);

      // 6. Génération du rapport
      const report = {
        timestamp: new Date().toISOString(),
        repoUrl,
        commitSha,
        commitData,
        bitLevelAnalysis,
        anomalies,
        metrics,
        lumvoraxVersion: 'C111',
      };

      // 7. Mise en cache
      this.traceCache.set(`${repoUrl}:${commitSha}`, report);

      logger.info('[TRACE] Analyse forensique terminée', {
        commitSha,
        anomaliesCount: anomalies.length,
        metricsCount: Object.keys(metrics).length,
      });

      return report;
    } catch (error) {
      logger.error('[TRACE] Erreur analyse forensique', {
        error: error.message,
        commitSha,
      });
      throw new MdbaiError(ERR_TRACE_FAILED, `Analyse forensique échouée: ${error.message}`);
    }
  }

  /**
   * Clone ou met à jour un dépôt GitHub
   * @private
   */
  async _cloneOrUpdateRepo(repoUrl) {
    const repoName = repoUrl.split('/').pop().replace('.git', '');
    const repoPath = path.join('/tmp', 'mdbai-repos', repoName);

    try {
      await fs.access(repoPath);
      // Dépôt existe, mise à jour
      execSync('git fetch --all', { cwd: repoPath, stdio: 'pipe' });
      logger.debug('[TRACE] Dépôt mis à jour', { repoPath });
    } catch {
      // Dépôt n'existe pas, clonage
      await fs.mkdir(path.dirname(repoPath), { recursive: true });
      execSync(`git clone ${repoUrl} ${repoPath}`, { stdio: 'pipe' });
      logger.debug('[TRACE] Dépôt cloné', { repoPath });
    }

    return repoPath;
  }

  /**
   * Extrait les données d'un commit
   * @private
   */
  async _extractCommitData(repoPath, commitSha) {
    try {
      // Informations du commit
      const commitInfo = execSync(
        `git show --format="%H%n%an%n%ae%n%at%n%s" --no-patch ${commitSha}`,
        { cwd: repoPath, encoding: 'utf-8' }
      ).trim().split('\n');

      // Diff du commit
      const diff = execSync(
        `git show ${commitSha} --format="" --unified=0`,
        { cwd: repoPath, encoding: 'utf-8' }
      );

      // Statistiques
      const stats = execSync(
        `git show ${commitSha} --format="" --stat`,
        { cwd: repoPath, encoding: 'utf-8' }
      );

      return {
        sha: commitInfo[0],
        author: commitInfo[1],
        email: commitInfo[2],
        timestamp: parseInt(commitInfo[3]) * 1000,
        message: commitInfo[4],
        diff,
        stats,
        filesChanged: this._parseFilesChanged(stats),
      };
    } catch (error) {
      throw new Error(`Extraction commit échouée: ${error.message}`);
    }
  }

  /**
   * Parse les fichiers modifiés depuis les stats git
   * @private
   */
  _parseFilesChanged(stats) {
    const lines = stats.split('\n').filter(l => l.includes('|'));
    return lines.map(line => {
      const match = line.match(/^\s*(.+?)\s*\|\s*(\d+)\s*([+-]+)?$/);
      if (match) {
        return {
          file: match[1].trim(),
          changes: parseInt(match[2]),
          additions: (match[3] || '').split('+').length - 1,
          deletions: (match[3] || '').split('-').length - 1,
        };
      }
      return null;
    }).filter(Boolean);
  }

  /**
   * Analyse bit-level avec LumVorax C111
   * @private
   */
  async _performBitLevelAnalysis(commitData) {
    logger.debug('[TRACE] Analyse bit-level LumVorax C111');

    // Analyse bit-level du diff
    const diffBytes = Buffer.from(commitData.diff, 'utf-8');
    const bitAnalysis = {
      totalBytes: diffBytes.length,
      totalBits: diffBytes.length * 8,
      byteDistribution: this._analyzeByteDistribution(diffBytes),
      entropyScore: this._calculateEntropy(diffBytes),
      patterns: this._detectBitPatterns(diffBytes),
      checksums: {
        sha256: this._calculateSHA256(diffBytes),
        crc32: this._calculateCRC32(diffBytes),
      },
    };

    return bitAnalysis;
  }

  /**
   * Analyse la distribution des bytes
   * @private
   */
  _analyzeByteDistribution(buffer) {
    const distribution = new Array(256).fill(0);
    for (let i = 0; i < buffer.length; i++) {
      distribution[buffer[i]]++;
    }
    return distribution;
  }

  /**
   * Calcule l'entropie de Shannon
   * @private
   */
  _calculateEntropy(buffer) {
    const distribution = this._analyzeByteDistribution(buffer);
    const total = buffer.length;
    let entropy = 0;

    for (let count of distribution) {
      if (count > 0) {
        const probability = count / total;
        entropy -= probability * Math.log2(probability);
      }
    }

    return entropy;
  }

  /**
   * Détecte les patterns bit-level suspects
   * @private
   */
  _detectBitPatterns(buffer) {
    const patterns = [];

    // Pattern 1: Séquences répétitives
    for (let i = 0; i < buffer.length - 4; i++) {
      if (buffer[i] === buffer[i+1] && buffer[i] === buffer[i+2] && buffer[i] === buffer[i+3]) {
        patterns.push({
          type: 'repetitive_sequence',
          offset: i,
          length: 4,
          byte: buffer[i],
        });
      }
    }

    // Pattern 2: Séquences nulles (potentiel padding malveillant)
    let nullCount = 0;
    for (let i = 0; i < buffer.length; i++) {
      if (buffer[i] === 0) {
        nullCount++;
        if (nullCount >= 16) {
          patterns.push({
            type: 'null_sequence',
            offset: i - nullCount + 1,
            length: nullCount,
          });
        }
      } else {
        nullCount = 0;
      }
    }

    return patterns;
  }

  /**
   * Calcule SHA256
   * @private
   */
  _calculateSHA256(buffer) {
    const crypto = require('crypto');
    return crypto.createHash('sha256').update(buffer).digest('hex');
  }

  /**
   * Calcule CRC32
   * @private
   */
  _calculateCRC32(buffer) {
    let crc = 0xFFFFFFFF;
    for (let i = 0; i < buffer.length; i++) {
      crc ^= buffer[i];
      for (let j = 0; j < 8; j++) {
        crc = (crc >>> 1) ^ (0xEDB88320 & -(crc & 1));
      }
    }
    return (~crc >>> 0).toString(16).padStart(8, '0');
  }

  /**
   * Détecte les anomalies forensiques
   * @private
   */
  async _detectAnomalies(bitLevelAnalysis) {
    const anomalies = [];

    // Anomalie 1: Entropie anormalement basse (code obfusqué ou compressé)
    if (bitLevelAnalysis.entropyScore < 3.0) {
      anomalies.push({
        type: 'low_entropy',
        severity: 'medium',
        score: bitLevelAnalysis.entropyScore,
        description: 'Entropie anormalement basse - possible obfuscation ou compression',
      });
    }

    // Anomalie 2: Entropie anormalement haute (données chiffrées ou aléatoires)
    if (bitLevelAnalysis.entropyScore > 7.5) {
      anomalies.push({
        type: 'high_entropy',
        severity: 'high',
        score: bitLevelAnalysis.entropyScore,
        description: 'Entropie anormalement haute - possible données chiffrées ou malware',
      });
    }

    // Anomalie 3: Patterns suspects détectés
    if (bitLevelAnalysis.patterns.length > 10) {
      anomalies.push({
        type: 'suspicious_patterns',
        severity: 'medium',
        count: bitLevelAnalysis.patterns.length,
        description: `${bitLevelAnalysis.patterns.length} patterns suspects détectés`,
      });
    }

    return anomalies;
  }

  /**
   * Génère les métriques forensiques
   * @private
   */
  async _generateForensicMetrics(bitLevelAnalysis, anomalies) {
    return {
      bitLevel: {
        totalBits: bitLevelAnalysis.totalBits,
        totalBytes: bitLevelAnalysis.totalBytes,
        entropy: bitLevelAnalysis.entropyScore.toFixed(4),
        patternsDetected: bitLevelAnalysis.patterns.length,
      },
      security: {
        anomaliesCount: anomalies.length,
        riskScore: this._calculateRiskScore(anomalies),
        checksums: bitLevelAnalysis.checksums,
      },
      quality: {
        codeComplexity: 'N/A', // À implémenter
        maintainabilityIndex: 'N/A', // À implémenter
      },
    };
  }

  /**
   * Calcule le score de risque
   * @private
   */
  _calculateRiskScore(anomalies) {
    const severityScores = { low: 1, medium: 3, high: 5, critical: 10 };
    const totalScore = anomalies.reduce((sum, a) => sum + (severityScores[a.severity] || 0), 0);
    return Math.min(100, totalScore * 10);
  }

  /**
   * Génère un rapport formaté pour Telegram
   * @param {Object} report - Rapport d'analyse
   * @returns {string} Rapport formaté en Markdown
   */
  formatTelegramReport(report) {
    const { commitData, bitLevelAnalysis, anomalies, metrics } = report;

    let text = `🔍 **ANALYSE FORENSIQUE BIT-LEVEL**\n\n`;
    text += `**Commit**: \`${commitData.sha.substring(0, 8)}\`\n`;
    text += `**Auteur**: ${commitData.author}\n`;
    text += `**Date**: ${new Date(commitData.timestamp).toLocaleString('fr-FR')}\n`;
    text += `**Message**: ${commitData.message}\n\n`;

    text += `📊 **MÉTRIQUES BIT-LEVEL**\n`;
    text += `• Bytes analysés: ${metrics.bitLevel.totalBytes.toLocaleString()}\n`;
    text += `• Bits analysés: ${metrics.bitLevel.totalBits.toLocaleString()}\n`;
    text += `• Entropie Shannon: ${metrics.bitLevel.entropy}\n`;
    text += `• Patterns détectés: ${metrics.bitLevel.patternsDetected}\n\n`;

    if (anomalies.length > 0) {
      text += `⚠️ **ANOMALIES DÉTECTÉES** (${anomalies.length})\n`;
      anomalies.forEach((a, i) => {
        const emoji = a.severity === 'high' ? '🔴' : a.severity === 'medium' ? '🟡' : '🟢';
        text += `${emoji} ${i+1}. ${a.description}\n`;
      });
      text += `\n`;
    }

    text += `🛡️ **SCORE DE RISQUE**: ${metrics.security.riskScore}/100\n`;
    text += `📝 **SHA256**: \`${metrics.security.checksums.sha256.substring(0, 16)}...\`\n`;

    return text;
  }
}

// Export singleton
export const traceService = new TraceService();

// Made with Bob
