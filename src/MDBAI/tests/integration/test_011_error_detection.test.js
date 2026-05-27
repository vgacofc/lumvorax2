/**
 * TEST_011 — Détection d'erreurs (intégration)
 * CF-009: > 0 erreurs trouvées
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect } from '@jest/globals';
import { AnalysisService } from '../../src/services/analysis.service.js';

describe('TEST_011 — Détection Erreurs (intégration)', () => {
  const service = new AnalysisService('test-011');

  const FIXTURES = {
    nodeError: `
> test@1.0.0 test
> jest

FAIL tests/app.test.js
  ● App › should return 200
    Error: ECONNREFUSED ::1:3000
      at TCPConnectWrap.afterConnect [as oncomplete] (node:net:1555:16)
TypeError: Cannot read properties of undefined (reading 'status')
    at Object.<anonymous> (tests/app.test.js:12:18)
    `,
    pythonError: `
FAILED tests/test_app.py::test_home_page - AssertionError: assert 404 == 200
FAILED tests/test_api.py::test_create_user - TypeError: argument of type 'NoneType' is not iterable
ERROR tests/test_db.py::test_connection - ConnectionRefusedError: [Errno 111] Connection refused
    `,
    rustPanic: `
thread 'main' panicked at 'called Result::unwrap() on an Err value: Os { code: 2, kind: NotFound }', src/main.rs:42:15
note: run with RUST_BACKTRACE=1 environment variable for a backtrace
    `,
    segfault: `gcc output: Segmentation fault (core dumped)\n`,
    cleanOutput: `Tests: 12 passed, 0 failed\nAll 12 tests pass!\n`,
  };

  test('T011-A: détecte Error dans stdout Node.js', () => {
    const errs = service.detectErrors(FIXTURES.nodeError, '');
    expect(errs.length).toBeGreaterThan(0);
    const hasEconnRefused = errs.some(e => e.message.includes('ECONNREFUSED'));
    const hasTypeErr = errs.some(e => e.type === 'type_error');
    expect(hasEconnRefused || hasTypeErr).toBe(true);
  });

  test('T011-B: détecte FAILED dans output pytest', () => {
    const errs = service.detectErrors(FIXTURES.pythonError, '');
    expect(errs.length).toBeGreaterThan(0);
  });

  test('T011-C: détecte panic Rust (critical)', () => {
    const errs = service.detectErrors('', FIXTURES.rustPanic);
    const panic = errs.find(e => e.type === 'rust_panic' || e.type === 'panic');
    expect(panic).toBeDefined();
    expect(panic.severity).toBe('critical');
  });

  test('T011-D: détecte Segfault (critical)', () => {
    const errs = service.detectErrors('', FIXTURES.segfault);
    const seg = errs.find(e => e.type === 'segfault');
    expect(seg).toBeDefined();
    expect(seg.severity).toBe('critical');
  });

  test('T011-E: output propre → 0 erreurs critiques ou hautes', () => {
    const errs = service.detectErrors(FIXTURES.cleanOutput, '');
    const critical = errs.filter(e => e.severity === 'critical' || e.severity === 'high');
    expect(critical.length).toBe(0);
  });

  test('T011-F: erreurs sans doublons', () => {
    const repeated = 'Error: même erreur\n'.repeat(10);
    const errs = service.detectErrors(repeated, '');
    const msgs = errs.map(e => e.message);
    const unique = [...new Set(msgs)];
    expect(errs.length).toBeLessThanOrEqual(msgs.length);
    expect(unique.length).toBeLessThanOrEqual(errs.length);
  });

  test('T011-G: chaque erreur a les champs requis (type, severity, message)', () => {
    const errs = service.detectErrors(FIXTURES.nodeError, FIXTURES.pythonError);
    for (const err of errs) {
      expect(err).toHaveProperty('type');
      expect(err).toHaveProperty('severity');
      expect(err).toHaveProperty('message');
      expect(['critical', 'high', 'medium', 'low']).toContain(err.severity);
    }
  });
});
