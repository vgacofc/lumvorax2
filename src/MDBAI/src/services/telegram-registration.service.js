/**
 * Telegram Registration Flow Service
 * Gestion du flux d'inscription complet multi-étapes
 * BUG-REGISTRATION-001 FIX
 * 
 * Flux complet:
 * 1. /start → Message bienvenue
 * 2. /register → Demander prénom
 * 3. Demander nom
 * 4. Demander sexe (homme/femme)
 * 5. Demander email
 * 6. Demander téléphone
 * 7. Demander password
 * 8. Générer code 6 chiffres
 * 9. Envoyer code via Telegram (Redis)
 * 10. Vérifier code
 * 11. Activer compte
 * 12. Demander URL GitHub
 */

import logger from '../utils/logger.js';
import { 
  getUserRegistrationState, 
  setUserRegistrationState, 
  clearUserRegistrationState,
  storeUser,
  getUserByTelegramId,
  getUserByEmail
} from './redis-registration.service.js';
import { sendVerificationCodeEmail, verifyEmailCode } from './email-redis.service.js';
import bcrypt from 'bcrypt';

/**
 * Démarre le processus d'inscription
 * @param {TelegramBot} bot - Instance du bot Telegram
 * @param {number} chatId - ID du chat
 * @param {string} telegramId - ID Telegram de l'utilisateur
 * @param {object} from - Objet from de Telegram
 */
export async function startRegistration(bot, chatId, telegramId, from) {
  try {
    // Vérifier si déjà inscrit
    const existingUser = await getUserByTelegramId(telegramId);
    
    if (existingUser && existingUser.isActive) {
      return bot.sendMessage(chatId,
        '✅ *Vous êtes déjà inscrit !*\n\n' +
        `Nom: ${existingUser.firstName} ${existingUser.lastName}\n` +
        `Email: ${existingUser.email}\n\n` +
        'Utilisez /github pour connecter votre compte GitHub.',
        { parse_mode: 'Markdown' }
      );
    }
    
    // Démarrer nouveau flux d'inscription
    await setUserRegistrationState(telegramId, 'ask_first_name', {
      telegramUsername: from.username || null,
      startedAt: Date.now()
    });
    
    return bot.sendMessage(chatId,
      '📝 *Inscription MDBAI*\n\n' +
      'Bienvenue ! Je vais vous guider à travers le processus d\'inscription.\n\n' +
      '👤 *Étape 1/7:* Quel est votre **prénom** ?',
      { parse_mode: 'Markdown' }
    );
  } catch (error) {
    logger.error('[TELEGRAM-REG] Erreur démarrage inscription:', error);
    return bot.sendMessage(chatId, '❌ Erreur lors du démarrage de l\'inscription. Réessayez avec /register');
  }
}

/**
 * Gère les entrées texte pendant le flux d'inscription
 * @param {TelegramBot} bot - Instance du bot Telegram
 * @param {object} msg - Message Telegram
 */
export async function handleRegistrationInput(bot, msg) {
  const chatId = msg.chat.id;
  const telegramId = String(msg.from.id);
  const text = msg.text?.trim();
  
  if (!text) {
    return;
  }
  
  try {
    // Récupérer l'état d'inscription
    const state = await getUserRegistrationState(telegramId);
    
    if (!state) {
      return; // Pas en cours d'inscription
    }
    
    const { step, data } = state;
    
    switch (step) {
      case 'ask_first_name':
        return await handleFirstName(bot, chatId, telegramId, text, data);
        
      case 'ask_last_name':
        return await handleLastName(bot, chatId, telegramId, text, data);
        
      case 'ask_gender':
        return await handleGender(bot, chatId, telegramId, text, data);
        
      case 'ask_email':
        return await handleEmail(bot, chatId, telegramId, text, data);
        
      case 'ask_phone':
        return await handlePhone(bot, chatId, telegramId, text, data);
        
      case 'ask_password':
        return await handlePassword(bot, chatId, telegramId, text, data);
        
      case 'verify_code':
        return await handleVerifyCode(bot, chatId, telegramId, text, data);
        
      case 'ask_github':
        return await handleGithubUrl(bot, chatId, telegramId, text, data);
        
      default:
        logger.warn(`[TELEGRAM-REG] Étape inconnue: ${step}`);
        return;
    }
  } catch (error) {
    logger.error('[TELEGRAM-REG] Erreur traitement input:', error);
    return bot.sendMessage(chatId, '❌ Erreur. Réessayez ou tapez /register pour recommencer.');
  }
}

/**
 * Gère l'entrée du prénom
 */
