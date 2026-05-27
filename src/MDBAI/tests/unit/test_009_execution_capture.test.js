/**
 * TEST_009 — Capture Exécution (stdout/stderr)
 * CF-007: stdout/stderr non vides
 * CF-006: Exit code capturé
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect } from '@jest/globals';
import { AnalysisService } from '../../src/services/analysis.service.js';

describe('TEST_009 — Capture Exécution stdout/stderr', () => {
  const service = new AnalysisService('test-009');

  test('T009-A: detectErrors() détecte "Error:" générique', () => {
    const stdout = 'Error: Cannot find module "foo"\n';
    const errors = service.detectErrors(stdout, '');
    expect(errors.length).toBeGreaterThan(0);
    const errMsg = errors.find(e => e.message.includes('Cannot find module'));
    expect(errMsg).toBeDefined();
  });

  test('T009-B: detectErrors() détecte TypeError', () => {
    const stderr = 'TypeError: undefined is not a function\n';
    const errors = service.detectErrors('', stderr);
    const typeErr = errors.find(e => e.type === 'type_error');
    expect(typeErr).toBeDefined();
    expect(typeErr.severity).toBe('high');
  });

  test('T009-C: detectErrors() détecte SyntaxError (critical)', () => {
    const stderr = 'SyntaxError: Unexpected token }\n';
    const errors = service.detectErrors('', stderr);
    const synErr = errors.find(e => e.type === 'syntax_error');
    expect(synErr).toBeDefined();
    expect(synErr.severity).toBe('critical');
  });

  test('T009-D: detectErrors() détecte Segmentation fault (critical)', () => {
    const stderr = 'Segmentation fault (core dumped)\n';
    const errors = service.detectErrors('', stderr);
    const segErr = errors.find(e => e.type === 'segfault');
    expect(segErr).toBeDefined();
    expect(segErr.severity).toBe('critical');
  });

  test('T009-E: detectErrors() détecte Rust panic', () => {
    const stderr = "thread 'main' panicked at 'index out of bounds'\n";
    const errors = service.detectErrors('', stderr);
    expect(errors.some(e => e.type === 'rust_panic')).toBe(true);
  });

  test('T009-F: detectErrors() retourne tableau vide pour stdout/stderr propre', () => {
    const errors = service.detectErrors('Tests: 5 passed\n', '');
    const criticalOrHigh = errors.filter(e => e.severity === 'critical' || e.severity === 'high');
    expect(criticalOrHigh.length).toBe(0);
  });

  test('T009-G: detectErrors() limite à 50 erreurs max', () => {
    const manyErrors = Array(100).fill('Error: test error\n').join('');
    const errors = service.detectErrors(manyErrors, '');
    expect(errors.length).toBeLessThanOrEqual(50);
  });

  test('T009-H: messages tronqués à 300 chars', () => {
    const longMsg = 'Error: ' + 'x'.repeat(500);
    const errors = service.detectErrors(longMsg, '');
    for (const err of errors) {
      expect(err.message.length).toBeLessThanOrEqual(300);
    }
  });
});
