/**
 * TEST_016 - Intégration CompareService dans Telegram Bot
 * 
 * Objectif: Valider l'intégration complète de la commande /compare
 * 
 * Scénarios testés:
 * 1. Validation arguments manquants
 * 2. Validation URL invalide
 * 3. Validation SHA invalide
 * 4. Comparaison réussie avec rapport .lum
 * 
 * @group integration
 * @requires telegram.service.js
 * @requires compare.service.js
 */

import { describe, test, expect, jest, beforeEach, afterEach } from '@jest/globals';
import TelegramService from '../../src/services/telegram.service.js';
import { CompareService } from '../../src/services/compare.service.js';

describe('TEST_016 - Intégration /compare', () => {
  let telegramService;
  let mockBot;
  let mockCompareService;

  beforeEach(() => {
    // Mock Telegram Bot
    mockBot = {
      sendMessage: jest.fn().mockResolvedValue({}),
      sendDocument: jest.fn().mockResolvedValue({}),
      onText: jest.fn(),
    };

    // Mock CompareService
    mockCompareService = {
      compareCommits: jest.fn().mockResolvedValue({
        metrics: {
          commit1: { entropy: 4.5, size: 1024 },
          commit2: { entropy: 4.8, size: 1200 },
          delta: { entropy: 0.3, size: 176 }
        },
        security: {
          patterns: { commit1: [], commit2: [] },
          regressions: []
        },
        diff: {
          files_changed: 3,
          lines_added: 45,
          lines_deleted: 12
        },
        lumFile: '/tmp/compare_abc123_def456.lum'
      })
    };

    // Créer instance TelegramService avec mocks
    telegramService = new TelegramService();
    telegramService.bot = mockBot;
    telegramService.compareService = mockCompareService;
  });

  afterEach(() => {
    jest.clearAllMocks();
  });

  /**
   * TEST_016_001: Arguments manquants
   */
  test('TEST_016_001: Doit rejeter commande sans arguments', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', ''];

    await telegramService.handleCompare(msg, match);

    expect(mockBot.sendMessage).toHaveBeenCalledWith(
      12345,
      expect.stringContaining('❌'),
      expect.objectContaining({ parse_mode: 'Markdown' })
    );
    expect(mockCompareService.compareCommits).not.toHaveBeenCalled();
  });

  /**
   * TEST_016_002: URL GitHub invalide
   */
  test('TEST_016_002: Doit rejeter URL GitHub invalide', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'invalid-url abc123 def456'];

    await telegramService.handleCompare(msg, match);

    expect(mockBot.sendMessage).toHaveBeenCalledWith(
      12345,
      expect.stringContaining('❌'),
      expect.objectContaining({ parse_mode: 'Markdown' })
    );
    expect(mockCompareService.compareCommits).not.toHaveBeenCalled();
  });

  /**
   * TEST_016_003: SHA invalide (trop court)
   */
  test('TEST_016_003: Doit rejeter SHA invalide (< 7 caractères)', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abc12 def456'];

    await telegramService.handleCompare(msg, match);

    expect(mockBot.sendMessage).toHaveBeenCalledWith(
      12345,
      expect.stringContaining('❌'),
      expect.objectContaining({ parse_mode: 'Markdown' })
    );
    expect(mockCompareService.compareCommits).not.toHaveBeenCalled();
  });

  /**
   * TEST_016_004: SHA invalide (caractères non-hex)
   */
  test('TEST_016_004: Doit rejeter SHA avec caractères non-hexadécimaux', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abcxyz def456'];

    await telegramService.handleCompare(msg, match);

    expect(mockBot.sendMessage).toHaveBeenCalledWith(
      12345,
      expect.stringContaining('❌'),
      expect.objectContaining({ parse_mode: 'Markdown' })
    );
    expect(mockCompareService.compareCommits).not.toHaveBeenCalled();
  });

  /**
   * TEST_016_005: Comparaison réussie
   */
  test('TEST_016_005: Doit comparer deux commits avec succès', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abc1234 def5678'];

    await telegramService.handleCompare(msg, match);

    // Vérifier message de démarrage
    expect(mockBot.sendMessage).toHaveBeenCalledWith(
      12345,
      expect.stringContaining('🔄'),
      expect.objectContaining({ parse_mode: 'Markdown' })
    );

    // Vérifier appel CompareService
    expect(mockCompareService.compareCommits).toHaveBeenCalledWith(
      'https://github.com/octocat/Hello-World',
      'abc1234',
      'def5678',
      expect.objectContaining({ userId: 67890, chatId: 12345 })
    );

    // Vérifier message de résultat
    expect(mockBot.sendMessage).toHaveBeenCalledWith(
      12345,
      expect.stringContaining('✅'),
      expect.objectContaining({ parse_mode: 'Markdown' })
    );

    // Vérifier envoi fichier .lum
    expect(mockBot.sendDocument).toHaveBeenCalledWith(
      12345,
      '/tmp/compare_abc123_def456.lum',
      expect.any(Object)
    );
  });

  /**
   * TEST_016_006: Gestion erreur CompareService
   */
  test('TEST_016_006: Doit gérer erreur CompareService', async () => {
    mockCompareService.compareCommits.mockRejectedValue(
      new Error('Commit not found')
    );

    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abc1234 def5678'];

    await telegramService.handleCompare(msg, match);

    expect(mockBot.sendMessage).toHaveBeenCalledWith(
      12345,
      expect.stringContaining('❌'),
      expect.objectContaining({ parse_mode: 'Markdown' })
    );
  });

  /**
   * TEST_016_007: Validation format résultat
   */
  test('TEST_016_007: Doit formater résultat correctement', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abc1234 def5678'];

    await telegramService.handleCompare(msg, match);

    // Récupérer le message de résultat
    const resultCall = mockBot.sendMessage.mock.calls.find(call => 
      call[1].includes('✅') && call[1].includes('Comparaison terminée')
    );

    expect(resultCall).toBeDefined();
    expect(resultCall[1]).toContain('📊');
    expect(resultCall[1]).toContain('Entropie');
    expect(resultCall[1]).toContain('Taille');
    expect(resultCall[1]).toContain('Sécurité');
    expect(resultCall[1]).toContain('Changements');
  });

  /**
   * TEST_016_008: Validation métriques delta
   */
  test('TEST_016_008: Doit afficher delta métriques', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abc1234 def5678'];

    await telegramService.handleCompare(msg, match);

    const resultCall = mockBot.sendMessage.mock.calls.find(call => 
      call[1].includes('✅')
    );

    expect(resultCall[1]).toContain('4.5');  // entropy commit1
    expect(resultCall[1]).toContain('4.8');  // entropy commit2
    expect(resultCall[1]).toContain('+0.3'); // delta entropy
    expect(resultCall[1]).toContain('1024'); // size commit1
    expect(resultCall[1]).toContain('1200'); // size commit2
    expect(resultCall[1]).toContain('+176'); // delta size
  });

  /**
   * TEST_016_009: Validation régressions sécurité
   */
  test('TEST_016_009: Doit afficher régressions sécurité', async () => {
    mockCompareService.compareCommits.mockResolvedValue({
      metrics: {
        commit1: { entropy: 4.5, size: 1024 },
        commit2: { entropy: 4.8, size: 1200 },
        delta: { entropy: 0.3, size: 176 }
      },
      security: {
        patterns: { 
          commit1: ['eval()'], 
          commit2: ['eval()', 'exec()'] 
        },
        regressions: [
          { pattern: 'exec()', file: 'src/main.js', line: 42 }
        ]
      },
      diff: {
        files_changed: 3,
        lines_added: 45,
        lines_deleted: 12
      },
      lumFile: '/tmp/compare.lum'
    });

    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abc1234 def5678'];

    await telegramService.handleCompare(msg, match);

    const resultCall = mockBot.sendMessage.mock.calls.find(call => 
      call[1].includes('✅')
    );

    expect(resultCall[1]).toContain('🔴'); // Emoji sécurité critique
    expect(resultCall[1]).toContain('1 → 2'); // Patterns suspects
    expect(resultCall[1]).toContain('1'); // Régressions détectées
  });

  /**
   * TEST_016_010: Validation diff changements
   */
  test('TEST_016_010: Doit afficher statistiques diff', async () => {
    const msg = {
      chat: { id: 12345 },
      from: { id: 67890 }
    };
    const match = ['/compare', 'https://github.com/octocat/Hello-World abc1234 def5678'];

    await telegramService.handleCompare(msg, match);

    const resultCall = mockBot.sendMessage.mock.calls.find(call => 
      call[1].includes('✅')
    );

    expect(resultCall[1]).toContain('3'); // files_changed
    expect(resultCall[1]).toContain('+45'); // lines_added
    expect(resultCall[1]).toContain('-12'); // lines_deleted
  });
});

/**
 * Résumé TEST_016:
 * 
 * Tests: 10 tests d'intégration
 * Couverture:
 * - Validation arguments (3 tests)
 * - Comparaison réussie (1 test)
 * - Gestion erreurs (1 test)
 * - Formatage résultat (5 tests)
 * 
 * Commande exécution:
 * npm test -- test_016_compare_integration.test.js
 * 
 * Attendu: 10/10 tests passants (100%)
 */

// Made with Bob
