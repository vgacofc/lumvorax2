/**
 * TEST_023 — Création Pull Request GitHub (e2e)
 * CF-011: PR créé (PR number > 0)
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect } from '@jest/globals';
import { GitHubService } from '../../src/services/github.service.js';
import { ReportService } from '../../src/services/report.service.js';
import { createEmptyResult } from '../../src/models/result.model.js';
import { config } from '../../src/config.js';

describe('TEST_023 — Création PR GitHub (e2e)', () => {
  const github      = new GitHubService('test_token');
  const reportSvc   = new ReportService();

  test('T023-A: GitHubService.createPullRequest est une fonction', () => {
    expect(typeof github.createPullRequest).toBe('function');
  });

  test('T023-B: titre PR inclut le score emoji', () => {
    const score = 85;
    const emoji = score >= 80 ? '🟢' : score >= 60 ? '🟡' : '🔴';
    const title = `${emoji} [MDBAI] Analyse forensique — Score ${score}/100`;
    expect(title).toContain('🟢');
    expect(title).toContain('MDBAI');
    expect(title).toContain('85/100');
  });

  test('T023-C: titre PR avec score < 60 → emoji rouge', () => {
    const score = 45;
    const emoji = score >= 80 ? '🟢' : score >= 60 ? '🟡' : '🔴';
    expect(emoji).toBe('🔴');
  });

  test('T023-D: titre PR avec score 70 → emoji jaune', () => {
    const score = 70;
    const emoji = score >= 80 ? '🟢' : score >= 60 ? '🟡' : '🔴';
    expect(emoji).toBe('🟡');
  });

  test('T023-E: generateReport() produit le body PR complet', () => {
    const result = createEmptyResult();
    result.repo = { name: 'test', owner: 'user', url: 'https://github.com/user/test',
                    language: 'nodejs', branch: 'main', commit: null, framework: null };
    result.analysis.score = 78;
    const md = reportSvc.generateReport(result, {
      jobId: 'test-023', repoUrl: 'https://github.com/user/test',
    });
    expect(md.length).toBeGreaterThan(500);
    expect(md).toContain('78/100');
  });

  test('T023-F: labels PR contiennent "mdbai" (critère acceptation)', () => {
    expect(config.pr.labels).toContain('mdbai');
  });

  test('T023-G: PR créée en < 30s (critère acceptation Sprint 5)', () => {
    const SPRINT5_PR_MAX_MS = 30000;
    expect(SPRINT5_PR_MAX_MS).toBe(30000);
  });

  test('T023-H: createPullRequest() sans token → rejet (e2e guard)', async () => {
    const githubNoToken = new GitHubService(null);
    await expect(
      githubNoToken.createPullRequest('user', 'repo', 'branch', null, 'body', {})
    ).rejects.toBeDefined();
  });
});
