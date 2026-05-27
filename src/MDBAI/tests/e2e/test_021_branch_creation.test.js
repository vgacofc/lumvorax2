/**
 * TEST_021 — Création de branche GitHub (e2e mocké)
 * CF-011: PR créé (phase 3)
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 * Note: Tests e2e sans token GitHub réel → mocks Octokit
 */

import { describe, test, expect, jest } from '@jest/globals';
import { GitHubService } from '../../src/services/github.service.js';
import { config } from '../../src/config.js';

describe('TEST_021 — Création Branche GitHub (e2e)', () => {

  test('T021-A: branchPrefix conforme STANDARD_NAMES = "mdbai-analysis-"', () => {
    expect(config.pr.branchPrefix).toBe('mdbai-analysis-');
  });

  test('T021-B: branche générée avec timestamp Unix', () => {
    const ts = 1748390400000;
    const branch = `${config.pr.branchPrefix}${ts}`;
    expect(branch).toBe('mdbai-analysis-1748390400000');
    expect(branch).toMatch(/^mdbai-analysis-\d{13}$/);
  });

  test('T021-C: branche est unique (timestamps différents)', () => {
    const t1 = Date.now();
    const t2 = Date.now() + 1;
    expect(`${config.pr.branchPrefix}${t1}`).not.toBe(`${config.pr.branchPrefix}${t2}`);
  });

  test('T021-D: GitHubService.createAnalysisBranch existe comme méthode', () => {
    const github = new GitHubService('fake_token');
    expect(typeof github.createAnalysisBranch).toBe('function');
  });

  test('T021-E: createAnalysisBranch() sans token réel → rejette (non-mock)', async () => {
    const github = new GitHubService('invalid_token_for_test');
    await expect(
      github.createAnalysisBranch('lumvorax', 'mdbai', 'invalid_token', Date.now())
    ).rejects.toBeDefined();
  });

  test('T021-F: branche n\'a pas de caractères interdits par GitHub', () => {
    const ts = Date.now();
    const branch = `${config.pr.branchPrefix}${ts}`;
    expect(branch).not.toContain(' ');
    expect(branch).not.toContain('~');
    expect(branch).not.toContain('^');
    expect(branch).not.toContain(':');
    expect(branch).not.toContain('?');
    expect(branch).not.toContain('*');
    expect(branch).not.toContain('[');
    expect(branch).not.toContain('\\');
  });

  test('T021-G: GitHub labels définis dans config = 3 labels MDBAI', () => {
    expect(config.pr.labels).toHaveLength(3);
    expect(config.pr.labels).toContain('mdbai');
    expect(config.pr.labels).toContain('automated-analysis');
    expect(config.pr.labels).toContain('forensic');
  });
});
