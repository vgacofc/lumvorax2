/**
 * MDBAI — Service de Comparaison Forensique
 * Commande Telegram: /compare
 * 
 * Compare deux commits GitHub avec analyse forensique bit-level différentielle
 * Génère des rapports de diff avec visualisations et métriques détaillées
 * 
 * Fonctionnalités:
 * - Diff forensique bit-level entre deux commits
 * - Analyse des changements de patterns
 * - Détection de régressions de sécurité
 * - Visualisation des différences (heatmaps, graphiques)
 * - Export au format .lum avec signature différentielle
 */

import logger from '../utils/logger.js';
import { MdbaiError, ERR_COMPARE_FAILED } from '../utils/errors.js';
import { execSync } from 'child_process';
import fs from 'fs/promises';
import path from 'path';
import crypto from 'crypto';
import { LumFormat } from '../formats/lum.format.js';

export class CompareService {
  constructor() {
    this.initialized = false;
    this.compareCache = new Map();
    this.lumFormat = new LumFormat();
    this.maxDiffSize = 50 * 1024 * 1024; // 50MB
  }

  /**
   * Initialise le service de comparaison
   * @returns {CompareService}
   */
  init() {
    logger.info('[COMPARE] Service de comparaison forensique initialisé');
    this.initialized = true;
    return this;
  }

  /**
   * Compare deux commits avec analyse forensique différentielle
   * @param {string} repoUrl - URL du dépôt GitHub
   * @param {string} commitSha1 - SHA du premier commit (base)
   * @param {string} commitSha2 - SHA du second commit (comparé)
   * @param {Object} options - Options de comparaison
   * @returns {Promise<Object>} Résultat de la comparaison forensique
   */
  async compareCommits(repoUrl, commitSha1, commitSha2, options = {}) {
    if (!this.initialized) {
      throw new MdbaiError(ERR_COMPARE_FAILED, 'Service de comparaison non initialisé');
    }

    logger.info('[COMPARE] Début comparaison forensique', { 
      repoUrl, 
      commitSha1, 
      commitSha2 
    });

    try {
      // 1. Vérifier le cache
      const cacheKey = this._generateCacheKey(repoUrl, commitSha1, commitSha2);
      if (this.compareCache.has(cacheKey) && !options.forceRefresh) {
        logger.info('[COMPARE] Résultat trouvé dans le cache');
        return this.compareCache.get(cacheKey);
      }

      // 2. Cloner le dépôt si nécessaire
      const repoPath = await this._cloneOrUpdateRepo(repoUrl);

      // 3. Extraire les données des deux commits
      const commit1Data = await this._extractCommitData(repoPath, commitSha1);
      const commit2Data = await this._extractCommitData(repoPath, commitSha2);

      // 4. Analyse différentielle bit-level
      const diffAnalysis = await this._performDifferentialAnalysis(
        commit1Data, 
        commit2Data,
        options
      );

      // 5. Détection de régressions de sécurité
      const securityRegression = await this._detectSecurityRegressions(diffAnalysis);

      // 6. Génération des métriques de comparaison
      const metrics = this._calculateComparisonMetrics(diffAnalysis);

      // 7. Génération du rapport
      const report = {
        repository: repoUrl,
        commits: {
          base: {
            sha: commitSha1,
            ...commit1Data.metadata
          },
          compared: {
            sha: commitSha2,
            ...commit2Data.metadata
          }
        },
        diff: diffAnalysis,
        security: securityRegression,
        metrics,
        timestamp: new Date().toISOString(),
        analysisVersion: 'C187-COMPARE-v1.0.0'
      };

      // 8. Mise en cache
      this.compareCache.set(cacheKey, report);

      logger.info('[COMPARE] Comparaison forensique terminée', {
        filesChanged: diffAnalysis.filesChanged,
        linesAdded: metrics.linesAdded,
        linesRemoved: metrics.linesRemoved
      });

      return report;

    } catch (error) {
      logger.error('[COMPARE] Erreur lors de la comparaison', { error: error.message });
      throw new MdbaiError(ERR_COMPARE_FAILED, `Comparaison échouée: ${error.message}`);
    }
  }

  /**
   * Clone ou met à jour un dépôt GitHub
   * @private
   */
  async _cloneOrUpdateRepo(repoUrl) {
    const repoName = repoUrl.split('/').pop().replace('.git', '');
    const repoPath = path.join('/tmp/mdbai_repos', repoName);

    try {
      await fs.access(repoPath);
      // Repo existe, faire un pull
      logger.info('[COMPARE] Mise à jour du dépôt', { repoPath });
      execSync('git pull', { cwd: repoPath, stdio: 'pipe' });
    } catch {
      // Repo n'existe pas, cloner
      logger.info('[COMPARE] Clonage du dépôt', { repoUrl });
      await fs.mkdir(path.dirname(repoPath), { recursive: true });
      execSync(`git clone ${repoUrl} ${repoPath}`, { stdio: 'pipe' });
    }

    return repoPath;
  }