async function handleFirstName(bot, chatId, telegramId, text, data) {
  if (text.length < 2) {
    return bot.sendMessage(chatId, '❌ Le prénom doit contenir au moins 2 caractères.');
  }
  
  data.firstName = text;
  await setUserRegistrationState(telegramId, 'ask_last_name', data);
  
  return bot.sendMessage(chatId,
    '👤 *Étape 2/7:* Quel est votre **nom de famille** ?',
    { parse_mode: 'Markdown' }
  );
}

/**
 * Gère l'entrée du nom
 */
async function handleLastName(bot, chatId, telegramId, text, data) {
  if (text.length < 2) {
    return bot.sendMessage(chatId, '❌ Le nom doit contenir au moins 2 caractères.');
  }
  
  data.lastName = text;
  await setUserRegistrationState(telegramId, 'ask_gender', data);
  
  return bot.sendMessage(chatId,
    '⚧️ *Étape 3/7:* Quel est votre **sexe** ?\n\n' +
    'Répondez: `homme` ou `femme`',
    { parse_mode: 'Markdown' }
  );
}

/**
 * Gère l'entrée du sexe
 */
async function handleGender(bot, chatId, telegramId, text, data) {
  const gender = text.toLowerCase();
  
  if (!['homme', 'femme'].includes(gender)) {
    return bot.sendMessage(chatId, '❌ Répondez "homme" ou "femme"');
  }
  
  data.gender = gender;
  await setUserRegistrationState(telegramId, 'ask_email', data);
  
  return bot.sendMessage(chatId,
    '📧 *Étape 4/7:* Quelle est votre **adresse email** ?\n\n' +
    '(Vous recevrez un code de vérification)',
    { parse_mode: 'Markdown' }
  );
}

/**
 * Gère l'entrée de l'email
 */
async function handleEmail(bot, chatId, telegramId, text, data) {
  // Validation email
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  if (!emailRegex.test(text)) {
    return bot.sendMessage(chatId, '❌ Email invalide. Format attendu: exemple@domaine.com');
  }
  
  // Vérifier si email déjà utilisé
  const existingUser = await getUserByEmail(text);
  if (existingUser) {
    return bot.sendMessage(chatId,
      '❌ Cet email est déjà utilisé.\n\n' +
      'Si c\'est votre compte, utilisez /login'
    );
  }
  
  data.email = text;
  await setUserRegistrationState(telegramId, 'ask_phone', data);
  
  return bot.sendMessage(chatId,
    '📱 *Étape 5/7:* Quel est votre **numéro de téléphone** ?\n\n' +
    'Format international recommandé: +33612345678',
    { parse_mode: 'Markdown' }
  );
}

/**
 * Gère l'entrée du téléphone
 */
async function handlePhone(bot, chatId, telegramId, text, data) {
  // Validation basique du téléphone
  const phoneRegex = /^[\d\s\+\-\(\)]+$/;
  if (!phoneRegex.test(text) || text.replace(/\D/g, '').length < 8) {
    return bot.sendMessage(chatId, '❌ Numéro de téléphone invalide. Minimum 8 chiffres.');
  }
  
  data.phoneNumber = text;
  await setUserRegistrationState(telegramId, 'ask_password', data);
  
  return bot.sendMessage(chatId,
    '🔒 *Étape 6/7:* Choisissez un **mot de passe**\n\n' +
    'Minimum 8 caractères, avec au moins:\n' +
    '• Une majuscule\n' +
    '• Une minuscule\n' +
    '• Un chiffre',
    { parse_mode: 'Markdown' }
  );
}

/**
 * Gère l'entrée du password
 */
async function handlePassword(bot, chatId, telegramId, text, data) {
  // Validation password
  if (text.length < 8) {
    return bot.sendMessage(chatId, '❌ Mot de passe trop court (minimum 8 caractères)');
  }
  
  if (!/[A-Z]/.test(text)) {
    return bot.sendMessage(chatId, '❌ Le mot de passe doit contenir au moins une majuscule');
  }
  
  if (!/[a-z]/.test(text)) {
    return bot.sendMessage(chatId, '❌ Le mot de passe doit contenir au moins une minuscule');
  }
  
  if (!/[0-9]/.test(text)) {
    return bot.sendMessage(chatId, '❌ Le mot de passe doit contenir au moins un chiffre');
  }
  
  // Hash du password
  const hashedPassword = await bcrypt.hash(text, 10);
  data.password = hashedPassword;
  
  // Générer code 6 chiffres
  const code = Math.floor(100000 + Math.random() * 900000).toString();
  data.verificationCode = code;
  
  // Envoyer code via email-redis
  await sendVerificationCodeEmail(data.email, code, telegramId);
  
  await setUserRegistrationState(telegramId, 'verify_code', data);
  
  return bot.sendMessage(chatId,
    '✅ *Étape 7/7:* Vérification email\n\n' +
    `📧 Un code à 6 chiffres a été envoyé à:\n\`${data.email}\`\n\n` +
    '🔑 Entrez le code reçu pour activer votre compte.\n\n' +
    '⏱️ Le code expire dans 10 minutes.',
    { parse_mode: 'Markdown' }
  );
}

