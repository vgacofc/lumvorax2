/**
 * MDBAI — Routes d'authentification hybride
 * Support 3 méthodes: GitHub OAuth + Email + Téléphone
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 */

import { Router } from 'express';
import Joi from 'joi';
import {
  registerUser,
  authenticateByEmail,
  authenticateByPhone,
  findUserByEmail,
  findUserByPhone,
  verifyEmail,
  verifyPhone,
  generatePhoneOTP,
} from '../services/user.service.js';
import { sendVerificationCodeEmail, sendPasswordResetEmail } from '../services/email.service.js';
import { TelegramService } from '../services/telegram.service.js';
import logger from '../utils/logger.js';
import { config } from '../config.js';
import {
  AUTH_METHOD_EMAIL,
  AUTH_METHOD_PHONE,
  AUTH_METHOD_GITHUB,
} from '../models/user.model.js';

const authHybridRouter = Router();

/**
 * Schémas de validation Joi
 */
const registerEmailSchema = Joi.object({
  email: Joi.string().email().required(),
  password: Joi.string().min(8).required(),
  telegram_id: Joi.string().optional(),
});

const registerPhoneSchema = Joi.object({
  phone: Joi.string().pattern(/^\+[1-9]\d{1,14}$/).required(),
  telegram_id: Joi.string().required(),
});

const loginEmailSchema = Joi.object({
  email: Joi.string().email().required(),
  password: Joi.string().required(),
});

const loginPhoneSchema = Joi.object({
  phone: Joi.string().pattern(/^\+[1-9]\d{1,14}$/).required(),
  code: Joi.string().length(6).required(),
});

const verifyEmailSchema = Joi.object({
  token: Joi.string().uuid().required(),
});

const verifyPhoneSchema = Joi.object({
  phone: Joi.string().pattern(/^\+[1-9]\d{1,14}$/).required(),
  code: Joi.string().length(6).required(),
});

/**
 * POST /auth/register/email
 * Inscription par email + mot de passe
 */
authHybridRouter.post('/register/email', async (req, res) => {
  try {
    const { error, value } = registerEmailSchema.validate(req.body);
    if (error) {
      return res.status(400).json({
        error: 'VALIDATION_ERROR',
        details: error.details.map(d => d.message),
      });
    }

    const user = await registerUser({
      authMethod: AUTH_METHOD_EMAIL,
      email: value.email,
      password: value.password,
      telegramId: value.telegram_id,
    });

    // Envoyer email de vérification
    await sendVerificationCodeEmail(user.email, user.email_verification_token);

    logger.info('[AUTH-HYBRID] Inscription email réussie', {
      userId: user.id,
      email: user.email,
    });

    return res.status(201).json({
      success: true,
      message: 'Inscription réussie. Vérifiez votre email pour activer votre compte.',
      user: {
        id: user.id,
        email: user.email,
        status: user.status,
        auth_method: user.auth_method,
      },
    });
  } catch (err) {
    logger.error('[AUTH-HYBRID] Échec inscription email', { error: err.message });
    
    if (err.message === 'EMAIL_ALREADY_EXISTS') {
      return res.status(409).json({
        error: 'EMAIL_ALREADY_EXISTS',
        message: 'Cet email est déjà utilisé',
      });
    }
    
    return res.status(500).json({
      error: 'REGISTRATION_FAILED',
      message: 'Échec de l\'inscription',
    });
  }
});

/**
 * POST /auth/register/phone
 * Inscription par téléphone (via Telegram)
 */
authHybridRouter.post('/register/phone', async (req, res) => {
  try {
    const { error, value } = registerPhoneSchema.validate(req.body);
    if (error) {
      return res.status(400).json({
        error: 'VALIDATION_ERROR',
        details: error.details.map(d => d.message),
      });
    }

    const user = await registerUser({
      authMethod: AUTH_METHOD_PHONE,
      phone: value.phone,
      telegramId: value.telegram_id,
    });

    // Envoyer code OTP via Telegram
    const telegram = new TelegramService();
    await telegram.sendMessage(
      value.telegram_id,
      `🔐 *Code de vérification MDBAI*\n\n` +
      `Votre code: \`${user.phone_verification_code}\`\n\n` +
      `Ce code expire dans 10 minutes.\n\n` +
      `Si vous n'avez pas demandé ce code, ignorez ce message.`,
      { parse_mode: 'Markdown' }
    );

    logger.info('[AUTH-HYBRID] Inscription téléphone réussie', {
      userId: user.id,
      phone: user.phone,
    });

    return res.status(201).json({
      success: true,
      message: 'Inscription réussie. Vérifiez votre Telegram pour le code de vérification.',
      user: {
        id: user.id,
        phone: user.phone,
        status: user.status,
        auth_method: user.auth_method,
      },
    });
  } catch (err) {
    logger.error('[AUTH-HYBRID] Échec inscription téléphone', { error: err.message });
    
    if (err.message === 'PHONE_ALREADY_EXISTS') {
      return res.status(409).json({
        error: 'PHONE_ALREADY_EXISTS',
        message: 'Ce numéro est déjà utilisé',
      });
    }
    
    return res.status(500).json({
      error: 'REGISTRATION_FAILED',
      message: 'Échec de l\'inscription',
    });
  }
});