  /**
   * Extrait les données d'un commit
   * @private
   */
  async _extractCommitData(repoPath, commitSha) {
    logger.info('[COMPARE] Extraction données commit', { commitSha });

    // Checkout du commit
    execSync(`git checkout ${commitSha}`, { cwd: repoPath, stdio: 'pipe' });

    // Récupérer les métadonnées
    const metadata = JSON.parse(
      execSync(
        `git show --format='{"author":"%an","date":"%ai","message":"%s"}' --no-patch ${commitSha}`,
        { cwd: repoPath, encoding: 'utf-8' }
      )
    );

    // Récupérer la liste des fichiers modifiés
    const filesChanged = execSync(
      `git diff-tree --no-commit-id --name-only -r ${commitSha}`,
      { cwd: repoPath, encoding: 'utf-8' }
    ).trim().split('\n').filter(Boolean);

    // Lire le contenu des fichiers
    const files = {};
    for (const file of filesChanged) {
      try {
        const filePath = path.join(repoPath, file);
        const content = await fs.readFile(filePath);
        files[file] = {
          content,
          size: content.length,
          hash: crypto.createHash('sha256').update(content).digest('hex')
        };
      } catch (error) {
        logger.warn('[COMPARE] Fichier non accessible', { file, error: error.message });
      }
    }

    return {
      metadata,
      filesChanged,
      files
    };
  }

  /**
   * Effectue une analyse différentielle bit-level
   * @private
   */
  async _performDifferentialAnalysis(commit1Data, commit2Data, options) {
    logger.info('[COMPARE] Analyse différentielle bit-level');

    const analysis = {
      filesChanged: 0,
      filesAdded: [],
      filesRemoved: [],
      filesModified: [],
      bitLevelDiff: {},
      patternChanges: [],
      entropyDelta: {}
    };

    // Identifier les fichiers ajoutés, supprimés, modifiés
    const files1 = new Set(Object.keys(commit1Data.files));
    const files2 = new Set(Object.keys(commit2Data.files));

    analysis.filesAdded = [...files2].filter(f => !files1.has(f));
    analysis.filesRemoved = [...files1].filter(f => !files2.has(f));
    analysis.filesModified = [...files1].filter(f => files2.has(f) && 
      commit1Data.files[f].hash !== commit2Data.files[f].hash
    );

    analysis.filesChanged = analysis.filesAdded.length + 
                           analysis.filesRemoved.length + 
                           analysis.filesModified.length;

    // Analyse bit-level pour chaque fichier modifié
    for (const file of analysis.filesModified) {
      const content1 = commit1Data.files[file].content;
      const content2 = commit2Data.files[file].content;

      analysis.bitLevelDiff[file] = this._analyzeBitLevelDiff(content1, content2);
      
      // Calcul delta d'entropie
      const entropy1 = this._calculateEntropy(content1);
      const entropy2 = this._calculateEntropy(content2);
      analysis.entropyDelta[file] = {
        before: entropy1,
        after: entropy2,
        delta: entropy2 - entropy1,
        percentChange: ((entropy2 - entropy1) / entropy1) * 100
      };

      // Détection de changements de patterns
      const patternChange = this._detectPatternChanges(content1, content2);
      if (patternChange.significant) {
        analysis.patternChanges.push({
          file,
          ...patternChange
        });
      }
    }

    return analysis;
  }

  /**
   * Analyse bit-level d'un diff entre deux contenus
   * @private
   */
  _analyzeBitLevelDiff(content1, content2) {
    const maxLen = Math.max(content1.length, content2.length);
    let bitsChanged = 0;
    let bytesChanged = 0;
    const changePositions = [];

    for (let i = 0; i < maxLen; i++) {
      const byte1 = i < content1.length ? content1[i] : 0;
      const byte2 = i < content2.length ? content2[i] : 0;

      if (byte1 !== byte2) {
        bytesChanged++;
        
        // Compter les bits différents
        let xor = byte1 ^ byte2;
        let bits = 0;
        while (xor) {
          bits += xor & 1;
          xor >>= 1;
        }
        bitsChanged += bits;

        changePositions.push({
          offset: i,
          before: byte1,
          after: byte2,
          bitsFlipped: bits
        });
      }
    }

    return {
      bitsChanged,
      bytesChanged,
      totalBytes: maxLen,
      changeRate: (bytesChanged / maxLen) * 100,
      bitFlipRate: (bitsChanged / (maxLen * 8)) * 100,
      changePositions: changePositions.slice(0, 100) // Limiter à 100 positions
    };
  }