/**
 * Gère la vérification du code
 */
async function handleVerifyCode(bot, chatId, telegramId, text, data) {
  const code = text.trim();
  
  // Vérifier le code via email-redis
  const verification = await verifyEmailCode(data.email, code);
  
  if (!verification.valid) {
    return bot.sendMessage(chatId,
      `❌ ${verification.error}\n\n` +
      `💡 ${verification.hint || 'Vérifiez le code et réessayez.'}`
    );
  }
  
  // Créer l'utilisateur
  const userId = `user_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  const username = `${data.firstName.toLowerCase()}_${data.lastName.toLowerCase()}`;
  
  const user = {
    id: userId,
    telegramId,
    username,
    firstName: data.firstName,
    lastName: data.lastName,
    gender: data.gender,
    email: data.email,
    phoneNumber: data.phoneNumber,
    password: data.password,
    isActive: true,
    registrationCompleted: true,
    credits: 10, // Crédits de bienvenue
    role: 'user',
    createdAt: new Date().toISOString(),
    updatedAt: new Date().toISOString(),
    preferences: {
      notifications: true,
      language: 'fr',
      theme: 'dark'
    },
    metadata: {
      telegramUsername: data.telegramUsername,
      registrationCompletedAt: new Date().toISOString()
    }
  };
  
  // Sauvegarder dans Redis
  await storeUser(user);
  
  // Nettoyer l'état d'inscription
  await clearUserRegistrationState(telegramId);
  
  logger.info(`[TELEGRAM-REG] Inscription réussie: ${userId}`, {
    email: user.email,
    telegramId
  });
  
  // Demander URL GitHub (optionnel)
  await setUserRegistrationState(telegramId, 'ask_github', { userId });
  
  return bot.sendMessage(chatId,
    '🎉 *Inscription réussie !*\n\n' +
    `Bienvenue ${data.firstName} ${data.lastName} !\n\n` +
    `✅ Votre compte MDBAI est maintenant actif.\n` +
    `💰 Vous avez reçu 10 crédits de bienvenue.\n\n` +
    `🔗 *Dernière étape (optionnelle):*\n` +
    `Connectez votre compte GitHub pour analyser vos dépôts.\n\n` +
    `Envoyez l'URL de votre profil GitHub ou tapez "skip" pour passer.`,
    { parse_mode: 'Markdown' }
  );
}

/**
 * Gère l'URL GitHub (optionnel)
 */
async function handleGithubUrl(bot, chatId, telegramId, text, data) {
  if (text.toLowerCase() === 'skip') {
    await clearUserRegistrationState(telegramId);
    return bot.sendMessage(chatId,
      '✅ *Configuration terminée !*\n\n' +
      'Vous pouvez connecter GitHub plus tard avec /github\n\n' +
      'Commandes disponibles:\n' +
      '• /analyze <url> - Analyser un dépôt\n' +
      '• /status - Voir vos analyses\n' +
      '• /help - Aide complète',
      { parse_mode: 'Markdown' }
    );
  }
  
  // Validation URL GitHub
  const githubRegex = /^https?:\/\/(www\.)?github\.com\/[\w-]+\/?$/;
  if (!githubRegex.test(text)) {
    return bot.sendMessage(chatId,
      '❌ URL GitHub invalide.\n\n' +
      'Format attendu: https://github.com/username\n\n' +
      'Ou tapez "skip" pour passer.'
    );
  }
  
  // Extraire le username
  const username = text.split('/').pop().replace(/\/$/, '');
  
  // Mettre à jour l'utilisateur
  const user = await getUserByTelegramId(telegramId);
  if (user) {
    user.metadata = user.metadata || {};
    user.metadata.githubUsername = username;
    user.metadata.githubProfileUrl = text;
    await storeUser(user);
  }
  
  await clearUserRegistrationState(telegramId);
  
  return bot.sendMessage(chatId,
    '✅ *GitHub connecté !*\n\n' +
    `Profil: ${text}\n\n` +
    'Vous pouvez maintenant analyser vos dépôts avec /analyze\n\n' +
    'Commandes disponibles:\n' +
    '• /analyze <url> - Analyser un dépôt\n' +
    '• /status - Voir vos analyses\n' +
    '• /help - Aide complète',
    { parse_mode: 'Markdown' }
  );
}

export default {
  startRegistration,
  handleRegistrationInput
};

// Made with Bob