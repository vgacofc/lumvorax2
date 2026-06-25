/**
 * Redis Registration Service
 * Gestion de l'état d'inscription et du stockage des utilisateurs dans Redis
 * BUG-REDIS-003 FIX + BUG-USER-001 FIX
 */

import { getRedisClient } from './redis.service.js';
import logger from '../utils/logger.js';

/**
 * ============================================================================
 * GESTION ÉTAT D'INSCRIPTION (BUG-REDIS-003 FIX)
 * ============================================================================
 */

/**
 * Récupère l'état d'inscription d'un utilisateur
 * @param {string} telegramId - ID Telegram de l'utilisateur
 * @returns {Promise<object|null>} État d'inscription ou null
 */
export async function getUserRegistrationState(telegramId) {
  try {
    const client = getRedisClient();
    const key = `registration:state:${telegramId}`;
    const data = await client.get(key);
    
    if (!data) {
      return null;
    }
    
    const state = JSON.parse(data);
    logger.debug(`[REDIS-REG] État inscription récupéré: ${telegramId}`, { step: state.step });
    return state;
  } catch (error) {
    logger.error('[REDIS-REG] Erreur récupération état inscription:', error);
    return null;
  }
}

/**
 * Définit l'état d'inscription d'un utilisateur
 * @param {string} telegramId - ID Telegram
 * @param {string} step - Étape actuelle (ask_first_name, ask_last_name, etc.)
 * @param {object} data - Données temporaires de l'inscription
 * @returns {Promise<object>} État sauvegardé
 */
export async function setUserRegistrationState(telegramId, step, data = {}) {
  try {
    const client = getRedisClient();
    const key = `registration:state:${telegramId}`;
    
    const state = {
      step,
      data,
      updatedAt: Date.now(),
      expiresAt: Date.now() + 1800000 // 30 minutes
    };
    
    // TTL 30 minutes - si l'utilisateur ne termine pas, on nettoie
    await client.setex(key, 1800, JSON.stringify(state));
    
    logger.info(`[REDIS-REG] État inscription sauvegardé: ${telegramId}`, { 
      step, 
      ttl: '1800s (30min)' 
    });
    
    return state;
  } catch (error) {
    logger.error('[REDIS-REG] Erreur sauvegarde état inscription:', error);
    throw error;
  }
}

/**
 * Supprime l'état d'inscription (après inscription réussie ou abandon)
 * @param {string} telegramId - ID Telegram
 * @returns {Promise<void>}
 */
export async function clearUserRegistrationState(telegramId) {
  try {
    const client = getRedisClient();
    const stateKey = `registration:state:${telegramId}`;
    const dataKey = `registration:data:${telegramId}`;
    
    await client.del(stateKey);
    await client.del(dataKey);
    
    logger.info(`[REDIS-REG] État inscription nettoyé: ${telegramId}`);
  } catch (error) {
    logger.error('[REDIS-REG] Erreur nettoyage état inscription:', error);
  }
}

/**
 * ============================================================================
 * GESTION UTILISATEURS DANS REDIS (BUG-USER-001 FIX)
 * ============================================================================
 */

/**
 * Stocke un utilisateur dans Redis
 * @param {object} user - Objet utilisateur complet
 * @returns {Promise<void>}
 */
export async function storeUser(user) {
  try {
    const client = getRedisClient();
    const userId = user.id;
    
    // Stocker l'utilisateur complet
    const userKey = `user:${userId}`;
    await client.set(userKey, JSON.stringify(user));
    
    // Créer les mappings pour recherche rapide
    if (user.telegramId) {
      await client.set(`user:telegram:${user.telegramId}`, userId);
    }
    
    if (user.email) {
      await client.set(`user:email:${user.email}`, userId);
    }
    
    if (user.githubId) {
      await client.set(`user:github:${user.githubId}`, userId);
    }
    
    logger.info(`[REDIS-REG] Utilisateur stocké: ${userId}`, {
      email: user.email,
      telegramId: user.telegramId
    });
  } catch (error) {
    logger.error('[REDIS-REG] Erreur stockage utilisateur:', error);
    throw error;
  }
}

/**
 * Récupère un utilisateur depuis Redis par son ID
 * @param {string} userId - ID de l'utilisateur
 * @returns {Promise<object|null>}
 */
export async function getUser(userId) {
  try {
    const client = getRedisClient();
    const key = `user:${userId}`;
    const data = await client.get(key);
    
    if (!data) {
      return null;
    }
    
    return JSON.parse(data);
  } catch (error) {
    logger.error('[REDIS-REG] Erreur récupération utilisateur:', error);
    return null;
  }
}

/**
 * Récupère un utilisateur par son Telegram ID
 * @param {string} telegramId - ID Telegram
 * @returns {Promise<object|null>}
 */
export async function getUserByTelegramId(telegramId) {
  try {
    const client = getRedisClient();
    const userId = await client.get(`user:telegram:${telegramId}`);
    
    if (!userId) {
      return null;
    }
    
    return await getUser(userId);
  } catch (error) {
    logger.error('[REDIS-REG] Erreur récupération utilisateur par Telegram:', error);
    return null;
  }
}

/**
 * Récupère un utilisateur par son email
 * @param {string} email - Email de l'utilisateur
 * @returns {Promise<object|null>}
 */
export async function getUserByEmail(email) {
  try {
    const client = getRedisClient();
    const userId = await client.get(`user:email:${email}`);
    
    if (!userId) {
      return null;
    }
    
    return await getUser(userId);
  } catch (error) {
    logger.error('[REDIS-REG] Erreur récupération utilisateur par email:', error);
    return null;
  }
}

/**
 * Met à jour un utilisateur dans Redis
 * @param {string} userId - ID de l'utilisateur
 * @param {object} updates - Champs à mettre à jour
 * @returns {Promise<object|null>} Utilisateur mis à jour
 */
export async function updateUserRedis(userId, updates) {
  try {
    const user = await getUser(userId);
    
    if (!user) {
      logger.warn(`[REDIS-REG] Utilisateur non trouvé pour mise à jour: ${userId}`);
      return null;
    }
    
    // Appliquer les mises à jour
    const updatedUser = {
      ...user,
      ...updates,
      updatedAt: new Date().toISOString()
    };
    
    // Sauvegarder
    await storeUser(updatedUser);
    
    logger.info(`[REDIS-REG] Utilisateur mis à jour: ${userId}`);
    return updatedUser;
  } catch (error) {
    logger.error('[REDIS-REG] Erreur mise à jour utilisateur:', error);
    throw error;
  }
}

export default {
  getUserRegistrationState,
  setUserRegistrationState,
  clearUserRegistrationState,
  storeUser,
  getUser,
  getUserByTelegramId,
  getUserByEmail,
  updateUserRedis
};

// Made with Bob