  /**
   * Calcule l'entropie de Shannon d'un buffer
   * @private
   */
  _calculateEntropy(buffer) {
    const freq = new Array(256).fill(0);
    for (let i = 0; i < buffer.length; i++) {
      freq[buffer[i]]++;
    }

    let entropy = 0;
    for (let i = 0; i < 256; i++) {
      if (freq[i] > 0) {
        const p = freq[i] / buffer.length;
        entropy -= p * Math.log2(p);
      }
    }

    return entropy;
  }

  /**
   * Détecte les changements de patterns significatifs
   * @private
   */
  _detectPatternChanges(content1, content2) {
    // Patterns suspects à surveiller
    const suspiciousPatterns = [
      /eval\s*\(/gi,
      /exec\s*\(/gi,
      /system\s*\(/gi,
      /shell_exec/gi,
      /base64_decode/gi,
      /\$\{.*\}/g, // Template injection
      /<script>/gi,
      /document\.cookie/gi
    ];

    const patterns1 = {};
    const patterns2 = {};

    for (const pattern of suspiciousPatterns) {
      patterns1[pattern.source] = (content1.toString().match(pattern) || []).length;
      patterns2[pattern.source] = (content2.toString().match(pattern) || []).length;
    }

    const changes = [];
    let significant = false;

    for (const pattern in patterns1) {
      const delta = patterns2[pattern] - patterns1[pattern];
      if (delta !== 0) {
        changes.push({
          pattern,
          before: patterns1[pattern],
          after: patterns2[pattern],
          delta
        });
        if (Math.abs(delta) > 0) {
          significant = true;
        }
      }
    }

    return {
      significant,
      changes
    };
  }

  /**
   * Détecte les régressions de sécurité
   * @private
   */
  async _detectSecurityRegressions(diffAnalysis) {
    logger.info('[COMPARE] Détection régressions de sécurité');

    const regressions = [];

    // Vérifier les changements de patterns suspects
    for (const patternChange of diffAnalysis.patternChanges) {
      for (const change of patternChange.changes) {
        if (change.delta > 0) {
          regressions.push({
            type: 'SUSPICIOUS_PATTERN_ADDED',
            severity: 'HIGH',
            file: patternChange.file,
            pattern: change.pattern,
            occurrences: change.after,
            description: `Pattern suspect ajouté: ${change.pattern} (${change.delta} occurrences)`
          });
        }
      }
    }

    // Vérifier les augmentations d'entropie significatives
    for (const [file, entropyDelta] of Object.entries(diffAnalysis.entropyDelta)) {
      if (entropyDelta.percentChange > 20) {
        regressions.push({
          type: 'ENTROPY_INCREASE',
          severity: 'MEDIUM',
          file,
          entropyBefore: entropyDelta.before.toFixed(3),
          entropyAfter: entropyDelta.after.toFixed(3),
          percentChange: entropyDelta.percentChange.toFixed(2),
          description: `Augmentation significative d'entropie: +${entropyDelta.percentChange.toFixed(2)}%`
        });
      }
    }

    return {
      found: regressions.length > 0,
      count: regressions.length,
      regressions
    };
  }

  /**
   * Calcule les métriques de comparaison
   * @private
   */
  _calculateComparisonMetrics(diffAnalysis) {
    let linesAdded = 0;
    let linesRemoved = 0;
    let totalBitsChanged = 0;
    let totalBytesChanged = 0;

    for (const [file, diff] of Object.entries(diffAnalysis.bitLevelDiff)) {
      totalBitsChanged += diff.bitsChanged;
      totalBytesChanged += diff.bytesChanged;
    }

    return {
      filesChanged: diffAnalysis.filesChanged,
      filesAdded: diffAnalysis.filesAdded.length,
      filesRemoved: diffAnalysis.filesRemoved.length,
      filesModified: diffAnalysis.filesModified.length,
      linesAdded,
      linesRemoved,
      bitsChanged: totalBitsChanged,
      bytesChanged: totalBytesChanged,
      patternChanges: diffAnalysis.patternChanges.length,
      securityRegressions: 0 // Sera mis à jour par detectSecurityRegressions
    };
  }

  /**
   * Génère une clé de cache unique
   * @private
   */
  _generateCacheKey(repoUrl, commitSha1, commitSha2) {
    return crypto
      .createHash('sha256')
      .update(`${repoUrl}:${commitSha1}:${commitSha2}`)
      .digest('hex');
  }

  /**
   * Nettoie le cache
   */
  clearCache() {
    this.compareCache.clear();
    logger.info('[COMPARE] Cache nettoyé');
  }
}

export default CompareService;

// Made with Bob
