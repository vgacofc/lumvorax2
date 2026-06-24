/**
 * MDBAI — Service d'intégration Bob CLI (BobIDE)
 * Permet au bot Telegram d'activer Bob RÉELLEMENT pour analyser le code
 * AUCUN STUB - AUCUN PLACEHOLDER - EXÉCUTION RÉELLE UNIQUEMENT
 */

import { execSync } from 'child_process';
import { writeFileSync, readFileSync, existsSync, mkdirSync } from 'fs';
import { join } from 'path';
import { createHash } from 'crypto';
import logger from '../utils/logger.js';

const BOB_CLI_PATH = '/usr/bin/bob'; // Bob CLI installé dans BobIDE
const FORENSIC_DIR = join(process.cwd(), 'forensic', 'bob-analysis');

export class BobCliService {
  constructor(jobId) {
    this.jobId = jobId;
    this.analysisDir = join(FORENSIC_DIR, jobId);
    mkdirSync(this.analysisDir, { recursive: true });
  }

  /**
   * APPEL RÉEL À BOB CLI pour analyser le code source
   * Génère des preuves forensiques tangibles et vérifiables
   * @param {string} repoDir - Répertoire du code source cloné
   * @param {string} language - Langage détecté
   * @param {object} forensicData - Données forensiques LumVorax
   * @returns {Promise<BobAnalysis>}
   */
  async analyzeCodeWithBob(repoDir, language, forensicData) {
    const startTime = Date.now();
    const timestamp = new Date().toISOString();
    
    logger.info(`[BOB-CLI] 🤖 Activation de Bob pour analyse réelle — job ${this.jobId}`);
    
    // 1. Créer fichier de contexte pour Bob
    const contextFile = join(this.analysisDir, 'bob-context.json');
    const context = {
      job_id: this.jobId,
      timestamp,
      repo_dir: repoDir,
      language,
      forensic_data: forensicData,
      task: `Analyse forensique complète du code source ${language}. 
Identifie:
1. Erreurs de logique et bugs
2. Fuites mémoire potentielles
3. Vulnérabilités de sécurité
4. Violations de bonnes pratiques
5. Optimisations possibles

Génère un rapport détaillé avec:
- Analyse ligne par ligne
- Raisonnement pour chaque problème identifié
- Recommandations de correction
- Score de qualité du code`
    };
    
    writeFileSync(contextFile, JSON.stringify(context, null, 2), 'utf8');
    logger.info(`[BOB-CLI] Contexte sauvegardé: ${contextFile}`);
    
    // 2. Lire TOUS les fichiers source du dépôt
    const sourceFiles = this._readAllSourceFiles(repoDir, language);
    const sourceFile = join(this.analysisDir, 'source-code.txt');
    writeFileSync(sourceFile, sourceFiles.content, 'utf8');
    
    const sourceHash = createHash('sha256').update(sourceFiles.content).digest('hex');
    logger.info(`[BOB-CLI] Code source lu: ${sourceFiles.file_count} fichiers, ${sourceFiles.line_count} lignes, SHA256=${sourceHash.substring(0, 16)}...`);
    
    // 3. Créer prompt pour Bob CLI
    const promptFile = join(this.analysisDir, 'bob-prompt.txt');
    const prompt = `Tu es Bob, expert en analyse de code. Analyse ce code ${language} de manière exhaustive.

CONTEXTE:
- Job ID: ${this.jobId}
- Langage: ${language}
- Fichiers: ${sourceFiles.file_count}
- Lignes: ${sourceFiles.line_count}
- Hash SHA256: ${sourceHash}

CODE SOURCE:
${sourceFiles.content}

DONNÉES FORENSIQUES LUMVORAX:
${JSON.stringify(forensicData, null, 2)}

TÂCHE:
Analyse ligne par ligne et identifie TOUS les problèmes. Pour chaque problème:
1. Numéro de ligne exact
2. Type de problème (erreur/fuite/vulnérabilité/optimisation)
3. Sévérité (critique/haute/moyenne/basse)
4. Explication détaillée
5. Recommandation de correction

Génère un rapport structuré en JSON avec:
{
  "analysis_metadata": {
    "bob_version": "...",
    "timestamp": "...",
    "duration_ms": ...,
    "files_analyzed": ...,
    "lines_analyzed": ...
  },
  "errors": [...],
  "memory_leaks": [...],
  "vulnerabilities": [...],
  "optimizations": [...],
  "quality_score": ...,
  "reasoning": "..."
}`;
    
    writeFileSync(promptFile, prompt, 'utf8');
    logger.info(`[BOB-CLI] Prompt créé: ${promptFile}`);
    
    // 4. APPEL RÉEL À BOB CLI
    const outputFile = join(this.analysisDir, 'bob-output.json');
    const stdoutFile = join(this.analysisDir, 'bob-stdout.log');
    const stderrFile = join(this.analysisDir, 'bob-stderr.log');
    
    let bobOutput = '';
    let exitCode = 0;
    
    try {
      logger.info(`[BOB-CLI] 🚀 Exécution Bob CLI: ${BOB_CLI_PATH}`);
      
      // Commande Bob CLI réelle
      const cmd = `${BOB_CLI_PATH} analyze --input "${promptFile}" --output "${outputFile}" --format json --verbose 2>"${stderrFile}" 1>"${stdoutFile}"`;
      
      logger.info(`[BOB-CLI] Commande: ${cmd}`);
      
      // EXÉCUTION RÉELLE
      execSync(cmd, {
        cwd: repoDir,
        timeout: 300000, // 5 minutes max
        maxBuffer: 50 * 1024 * 1024, // 50MB
        env: {
          ...process.env,
          BOB_JOB_ID: this.jobId,
          BOB_LANGUAGE: language,
          BOB_FORENSIC_MODE: 'true'
        }
      });
      
      // Lire la sortie de Bob
      if (existsSync(outputFile)) {
        bobOutput = readFileSync(outputFile, 'utf8');
        logger.info(`[BOB-CLI] ✅ Analyse Bob terminée: ${outputFile} (${bobOutput.length} bytes)`);
      } else {
        throw new Error(`Bob CLI n'a pas généré de fichier de sortie: ${outputFile}`);
      }
      
    } catch (error) {
      exitCode = error.status || 1;
      logger.error(`[BOB-CLI] ❌ Erreur Bob CLI: ${error.message}`);
      
      // Lire stderr pour diagnostique
      if (existsSync(stderrFile)) {
        const stderr = readFileSync(stderrFile, 'utf8');
        logger.error(`[BOB-CLI] Stderr: ${stderr}`);
      }
      
      // Créer sortie d'erreur
      bobOutput = JSON.stringify({
        error: true,
        message: error.message,
        exit_code: exitCode,
        timestamp
      });
      writeFileSync(outputFile, bobOutput, 'utf8');
    }
    
    const duration = Date.now() - startTime;
    
    // 5. Parser la réponse de Bob
    let analysis;
    try {
      analysis = JSON.parse(bobOutput);
    } catch (e) {
      logger.error(`[BOB-CLI] Erreur parsing JSON Bob: ${e.message}`);
      analysis = {
        error: true,
        message: 'Erreur parsing réponse Bob',
        raw_output: bobOutput
      };
    }
    
    // 6. Créer rapport forensique de l'intervention de Bob
    const forensicReport = {
      job_id: this.jobId,
      timestamp,
      bob_invoked: true,
      bob_cli_path: BOB_CLI_PATH,
      exit_code: exitCode,
      duration_ms: duration,
      source_files: {
        count: sourceFiles.file_count,
        lines: sourceFiles.line_count,
        sha256: sourceHash
      },
      files_created: {
        context: contextFile,
        prompt: promptFile,
        output: outputFile,
        stdout: stdoutFile,
        stderr: stderrFile
      },
      analysis: analysis,
      verification: {
        context_exists: existsSync(contextFile),
        prompt_exists: existsSync(promptFile),
        output_exists: existsSync(outputFile),
        stdout_exists: existsSync(stdoutFile),
        stderr_exists: existsSync(stderrFile)
      }
    };
    
    // 7. Sauvegarder rapport forensique
    const forensicFile = join(this.analysisDir, 'bob-forensic-report.json');
    writeFileSync(forensicFile, JSON.stringify(forensicReport, null, 2), 'utf8');
    
    logger.info(`[BOB-CLI] 📊 Rapport forensique: ${forensicFile}`);
    logger.info(`[BOB-CLI] ✅ Analyse Bob complète en ${duration}ms`);
    logger.info(`[BOB-CLI] 📁 Preuves tangibles dans: ${this.analysisDir}`);
    
    return forensicReport;
  }

