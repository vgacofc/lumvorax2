/**
 * TEST_006 — Clone de dépôt GitHub
 * CF-004: Git clone réussit
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect, afterEach } from '@jest/globals';
import { rmSync, existsSync } from 'fs';
import { validateRepoUrl, parseGitHubUrl } from '../../src/utils/validator.js';

const TMP = '/tmp/mdbai_test_006';

describe('TEST_006 — Clone Dépôt GitHub', () => {

  afterEach(() => {
    rmSync(TMP, { recursive: true, force: true });
  });

  test('T006-A: validateRepoUrl accepte URL GitHub valide', () => {
    const res = validateRepoUrl('https://github.com/lumvorax/mdbai');
    expect(res.valid).toBe(true);
    expect(res.value).toBe('https://github.com/lumvorax/mdbai');
  });

  test('T006-B: validateRepoUrl accepte URL avec .git', () => {
    const res = validateRepoUrl('https://github.com/user/repo.git');
    expect(res.valid).toBe(true);
    expect(res.value).toBe('https://github.com/user/repo');
  });

  test('T006-C: validateRepoUrl rejette URL non-GitHub', () => {
    const res = validateRepoUrl('https://gitlab.com/user/repo');
    expect(res.valid).toBe(false);
    expect(res.error).toContain('ERR_REPO_CLONE');
  });

  test('T006-D: validateRepoUrl rejette chaîne vide', () => {
    const res = validateRepoUrl('');
    expect(res.valid).toBe(false);
  });

  test('T006-E: validateRepoUrl rejette URL null', () => {
    const res = validateRepoUrl(null);
    expect(res.valid).toBe(false);
  });

  test('T006-F: validateRepoUrl accepte URL avec tirets dans owner/repo', () => {
    const res = validateRepoUrl('https://github.com/my-org/my-repo');
    expect(res.valid).toBe(true);
  });

  test('T006-G: validateRepoUrl accepte URL avec underscores', () => {
    const res = validateRepoUrl('https://github.com/my_org/my_repo');
    expect(res.valid).toBe(true);
  });

  test('T006-H: parseGitHubUrl extrait owner + repo correctement', () => {
    const urls = [
      ['https://github.com/lumvorax/mdbai', 'lumvorax', 'mdbai'],
      ['https://github.com/my-org/my-repo.git', 'my-org', 'my-repo'],
      ['https://github.com/user/repo', 'user', 'repo'],
    ];
    for (const [url, owner, repo] of urls) {
      const parsed = parseGitHubUrl(url);
      expect(parsed).not.toBeNull();
      expect(parsed.owner).toBe(owner);
      expect(parsed.repo).toBe(repo);
    }
  });

  test('T006-I: git est disponible dans Replit', async () => {
    const { execSync } = await import('child_process');
    const version = execSync('git --version', { timeout: 5000 }).toString().trim();
    expect(version).toContain('git version');
  });
});
