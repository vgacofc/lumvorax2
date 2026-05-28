/**
 * MDBAI — Service Telegram Bot
 * Bot: @masterdebugai_bot
 * Conforme STANDARD_NAMES_MDBAI.md Section 5 (handleStart, handleAnalyze...)
 * Token via TELEGRAM_BOT_TOKEN (Replit Secret) — jamais en dur
 */

import TelegramBot from 'node-telegram-bot-api';
import logger from '../utils/logger.js';
import { config } from '../config.js';
import { validateRepoUrl } from '../utils/validator.js';
import { MdbaiError, ERR_TELEGRAM_CONNECT } from '../utils/errors.js';

export class TelegramService {
  constructor() {
    this.bot = null;
    this.initialized = false;
    this.pendingAnalyses = new Map();
  }

  /**
   * Initialise et démarre le bot Telegram
   * @param {function} onAnalyzeRequest - Callback(chatId, repoUrl, userId)
   * @returns {TelegramService}
   */
  init(onAnalyzeRequest) {
    if (!config.telegram.token) {
      logger.warn('[TELEGRAM] TELEGRAM_BOT_TOKEN absent — bot désactivé (mode dégradé)');
      return this;
    }

    try {
      this.bot = new TelegramBot(config.telegram.token, {
        polling: {
          interval: 1000,
          autoStart: false,       // on démarre manuellement après délai anti-409
          params: { timeout: 10 },
        },
      });

      this.onAnalyzeRequest = onAnalyzeRequest;
      this._registerCommands();
      this.initialized = true;

      // Délai 3s avant polling pour éviter 409 Conflict après restart rapide
      setTimeout(() => {
        this.bot.startPolling().catch(e => {
          logger.warn('[TELEGRAM] startPolling retardé échoué (409 attendu)', { error: e.message });
        });
      }, 3000);

      // Gestion gracieuse erreurs 409 / réseau sans crash
      this.bot.on('polling_error', (err) => {
        if (err.code === 'ETELEGRAM' && err.message?.includes('409')) {
          logger.warn('[TELEGRAM] 409 Conflict — autre instance active, retry dans 5s');
          this.bot.stopPolling().then(() => {
            setTimeout(() => this.bot.startPolling().catch(() => {}), 5000);
          }).catch(() => {});
        } else {
          logger.error('[TELEGRAM] Polling error', { error: err.message });
        }
      });

      logger.info('[TELEGRAM] Bot @masterdebugai_bot initialisé ✅');
    } catch (e) {
      logger.error('[TELEGRAM] Échec initialisation', { error: e.message });
      throw new MdbaiError(ERR_TELEGRAM_CONNECT, `Bot Telegram non connecté: ${e.message}`);
    }
    return this;
  }

  /**
   * Enregistre toutes les commandes Telegram
   */
  _registerCommands() {
    this.bot.onText(/\/start/, (msg) => this.handleStart(msg));
    this.bot.onText(/\/help/,  (msg) => this.handleHelp(msg));
    this.bot.onText(/\/analyze(?:\s+(.+))?/, (msg, match) => this.handleAnalyze(msg, match));
    this.bot.onText(/\/status(?:\s+(.+))?/,  (msg, match) => this.handleStatus(msg, match));
    this.bot.on('message', (msg) => this._handleTextInput(msg));
    this.bot.on('polling_error', (e) =>
      logger.error('[TELEGRAM] Polling error', { error: e.message }));
    logger.debug('[TELEGRAM] Commandes enregistrées: /start /help /analyze /status');
  }

