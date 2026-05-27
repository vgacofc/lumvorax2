/**
 * TEST_004 — Secrets et Variables d'Environnement
 * Conforme STANDARD_NAMES_MDBAI.md Section 2 (variables env obligatoires)
 * Vérifie la présence et le format des secrets critiques
 */

import { describe, test, expect } from '@jest/globals';
import { config, validateConfig } from '../../src/config.js';

describe('TEST_004 — Secrets et Variables Environnement', () => {

  test('T004-A: config.app.name = "mdbai"', () => {
    expect(config.app.name).toBe('mdbai');
  });

  test('T004-B: config.app.version = "0.1.0"', () => {
    expect(config.app.version).toBe('0.1.0');
  });

  test('T004-C: config.telegram.botUsername = "masterdebugai_bot"', () => {
    expect(config.telegram.botUsername).toBe('masterdebugai_bot');
  });

  test('T004-D: SESSION_SECRET défini', () => {
    expect(config.session.secret).toBeTruthy();
    expect(config.session.secret.length).toBeGreaterThan(8);
  });

  test('T004-E: config.redis est configuré avec host + port', () => {
    expect(config.redis.host).toBeTruthy();
    expect(config.redis.port).toBeGreaterThan(0);
    expect(config.redis.port).toBeLessThan(65536);
  });

  test('T004-F: Redis host = redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com', () => {
    const expectedHost = 'redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com';
    expect(config.redis.host).toBe(expectedHost);
  });

  test('T004-G: Redis port = 17068', () => {
    expect(config.redis.port).toBe(17068);
  });

  test('T004-H: config.pr.branchPrefix = "mdbai-analysis-"', () => {
    expect(config.pr.branchPrefix).toBe('mdbai-analysis-');
  });

  test('T004-I: config.pr.labels contient les 3 labels MDBAI', () => {
    expect(config.pr.labels).toContain('mdbai');
    expect(config.pr.labels).toContain('automated-analysis');
    expect(config.pr.labels).toContain('forensic');
  });

  test('T004-J: validateConfig() retourne bool (pas de throw)', () => {
    const result = validateConfig();
    expect(typeof result).toBe('boolean');
  });

  test('T004-K: TELEGRAM_BOT_TOKEN jamais hardcodé (vient de env)', () => {
    expect(config.telegram.token).toBe(process.env.TELEGRAM_BOT_TOKEN || '');
  });

  test('T004-L: config.forensic.logRotationMb = 20', () => {
    expect(config.forensic.logRotationMb).toBe(20);
  });
});
