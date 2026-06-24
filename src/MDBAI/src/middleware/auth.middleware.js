/**
 * MDBAI — Middleware d'authentification
 * Protection des routes selon méthode d'authentification
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 */

import { findUserById } from '../services/user.service.js';
import logger from '../utils/logger.js';
import {
  AUTH_METHOD_EMAIL,
  AUTH_METHOD_PHONE,
  AUTH_METHOD_GITHUB,
  USER_STATUS_ACTIVE,
} from '../models/user.model.js';

/**
 * Middleware: Vérifie que l'utilisateur est authentifié
 * Fonctionne avec les 3 méthodes d'authentification
 */
export function requireAuth(req, res, next) {
  const userId = req.session?.userId;
  
  if (!userId) {
    logger.warn('[AUTH-MIDDLEWARE] Accès refusé - non authentifié', {
      path: req.path,
      ip: req.ip,
    });
    
    return res.status(401).json({
      error: 'UNAUTHORIZED',
      message: 'Authentification requise',
      hint: 'Connectez-vous via /auth/login/email, /auth/login/phone ou /auth/github',
    });
  }
  
  // Attacher userId à la requête pour les handlers suivants
  req.userId = userId;
  req.authMethod = req.session.authMethod;
  
  next();
}

/**
 * Middleware: Vérifie que l'utilisateur est authentifié ET actif
 * Charge les données utilisateur complètes depuis Redis
 */
export async function requireActiveUser(req, res, next) {
  const userId = req.session?.userId;
  
  if (!userId) {
    return res.status(401).json({
      error: 'UNAUTHORIZED',
      message: 'Authentification requise',
    });
  }
  
  try {
    const user = await findUserById(userId);
    
    if (!user) {
      logger.warn('[AUTH-MIDDLEWARE] Utilisateur introuvable', { userId });
      return res.status(401).json({
        error: 'USER_NOT_FOUND',
        message: 'Utilisateur introuvable',
      });
    }
    
    if (user.status !== USER_STATUS_ACTIVE) {
      logger.warn('[AUTH-MIDDLEWARE] Utilisateur non actif', {
        userId,
        status: user.status,
      });
      
      return res.status(403).json({
        error: 'USER_NOT_ACTIVE',
        message: 'Compte non actif',
        status: user.status,
      });
    }
    
    // Attacher l'utilisateur complet à la requête
    req.user = user;
    req.userId = userId;
    req.authMethod = user.auth_method;
    
    next();
  } catch (error) {
    logger.error('[AUTH-MIDDLEWARE] Erreur vérification utilisateur', {
      userId,
      error: error.message,
    });
    
    return res.status(500).json({
      error: 'AUTH_CHECK_FAILED',
      message: 'Erreur de vérification',
    });
  }
}

/**
 * Middleware: Vérifie que l'email est vérifié
 * Uniquement pour les utilisateurs avec auth_method = 'email'
 */
export async function requireEmailVerified(req, res, next) {
  const userId = req.session?.userId;
  
  if (!userId) {
    return res.status(401).json({
      error: 'UNAUTHORIZED',
      message: 'Authentification requise',
    });
  }
  
  try {
    const user = await findUserById(userId);
    
    if (!user) {
      return res.status(401).json({
        error: 'USER_NOT_FOUND',
        message: 'Utilisateur introuvable',
      });
    }
    
    // Vérification uniquement pour auth email
    if (user.auth_method === AUTH_METHOD_EMAIL && !user.email_verified) {
      logger.warn('[AUTH-MIDDLEWARE] Email non vérifié', {
        userId,
        email: user.email,
      });
      
      return res.status(403).json({
        error: 'EMAIL_NOT_VERIFIED',
        message: 'Veuillez vérifier votre email avant d\'accéder à cette ressource',
        hint: 'Consultez votre boîte mail pour le lien de vérification',
      });
    }
    
    req.user = user;
    req.userId = userId;
    next();
  } catch (error) {
    logger.error('[AUTH-MIDDLEWARE] Erreur vérification email', {
      userId,
      error: error.message,
    });
    
    return res.status(500).json({
      error: 'AUTH_CHECK_FAILED',
      message: 'Erreur de vérification',
    });
  }
}

