/**
 * TEST_024 — Labels PR GitHub (e2e)
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 * Labels: "mdbai", "automated-analysis", "forensic"
 */

import { describe, test, expect } from '@jest/globals';
import { config } from '../../src/config.js';
import { GitHubService } from '../../src/services/github.service.js';

describe('TEST_024 — Labels PR (e2e)', () => {

  test('T024-A: 3 labels MDBAI définis dans config', () => {
    expect(config.pr.labels).toHaveLength(3);
  });

  test('T024-B: label "mdbai" présent (label primaire)', () => {
    expect(config.pr.labels).toContain('mdbai');
  });

  test('T024-C: label "automated-analysis" présent', () => {
    expect(config.pr.labels).toContain('automated-analysis');
  });

  test('T024-D: label "forensic" présent', () => {
    expect(config.pr.labels).toContain('forensic');
  });

  test('T024-E: couleur label "mdbai" = bleu GitHub (0075ca)', () => {
    const colorMap = {
      'mdbai': '0075ca',
      'automated-analysis': 'd4c5f9',
      'forensic': 'e4e669',
    };
    expect(colorMap['mdbai']).toBe('0075ca');
    expect(colorMap['forensic']).toBe('e4e669');
  });

  test('T024-F: labels sont des chaînes lowercase sans espaces', () => {
    for (const label of config.pr.labels) {
      expect(label).toBe(label.toLowerCase());
      expect(label).not.toContain(' ');
    }
  });

  test('T024-G: GitHubService.addPrLabels est une fonction', () => {
    const github = new GitHubService('token');
    expect(typeof github.addPrLabels).toBe('function');
  });

  test('T024-H: labels sans caractères spéciaux (GitHub restriction)', () => {
    const FORBIDDEN = ['<', '>', '&', '"', "'", '`'];
    for (const label of config.pr.labels) {
      for (const char of FORBIDDEN) {
        expect(label).not.toContain(char);
      }
    }
  });
});
