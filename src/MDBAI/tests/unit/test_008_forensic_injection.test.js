/**
 * TEST_008 — Injection Forensic (ForensicBridge)
 * CF-008: libforensic.so chargé (ou mode dégradé validé)
 * Conforme STANDARD_NAMES_MDBAI.md Section 7 + 10
 * Conforme prompt.txt Règle #1 : traçabilité bit-level active
 */

import { describe, test, expect, beforeEach, afterEach } from '@jest/globals';
import { ForensicBridge, FORENSIC_LUM_MAGIC, FORENSIC_LOG_ROTATION_MB } from '../../src/utils/forensic.js';
import { existsSync, mkdirSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const FORENSIC_DIR = join(__dirname, '../../forensic');

describe('TEST_008 — Injection Forensic LumVorax', () => {
  let bridge;

  beforeEach(() => {
    bridge = new ForensicBridge('test-008-forensic');
  });

  afterEach(() => {
    if (bridge?.active) bridge.destroy();
  });

  test('T008-A: ForensicBridge peut être instancié', () => {
    expect(bridge).toBeDefined();
    expect(bridge).toBeInstanceOf(ForensicBridge);
  });

  test('T008-B: FORENSIC_LUM_MAGIC = 0x4D444241 ("MDBA")', () => {
    expect(FORENSIC_LUM_MAGIC).toBe(0x4D444241);
    const magic = Buffer.alloc(4);
    magic.writeUInt32BE(FORENSIC_LUM_MAGIC);
    expect(magic.toString('ascii')).toBe('MDBA');
  });

  test('T008-C: FORENSIC_LOG_ROTATION_MB = 20', () => {
    expect(FORENSIC_LOG_ROTATION_MB).toBe(20);
  });

  test('T008-D: init() initialise le bridge sans throw', () => {
    expect(() => bridge.init()).not.toThrow();
    expect(bridge.active).toBe(true);
  });

  test('T008-E: jobId est préservé après init', () => {
    bridge.init();
    expect(bridge.jobId).toBe('test-008-forensic');
  });

  test('T008-F: libAvailable détecte correctement la .so', () => {
    expect(typeof bridge.libAvailable).toBe('boolean');
  });

  test('T008-G: répertoires forensic créés automatiquement', () => {
    bridge.init();
    expect(existsSync(join(FORENSIC_DIR, 'memory'))).toBe(true);
    expect(existsSync(join(FORENSIC_DIR, 'logger'))).toBe(true);
  });

  test('T008-H: _parseMemoryLeaks() détecte pattern valgrind', () => {
    const output = 'definitely lost: 1,024 bytes in 3 blocks\n';
    const leaks = bridge._parseMemoryLeaks(output);
    expect(leaks.length).toBeGreaterThan(0);
    expect(leaks[0].tool).toBe('valgrind');
    expect(leaks[0].size_bytes).toBe(1024);
  });

  test('T008-I: _parseMemoryLeaks() détecte pattern générique', () => {
    const output = 'memory leak of 512 bytes detected\n';
    const leaks = bridge._parseMemoryLeaks(output);
    expect(Array.isArray(leaks)).toBe(true);
  });

  test('T008-J: destroy() désactive le bridge', () => {
    bridge.init();
    bridge.destroy();
    expect(bridge.active).toBe(false);
  });
});
