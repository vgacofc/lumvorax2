#!/usr/bin/env node
/**
 * SUPPRESSION FORCÉE UTILISATEUR - SERVEUR EN COURS D'EXÉCUTION
 * Supprime l'utilisateur de Redis pendant que le serveur tourne
 */

import { getRedisClient } from './src/services/redis.service.js';
import logger from './src/utils/logger.js';

const TELEGRAM_ID = '7324485506';
const USER_ID = 'user-a68cabd8-3c26-494a-ba22-98833eb05f6a';
const EMAIL = 'vgac42@gmail.com';

async function forceDeleteUser() {
  console.log('🔥 SUPPRESSION FORCÉE - SERVEUR EN COURS\n');
  
  const redis = getRedisClient();
  
  // 1. Supprimer TOUTES les clés utilisateur
  const keys = [
    `mdbai:user:${USER_ID}`,
    `mdbai:user:telegram:${TELEGRAM_ID}`,
    `mdbai:user:email:${EMAIL}`,
    `mdbai:user:email:${EMAIL.toLowerCase()}`,
    `mdbai:user:telegram_id:${TELEGRAM_ID}`,
    `mdbai:session:${TELEGRAM_ID}`,
    `mdbai:pending:${TELEGRAM_ID}`,
    `mdbai:verification:${EMAIL}`,
    `mdbai:verification:${EMAIL.toLowerCase()}`
  ];
  
  console.log('🗑️  Suppression de toutes les clés possibles...\n');
  
  let deleted = 0;
  for (const key of keys) {
    const exists = await redis.exists(key);
    if (exists) {
      await redis.del(key);
      console.log(`  ✅ SUPPRIMÉ: ${key}`);
      deleted++;
    } else {
      console.log(`  ⚪ N'existe pas: ${key}`);
    }
  }
  
  // 2. Scanner TOUTES les clés contenant l'email ou telegram_id
  console.log('\n🔍 Scan de toutes les clés Redis...\n');
  
  const patterns = [
    `*${TELEGRAM_ID}*`,
    `*${EMAIL}*`,
    `*${USER_ID}*`
  ];
  
  for (const pattern of patterns) {
    const foundKeys = await redis.keys(pattern);
    if (foundKeys.length > 0) {
      console.log(`  📦 Pattern "${pattern}": ${foundKeys.length} clés trouvées`);
      for (const key of foundKeys) {
        await redis.del(key);
        console.log(`    ✅ SUPPRIMÉ: ${key}`);
        deleted++;
      }
    }
  }
  
  // 3. Vérification finale
  console.log('\n📊 VÉRIFICATION FINALE:\n');
  
  const checkKeys = [
    `mdbai:user:telegram:${TELEGRAM_ID}`,
    `mdbai:user:email:${EMAIL}`,
    `mdbai:user:${USER_ID}`
  ];
  
  let allClear = true;
  for (const key of checkKeys) {
    const exists = await redis.exists(key);
    if (exists) {
      console.log(`  ❌ ERREUR: ${key} existe toujours !`);
      allClear = false;
    } else {
      console.log(`  ✅ OK: ${key} supprimé`);
    }
  }
  
  console.log(`\n📈 RÉSULTAT: ${deleted} clés supprimées`);
  console.log(allClear ? '\n✅ SUCCÈS TOTAL\n' : '\n❌ ÉCHEC - Clés persistent\n');
  
  process.exit(allClear ? 0 : 1);
}

forceDeleteUser().catch(err => {
  console.error('❌ ERREUR:', err);
  process.exit(1);
});

// Made with Bob
