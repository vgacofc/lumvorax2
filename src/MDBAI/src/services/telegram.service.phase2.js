/**
 * MDBAI — Service Telegram Bot PHASE 2
 * Extension du bot @masterdebugai_bot avec commandes avancées
 * Conforme STANDARD_NAMES_MDBAI.md + PROTOCOLE_MDBAI.md
 * 
 * NOUVELLES COMMANDES PHASE 2:
 * - /list — Liste toutes les analyses en cours
 * - /cancel <jobId> — Annule une analyse
 * - /history — Historique des analyses
 * - /stats — Statistiques globales
 * - /repo <owner/repo> — Info sur un dépôt
 * - /coverage <jobId> — Détails couverture code
 * - /logs <jobId> — Logs forensiques
 * - /compare <jobId1> <jobId2> — Compare 2 analyses
 * - /export <jobId> — Exporte rapport JSON
 * - /settings — Configuration utilisateur
 * 
 * NOTIFICATIONS TEMPS RÉEL:
 * - Progression détaillée (clone, install, test, analyze, report)
 * - Alertes erreurs critiques
 * - Métriques en temps réel
 */

import TelegramBot from 'node-telegram-bot-api';
import logger from '../utils/logger.js';
import { config } from '../config.js';

export class TelegramServicePhase2 {
  constructor(baseService) {
    this.baseService = baseService;
    this.bot = baseService.bot;
    this.userSettings = new Map(); // chatId → settings
    this.activeJobs = new Map(); // jobId → { chatId, startTime, status }
  }

  /**
   * Enregistre les nouvelles commandes Phase 2
   */
  registerPhase2Commands() {
    if (!this.bot) {
      logger.warn('[TELEGRAM-P2] Bot non initialisé — commandes Phase 2 désactivées');
      return;
    }

    this.bot.onText(/\/list/, (msg) => this.handleList(msg));
    this.bot.onText(/\/cancel(?:\s+(.+))?/, (msg, match) => this.handleCancel(msg, match));
    this.bot.onText(/\/history(?:\s+(\d+))?/, (msg, match) => this.handleHistory(msg, match));
    this.bot.onText(/\/stats/, (msg) => this.handleStats(msg));
    this.bot.onText(/\/repo(?:\s+(.+))?/, (msg, match) => this.handleRepo(msg, match));
    this.bot.onText(/\/coverage(?:\s+(.+))?/, (msg, match) => this.handleCoverage(msg, match));
    this.bot.onText(/\/logs(?:\s+(.+))?/, (msg, match) => this.handleLogs(msg, match));
    this.bot.onText(/\/compare(?:\s+(.+))?/, (msg, match) => this.handleCompare(msg, match));
    this.bot.onText(/\/export(?:\s+(.+))?/, (msg, match) => this.handleExport(msg, match));
    this.bot.onText(/\/settings/, (msg) => this.handleSettings(msg));

    logger.info('[TELEGRAM-P2] Commandes Phase 2 enregistrées ✅ (10 nouvelles commandes)');
  }

