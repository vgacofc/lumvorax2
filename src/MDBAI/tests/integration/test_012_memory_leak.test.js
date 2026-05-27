/**
 * TEST_012 — Détection Fuites Mémoire (intégration)
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 * Conforme prompt.txt Règle #1 : traçabilité bit-level active
 */

import { describe, test, expect } from '@jest/globals';
import { ForensicBridge } from '../../src/utils/forensic.js';
import { AnalysisService } from '../../src/services/analysis.service.js';

describe('TEST_012 — Détection Fuites Mémoire', () => {
  const service = new AnalysisService('test-012');
  const bridge  = new ForensicBridge('test-012');

  const VALGRIND_OUTPUT = `
==12345== HEAP SUMMARY:
==12345==     in use at exit: 4,096 bytes in 2 blocks
==12345==   total heap usage: 15 allocs, 13 frees, 5,120 bytes allocated
==12345== LEAK SUMMARY:
==12345==    definitely lost: 2,048 bytes in 1 blocks
==12345==    indirectly lost: 1,024 bytes in 1 blocks
==12345==      possibly lost: 512 bytes in 1 blocks
==12345==    still reachable: 64 bytes in 1 blocks
  `;

  const ASAN_OUTPUT = `
=================================================================
==13579==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 256 bytes in 1 object(s) allocated from:
    #0 0x7f4a3b2c1234 in malloc (/lib/x86_64-linux-gnu/libasan.so.6+0xb234)
    #1 0x55f8c7d01abc in allocate_buffer (/app/src/main.c:42)

SUMMARY: AddressSanitizer: 256 byte(s) leaked in 1 allocation(s).
  `;

  const LUM_OUTPUT = `
[LUM-LEAK] addr=0x7ffd12345678 size=128 file=main.c:99
[LUM-LEAK] addr=0x7ffd87654321 size=256 file=utils.c:47
  `;

  test('T012-A: _parseMemoryLeaks() détecte valgrind "definitely lost"', () => {
    const leaks = bridge._parseMemoryLeaks(VALGRIND_OUTPUT);
    expect(leaks.some(l => l.tool === 'valgrind')).toBe(true);
    const valgrindLeak = leaks.find(l => l.tool === 'valgrind');
    expect(valgrindLeak.size_bytes).toBeGreaterThan(0);
  });

  test('T012-B: _parseMemoryLeaks() détecte AddressSanitizer', () => {
    const leaks = bridge._parseMemoryLeaks(ASAN_OUTPUT);
    expect(leaks.some(l => l.tool === 'asan')).toBe(true);
  });

  test('T012-C: _parseMemoryLeaks() détecte format LumVorax [LUM-LEAK]', () => {
    const leaks = bridge._parseMemoryLeaks(LUM_OUTPUT);
    expect(leaks.some(l => l.tool === 'lumvorax')).toBe(true);
    expect(leaks.filter(l => l.tool === 'lumvorax')).toHaveLength(2);
  });

  test('T012-D: detectMemoryLeaks() combine forensicData.memory_leaks + stdout', () => {
    const forensicData = {
      stdout: VALGRIND_OUTPUT,
      stderr: '',
      memory_leaks: [{ tool: 'pre-detected', size_bytes: 64, raw: 'test' }],
    };
    const leaks = service.detectMemoryLeaks(forensicData);
    expect(leaks.length).toBeGreaterThanOrEqual(1);
  });

  test('T012-E: output propre → 0 fuite valgrind', () => {
    const clean = '==12345== All heap blocks were freed -- no leaks are possible\n';
    const leaks = bridge._parseMemoryLeaks(clean);
    expect(leaks.filter(l => l.tool === 'valgrind')).toHaveLength(0);
  });

  test('T012-F: chaque fuite a size_bytes numérique', () => {
    const leaks = bridge._parseMemoryLeaks(VALGRIND_OUTPUT + ASAN_OUTPUT);
    for (const leak of leaks) {
      expect(typeof leak.size_bytes).toBe('number');
      expect(leak.size_bytes).toBeGreaterThanOrEqual(0);
    }
  });
});
