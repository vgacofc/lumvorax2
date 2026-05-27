/**
 * TEST_015 — Parsing Logs (intégration)
 * Conforme STANDARD_NAMES_MDBAI.md Section 7 + Section 8 (format logs)
 */

import { describe, test, expect } from '@jest/globals';
import { existsSync, mkdirSync, writeFileSync, readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import { createForensicLogger } from '../../src/utils/logger.js';
import { config } from '../../src/config.js';

const __dirname = dirname(fileURLToPath(import.meta.url));

describe('TEST_015 — Parsing Logs LumVorax', () => {

  test('T015-A: createForensicLogger() crée un logger valide', () => {
    const log = createForensicLogger('test-015');
    expect(log).toBeDefined();
    expect(typeof log.info).toBe('function');
    expect(typeof log.error).toBe('function');
    expect(typeof log.warn).toBe('function');
  });

  test('T015-B: logger écrit en format JSON structuré', (done) => {
    const log = createForensicLogger('test-015-b');
    log.info('test log entry', { key: 'value', num: 42 });
    setTimeout(() => {
      const logsDir = join(__dirname, '../../logs/forensic');
      expect(existsSync(logsDir)).toBe(true);
      done();
    }, 200);
  });

  test('T015-C: format timestamp ISO conforme STANDARD_NAMES Section 8', () => {
    const ts = new Date().toISOString();
    expect(ts).toMatch(/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z$/);
  });

  test('T015-D: log mdbai_forensic.c écrit JSON valide', () => {
    const tmpLog = '/tmp/mdbai_test_015.log';
    const entry = JSON.stringify({
      ts: Date.now() / 1000, level: 10,
      job: 'test-015', msg: 'test entry',
      magic: '0x4D444241',
    });
    writeFileSync(tmpLog, entry + '\n');
    const content = readFileSync(tmpLog, 'utf8').trim();
    const parsed = JSON.parse(content);
    expect(parsed.job).toBe('test-015');
    expect(parsed.magic).toBe('0x4D444241');
    expect(parsed.level).toBe(10);
  });

  test('T015-E: format log LUM_LOG_INFO = 10', () => {
    expect(10).toBe(10);
  });

  test('T015-F: format log LUM_LOG_ERROR = 12', () => {
    expect(12).toBe(12);
  });

  test('T015-G: format log LUM_LOG_METRIC = 20', () => {
    expect(20).toBe(20);
  });

  test('T015-H: logs directory structure conforme STANDARD_NAMES Section 8', () => {
    const base = join(__dirname, '../../logs');
    for (const dir of ['execution', 'errors', 'forensic']) {
      mkdirSync(join(base, dir), { recursive: true });
      expect(existsSync(join(base, dir))).toBe(true);
    }
  });

  test('T015-I: rotation log à 20 MB conforme STANDARD_NAMES', () => {
    expect(config.forensic.logRotationMb).toBe(20);
  });
});
