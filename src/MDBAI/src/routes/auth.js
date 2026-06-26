/**
 * MDBAI — Route OAuth GitHub
 * GET /auth/github          — Redirige vers GitHub
 * GET /auth/github/callback — Callback OAuth (page HTML avec retry)
 * POST /auth/github/callback — API pour échange de code
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 * CF-002: Autorisation → Token valide
 */

import { Router } from 'express';
import { GitHubService } from '../services/github.service.js';
import { getUserByTelegramId, updateUserRedis } from '../services/redis-registration.service.js';
import TelegramBot from 'node-telegram-bot-api';
import logger from '../utils/logger.js';
import { config } from '../config.js';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const authRouter = Router();
const pendingStates = new Map();

/**
 * GET /auth/github
 * Génère l'URL OAuth et redirige l'utilisateur vers GitHub
 */
authRouter.get('/github', (req, res) => {
  if (!config.github.clientId) {
    return res.status(503).json({
      error: 'GitHub OAuth non configuré',
      hint: 'Définir GITHUB_APP_CLIENT_ID dans Replit Secrets',
    });
  }

  const userId = req.query.user_id || req.session?.userId || 'anonymous';
  const state  = `${userId}:${Math.random().toString(36).slice(2)}`;
  pendingStates.set(state, { userId, createdAt: Date.now() });

  setTimeout(() => pendingStates.delete(state), 10 * 60 * 1000);

  const github = new GitHubService();
  const authUrl = github.getAuthUrl(state);
  logger.info('[AUTH] Redirection OAuth GitHub', { userId });
  return res.redirect(authUrl);
});

/**
 * GET /auth/github/callback
 * GitHub redirige ici après autorisation
 * Affiche la page HTML avec retry automatique
 */
authRouter.get('/github/callback', async (req, res) => {
  const { code, state, error } = req.query;

  if (error) {
    logger.warn('[AUTH] OAuth refusé', { error });
    return res.status(401).send(`
      <h1>❌ Autorisation refusée</h1>
      <p>${error}</p>
      <a href="/">Retour</a>
    `);
  }

  if (!code || !state) {
    return res.status(400).send(`
      <h1>❌ Paramètres manquants</h1>
      <p>Code ou state OAuth manquant</p>
      <a href="/">Retour</a>
    `);
  }

  // Extraire telegram_id du state si présent
  const isTelegramFlow = state && !state.includes(':') && /^\d+$/.test(state);
  const telegramId = isTelegramFlow ? state : null;

  // Servir la page HTML avec les paramètres
  const htmlPath = path.join(__dirname, '../views/oauth-callback.html');
  logger.info('[AUTH] Affichage page callback OAuth', {
    code: code.substring(0, 10) + '...',
    state,
    telegramId
  });
  
  return res.sendFile(htmlPath);
});

/**
 * POST /auth/github/callback
 * API appelée par la page HTML pour échanger le code OAuth
 * Support retry automatique côté client
 */
authRouter.post('/github/callback', async (req, res) => {
  const { code, state, telegram_id } = req.body;

  if (!code || !state) {
    logger.warn('[AUTH] POST callback: paramètres manquants', { code: !!code, state: !!state });
    return res.status(400).json({
      success: false,
      error: 'Code ou state manquant'
    });
  }

  const pending = state ? pendingStates.get(state) : null;
  if (state) pendingStates.delete(state);

  try {
    logger.info('[AUTH] Tentative échange code OAuth', {
      codePrefix: code.substring(0, 10) + '...',
      state,
      telegram_id
    });

    const github = new GitHubService();
    const accessToken = await github.exchangeCode(code);
    const authedGitHub = new GitHubService(accessToken);

    const { data: githubUser } = await authedGitHub.octokit.users.getAuthenticated();
    logger.info('[AUTH] OAuth réussi', { login: githubUser.login, state });

    // FLUX TELEGRAM: state contient telegram_id
    const isTelegramFlow = state && !state.includes(':') && /^\d+$/.test(state);
    
    if (isTelegramFlow) {
      const telegramId = state;
      logger.info('[AUTH] Flux Telegram détecté', { telegramId, githubLogin: githubUser.login });
      
      // Récupérer utilisateur Redis via telegram_id
      const user = await getUserByTelegramId(telegramId);
      
      if (!user) {
        logger.error('[AUTH] Utilisateur Telegram non trouvé', { telegramId });
        return res.status(404).json({
          success: false,
          error: 'Utilisateur non trouvé. Utilisez /start sur Telegram pour créer un compte.'
        });
      }
      
      // Mettre à jour Redis avec github_login et github_token
      await updateUserRedis(user.id, {
        github_login: githubUser.login,
        github_token: accessToken,
        github_id: githubUser.id,
        github_avatar: githubUser.avatar_url
      });
      logger.info('[AUTH] User Redis mis à jour', {
        userId: user.id,
        email: user.email,
        githubLogin: githubUser.login
      });
      
      // Envoyer notification Telegram
      try {
        const bot = new TelegramBot(config.telegram.token);
        await bot.sendMessage(
          telegramId,
          `✅ GitHub connecté avec succès!\n\n` +
          `🔗 Compte GitHub: ${githubUser.login}\n` +
          `📧 Email MDBAI: ${user.email}\n\n` +
          `Vous pouvez maintenant utiliser /analyze pour analyser vos dépôts.`
        );
        logger.info('[AUTH] Notification Telegram envoyée', { telegramId });
      } catch (telegramErr) {
        logger.error('[AUTH] Échec notification Telegram', { error: telegramErr.message });
      }
      
      return res.json({
        success: true,
        message: 'Connexion GitHub réussie',
        github_login: githubUser.login,
        email: user.email
      });
    }
    
    // FLUX WEB: Stockage session Express (comportement original)
    if (req.session) {
      req.session.githubToken  = accessToken;
      req.session.githubLogin  = githubUser.login;
      req.session.userId       = pending?.userId || githubUser.login;
    }

    return res.json({
      success: true,
      message: 'Connexion GitHub réussie',
      github_login: githubUser.login
    });
  } catch (e) {
    logger.error('[AUTH] Échange code OAuth échoué', {
      error: e.message,
      stack: e.stack,
      code: code?.substring(0, 10) + '...'
    });
    return res.status(500).json({
      success: false,
      error: e.message || 'Erreur lors de l\'échange du code OAuth'
    });
  }
});

/**
 * GET /auth/status
 * Vérifie si l'utilisateur est connecté GitHub
 */
authRouter.get('/status', (req, res) => {
  const connected = !!(req.session?.githubToken);
  return res.json({
    connected,
    login: req.session?.githubLogin || null,
    userId: req.session?.userId || null,
  });
});

export default authRouter;
