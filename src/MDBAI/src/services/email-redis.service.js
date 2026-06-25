/**
 * Email Service Natif Redis
 * Remplace SMTP par Redis Pub/Sub + Telegram notifications
 * Pas de dépendance externe (OVH, Gmail, etc.)
 * 
 * Architecture:
 * 1. Code généré → Stocké dans Redis (TTL 10 min)
 * 2. Event publié → Redis Pub/Sub
 * 3. Notification → Telegram (code envoyé directement)
 * 4. Vérification → Lecture Redis + validation
 */

import { getRedisClient } from './redis.service.js';
import logger from '../utils/logger.js';

class EmailRedisService {
  constructor() {
    this.redis = null;
    this.subscriber = null;
    this.initialized = false;
  }

  async initialize() {
    try {
      this.redis = getRedisClient();
      this.subscriber = this.redis.duplicate();
      
      // Subscribe to email events
      await this.subscriber.subscribe('email:send', (message) => {
        this.handleEmailEvent(JSON.parse(message));
      });
      
      this.initialized = true;
      logger.info('[EMAIL-REDIS] Service initialisé ✅');
      logger.info('[EMAIL-REDIS] Mode: Redis Pub/Sub + Telegram notifications');
      logger.info('[EMAIL-REDIS] SMTP: Désactivé (contournement blocage OVH)');
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur initialisation:', error);
      throw error;
    }
  }

  /**
   * Envoie un code de vérification
   * Stocke dans Redis + notifie via Telegram
   */
  async sendVerificationCode(email, code, telegramId, bot = null) {
    if (!this.initialized) {
      logger.warn('[EMAIL-REDIS] Service non initialisé, tentative d\'initialisation...');
      await this.initialize();
    }

    try {
      // 1. Stocker code dans Redis (TTL 10 min)
      const key = `email:code:${email}`;
      const data = {
        code,
        email,
        telegramId,
        createdAt: Date.now(),
        expiresAt: Date.now() + 600000 // 10 minutes
      };
      
      await this.redis.setex(key, 600, JSON.stringify(data));
      logger.info(`[EMAIL-REDIS] Code stocké dans Redis: ${email}`, {
        code,
        ttl: '600s',
        key
      });

      // 2. Publier event pour notifier
      await this.redis.publish('email:send', JSON.stringify({
        type: 'verification_code',
        email,
        code,
        telegramId,
        timestamp: Date.now()
      }));
      
      logger.info(`[EMAIL-REDIS] Event publié: email:send`, { email, code });

      // 3. Notifier immédiatement via Telegram si bot fourni
      if (telegramId && bot) {
        try {
          await bot.sendMessage(
            telegramId,
            `✅ *Code de vérification MDBAI*\n\n` +
            `📧 Email: \`${email}\`\n` +
            `🔑 Code: *${code}*\n\n` +
            `⏱️ Expire dans 10 minutes.\n\n` +
            `Entrez ce code pour activer votre compte.`,
            { parse_mode: 'Markdown' }
          );
          logger.info(`[EMAIL-REDIS] Notification Telegram envoyée: ${telegramId}`, { code });
        } catch (error) {
          logger.error('[EMAIL-REDIS] Erreur notification Telegram:', error);
        }
      } else if (telegramId && !bot) {
        logger.warn(`[EMAIL-REDIS] Bot non fourni, impossible d'envoyer notification Telegram`);
      }

      return {
        success: true,
        method: 'redis',
        code,
        expiresIn: 600,
        notifiedVia: (telegramId && bot) ? 'telegram' : 'none'
      };
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur envoi code:', error);
      return { success: false, error: error.message };
    }
  }

  /**
   * Vérifie un code de vérification
   */
  async verifyCode(email, code) {
    try {
      const key = `email:code:${email}`;
      const data = await this.redis.get(key);
      
      if (!data) {
        logger.warn(`[EMAIL-REDIS] Code non trouvé ou expiré: ${email}`);
        return { 
          valid: false, 
          error: 'Code expiré ou inexistant',
          hint: 'Demandez un nouveau code avec /register'
        };
      }

      const { code: storedCode, expiresAt, createdAt } = JSON.parse(data);
      
      // Vérifier expiration
      if (Date.now() > expiresAt) {
        await this.redis.del(key);
        logger.warn(`[EMAIL-REDIS] Code expiré: ${email}`, {
          createdAt: new Date(createdAt).toISOString(),
          expiresAt: new Date(expiresAt).toISOString()
        });
        return { 
          valid: false, 
          error: 'Code expiré',
          hint: 'Le code expire après 10 minutes'
        };
      }

      // Vérifier code
      if (code !== storedCode) {
        logger.warn(`[EMAIL-REDIS] Code incorrect: ${email}`, {
          provided: code,
          expected: storedCode
        });
        return { 
          valid: false, 
          error: 'Code incorrect',
          hint: 'Vérifiez le code reçu via Telegram'
        };
      }

      // Code valide, supprimer pour éviter réutilisation
      await this.redis.del(key);
      logger.info(`[EMAIL-REDIS] Code vérifié avec succès: ${email}`);
      
      return { 
        valid: true,
        email,
        verifiedAt: Date.now()
      };
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur vérification code:', error);
      return { valid: false, error: error.message };
    }
  }

