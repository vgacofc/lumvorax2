/**
 * MDBAI — Route OAuth GitHub
 * GET /auth/github          — Redirige vers GitHub
 * GET /auth/github/callback — Callback OAuth
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 * CF-002: Autorisation → Token valide
 */

import { Router } from 'express';
import { GitHubService } from '../services/github.service.js';
import logger from '../utils/logger.js';
import { config } from '../config.js';

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

  if (!code) {
    return res.status(400).json({ error: 'Code OAuth manquant' });
  }

  const pending = state ? pendingStates.get(state) : null;
  if (state) pendingStates.delete(state);

  try {
    const github = new GitHubService();
    const accessToken = await github.exchangeCode(code);
    const authedGitHub = new GitHubService(accessToken);

    const { data: user } = await authedGitHub.octokit.users.getAuthenticated();
    logger.info('[AUTH] OAuth réussi', { login: user.login });

    if (req.session) {
      req.session.githubToken  = accessToken;
      req.session.githubLogin  = user.login;
      req.session.userId       = pending?.userId || user.login;
    }

    return res.send(`
      <h1>✅ Connexion GitHub réussie!</h1>
      <p>Connecté en tant que <strong>${user.login}</strong></p>
      <p>Vous pouvez maintenant utiliser <code>/analyze</code> sur Telegram.</p>
      <p>Token stocké en session — jamais transmis en clair.</p>
      <script>setTimeout(() => window.close(), 3000);</script>
    `);
  } catch (e) {
    logger.error('[AUTH] Échange code OAuth échoué', { error: e.message });
    return res.status(500).json({ error: 'OAuth échoué', detail: e.message });
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