/**
 * POST /auth/login/email
 * Connexion par email + mot de passe
 */
authHybridRouter.post('/login/email', async (req, res) => {
  try {
    const { error, value } = loginEmailSchema.validate(req.body);
    if (error) {
      return res.status(400).json({
        error: 'VALIDATION_ERROR',
        details: error.details.map(d => d.message),
      });
    }

    const user = await authenticateByEmail(value.email, value.password);

    // Stocker en session
    if (req.session) {
      req.session.userId = user.id;
      req.session.authMethod = AUTH_METHOD_EMAIL;
      req.session.email = user.email;
    }

    logger.info('[AUTH-HYBRID] Connexion email réussie', {
      userId: user.id,
      email: user.email,
    });

    return res.json({
      success: true,
      message: 'Connexion réussie',
      user: {
        id: user.id,
        email: user.email,
        auth_method: user.auth_method,
        last_login: user.last_login,
      },
    });
  } catch (err) {
    logger.error('[AUTH-HYBRID] Échec connexion email', { error: err.message });
    
    const errorMessages = {
      USER_NOT_FOUND: 'Email ou mot de passe incorrect',
      INVALID_PASSWORD: 'Email ou mot de passe incorrect',
      EMAIL_NOT_VERIFIED: 'Veuillez vérifier votre email avant de vous connecter',
      USER_NOT_ACTIVE: 'Compte suspendu. Contactez le support.',
      INVALID_AUTH_METHOD: 'Méthode d\'authentification incorrecte',
    };
    
    return res.status(401).json({
      error: err.message,
      message: errorMessages[err.message] || 'Échec de la connexion',
    });
  }
});

/**
 * POST /auth/login/phone
 * Connexion par téléphone + code OTP
 */
authHybridRouter.post('/login/phone', async (req, res) => {
  try {
    const { error, value } = loginPhoneSchema.validate(req.body);
    if (error) {
      return res.status(400).json({
        error: 'VALIDATION_ERROR',
        details: error.details.map(d => d.message),
      });
    }

    const user = await authenticateByPhone(value.phone, value.code);

    // Stocker en session
    if (req.session) {
      req.session.userId = user.id;
      req.session.authMethod = AUTH_METHOD_PHONE;
      req.session.phone = user.phone;
    }

    logger.info('[AUTH-HYBRID] Connexion téléphone réussie', {
      userId: user.id,
      phone: user.phone,
    });

    return res.json({
      success: true,
      message: 'Connexion réussie',
      user: {
        id: user.id,
        phone: user.phone,
        auth_method: user.auth_method,
        last_login: user.last_login,
      },
    });
  } catch (err) {
    logger.error('[AUTH-HYBRID] Échec connexion téléphone', { error: err.message });
    
    const errorMessages = {
      USER_NOT_FOUND: 'Numéro ou code incorrect',
      INVALID_CODE: 'Code incorrect',
      CODE_EXPIRED: 'Code expiré. Demandez un nouveau code.',
      PHONE_NOT_VERIFIED: 'Veuillez vérifier votre téléphone avant de vous connecter',
      USER_NOT_ACTIVE: 'Compte suspendu. Contactez le support.',
      INVALID_AUTH_METHOD: 'Méthode d\'authentification incorrecte',
    };
    
    return res.status(401).json({
      error: err.message,
      message: errorMessages[err.message] || 'Échec de la connexion',
    });
  }
});

/**
 * GET /auth/verify-email
 * Vérification email via token
 */
