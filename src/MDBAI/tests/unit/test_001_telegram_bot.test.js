/**
 * TEST_001 — Telegram Bot Service
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 * Vérifie l'instanciation et les commandes du bot sans connexion réelle
 */

import { describe, test, expect, beforeEach } from '@jest/globals';
import { TelegramService } from '../../src/services/telegram.service.js';

describe('TEST_001 — TelegramService', () => {
  let service;

  beforeEach(() => {
    service = new TelegramService();
  });

  test('T001-A: TelegramService peut être instancié', () => {
    expect(service).toBeDefined();
    expect(service).toBeInstanceOf(TelegramService);
  });

  test('T001-B: initialized = false avant init()', () => {
    expect(service.initialized).toBe(false);
  });

  test('T001-C: pendingAnalyses est un Map vide', () => {
    expect(service.pendingAnalyses).toBeInstanceOf(Map);
    expect(service.pendingAnalyses.size).toBe(0);
  });

  test('T001-D: init() sans token → mode dégradé (pas de throw)', () => {
    const originalToken = process.env.TELEGRAM_BOT_TOKEN;
    delete process.env.TELEGRAM_BOT_TOKEN;
    expect(() => service.init(null)).not.toThrow();
    process.env.TELEGRAM_BOT_TOKEN = originalToken;
  });

  test('T001-E: stop() sans bot actif → pas de throw', async () => {
    await expect(service.stop()).resolves.not.toThrow();
  });

  test('T001-F: TelegramService a toutes les méthodes STANDARD_NAMES', () => {
    const methods = [
      'handleStart', 'handleHelp', 'handleAnalyze', 'handleStatus',
      'sendAnalysisStarted', 'sendProgress', 'sendAnalysisComplete',
      'sendAnalysisError', 'init', 'stop',
    ];
    for (const m of methods) {
      expect(typeof service[m]).toBe('function');
    }
  });
});
