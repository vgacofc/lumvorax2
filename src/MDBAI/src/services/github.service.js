/**
 * MDBAI — Service GitHub (API + OAuth + Git)
 * Conforme STANDARD_NAMES_MDBAI.md Section 5 (getAuthUrl, cloneRepository...)
 * Utilise @octokit/rest + exec git — credentials via env (jamais en dur)
 */

import { Octokit } from '@octokit/rest';
import { execFileSync, execSync } from 'child_process';
import { mkdirSync, rmSync, existsSync, writeFileSync } from 'fs';
import { join } from 'path';
import logger from '../utils/logger.js';
import { config } from '../config.js';
import { parseGitHubUrl } from '../utils/validator.js';
import { MdbaiError, ERR_GITHUB_AUTH, ERR_REPO_CLONE, ERR_PR_CREATE } from '../utils/errors.js';

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
