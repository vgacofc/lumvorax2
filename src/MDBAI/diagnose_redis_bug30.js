/**
 * DIAGNOSTIC BUG #30 - Bot affiche message INSCRIT au lieu de NON inscrit
 * 
 * Ce script liste TOUTES les clés Redis avec préfixe mdbai:* pour identifier
 * les clés orphelines qui font croire au bot que l'utilisateur est inscrit.
 * 
 * Utilisation:
 *   node diagnose_redis_bug30.js
 * 
 * Le script affiche:
 * - Toutes les clés mdbai:*
 * - Les clés associées au Telegram ID 7324485506
 * - Les clés associées à l'email vgac42@gmail.com
 */

import { getRedisClient } from './src/services/redis.service.js';
import logger from './src/utils/logger.js';

const TELEGRAM_ID = '7324485506';
const EMAIL = 'vgac42@gmail.com';

async function diagnoseRedis() {
  console.log('\n=== DIAGNOSTIC BUG #30 - Redis Keys ===\n');
  
  const redis = getRedisClient();
  
  try {
    // 1. Lister TOUTES les clés mdbai:*
    console.log('1. Recherche de toutes les clés mdbai:*...\n');
    const allKeys = await redis.keys('mdbai:*');
    
    if (allKeys.length === 0) {
      console.log('✓ Aucune clé mdbai:* trouvée dans Redis\n');
    } else {
      console.log(`✗ ${allKeys.length} clés mdbai:* trouvées:\n`);
      for (const key of allKeys) {
        const value = await redis.get(key);
        console.log(`  - ${key}`);
        if (value && value.length < 200) {
          console.log(`    Valeur: ${value}`);
        } else if (value) {
          console.log(`    Valeur: ${value.substring(0, 100)}... (tronqué)`);
        }
      }
      console.log('');
    }
    
    // 2. Chercher spécifiquement la clé telegram
    console.log(`2. Recherche de la clé Telegram ID ${TELEGRAM_ID}...\n`);
    const telegramKey = `mdbai:user:telegram:${TELEGRAM_ID}`;
    const telegramValue = await redis.get(telegramKey);
    
    if (telegramValue) {
      console.log(`✗ CLÉ TROUVÉE: ${telegramKey}`);
      console.log(`  Valeur (userId): ${telegramValue}\n`);
      
      // Chercher l'objet utilisateur complet
      const userKey = `mdbai:user:${telegramValue}`;
      const userData = await redis.get(userKey);
      if (userData) {
        console.log(`✗ Objet utilisateur trouvé: ${userKey}`);
        console.log(`  Données: ${userData}\n`);
      }
    } else {
      console.log(`✓ Aucune clé trouvée pour ${telegramKey}\n`);
    }
    
    // 3. Chercher la clé email
    console.log(`3. Recherche de la clé Email ${EMAIL}...\n`);
    const emailKey = `mdbai:user:email:${EMAIL}`;
    const emailValue = await redis.get(emailKey);
    
    if (emailValue) {
      console.log(`✗ CLÉ TROUVÉE: ${emailKey}`);
      console.log(`  Valeur (userId): ${emailValue}\n`);
    } else {
      console.log(`✓ Aucune clé trouvée pour ${emailKey}\n`);
    }
    
    // 4. Résumé et recommandations
    console.log('=== RÉSUMÉ ===\n');
    
    if (telegramValue || emailValue) {
      console.log('✗ PROBLÈME DÉTECTÉ: Des clés orphelines existent dans Redis\n');
      console.log('CLÉS À SUPPRIMER:');
      
      if (telegramValue) {
        console.log(`  - ${telegramKey}`);
        console.log(`  - mdbai:user:${telegramValue}`);
      }
      if (emailValue) {
        console.log(`  - ${emailKey}`);
        if (emailValue !== telegramValue) {
          console.log(`  - mdbai:user:${emailValue}`);
        }
      }
      
      console.log('\nCOMMANDE DE NETTOYAGE:');
      console.log('  node lumvorax2/src/MDBAI/cleanup_redis_bug30.js\n');
    } else {
      console.log('✓ Aucune clé orpheline détectée pour cet utilisateur\n');
      console.log('Le problème peut être ailleurs. Vérifier:');
      console.log('  1. Les logs du serveur');
      console.log('  2. Le cache applicatif');
      console.log('  3. Une autre instance Redis\n');
    }
    
  } catch (error) {
    console.error('✗ ERREUR lors du diagnostic:', error.message);
    console.error(error.stack);
  } finally {
    await redis.quit();
    console.log('Connexion Redis fermée.\n');
  }
}

// Exécuter le diagnostic
diagnoseRedis().catch(console.error);

// Made with Bob
