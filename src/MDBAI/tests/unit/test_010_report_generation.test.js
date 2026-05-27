/**
 * TEST_010 — Génération Rapport Markdown
 * CF-010: RAPPORT_MDBAI_*.md créé
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect } from '@jest/globals';
import { ReportService } from '../../src/services/report.service.js';
import { createEmptyResult, calculateScore } from '../../src/models/result.model.js';

describe('TEST_010 — Génération Rapport Markdown', () => {
  const reportService = new ReportService();

  function makeResult(overrides = {}) {
    const r = createEmptyResult();
    r.repo = { name: 'mdbai', owner: 'lumvorax', url: 'https://github.com/lumvorax/mdbai',
               language: 'nodejs', branch: 'main', commit: null, framework: null };
    r.execution = { duration_ms: 5000, exit_code: 0, stdout: 'Tests: 3 passed', stderr: '', timed_out: false };
    r.analysis.score = 85;
    Object.assign(r, overrides);
    return r;
  }

  test('T010-A: generateReport() retourne une chaîne non vide', () => {
    const result = makeResult();
    const md = reportService.generateReport(result, { jobId: 'test-010', repoUrl: 'https://github.com/lumvorax/mdbai' });
    expect(typeof md).toBe('string');
    expect(md.length).toBeGreaterThan(100);
  });

  test('T010-B: rapport contient le titre MDBAI', () => {
    const result = makeResult();
    const md = reportService.generateReport(result, { jobId: 'test-010' });
    expect(md).toContain('Rapport MDBAI');
  });

  test('T010-C: rapport contient le score', () => {
    const result = makeResult();
    const md = reportService.generateReport(result, { jobId: 'test-010' });
    expect(md).toContain('85/100');
  });

  test('T010-D: rapport contient section Résumé Exécutif', () => {
    const result = makeResult();
    const md = reportService.generateReport(result, { jobId: 'test-010' });
    expect(md).toContain('Résumé Exécutif');
  });

  test('T010-E: rapport contient section LumVorax forensic', () => {
    const result = makeResult();
    const md = reportService.generateReport(result, { jobId: 'test-010' });
    expect(md).toContain('LumVorax');
  });

  test('T010-F: formatErrors() avec 0 erreur → message "Aucune erreur"', () => {
    const section = reportService.formatErrors([]);
    expect(section).toContain('Aucune erreur');
  });

  test('T010-G: formatErrors() avec erreurs critiques → section visible', () => {
    const errors = [
      { type: 'segfault', severity: 'critical', message: 'Segmentation fault', raw: '' },
      { type: 'type_error', severity: 'high', message: 'TypeError: x is null', raw: '' },
    ];
    const section = reportService.formatErrors(errors);
    expect(section.toLowerCase()).toContain('critical');
    expect(section).toContain('Segmentation fault');
  });

  test('T010-H: formatMemoryLeaks() avec 0 fuite → "Aucune fuite"', () => {
    const section = reportService.formatMemoryLeaks([]);
    expect(section).toContain('Aucune fuite');
  });

  test('T010-I: formatVulnerabilities() avec 0 vuln → "Aucune vulnérabilité"', () => {
    const section = reportService.formatVulnerabilities([]);
    expect(section).toContain('Aucune vulnérabilité');
  });

  test('T010-J: calculateScore() part de 100 et descend selon erreurs', () => {
    const result = makeResult();
    result.analysis.errors = [{ severity: 'critical' }];
    result.analysis.score = calculateScore(result);
    expect(result.analysis.score).toBeLessThan(100);
    expect(result.analysis.score).toBeGreaterThanOrEqual(0);
  });

  test('T010-K: rapport contient magic LumVorax "MDBA"', () => {
    const result = makeResult();
    const md = reportService.generateReport(result, { jobId: 'test-010' });
    expect(md).toContain('4D444241');
  });
});