authHybridRouter.get('/verify-email', async (req, res) => {
  try {
    const { error, value } = verifyEmailSchema.validate(req.query);
    if (error) {
      return res.status(400).send(`
        <h1>❌ Lien de vérification invalide</h1>
        <p>Le lien de vérification est invalide ou a expiré.</p>
        <a href="${config.app.baseUrl}">Retour à l'accueil</a>
      `);
    }

    const user = await verifyEmail(value.token);

    logger.info('[AUTH-HYBRID] Email vérifié', {
      userId: user.id,
      email: user.email,
    });

    return res.send(`
      <h1>✅ Email vérifié avec succès!</h1>
      <p>Votre compte <strong>${user.email}</strong> est maintenant actif.</p>
      <p>Vous pouvez maintenant vous connecter et utiliser MDBAI.</p>
      <a href="${config.app.baseUrl}/auth/login">Se connecter</a>
    `);
  } catch (err) {
    logger.error('[AUTH-HYBRID] Échec vérification email', { error: err.message });
    
    return res.status(400).send(`
      <h1>❌ Vérification échouée</h1>
      <p>${err.message === 'INVALID_TOKEN' ? 'Token invalide ou expiré' : 'Erreur de vérification'}</p>
      <a href="${config.app.baseUrl}">Retour à l'accueil</a>
    `);
  }
});

/**
 * POST /auth/verify-phone
 * Vérification téléphone via code OTP
 */
authHybridRouter.post('/verify-phone', async (req, res) => {
  try {
    const { error, value } = verifyPhoneSchema.validate(req.body);
    if (error) {
      return res.status(400).json({
        error: 'VALIDATION_ERROR',
        details: error.details.map(d => d.message),
      });
    }

    const user = await verifyPhone(value.phone, value.code);

    logger.info('[AUTH-HYBRID] Téléphone vérifié', {
      userId: user.id,
      phone: user.phone,
    });

    return res.json({
      success: true,
      message: 'Téléphone vérifié avec succès',
      user: {
        id: user.id,
        phone: user.phone,
        status: user.status,
      },
    });
  } catch (err) {
    logger.error('[AUTH-HYBRID] Échec vérification téléphone', { error: err.message });
    
    const errorMessages = {
      USER_NOT_FOUND: 'Numéro non trouvé',
      INVALID_CODE: 'Code incorrect',
      CODE_EXPIRED: 'Code expiré. Demandez un nouveau code.',
    };
    
    return res.status(400).json({
      error: err.message,
      message: errorMessages[err.message] || 'Échec de la vérification',
    });
  }
});

/**
 * POST /auth/resend-otp
 * Renvoyer code OTP téléphone
 */
authHybridRouter.post('/resend-otp', async (req, res) => {
  try {
    const { phone } = req.body;
    
    if (!phone) {
      return res.status(400).json({
        error: 'VALIDATION_ERROR',
        message: 'Numéro de téléphone requis',
      });
    }

    const { user, code } = await generatePhoneOTP(phone);

    // Envoyer nouveau code via Telegram
    const telegram = new TelegramService();
    await telegram.sendMessage(
      user.telegram_id,
      `🔐 *Nouveau code de vérification MDBAI*\n\n` +
      `Votre code: \`${code}\`\n\n` +
      `Ce code expire dans 10 minutes.`,
      { parse_mode: 'Markdown' }
    );

    logger.info('[AUTH-HYBRID] Code OTP renvoyé', {
      userId: user.id,
      phone: user.phone,
    });

    return res.json({
      success: true,
      message: 'Nouveau code envoyé sur Telegram',
    });
  } catch (err) {
    logger.error('[AUTH-HYBRID] Échec renvoi OTP', { error: err.message });
    
    return res.status(400).json({
      error: err.message,
      message: 'Échec de l\'envoi du code',
    });
  }
});

/**
 * GET /auth/status-hybrid
 * Statut de connexion (toutes méthodes)
 */
authHybridRouter.get('/status-hybrid', (req, res) => {
  const connected = !!(req.session?.userId);
  
  return res.json({
    connected,
    user: connected ? {
      id: req.session.userId,
      auth_method: req.session.authMethod,
      email: req.session.email || null,
      phone: req.session.phone || null,
      github_login: req.session.githubLogin || null,
    } : null,
  });
});

/**
 * POST /auth/logout
 * Déconnexion
 */
authHybridRouter.post('/logout', (req, res) => {
  const userId = req.session?.userId;
  
  req.session?.destroy((err) => {
    if (err) {
      logger.error('[AUTH-HYBRID] Échec déconnexion', { error: err.message });
      return res.status(500).json({
        error: 'LOGOUT_FAILED',
        message: 'Échec de la déconnexion',
      });
    }
    
    logger.info('[AUTH-HYBRID] Déconnexion réussie', { userId });
    
    return res.json({
      success: true,
      message: 'Déconnexion réussie',
    });
  });
});

export default authHybridRouter;

// Made with Bob
