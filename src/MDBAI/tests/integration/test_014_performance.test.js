/**
 * TEST_014 — Analyse Performance (intégration)
 * CNF-001: Temps analyse < 5 min
 */

import { describe, test, expect } from '@jest/globals';
import { AnalysisService } from '../../src/services/analysis.service.js';
import { ReportService } from '../../src/services/report.service.js';
import { createEmptyResult, calculateScore } from '../../src/models/result.model.js';
import { config } from '../../src/config.js';

describe('TEST_014 — Analyse Performance', () => {
  const service = new AnalysisService('test-014');
  const reportService = new ReportService();

  test('T014-A: analyzePerformance() retourne un objet valide', () => {
    const forensicData = {
      cpu_percent: 42.5, memory_bytes: 125 * 1024 * 1024,
      duration_ms: 12500, lum_snapshots: [{ file: 'test.lum', size_bytes: 4096 }],
    };
    const perf = service.analyzePerformance(forensicData);
    expect(perf).toHaveProperty('cpu_percent');
    expect(perf).toHaveProperty('memory_mb');
    expect(perf).toHaveProperty('duration_ms');
    expect(perf).toHaveProperty('lum_snapshots_count');
  });

  test('T014-B: analyzePerformance() convertit bytes en MB', () => {
    const perf = service.analyzePerformance({ memory_bytes: 200 * 1024 * 1024 });
    expect(perf.memory_mb).toBe(200);
  });

  test('T014-C: analyzePerformance() compte les snapshots LUM', () => {
    const perf = service.analyzePerformance({
      lum_snapshots: [{ file: 'a.lum', size_bytes: 1000 }, { file: 'b.lum', size_bytes: 2000 }],
    });
    expect(perf.lum_snapshots_count).toBe(2);
  });

  test('T014-D: formatPerformance() retourne section Markdown valide', () => {
    const metrics = { cpu_percent: 35, memory_mb: 120, duration_ms: 8500, lum_snapshots_count: 2 };
    const section = reportService.formatPerformance(metrics);
    expect(section).toContain('Performance');
    expect(section).toContain('35%');
    expect(section).toContain('120');
    expect(section).toContain('8500');
  });

  test('T014-E: formatPerformance() avec valeurs nulles → pas de crash', () => {
    expect(() => reportService.formatPerformance({})).not.toThrow();
  });

  test('T014-F: timeout configuré à 300s (5 min)', () => {
    expect(config.analysis.execTimeoutMs).toBe(300000);
  });

  test('T014-G: calculateScore() avec perf normale → score entre 0 et 100', () => {
    const r = createEmptyResult();
    r.forensic.performance = { cpu_percent: 80, memory_mb: 500 };
    const score = calculateScore(r);
    expect(score).toBeGreaterThanOrEqual(0);
    expect(score).toBeLessThanOrEqual(100);
  });

  test('T014-H: job timeout = 600000ms (10 min)', () => {
    expect(config.jobs.timeoutMs).toBe(600000);
  });
});
