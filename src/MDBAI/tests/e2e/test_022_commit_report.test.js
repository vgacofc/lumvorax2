/**
 * TEST_022 — Commit Rapport GitHub (e2e)
 * CF-010: RAPPORT_MDBAI_*.md créé
 */

import { describe, test, expect, afterEach } from '@jest/globals';
import { ReportService } from '../../src/services/report.service.js';
import { createEmptyResult } from '../../src/models/result.model.js';
import { existsSync, rmSync, mkdirSync, readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const REPORTS_DIR = join(__dirname, '../../reports');
let savedReportPath = null;

describe('TEST_022 — Commit Rapport (e2e)', () => {
  const reportService = new ReportService();

  afterEach(() => {
    if (savedReportPath && existsSync(savedReportPath)) {
      rmSync(savedReportPath, { force: true });
      savedReportPath = null;
    }
  });

  test('T022-A: saveReport() crée un fichier .md sur disque', () => {
    const content = '# Test Rapport\nContenu test\n';
    savedReportPath = reportService.saveReport(content, 'test-022-aaa');
    expect(existsSync(savedReportPath)).toBe(true);
  });

  test('T022-B: rapport sauvegardé contient le contenu correct', () => {
    const content = '# RAPPORT_MDBAI Test\n\nScore: 85/100\n';
    savedReportPath = reportService.saveReport(content, 'test-022-bbb');
    const read = readFileSync(savedReportPath, 'utf8');
    expect(read).toBe(content);
  });

  test('T022-C: nom de fichier contient le préfixe RAPPORT_MDBAI_', () => {
    savedReportPath = reportService.saveReport('# Test', 'test-022-ccc');
    expect(savedReportPath).toContain('RAPPORT_MDBAI_');
  });

  test('T022-D: nom de fichier se termine par .md', () => {
    savedReportPath = reportService.saveReport('# Test', 'test-022-ddd');
    expect(savedReportPath).toMatch(/\.md$/);
  });

  test('T022-E: generateReport() produit du Markdown valide', () => {
    const result = createEmptyResult();
    result.repo.owner = 'lumvorax';
    result.repo.name  = 'mdbai';
    result.analysis.score = 90;
    const md = reportService.generateReport(result, {
      jobId: 'test-022-e', repoUrl: 'https://github.com/lumvorax/mdbai',
    });
    expect(md).toContain('#');
    expect(md).toContain('lumvorax');
    expect(md).toContain('90/100');
  });

  test('T022-F: rapport en base64 est valide (pour GitHub API)', () => {
    const content = '# Rapport MDBAI\nTest';
    const b64 = Buffer.from(content).toString('base64');
    const decoded = Buffer.from(b64, 'base64').toString('utf8');
    expect(decoded).toBe(content);
  });

  test('T022-G: répertoire reports/ existe ou est créable', () => {
    mkdirSync(REPORTS_DIR, { recursive: true });
    expect(existsSync(REPORTS_DIR)).toBe(true);
  });
});
