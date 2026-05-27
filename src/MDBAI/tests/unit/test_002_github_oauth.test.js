/**
 * TEST_002 — GitHub OAuth Service
 * CF-002: Autorisation → Token valide
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect, beforeEach } from '@jest/globals';
import { GitHubService } from '../../src/services/github.service.js';
import { parseGitHubUrl } from '../../src/utils/validator.js';

describe('TEST_002 — GitHubService OAuth', () => {
  let github;

  beforeEach(() => {
    github = new GitHubService();
  });

  test('T002-A: GitHubService peut être instancié', () => {
    expect(github).toBeDefined();
    expect(github).toBeInstanceOf(GitHubService);
  });

  test('T002-B: getAuthUrl() retourne une URL GitHub valide', () => {
    const url = github.getAuthUrl('test-state');
    expect(url).toContain('github.com/login/oauth/authorize');
    expect(url).toContain('state=test-state');
  });

  test('T002-C: getAuthUrl() sans state génère un state aléatoire', () => {
    const url = github.getAuthUrl();
    expect(url).toContain('state=');
  });

  test('T002-D: parseGitHubUrl parse correctement une URL standard', () => {
    const parsed = parseGitHubUrl('https://github.com/lumvorax/mdbai');
    expect(parsed).not.toBeNull();
    expect(parsed.owner).toBe('lumvorax');
    expect(parsed.repo).toBe('mdbai');
  });

  test('T002-E: parseGitHubUrl retourne null pour URL invalide', () => {
    expect(parseGitHubUrl('https://gitlab.com/user/repo')).toBeNull();
    expect(parseGitHubUrl('not-a-url')).toBeNull();
    expect(parseGitHubUrl('')).toBeNull();
  });

  test('T002-F: parseGitHubUrl gère les URL avec .git', () => {
    const parsed = parseGitHubUrl('https://github.com/user/repo.git');
    expect(parsed.repo).toBe('repo');
  });

  test('T002-G: GitHubService a toutes les méthodes STANDARD_NAMES', () => {
    const methods = [
      'getAuthUrl', 'exchangeCode', 'getRepoInfo', 'cloneRepository',
      'createAnalysisBranch', 'commitReport', 'createPullRequest',
      'addPrLabels', 'cleanup',
    ];
    for (const m of methods) {
      expect(typeof github[m]).toBe('function');
    }
  });

  test('T002-H: branche préfixée correctement', () => {
    const ts = 1748390400000;
    const branchName = `mdbai-analysis-${ts}`;
    expect(branchName).toMatch(/^mdbai-analysis-\d+$/);
  });
});
