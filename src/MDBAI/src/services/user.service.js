/**
 * User Service
 * Service de gestion des utilisateurs MDBAI
 */

import logger from '../utils/logger.js';
import { User } from '../models/user.model.js';

// Stockage en mémoire (à remplacer par une vraie DB)
const users = new Map();

/**
 * Crée un nouvel utilisateur
 */
export async function createUser(userData) {
  try {
    const user = new User(userData);
    const validation = user.validate();
    
    if (!validation.isValid) {
      return {
        success: false,
        errors: validation.errors
      };
    }

    // Générer un ID si non fourni
    if (!user.id) {
      user.id = `user_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    }

    users.set(user.id, user);
    logger.info(`Utilisateur créé: ${user.id}`, { username: user.username });

    return {
      success: true,
      user: user.toJSON()
    };
  } catch (error) {
    logger.error('Erreur lors de la création de l\'utilisateur:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Récupère un utilisateur par son ID
 */
export async function getUserById(userId) {
  try {
    const user = users.get(userId);
    
    if (!user) {
      return {
        success: false,
        error: 'User not found'
      };
    }

    return {
      success: true,
      user: user.toJSON()
    };
  } catch (error) {
    logger.error('Erreur lors de la récupération de l\'utilisateur:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Récupère un utilisateur par son GitHub ID
 */
export async function getUserByGithubId(githubId) {
  try {
    for (const user of users.values()) {
      if (user.githubId === githubId) {
        return {
          success: true,
          user: user.toJSON()
        };
      }
    }

    return {
      success: false,
      error: 'User not found'
    };
  } catch (error) {
    logger.error('Erreur lors de la récupération de l\'utilisateur par GitHub ID:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Trouve un utilisateur par son GitHub ID (alias pour compatibilité)
 */
export async function findUserByGithub(githubId) {
  return getUserByGithubId(githubId);
}

/**
 * Récupère un utilisateur par son Telegram ID
 */
export async function getUserByTelegramId(telegramId) {
  try {
    for (const user of users.values()) {
      if (user.telegramId === telegramId) {
        return {
          success: true,
          user: user.toJSON()
        };
      }
    }

    return {
      success: false,
      error: 'User not found'
    };
  } catch (error) {
    logger.error('Erreur lors de la récupération de l\'utilisateur par Telegram ID:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Trouve un utilisateur par son Telegram ID (alias pour compatibilité)
 */
export async function findUserByTelegram(telegramId) {
  return getUserByTelegramId(telegramId);
}

/**
 * Met à jour un utilisateur
 */
export async function updateUser(userId, updates) {
  try {
    const user = users.get(userId);
    
    if (!user) {
      return {
        success: false,
        error: 'User not found'
      };
    }

    // Appliquer les mises à jour
    Object.assign(user, updates);
    user.updatedAt = new Date();

    // Valider
    const validation = user.validate();
    if (!validation.isValid) {
      return {
        success: false,
        errors: validation.errors
      };
    }

    users.set(userId, user);
    logger.info(`Utilisateur mis à jour: ${userId}`);

    return {
      success: true,
      user: user.toJSON()
    };
  } catch (error) {
    logger.error('Erreur lors de la mise à jour de l\'utilisateur:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Supprime un utilisateur
 */
export async function deleteUser(userId) {
  try {
    const user = users.get(userId);
    
    if (!user) {
      return {
        success: false,
        error: 'User not found'
      };
    }

    users.delete(userId);
    logger.info(`Utilisateur supprimé: ${userId}`);

    return {
      success: true
    };
  } catch (error) {
    logger.error('Erreur lors de la suppression de l\'utilisateur:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Liste tous les utilisateurs
 */
export async function listUsers(options = {}) {
  try {
    const { limit = 100, offset = 0 } = options;
    const allUsers = Array.from(users.values());
    const paginatedUsers = allUsers.slice(offset, offset + limit);

    return {
      success: true,
      users: paginatedUsers.map(u => u.toJSON()),
      total: allUsers.length
    };
  } catch (error) {
    logger.error('Erreur lors de la liste des utilisateurs:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Ajoute des crédits à un utilisateur
 */
export async function addCredits(userId, amount) {
  try {
    const user = users.get(userId);
    
    if (!user) {
      return {
        success: false,
        error: 'User not found'
      };
    }

    user.addCredits(amount);
    users.set(userId, user);
    
    logger.info(`Crédits ajoutés: ${amount} pour ${userId}`);

    return {
      success: true,
      credits: user.credits
    };
  } catch (error) {
    logger.error('Erreur lors de l\'ajout de crédits:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

/**
 * Retire des crédits à un utilisateur
 */
export async function removeCredits(userId, amount) {
  try {
    const user = users.get(userId);
    
    if (!user) {
      return {
        success: false,
        error: 'User not found'
      };
    }

    const success = user.removeCredits(amount);
    
    if (!success) {
      return {
        success: false,
        error: 'Insufficient credits'
      };
    }

    users.set(userId, user);
    logger.info(`Crédits retirés: ${amount} pour ${userId}`);

    return {
      success: true,
      credits: user.credits
    };
  } catch (error) {
    logger.error('Erreur lors du retrait de crédits:', error);
    return {
      success: false,
      error: error.message
    };
  }
}

export default {
  createUser,
  getUserById,
  getUserByGithubId,
  getUserByTelegramId,
  findUserByGithub,
  findUserByTelegram,
  updateUser,
  deleteUser,
  listUsers,
  addCredits,
  removeCredits
};

// Made with Bob
