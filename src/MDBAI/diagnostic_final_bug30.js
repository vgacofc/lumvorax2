/**
 * DIAGNOSTIC FINAL BUG #30
 * Se connecte à Redis EXACTEMENT comme le serveur
 * et vérifie les données utilisateur
 */

import { getRedisClient } from './src/services/redis.service.js';
import { findUserByTelegram } from './src/services/user.service.js';
import logger from './src/utils/logger.js';

const TELEGRAM_ID = '7324485506';

(async () => {
  try {
    console.log('🔍 DIAGNOSTIC FINAL BUG #30\n');
    console.log('Telegram ID:', TELEGRAM_ID);
    console.log('REDIS_URL:', process.env.REDIS_URL ? 'Défini' : 'Non défini');
    console.log('');
    
    // Obtenir le client Redis
    const redis = getRedisClient();
    
    // 1. Lister TOUTES les clés
    console.log('📊 ÉTAPE 1: Scan de toutes les clés Redis');
    const allKeys = await redis.keys('*');
    console.log('Total clés:', allKeys.length);
    
    if (allKeys.length > 0) {
      console.log('\nClés trouvées:');
      for (const key of allKeys) {
        console.log('  -', key);
      }
    }
    
    // 2. Chercher l'utilisateur via le service
    console.log('\n📊 ÉTAPE 2: Recherche utilisateur via findUserByTelegram()');
    const user = await findUserByTelegram(TELEGRAM_ID);
    
    if (user) {
      console.log('⚠️  UTILISATEUR TROUVÉ:');
      console.log('  ID:', user.id);
      console.log('  Email:', user.email);
      console.log('  Status:', user.status);
      console.log('  Email vérifié:', user.email_verified);
      console.log('  Créé le:', new Date(user.created_at).toLocaleString('fr-FR'));
      console.log('  GitHub:', user.github_login || 'Non connecté');
    } else {
      console.log('✅ Aucun utilisateur trouvé (comportement attendu)');
    }
    
    // 3. Vérifier les clés spécifiques
    console.log('\n📊 ÉTAPE 3: Vérification clés spécifiques');
    const telegramKey = `mdbai:user:telegram:${TELEGRAM_ID}`;
    const telegramValue = await redis.get(telegramKey);
    console.log('Clé', telegramKey + ':', telegramValue || 'N/A');
    
    if (telegramValue) {
      const userKey = `mdbai:user:${telegramValue}`;
      const userData = await redis.get(userKey);
      console.log('Clé', userKey + ':', userData ? 'EXISTE' : 'N/A');
    }
    
    await redis.quit();
    console.log('\n✅ Diagnostic terminé');
    process.exit(0);
  } catch (err) {
    console.error('❌ Erreur:', err.message);
    console.error(err.stack);
    process.exit(1);
  }
})();

// Made with Bob
