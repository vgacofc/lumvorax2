/**
 * Script de correction: Ajouter telegram_id aux utilisateurs existants
 * Usage: node fix_telegram_id.js <email> <telegram_id>
 */

import 'dotenv/config';
import { findUserByEmail, updateUser } from './src/services/user.service.js';
import logger from './src/utils/logger.js';

const email = process.argv[2];
const telegramId = process.argv[3];

if (!email || !telegramId) {
  console.error('Usage: node fix_telegram_id.js <email> <telegram_id>');
  console.error('Exemple: node fix_telegram_id.js vgac42@gmail.com 123456789');
  process.exit(1);
}

async function fixTelegramId() {
  try {
    logger.info(`[FIX] Recherche utilisateur: ${email}`);
    const user = await findUserByEmail(email);
    
    if (!user) {
      logger.error(`[FIX] Utilisateur introuvable: ${email}`);
      process.exit(1);
    }
    
    logger.info(`[FIX] Utilisateur trouvé:`, {
      id: user.id,
      email: user.email,
      telegram_id_actuel: user.telegram_id,
      email_verified: user.email_verified
    });
    
    user.telegram_id = telegramId;
    await updateUser(user);
    
    logger.info(`[FIX] ✅ telegram_id mis à jour: ${telegramId}`);
    console.log(`\n✅ Succès! telegram_id=${telegramId} ajouté pour ${email}\n`);
    
    process.exit(0);
  } catch (error) {
    logger.error('[FIX] Erreur:', error);
    process.exit(1);
  }
}

fixTelegramId();

// Made with Bob
