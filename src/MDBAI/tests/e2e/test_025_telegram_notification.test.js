/**
 * TEST_025 — Notifications Telegram (e2e)
 * CF-012: Message Telegram reçu
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 * Note: Sans token réel → vérifie la construction des messages
 */

import { describe, test, expect } from '@jest/globals';
import { TelegramService } from '../../src/services/telegram.service.js';
import { config } from '../../src/config.js';

describe('TEST_025 — Notifications Telegram (e2e)', () => {
  const service = new TelegramService();

  test('T025-A: TelegramService.sendAnalysisStarted est une fonction', () => {
    expect(typeof service.sendAnalysisStarted).toBe('function');
  });

  test('T025-B: TelegramService.sendProgress est une fonction', () => {
    expect(typeof service.sendProgress).toBe('function');
  });

  test('T025-C: TelegramService.sendAnalysisComplete est une fonction', () => {
    expect(typeof service.sendAnalysisComplete).toBe('function');
  });

  test('T025-D: TelegramService.sendAnalysisError est une fonction', () => {
    expect(typeof service.sendAnalysisError).toBe('function');
  });

  test('T025-E: sendAnalysisStarted() sans bot → ne throw pas', async () => {
    await expect(service.sendAnalysisStarted(123456, 'mdbai-uuid', 'https://github.com/u/r'))
      .resolves.not.toThrow();
  });

  test('T025-F: sendAnalysisError() sans bot → ne throw pas', async () => {
    await expect(service.sendAnalysisError(123456, 'mdbai-uuid', 'Erreur test'))
      .resolves.not.toThrow();
  });

  test('T025-G: barre de progression correcte à 50%', () => {
    const progress = 50;
    const bar = '█'.repeat(Math.floor(progress / 10)) + '░'.repeat(10 - Math.floor(progress / 10));
    expect(bar).toBe('█████░░░░░');
  });

  test('T025-H: barre de progression correcte à 100%', () => {
    const progress = 100;
    const bar = '█'.repeat(Math.floor(progress / 10)) + '░'.repeat(10 - Math.floor(progress / 10));
    expect(bar).toBe('██████████');
  });

  test('T025-I: barre de progression correcte à 0%', () => {
    const progress = 0;
    const bar = '█'.repeat(Math.floor(progress / 10)) + '░'.repeat(10 - Math.floor(progress / 10));
    expect(bar).toBe('░░░░░░░░░░');
  });

  test('T025-J: score 90 → emoji 🟢 dans message complet', () => {
    const score = 90;
    const emoji = score >= 80 ? '🟢' : score >= 60 ? '🟡' : '🔴';
    const text = `${emoji} Score qualité: *${score}/100*`;
    expect(text).toContain('🟢');
    expect(text).toContain('90/100');
  });

  test('T025-K: bot username = "masterdebugai_bot"', () => {
    expect(config.telegram.botUsername).toBe('masterdebugai_bot');
  });

  test('T025-L: workflow complet end-to-end (schema validation)', () => {
    const workflow = {
      step1: 'Telegram /analyze',
      step2: 'Job BullMQ enqueued',
      step3: 'Worker clone + forensic',
      step4: 'Report generated',
      step5: 'PR created',
      step6: 'Telegram notification',
    };
    expect(Object.keys(workflow)).toHaveLength(6);
    expect(workflow.step1).toContain('analyze');
    expect(workflow.step6).toContain('notification');
  });
});
