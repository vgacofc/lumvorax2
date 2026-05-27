/**
 * TEST_013 — Scan Vulnérabilités (intégration)
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect } from '@jest/globals';
import { VULN_SEVERITY_LOW, VULN_SEVERITY_MEDIUM,
  VULN_SEVERITY_HIGH, VULN_SEVERITY_CRITICAL,
  createEmptyResult, calculateScore } from '../../src/models/result.model.js';
import { ReportService } from '../../src/services/report.service.js';

describe('TEST_013 — Scan Vulnérabilités', () => {
  const reportService = new ReportService();

  const MOCK_VULNS = [
    { package: 'lodash', version: '<4.17.21', severity: VULN_SEVERITY_CRITICAL,
      title: 'Prototype Pollution', cve: 'https://nvd.nist.gov/vuln/detail/CVE-2021-23337' },
    { package: 'axios', version: '<1.6.0', severity: VULN_SEVERITY_HIGH,
      title: 'SSRF in axios', cve: 'CVE-2023-45857' },
    { package: 'semver', version: '<7.5.2', severity: VULN_SEVERITY_MEDIUM,
      title: 'ReDoS in semver', cve: null },
    { package: 'debug', version: '2.6.9', severity: VULN_SEVERITY_LOW,
      title: 'RegEx DoS', cve: null },
  ];

  test('T013-A: constantes de sévérité définies correctement', () => {
    expect(VULN_SEVERITY_LOW).toBe('low');
    expect(VULN_SEVERITY_MEDIUM).toBe('medium');
    expect(VULN_SEVERITY_HIGH).toBe('high');
    expect(VULN_SEVERITY_CRITICAL).toBe('critical');
  });

  test('T013-B: formatVulnerabilities() liste toutes les vulns', () => {
    const section = reportService.formatVulnerabilities(MOCK_VULNS);
    expect(section).toContain('lodash');
    expect(section).toContain('axios');
    expect(section).toContain('semver');
    expect(section).toContain('debug');
  });

  test('T013-C: formatVulnerabilities() indique le count correct', () => {
    const section = reportService.formatVulnerabilities(MOCK_VULNS);
    expect(section).toContain('4');
  });

  test('T013-D: formatVulnerabilities() indique critical count', () => {
    const section = reportService.formatVulnerabilities(MOCK_VULNS);
    expect(section).toContain('critical');
  });

  test('T013-E: formatVulnerabilities() 0 vulns → message "Aucune vulnérabilité"', () => {
    const section = reportService.formatVulnerabilities([]);
    expect(section).toContain('Aucune vulnérabilité');
  });

  test('T013-F: vulnérabilité avec CVE URL contient un lien', () => {
    const section = reportService.formatVulnerabilities([MOCK_VULNS[0]]);
    expect(section).toContain('nvd.nist.gov');
  });

  test('T013-G: calculateScore pénalise les critical (-25pts)', () => {
    const r = createEmptyResult();
    r.analysis.vulnerabilities = [{ severity: VULN_SEVERITY_CRITICAL }];
    const score = calculateScore(r);
    expect(score).toBeLessThanOrEqual(75);
  });

  test('T013-H: formatVulnerabilities limite à 20 entrées affichées', () => {
    const manyVulns = Array(30).fill(null).map((_, i) => ({
      package: `pkg${i}`, version: '1.0.0',
      severity: VULN_SEVERITY_LOW, title: `Issue ${i}`, cve: null,
    }));
    const section = reportService.formatVulnerabilities(manyVulns);
    expect(section).not.toContain('pkg25');
  });
});
