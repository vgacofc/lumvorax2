/**
 * MDBAI — Service GitHub (API + OAuth + Git)
 * Conforme STANDARD_NAMES_MDBAI.md Section 5 (getAuthUrl, cloneRepository...)
 * Utilise @octokit/rest + exec git — credentials via env (jamais en dur)
 */

import { Octokit } from '@octokit/rest';
import { createAppAuth } from '@octokit/auth-app';
import { execFileSync, execSync } from 'child_process';
import { mkdirSync, rmSync, existsSync, writeFileSync } from 'fs';
import { join } from 'path';
import { createSign } from 'crypto';
import logger from '../utils/logger.js';
import { config } from '../config.js';
import { parseGitHubUrl } from '../utils/validator.js';
import { MdbaiError, ERR_GITHUB_AUTH, ERR_REPO_CLONE, ERR_PR_CREATE } from '../utils/errors.js';

/**
 * Génère un JWT signé RS256 pour GitHub App
 * Valide 10 minutes max (GitHub impose ≤10 min)
 * @param {string|number} appId
 * @param {string} privateKey - PEM RSA 2048
 * @returns {string} JWT signé
 */
function createAppJwt(appId, privateKey) {
  const now = Math.floor(Date.now() / 1000);
  const header  = Buffer.from(JSON.stringify({ alg: 'RS256', typ: 'JWT' })).toString('base64url');
  const payload = Buffer.from(JSON.stringify({
    iat: now - 60,        // 60s de tolérance horloge
    exp: now + 600,       // 10 min max autorisé par GitHub
    iss: String(appId),
  })).toString('base64url');
  const signingInput = `${header}.${payload}`;
  const signer = createSign('RSA-SHA256');
  signer.update(signingInput);
  const sig = signer.sign(privateKey, 'base64url');
  return `${signingInput}.${sig}`;
}

export class GitHubService {
  constructor(accessToken = null) {
    this.accessToken = accessToken;
    this.octokit = accessToken ? new Octokit({ auth: accessToken }) : null;
  }

  /**
   * Retourne l'URL d'autorisation OAuth GitHub
   * CF-002: OAuth GitHub fonctionne
   */
  getAuthUrl(state = '') {
    const params = new URLSearchParams({
      client_id: config.github.clientId,
      redirect_uri: config.github.callbackUrl,
      scope: config.github.oauthScopes.join(' '),
      state: state || Math.random().toString(36).slice(2),
    });
    return `https://github.com/login/oauth/authorize?${params}`;
  }