/**
 * Middleware: Vérifie que le téléphone est vérifié
 * Uniquement pour les utilisateurs avec auth_method = 'phone'
 */
export async function requirePhoneVerified(req, res, next) {
  const userId = req.session?.userId;
  
  if (!userId) {
    return res.status(401).json({
      error: 'UNAUTHORIZED',
      message: 'Authentification requise',
    });
  }
  
  try {
    const user = await findUserById(userId);
    
    if (!user) {
      return res.status(401).json({
        error: 'USER_NOT_FOUND',
        message: 'Utilisateur introuvable',
      });
    }
    
    // Vérification uniquement pour auth téléphone
    if (user.auth_method === AUTH_METHOD_PHONE && !user.phone_verified) {
      logger.warn('[AUTH-MIDDLEWARE] Téléphone non vérifié', {
        userId,
        phone: user.phone,
      });
      
      return res.status(403).json({
        error: 'PHONE_NOT_VERIFIED',
        message: 'Veuillez vérifier votre téléphone avant d\'accéder à cette ressource',
        hint: 'Utilisez le code OTP envoyé sur Telegram',
      });
    }
    
    req.user = user;
    req.userId = userId;
    next();
  } catch (error) {
    logger.error('[AUTH-MIDDLEWARE] Erreur vérification téléphone', {
      userId,
      error: error.message,
    });
    
    return res.status(500).json({
      error: 'AUTH_CHECK_FAILED',
      message: 'Erreur de vérification',
    });
  }
}

/**
 * Middleware: Vérifie que l'utilisateur utilise une méthode spécifique
 * @param {string} method - AUTH_METHOD_EMAIL | AUTH_METHOD_PHONE | AUTH_METHOD_GITHUB
 */
export function requireAuthMethod(method) {
  return async (req, res, next) => {
    const userId = req.session?.userId;
    
    if (!userId) {
      return res.status(401).json({
        error: 'UNAUTHORIZED',
        message: 'Authentification requise',
      });
    }
    
    try {
      const user = await findUserById(userId);
      
      if (!user) {
        return res.status(401).json({
          error: 'USER_NOT_FOUND',
          message: 'Utilisateur introuvable',
        });
      }
      
      if (user.auth_method !== method) {
        logger.warn('[AUTH-MIDDLEWARE] Méthode auth incorrecte', {
          userId,
          expected: method,
          actual: user.auth_method,
        });
        
        return res.status(403).json({
          error: 'INVALID_AUTH_METHOD',
          message: `Cette ressource nécessite l'authentification ${method}`,
          current_method: user.auth_method,
        });
      }
      
      req.user = user;
      req.userId = userId;
      next();
    } catch (error) {
      logger.error('[AUTH-MIDDLEWARE] Erreur vérification méthode', {
        userId,
        error: error.message,
      });
      
      return res.status(500).json({
        error: 'AUTH_CHECK_FAILED',
        message: 'Erreur de vérification',
      });
    }
  };
}

/**
 * Middleware: Optionnel - charge l'utilisateur si authentifié
 * Ne bloque pas si non authentifié
 */
export async function optionalAuth(req, res, next) {
  const userId = req.session?.userId;
  
  if (!userId) {
    return next();
  }
  
  try {
    const user = await findUserById(userId);
    
    if (user) {
      req.user = user;
      req.userId = userId;
      req.authMethod = user.auth_method;
    }
  } catch (error) {
    logger.error('[AUTH-MIDDLEWARE] Erreur chargement utilisateur optionnel', {
      userId,
      error: error.message,
    });
  }
  
  next();
}

export default {
  requireAuth,
  requireActiveUser,
  requireEmailVerified,
  requirePhoneVerified,
  requireAuthMethod,
  optionalAuth,
};

// Made with Bob