  /**
   * Lit TOUS les fichiers source du dépôt
   * @param {string} dir - Répertoire racine
   * @param {string} language - Langage pour filtrer extensions
   * @returns {object} {content, file_count, line_count, files}
   */
  _readAllSourceFiles(dir, language) {
    const extensions = this._getExtensionsForLanguage(language);
    const files = [];
    let content = '';
    let lineCount = 0;
    
    const walk = (currentDir) => {
      const { readdirSync, statSync } = require('fs');
      const entries = readdirSync(currentDir);
      
      for (const entry of entries) {
        const fullPath = join(currentDir, entry);
        const stat = statSync(fullPath);
        
        // Ignorer node_modules, .git, etc.
        if (entry === 'node_modules' || entry === '.git' || entry === 'dist' || entry === 'build') {
          continue;
        }
        
        if (stat.isDirectory()) {
          walk(fullPath);
        } else if (stat.isFile()) {
          const ext = entry.substring(entry.lastIndexOf('.'));
          if (extensions.includes(ext)) {
            try {
              const fileContent = readFileSync(fullPath, 'utf8');
              const lines = fileContent.split('\n').length;
              
              content += `\n\n// ========================================\n`;
              content += `// FILE: ${fullPath}\n`;
              content += `// LINES: ${lines}\n`;
              content += `// ========================================\n\n`;
              content += fileContent;
              
              files.push({ path: fullPath, lines });
              lineCount += lines;
            } catch (e) {
              logger.warn(`[BOB-CLI] Impossible de lire ${fullPath}: ${e.message}`);
            }
          }
        }
      }
    };
    
    walk(dir);
    
    return {
      content,
      file_count: files.length,
      line_count: lineCount,
      files
    };
  }

  /**
   * Retourne les extensions de fichiers pour un langage
   */
  _getExtensionsForLanguage(language) {
    const map = {
      'nodejs': ['.js', '.ts', '.mjs', '.cjs', '.jsx', '.tsx'],
      'python': ['.py', '.pyw', '.pyx'],
      'rust': ['.rs'],
      'go': ['.go'],
      'c_cpp': ['.c', '.cpp', '.cc', '.cxx', '.h', '.hpp'],
      'java': ['.java'],
      'php': ['.php'],
      'ruby': ['.rb'],
      'haskell': ['.hs']
    };
    
    return map[language] || ['.txt'];
  }
}

// Made with Bob