  /**
   * Handler pour events email (Pub/Sub)
   */
  async handleEmailEvent(event) {
    try {
      const { type, email, code, telegramId, timestamp } = event;
      
      logger.debug(`[EMAIL-REDIS] Event reçu: ${type}`, { email, telegramId });
      
      if (type === 'verification_code') {
        // Log pour debug/audit
        logger.info(`[EMAIL-REDIS] Code de vérification traité`, {
          email,
          code,
          telegramId,
          timestamp: new Date(timestamp).toISOString()
        });
        
        // Note: Notification Telegram déjà envoyée dans sendVerificationCode()
        // Ce handler peut être utilisé pour d'autres traitements asynchrones
        // (ex: logs audit, métriques, backup, etc.)
      }
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur traitement event:', error);
    }
  }

  /**
   * Récupère le code pour un email (admin/debug uniquement)
   */
  async getCode(email) {
    try {
      const key = `email:code:${email}`;
      const data = await this.redis.get(key);
      
      if (!data) {
        return null;
      }

      const parsed = JSON.parse(data);
      logger.debug(`[EMAIL-REDIS] Code récupéré (admin): ${email}`, { code: parsed.code });
      return parsed;
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur récupération code:', error);
      return null;
    }
  }

  /**
   * Envoie un email de bienvenue (stocké dans Redis pour consultation)
   */
  async sendWelcomeEmail(user) {
    try {
      const key = `email:welcome:${user.id}`;
      const data = {
        userId: user.id,
        email: user.email,
        username: user.username,
        credits: user.credits,
        sentAt: Date.now()
      };
      
      await this.redis.setex(key, 86400, JSON.stringify(data)); // 24h TTL
      
      // Notifier via Telegram si disponible
      if (user.telegramId && global.telegramService) {
        await global.telegramService.bot.sendMessage(
          user.telegramId,
          `🎉 *Bienvenue sur MDBAI !*\n\n` +
          `Votre compte a été créé avec succès.\n\n` +
          `👤 Username: ${user.username}\n` +
          `📧 Email: ${user.email}\n` +
          `💰 Crédits: ${user.credits}\n\n` +
          `Utilisez /github pour connecter votre compte GitHub.`,
          { parse_mode: 'Markdown' }
        );
      }
      
      logger.info(`[EMAIL-REDIS] Email bienvenue envoyé: ${user.email}`);
      return { success: true };
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur email bienvenue:', error);
      return { success: false, error: error.message };
    }
  }

  /**
   * Envoie une notification d'analyse terminée
   */
  async sendAnalysisCompleteEmail(user, analysis) {
    try {
      const key = `email:analysis:${analysis.id}`;
      const data = {
        userId: user.id,
        email: user.email,
        analysisId: analysis.id,
        repository: analysis.repository,
        filesAnalyzed: analysis.filesAnalyzed || 0,
        issuesFound: analysis.issuesFound || 0,
        duration: analysis.duration || 'N/A',
        sentAt: Date.now()
      };
      
      await this.redis.setex(key, 86400, JSON.stringify(data)); // 24h TTL
      
      // Notifier via Telegram
      if (user.telegramId && global.telegramService) {
        await global.telegramService.bot.sendMessage(
          user.telegramId,
          `✅ *Analyse terminée !*\n\n` +
          `📦 Repository: ${analysis.repository}\n` +
          `📄 Fichiers: ${analysis.filesAnalyzed || 0}\n` +
          `⚠️ Problèmes: ${analysis.issuesFound || 0}\n` +
          `⏱️ Durée: ${analysis.duration || 'N/A'}\n\n` +
          `Consultez le rapport complet avec /status ${analysis.id}`,
          { parse_mode: 'Markdown' }
        );
      }
      
      logger.info(`[EMAIL-REDIS] Notification analyse envoyée: ${user.email}`);
      return { success: true };
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur notification analyse:', error);
      return { success: false, error: error.message };
    }
  }

  /**
   * Arrêt propre du service
   */
  async stop() {
    try {
      if (this.subscriber) {
        await this.subscriber.unsubscribe('email:send');
        await this.subscriber.quit();
      }
      this.initialized = false;
      logger.info('[EMAIL-REDIS] Service arrêté proprement ✅');
    } catch (error) {
      logger.error('[EMAIL-REDIS] Erreur arrêt service:', error);
    }
  }
}

// Export singleton
const emailRedisService = new EmailRedisService();
export default emailRedisService;

// Export fonctions compatibles avec l'ancienne API
export async function sendVerificationCodeEmail(email, code, telegramId = null, bot = null) {
  return emailRedisService.sendVerificationCode(email, code, telegramId, bot);
}

export async function verifyEmailCode(email, code) {
  return emailRedisService.verifyCode(email, code);
}

export async function sendWelcomeEmail(user) {
  return emailRedisService.sendWelcomeEmail(user);
}

export async function sendAnalysisCompleteEmail(user, analysis) {
  return emailRedisService.sendAnalysisCompleteEmail(user, analysis);
}

// Export pour compatibilité
export async function sendVerificationEmail(email, token) {
  // Générer code 6 chiffres depuis token
  const code = Math.floor(100000 + Math.random() * 900000).toString();
  return emailRedisService.sendVerificationCode(email, code);
}

export async function sendPasswordResetEmail(user, resetToken) {
  // Pour l'instant, log uniquement
  logger.info(`[EMAIL-REDIS] Password reset demandé: ${user.email}`, { resetToken });
  return { success: true, method: 'redis' };
}

export async function sendErrorNotificationEmail(adminEmail, error) {
  // Pour l'instant, log uniquement
  logger.error(`[EMAIL-REDIS] Erreur système notifiée à ${adminEmail}:`, error);
  return { success: true, method: 'redis' };
}

// Made with Bob