  /**
   * Échange un code OAuth contre un token d'accès
   * @param {string} code - Code OAuth GitHub
   * @returns {Promise<string>} Token d'accès
   */
  async exchangeCode(code) {
    const url = 'https://github.com/login/oauth/access_token';
    const body = {
      client_id: config.github.clientId,
      client_secret: config.github.clientSecret,
      code,
      redirect_uri: config.github.callbackUrl,
    };

    const resp = await fetch(url, {
      method: 'POST',
      headers: { 'Accept': 'application/json', 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });

    if (!resp.ok) throw new MdbaiError(ERR_GITHUB_AUTH, `GitHub OAuth erreur HTTP ${resp.status}`);

    const data = await resp.json();
    if (data.error) throw new MdbaiError(ERR_GITHUB_AUTH, `GitHub OAuth: ${data.error_description}`);
    logger.info('[GITHUB] Token OAuth obtenu');
    return data.access_token;
  }

  /**
   * Retourne les informations d'un dépôt
   * @param {string} owner
   * @param {string} repo
   * @returns {Promise<object>}
   */
  async getRepoInfo(owner, repo) {
    if (!this.octokit) throw new MdbaiError(ERR_GITHUB_AUTH, 'Octokit non initialisé');
    const { data } = await this.octokit.repos.get({ owner, repo });
    return {
      name: data.name, owner: data.owner.login, url: data.html_url,
      default_branch: data.default_branch, language: data.language,
      private: data.private, size_kb: data.size, description: data.description,
    };
  }

  /**
   * Clone un dépôt GitHub dans un répertoire temporaire
   * CF-004: Repo cloné
   * @param {string} repoUrl
   * @param {string} token
   * @param {string} destDir
   * @param {string} [branch]
   */
  async cloneRepository(repoUrl, token, destDir, branch = 'main') {
    mkdirSync(destDir, { recursive: true });
    const parsed = parseGitHubUrl(repoUrl);
    if (!parsed) throw new MdbaiError(ERR_REPO_CLONE, `URL GitHub invalide: ${repoUrl}`);

    const authUrl = token
      ? repoUrl.replace('https://', `https://x-access-token:${token}@`)
      : repoUrl;

    logger.info(`[GITHUB] Clone ${parsed.owner}/${parsed.repo} → ${destDir}`);
    const gitEnv = {
      ...process.env,
      GIT_TERMINAL_PROMPT: '0',
      GIT_ASKPASS: 'echo',
      GIT_SSH_COMMAND: 'ssh -o StrictHostKeyChecking=no -o BatchMode=yes',
    };
    try {
      execFileSync('git', ['clone', '--depth=1', `--branch=${branch}`, authUrl, destDir], {
        timeout: 120000, stdio: 'pipe', env: gitEnv,
      });
    } catch (shallowErr) {
      logger.warn(`[GITHUB] Clone branch=${branch} échoué, tentative sans --branch`);
      try {
        execFileSync('git', ['clone', '--depth=1', authUrl, destDir], {
          timeout: 120000, stdio: 'pipe', env: gitEnv,
        });
      } catch (e) {
        throw new MdbaiError(ERR_REPO_CLONE, `Clone échoué: ${e.message}`);
      }
    }
    logger.info(`[GITHUB] Clone réussi ✅ — ${destDir}`);
  }

  /**
   * Crée une branche d'analyse
   * Préfixe: mdbai-analysis-{timestamp}
   * @returns {Promise<string>} Nom de la branche créée
   */
  async createAnalysisBranch(owner, repo, token, timestamp = Date.now()) {
    const octokit = new Octokit({ auth: token });
    const branchName = `${config.pr.branchPrefix}${timestamp}`;
    logger.info(`[GITHUB] Création branche ${branchName}`);

    const { data: refData } = await octokit.git.getRef({
      owner, repo, ref: 'heads/main',
    }).catch(() => octokit.git.getRef({ owner, repo, ref: 'heads/master' }));

    await octokit.git.createRef({
      owner, repo,
      ref: `refs/heads/${branchName}`,
      sha: refData.object.sha,
    });

    logger.info(`[GITHUB] Branche créée: ${branchName} ✅`);
    return branchName;
  }

  /**
   * Commit le rapport Markdown sur la branche d'analyse
   * @param {string} owner
   * @param {string} repo
   * @param {string} branch
   * @param {string} token
   * @param {string} markdownContent
   * @param {string} [filename]
   * @returns {Promise<string>} SHA du commit
   */
  async commitReport(owner, repo, branch, token, markdownContent, filename = null) {
    const octokit = new Octokit({ auth: token });
    const ts = new Date().toISOString().replace(/[:.]/g, '').slice(0, 15);
    const reportFile = filename || `${config.pr.reportFilename}${ts}.md`;
    const path = `mdbai-reports/${reportFile}`;

    logger.info(`[GITHUB] Commit rapport → ${path} sur ${branch}`);

    const content = Buffer.from(markdownContent).toString('base64');
    let sha = undefined;
    try {
      const { data } = await octokit.repos.getContent({ owner, repo, path, ref: branch });
      sha = data.sha;
    } catch {}

    const { data } = await octokit.repos.createOrUpdateFileContents({
      owner, repo, path, branch,
      message: `🤖 [MDBAI] Rapport d'analyse forensique automatique`,
      content, sha,
    });

    logger.info(`[GITHUB] Commit réussi SHA=${data.commit.sha}`);
    return data.commit.sha;
  }

  /**
   * Crée une Pull Request avec le rapport
   * CF-011: PR créé
   * @returns {Promise<{url: string, number: number}>}
   */
  async createPullRequest(owner, repo, branch, token, reportBody, result) {
    const octokit = new Octokit({ auth: token });
    const score = result?.analysis?.score || 0;
    const emoji = score >= 80 ? '🟢' : score >= 60 ? '🟡' : '🔴';
    const title = `${emoji} [MDBAI] Analyse forensique — Score ${score}/100`;

    logger.info(`[GITHUB] Création PR: ${title}`);
    const { data } = await octokit.pulls.create({
      owner, repo, head: branch, base: 'main',
      title, body: reportBody, draft: false,
    }).catch(() => octokit.pulls.create({
      owner, repo, head: branch, base: 'master',
      title, body: reportBody, draft: false,
    }));

    logger.info(`[GITHUB] PR créée #${data.number} ✅ — ${data.html_url}`);
    return { url: data.html_url, number: data.number };
  }

  /**
   * Applique les labels MDBAI sur une PR
   * @param {string} owner
   * @param {string} repo
   * @param {number} prNumber
   * @param {string} token
   */
  async addPrLabels(owner, repo, prNumber, token) {
    const octokit = new Octokit({ auth: token });
    const labels = config.pr.labels;
    logger.info(`[GITHUB] Ajout labels PR #${prNumber}: ${labels.join(', ')}`);

    for (const label of labels) {
      try {
        await octokit.issues.createLabel({
          owner, repo, name: label,
          color: label === 'mdbai' ? '0075ca' :
                 label === 'forensic' ? 'e4e669' : 'd4c5f9',
        });
      } catch {}
    }

    await octokit.issues.addLabels({
      owner, repo, issue_number: prNumber, labels,
    });
    logger.info(`[GITHUB] Labels appliqués ✅`);
  }

  /**
   * Obtient un token d'installation GitHub App pour un dépôt donné
   * Utilisé quand l'utilisateur n'a pas fourni de token OAuth
   * Nécessite GITHUB_APP_ID + GITHUB_PRIVATE_KEY dans l'env
   *
   * Flow :
   *  1. Génère JWT App RS256 (valide 10 min)
   *  2. GET /repos/{owner}/{repo}/installation → installation_id
   *  3. POST /app/installations/{id}/access_tokens → token (valide 1h)
   *
   * @param {string} owner
   * @param {string} repo
   * @returns {Promise<string>} Token d'installation (valide 1h)
   */
  async getInstallationToken(owner, repo) {
    const appId     = config.github.appId;
    const privateKey = config.github.privateKey;

    if (!appId || !privateKey) {
      throw new MdbaiError(ERR_GITHUB_AUTH,
        'GITHUB_APP_ID et GITHUB_PRIVATE_KEY requis pour getInstallationToken');
    }

    logger.info(`[GITHUB] Obtention installation token pour ${owner}/${repo}`);

    const appJwt = createAppJwt(appId, privateKey);
    const appOctokit = new Octokit({ auth: appJwt });

    const { data: installation } = await appOctokit.apps.getRepoInstallation({ owner, repo });
    logger.info(`[GITHUB] Installation ID: ${installation.id} pour ${owner}/${repo}`);

    const { data: tokenData } = await appOctokit.apps.createInstallationAccessToken({
      installation_id: installation.id,
    });

    logger.info(`[GITHUB] Token d'installation obtenu ✅ — expire: ${tokenData.expires_at}`);
    return tokenData.token;
  }

  /**
   * Vérifie si la GitHub App est installée sur un dépôt donné
   * @returns {Promise<boolean>}
   */
  async isAppInstalledOn(owner, repo) {
    try {
      await this.getInstallationToken(owner, repo);
      return true;
    } catch {
      return false;
    }
  }

  /**
   * Nettoie un répertoire temporaire d'analyse
   */
  cleanup(dir) {
    if (existsSync(dir)) {
      rmSync(dir, { recursive: true, force: true });
      logger.debug(`[GITHUB] Nettoyage dir: ${dir}`);
    }
  }
}

export default GitHubService;
