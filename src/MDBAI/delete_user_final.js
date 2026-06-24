/**
 * SUPPRESSION FINALE UTILISATEUR
 * Utilise EXACTEMENT le même Redis que le serveur
 */

import { getRedisClient } from './src/services/redis.service.js';
import logger from './src/utils/logger.js';

const userId = 'user-a68cabd8-3c26-494a-ba22-98833eb05f6a';
const telegramId = '7324485506';
const email = 'vgac42@gmail.com';

(async () => {
  try {
    console.log('🗑️  SUPPRESSION UTILISATEUR AVEC LE MÊME REDIS QUE LE SERVEUR\n');
    
    const redis = getRedisClient();
    
    // Supprimer toutes les clés
    const keys = [
      `mdbai:user:${userId}`,
      `mdbai:user:telegram:${telegramId}`,
      `mdbai:user:email:${email}`
    ];
    
    for (const key of keys) {
      const result = await redis.del(key);
      console.log(`  ${result ? '✅ SUPPRIMÉ' : '❌ N\'EXISTE PAS'}: ${key}`);
    }
    
    // Vérification
    console.log('\n📊 VÉRIFICATION:');
    const check = await redis.get(`mdbai:user:telegram:${telegramId}`);
    console.log(`  Clé Telegram: ${check || 'NULL (OK)'}`);
    
    await redis.quit();
    console.log('\n✅ Terminé');
    process.exit(0);
  } catch (err) {
    console.error('❌ Erreur:', err.message);
    process.exit(1);
  }
})();

// Made with Bob