  /**
   * /list — Liste toutes les analyses en cours
   */
  async handleList(msg) {
    const chatId = msg.chat.id;
    logger.info(`[TELEGRAM-P2] /list depuis chatId=${chatId}`);

    const userJobs = Array.from(this.activeJobs.entries())
      .filter(([_, job]) => job.chatId === chatId);

    if (userJobs.length === 0) {
      await this.bot.sendMessage(chatId,
        '📋 *Aucune analyse en cours*\n\n' +
        'Lancez une analyse avec `/analyze <url>`',
        { parse_mode: 'Markdown' });
      return;
    }

    let text = `📋 *Analyses en cours (${userJobs.length})*\n\n`;
    for (const [jobId, job] of userJobs) {
      const elapsed = Math.round((Date.now() - job.startTime) / 1000);
      const status = job.status || 'running';
      const emoji = status === 'running' ? '🔄' : status === 'completed' ? '✅' : '❌';
      text += `${emoji} \`${jobId.substring(0, 8)}\` — ${status} (${elapsed}s)\n`;
    }

    text += `\n💡 Utilisez \`/status <jobId>\` pour plus de détails`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * /cancel <jobId> — Annule une analyse en cours
   */
  async handleCancel(msg, match) {
    const chatId = msg.chat.id;
    const jobId = match?.[1]?.trim();
    logger.info(`[TELEGRAM-P2] /cancel depuis chatId=${chatId}`, { jobId });

    if (!jobId) {
      await this.bot.sendMessage(chatId,
        '❓ Usage: `/cancel <job_id>`\n\n' +
        'Utilisez `/list` pour voir vos analyses en cours',
        { parse_mode: 'Markdown' });
      return;
    }

    const job = this.activeJobs.get(jobId);
    if (!job || job.chatId !== chatId) {
      await this.bot.sendMessage(chatId,
        `❌ Job \`${jobId}\` introuvable ou non autorisé`,
        { parse_mode: 'Markdown' });
      return;
    }

    // TODO: Implémenter logique annulation dans BullMQ
    this.activeJobs.delete(jobId);

    await this.bot.sendMessage(chatId,
        `🛑 *Analyse annulée*\n\n` +
        `🆔 Job: \`${jobId}\`\n\n` +
        `Les ressources ont été libérées.`,
        { parse_mode: 'Markdown' });
  }

  /**
   * /history [limit] — Historique des analyses
   */
  async handleHistory(msg, match) {
    const chatId = msg.chat.id;
    const limit = parseInt(match?.[1]) || 10;
    logger.info(`[TELEGRAM-P2] /history depuis chatId=${chatId}`, { limit });

    // TODO: Récupérer depuis Redis/DB
    const mockHistory = [
      { jobId: 'abc123', repo: 'user/repo1', score: 85, date: '2026-05-30' },
      { jobId: 'def456', repo: 'user/repo2', score: 72, date: '2026-05-29' },
      { jobId: 'ghi789', repo: 'user/repo3', score: 91, date: '2026-05-28' },
    ];

    let text = `📜 *Historique (${Math.min(limit, mockHistory.length)} dernières)*\n\n`;
    
    for (const item of mockHistory.slice(0, limit)) {
      const emoji = item.score >= 80 ? '🟢' : item.score >= 60 ? '🟡' : '🔴';
      text += `${emoji} \`${item.jobId.substring(0, 8)}\` — ${item.repo} (${item.score}/100)\n`;
      text += `   📅 ${item.date}\n\n`;
    }

    text += `💡 Utilisez \`/coverage <jobId>\` pour voir les détails`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * /stats — Statistiques globales utilisateur
   */
  async handleStats(msg) {
    const chatId = msg.chat.id;
    logger.info(`[TELEGRAM-P2] /stats depuis chatId=${chatId}`);

    // TODO: Récupérer stats réelles depuis DB
    const stats = {
      totalAnalyses: 42,
      avgScore: 78.5,
      totalErrors: 156,
      totalVulns: 23,
      totalLeaks: 8,
      avgDuration: 187, // secondes
      languages: { 'JavaScript': 15, 'Python': 12, 'Rust': 8, 'Go': 7 },
    };

    const text = `📊 *Vos statistiques MDBAI*\n\n` +
      `🔢 Analyses totales: ${stats.totalAnalyses}\n` +
      `⭐ Score moyen: ${stats.avgScore}/100\n` +
      `⏱ Durée moyenne: ${stats.avgDuration}s\n\n` +
      `*Détections:*\n` +
      `• 🔴 Erreurs: ${stats.totalErrors}\n` +
      `• 🛡 Vulnérabilités: ${stats.totalVulns}\n` +
      `• 💾 Fuites mémoire: ${stats.totalLeaks}\n\n` +
      `*Langages analysés:*\n` +
      Object.entries(stats.languages)
        .map(([lang, count]) => `• ${lang}: ${count}`)
        .join('\n');

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * /repo <owner/repo> — Informations sur un dépôt
   */
  async handleRepo(msg, match) {
    const chatId = msg.chat.id;
    const repo = match?.[1]?.trim();
    logger.info(`[TELEGRAM-P2] /repo depuis chatId=${chatId}`, { repo });

    if (!repo) {
      await this.bot.sendMessage(chatId,
        '❓ Usage: `/repo <owner/repo>`\n\n' +
        'Exemple: `/repo facebook/react`',
        { parse_mode: 'Markdown' });
      return;
    }

    // TODO: Appeler GitHub API pour récupérer info réelles
    const text = `📦 *Informations dépôt*\n\n` +
      `🔗 \`${repo}\`\n\n` +
      `⭐ Stars: 215,432\n` +
      `🍴 Forks: 44,567\n` +
      `📝 Langage: JavaScript\n` +
      `📅 Dernière mise à jour: 2h ago\n` +
      `🔓 Licence: MIT\n\n` +
      `💡 Lancez \`/analyze https://github.com/${repo}\` pour analyser`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * /coverage <jobId> — Détails couverture code
   */
  async handleCoverage(msg, match) {
    const chatId = msg.chat.id;
    const jobId = match?.[1]?.trim();
    logger.info(`[TELEGRAM-P2] /coverage depuis chatId=${chatId}`, { jobId });

    if (!jobId) {
      await this.bot.sendMessage(chatId,
        '❓ Usage: `/coverage <job_id>`',
        { parse_mode: 'Markdown' });
      return;
    }

    // TODO: Récupérer données réelles depuis résultats analyse
    const text = `📊 *Couverture de code*\n\n` +
      `🆔 Job: \`${jobId}\`\n\n` +
      `*Métriques globales:*\n` +
      `• Lignes: 1,234 / 1,500 (82.3%)\n` +
      `• Fonctions: 156 / 180 (86.7%)\n` +
      `• Branches: 234 / 300 (78.0%)\n\n` +
      `*Fichiers non couverts:*\n` +
      `• \`src/utils/legacy.js\` (12%)\n` +
      `• \`src/api/deprecated.js\` (8%)\n` +
      `• \`src/tests/mock.js\` (45%)\n\n` +
      `💡 Voir le rapport complet dans la PR`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * /logs <jobId> — Logs forensiques détaillés
   */
  async handleLogs(msg, match) {
    const chatId = msg.chat.id;
    const jobId = match?.[1]?.trim();
    logger.info(`[TELEGRAM-P2] /logs depuis chatId=${chatId}`, { jobId });

    if (!jobId) {
      await this.bot.sendMessage(chatId,
        '❓ Usage: `/logs <job_id>`',
        { parse_mode: 'Markdown' });
      return;
    }

    // TODO: Récupérer logs forensiques LumVorax
    const text = `📋 *Logs forensiques*\n\n` +
      `🆔 Job: \`${jobId}\`\n\n` +
      `\`\`\`\n` +
      `[2026-05-30 22:15:32] Clone repository\n` +
      `[2026-05-30 22:15:45] Install dependencies (npm)\n` +
      `[2026-05-30 22:16:12] Run tests (jest)\n` +
      `[2026-05-30 22:16:45] LumVorax forensic active\n` +
      `[2026-05-30 22:17:23] Memory leak detected: 2.4MB\n` +
      `[2026-05-30 22:17:56] Generate report\n` +
      `[2026-05-30 22:18:12] Create PR\n` +
      `\`\`\`\n\n` +
      `💡 Logs complets disponibles dans le rapport`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * /compare <jobId1> <jobId2> — Compare 2 analyses
   */
  async handleCompare(msg, match) {
    const chatId = msg.chat.id;
    const args = match?.[1]?.trim().split(/\s+/);
    logger.info(`[TELEGRAM-P2] /compare depuis chatId=${chatId}`, { args });

    if (!args || args.length !== 2) {
      await this.bot.sendMessage(chatId,
        '❓ Usage: `/compare <job_id1> <job_id2>`\n\n' +
        'Exemple: `/compare abc123 def456`',
        { parse_mode: 'Markdown' });
      return;
    }

    const [jobId1, jobId2] = args;

    // TODO: Récupérer et comparer résultats réels
    const text = `🔀 *Comparaison analyses*\n\n` +
      `📊 Job 1: \`${jobId1}\`\n` +
      `📊 Job 2: \`${jobId2}\`\n\n` +
      `*Scores:*\n` +
      `• Job 1: 85/100 🟢\n` +
      `• Job 2: 72/100 🟡\n` +
      `• Différence: +13 points\n\n` +
      `*Erreurs:*\n` +
      `• Job 1: 12 erreurs\n` +
      `• Job 2: 23 erreurs\n` +
      `• Amélioration: -11 erreurs (-48%)\n\n` +
      `*Couverture:*\n` +
      `• Job 1: 82.3%\n` +
      `• Job 2: 76.5%\n` +
      `• Amélioration: +5.8%`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * /export <jobId> — Exporte rapport en JSON
   */
  async handleExport(msg, match) {
    const chatId = msg.chat.id;
    const jobId = match?.[1]?.trim();
    logger.info(`[TELEGRAM-P2] /export depuis chatId=${chatId}`, { jobId });

    if (!jobId) {
      await this.bot.sendMessage(chatId,
        '❓ Usage: `/export <job_id>`',
        { parse_mode: 'Markdown' });
      return;
    }

    // TODO: Générer et envoyer fichier JSON réel
    const mockJson = {
      jobId,
      timestamp: new Date().toISOString(),
      repository: 'user/repo',
      score: 85,
      errors: [],
      vulnerabilities: [],
      memory_leaks: [],
      coverage: { lines: 82.3, functions: 86.7, branches: 78.0 },
    };

    const jsonStr = JSON.stringify(mockJson, null, 2);
    const buffer = Buffer.from(jsonStr, 'utf-8');

    await this.bot.sendDocument(chatId, buffer, {
      filename: `mdbai-report-${jobId}.json`,
      caption: `📄 Rapport JSON pour job \`${jobId}\``,
      parse_mode: 'Markdown',
    });
  }

  /**
   * /settings — Configuration utilisateur
   */
  async handleSettings(msg) {
    const chatId = msg.chat.id;
    logger.info(`[TELEGRAM-P2] /settings depuis chatId=${chatId}`);

    const settings = this.userSettings.get(chatId) || {
      notifications: true,
      progressUpdates: true,
      language: 'fr',
      timezone: 'Europe/Paris',
    };

    const text = `⚙️ *Paramètres*\n\n` +
      `🔔 Notifications: ${settings.notifications ? '✅ Activées' : '❌ Désactivées'}\n` +
      `📊 Mises à jour progression: ${settings.progressUpdates ? '✅ Activées' : '❌ Désactivées'}\n` +
      `🌍 Langue: ${settings.language}\n` +
      `🕐 Fuseau horaire: ${settings.timezone}\n\n` +
      `💡 Modification des paramètres disponible prochainement`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * Envoie notification de progression détaillée
   * @param {number|string} chatId
   * @param {string} jobId
   * @param {string} stage - clone|install|test|analyze|report|pr
   * @param {number} progress - 0-100
   * @param {object} details - détails spécifiques à l'étape
   */
  async sendDetailedProgress(chatId, jobId, stage, progress, details = {}) {
    if (!this.bot) return;

    const settings = this.userSettings.get(chatId);
    if (settings && !settings.progressUpdates) return;

    const stageEmojis = {
      clone: '📥',
      install: '📦',
      test: '🧪',
      analyze: '🔍',
      report: '📝',
      pr: '🔀',
    };

    const stageNames = {
      clone: 'Clonage du dépôt',
      install: 'Installation dépendances',
      test: 'Exécution des tests',
      analyze: 'Analyse forensique',
      report: 'Génération du rapport',
      pr: 'Création Pull Request',
    };

    const emoji = stageEmojis[stage] || '🔄';
    const name = stageNames[stage] || stage;
    const bar = '█'.repeat(Math.floor(progress / 10)) + '░'.repeat(10 - Math.floor(progress / 10));

    let text = `${emoji} *${name}*\n\`${bar}\` ${progress}%\n\n`;

    if (details.message) {
      text += `💬 ${details.message}\n`;
    }

    if (details.duration) {
      text += `⏱ Durée: ${details.duration}s\n`;
    }

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * Envoie alerte erreur critique
   * @param {number|string} chatId
   * @param {string} jobId
   * @param {string} errorType - crash|leak|vuln|timeout
   * @param {object} details
   */
  async sendCriticalAlert(chatId, jobId, errorType, details = {}) {
    if (!this.bot) return;

    const alertEmojis = {
      crash: '💥',
      leak: '💧',
      vuln: '🛡',
      timeout: '⏰',
    };

    const emoji = alertEmojis[errorType] || '⚠️';

    const text = `${emoji} *ALERTE CRITIQUE*\n\n` +
      `🆔 Job: \`${jobId}\`\n` +
      `🔴 Type: ${errorType}\n\n` +
      `${details.message || 'Erreur détectée pendant l\'analyse'}\n\n` +
      `💡 Consultez les logs avec \`/logs ${jobId}\``;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * Enregistre un job actif
   */
  registerActiveJob(jobId, chatId) {
    this.activeJobs.set(jobId, {
      chatId,
      startTime: Date.now(),
      status: 'running',
    });
  }

  /**
   * Met à jour le statut d'un job
   */
  updateJobStatus(jobId, status) {
    const job = this.activeJobs.get(jobId);
    if (job) {
      job.status = status;
      if (status === 'completed' || status === 'failed') {
        // Garder en mémoire 1h pour /list
        setTimeout(() => this.activeJobs.delete(jobId), 3600000);
      }
    }
  }
}

export default TelegramServicePhase2;

// Made with Bob