  /**
   * Gère la commande /start
   * CF-001: Bot Telegram répond à /start → Message bienvenue
   */
  async handleStart(msg) {
    const chatId = msg.chat.id;
    const name = msg.from?.first_name || 'Dev';
    logger.info(`[TELEGRAM] /start depuis chatId=${chatId} user=${name}`);

    const text = `🤖 *Master Debug AI — MDBAI v0.1.0*\n\n` +
      `Bienvenue ${name}! Je suis votre assistant d'analyse forensique automatisée.\n\n` +
      `🔍 *Ce que je fais:*\n` +
      `• Clone votre dépôt GitHub\n` +
      `• Exécute vos tests avec instrumentation bit-level LumVorax\n` +
      `• Détecte erreurs, fuites mémoire et vulnérabilités\n` +
      `• Génère un rapport complet et ouvre une PR\n\n` +
      `📋 *Commandes disponibles:*\n` +
      `\`/analyze <url>\` — Analyser un dépôt GitHub\n` +
      `\`/status <jobId>\` — Vérifier l'état d'une analyse\n` +
      `\`/help\` — Aide complète\n\n` +
      `💡 Exemple: \`/analyze https://github.com/user/repo\``;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * Gère la commande /help
   */
  async handleHelp(msg) {
    const chatId = msg.chat.id;
    logger.info(`[TELEGRAM] /help depuis chatId=${chatId}`);

    const text = `📖 *Aide MDBAI — Master Debug AI*\n\n` +
      `*Analyse forensique automatisée de dépôts GitHub*\n\n` +
      `*Commandes:*\n` +
      `• \`/analyze <url_github>\` — Lance une analyse complète\n` +
      `• \`/status <job_id>\` — Vérifie l'état d'un job\n` +
      `• \`/start\` — Message de bienvenue\n\n` +
      `*Langages supportés:*\nNode.js, Python, Rust, Go, C/C++\n\n` +
      `*Ce que l'analyse détecte:*\n` +
      `🔴 Erreurs critiques et crashes\n` +
      `🟡 Fuites mémoire (valgrind/ASan/LumVorax)\n` +
      `🟠 Vulnérabilités CVE (npm audit / pip-audit)\n` +
      `⚡ Métriques de performance\n` +
      `📊 Traçabilité bit-level LumVorax\n\n` +
      `*Résultat:*\nRapport Markdown + PR GitHub automatique\n\n` +
      `🌐 Budget: 0€ — Entièrement sur tier gratuit`;

    await this.bot.sendMessage(chatId, text, { parse_mode: 'Markdown' });
  }

  /**
   * Gère la commande /analyze <url>
   * CF-003: /analyze → Job enqueued
   */
  async handleAnalyze(msg, match) {
    const chatId = msg.chat.id;
    const userId = String(msg.from?.id || chatId);
    const url = match?.[1]?.trim();

    logger.info(`[TELEGRAM] /analyze depuis chatId=${chatId}`, { url });

    if (!url) {
      this.pendingAnalyses.set(chatId, { waitingForUrl: true });
      await this.bot.sendMessage(chatId,
        '📎 Envoyez l\'URL GitHub à analyser:\n`https://github.com/owner/repo`',
        { parse_mode: 'Markdown' });
      return;
    }

    const validation = validateRepoUrl(url);
    if (!validation.valid) {
      await this.bot.sendMessage(chatId,
        `❌ URL invalide: ${validation.error}\n\n` +
        `Format attendu: \`https://github.com/owner/repo\``,
        { parse_mode: 'Markdown' });
      return;
    }

    await this.bot.sendMessage(chatId,
      `⏳ *Analyse démarrée!*\n\nDépôt: \`${validation.value}\`\n\nJe vous notifie dès que c'est terminé.`,
      { parse_mode: 'Markdown' });

    if (this.onAnalyzeRequest) {
      await this.onAnalyzeRequest(chatId, validation.value, userId);
    }
  }

  /**
   * Gère la commande /status <jobId>
   */
  async handleStatus(msg, match) {
    const chatId = msg.chat.id;
    const jobId = match?.[1]?.trim();
    logger.info(`[TELEGRAM] /status depuis chatId=${chatId}`, { jobId });

    if (!jobId) {
      await this.bot.sendMessage(chatId,
        '❓ Usage: `/status <job_id>`', { parse_mode: 'Markdown' });
      return;
    }
    await this.bot.sendMessage(chatId,
      `🔍 Vérification du job \`${jobId}\`...`, { parse_mode: 'Markdown' });
  }

  /**
   * Gère les messages texte (entrée URL après /analyze sans argument)
   */
  async _handleTextInput(msg) {
    if (!msg.text || msg.text.startsWith('/')) return;
    const chatId = msg.chat.id;
    const pending = this.pendingAnalyses.get(chatId);
    if (pending?.waitingForUrl) {
      this.pendingAnalyses.delete(chatId);
      const fakeMatch = [null, msg.text.trim()];
      await this.handleAnalyze(msg, fakeMatch);
    }
  }

  /**
   * Envoie une notification de démarrage d'analyse
   * @param {number|string} chatId
   * @param {string} jobId
   * @param {string} repoUrl
   */
  async sendAnalysisStarted(chatId, jobId, repoUrl) {
    if (!this.bot) return;
    await this.bot.sendMessage(chatId,
      `🚀 *Analyse démarrée!*\n\n` +
      `📦 Dépôt: \`${repoUrl}\`\n` +
      `🆔 Job ID: \`${jobId}\`\n\n` +
      `⏱ Durée estimée: 2–5 minutes\n` +
      `Je vous notifie dès la fin.`,
      { parse_mode: 'Markdown' });
  }

  /**
   * Envoie une notification de progression (%)
   * @param {number|string} chatId
   * @param {string} jobId
   * @param {number} progress - 0-100
   * @param {string} stage - étape en cours
   */
  async sendProgress(chatId, jobId, progress, stage) {
    if (!this.bot) return;
    const bar = '█'.repeat(Math.floor(progress / 10)) + '░'.repeat(10 - Math.floor(progress / 10));
    await this.bot.sendMessage(chatId,
      `📊 *Progression ${progress}%*\n\`${bar}\`\n\n🔄 ${stage}`,
      { parse_mode: 'Markdown' });
  }

  /**
   * Envoie une notification de fin d'analyse avec lien PR
   * @param {number|string} chatId
   * @param {string} prUrl
   * @param {object} summary
   */
  async sendAnalysisComplete(chatId, prUrl, summary) {
    if (!this.bot) return;

    const score = summary.score || 0;
    const emoji = score >= 80 ? '🟢' : score >= 60 ? '🟡' : '🔴';
    const errCount = summary.errors?.length || 0;
    const vulnCount = summary.vulnerabilities?.length || 0;
    const leakCount = summary.memory_leaks?.length || 0;

    const text = `✅ *Analyse terminée!*\n\n` +
      `${emoji} Score qualité: *${score}/100*\n\n` +
      `📊 *Résumé:*\n` +
      `• 🔴 Erreurs: ${errCount}\n` +
      `• 🛡 Vulnérabilités: ${vulnCount}\n` +
      `• 💾 Fuites mémoire: ${leakCount}\n\n` +
      `${prUrl ? `🔗 *Pull Request:* [Voir le rapport](${prUrl})\n\n` : ''}` +
      `Durée: ${summary.duration_ms ? Math.round(summary.duration_ms / 1000) + 's' : 'N/A'}`;

    await this.bot.sendMessage(chatId, text, {
      parse_mode: 'Markdown',
      disable_web_page_preview: false,
    });
  }

  /**
   * Envoie une notification d'erreur
   * @param {number|string} chatId
   * @param {string} jobId
   * @param {string} errorMsg
   */
  async sendAnalysisError(chatId, jobId, errorMsg) {
    if (!this.bot) return;
    await this.bot.sendMessage(chatId,
      `❌ *Analyse échouée*\n\n` +
      `🆔 Job: \`${jobId}\`\n` +
      `💬 Erreur: ${errorMsg}\n\n` +
      `Réessayez avec \`/analyze <url>\``,
      { parse_mode: 'Markdown' });
  }

  /**
   * Arrête le bot proprement
   */
  async stop() {
    if (this.bot && config.telegram.polling) {
      await this.bot.stopPolling();
      logger.info('[TELEGRAM] Bot arrêté');
    }
  }
}

export default TelegramService;
