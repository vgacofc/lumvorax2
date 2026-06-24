#!/usr/bin/env node
/**
 * SUPPRESSION NUCLÉAIRE - CONNEXION DIRECTE REDIS CLOUD
 * Bypass complet du système de cache du serveur
 */

import Redis from 'ioredis';

const REDIS_URL = 'redis://default:lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip@redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068';
const TELEGRAM_ID = '7324485506';
const USER_ID = 'user-a68cabd8-3c26-494a-ba22-98833eb05f6a';
const EMAIL = 'vgac42@gmail.com';

async function nuclearDelete() {
  console.log('☢️  SUPPRESSION NUCLÉAIRE - CONNEXION DIRECTE REDIS CLOUD\n');
  console.log(`📡 URL: ${REDIS_URL.replace(/:[^:@]+@/, ':***@')}\n`);
  
  // Connexion DIRECTE sans passer par redis.service.js
  const redis = new Redis(REDIS_URL, {
    maxRetriesPerRequest: null,
    enableReadyCheck: false,
  });
  
  await new Promise((resolve) => {
    redis.on('ready', () => {
      console.log('✅ Connecté à Redis Cloud\n');
      resolve();
    });
  });
  
  // 1. Lister TOUTES les clés mdbai:*
  console.log('🔍 Scan de toutes les clés mdbai:*...\n');
  const allKeys = await redis.keys('mdbai:*');
  console.log(`📦 ${allKeys.length} clés trouvées\n`);
  
  // 2. Filtrer les clés contenant nos identifiants
  const targetKeys = allKeys.filter(key => 
    key.includes(TELEGRAM_ID) || 
    key.includes(EMAIL) || 
    key.includes(USER_ID)
  );
  
  console.log(`🎯 ${targetKeys.length} clés cibles identifiées:\n`);
  targetKeys.forEach(key => console.log(`  - ${key}`));
  console.log();
  
  // 3. Suppression FORCÉE
  if (targetKeys.length > 0) {
    console.log('🗑️  Suppression en cours...\n');
    for (const key of targetKeys) {
      await redis.del(key);
      console.log(`  ✅ SUPPRIMÉ: ${key}`);
    }
  } else {
    console.log('⚠️  Aucune clé à supprimer\n');
  }
  
  // 4. Vérification finale
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
      const value = await redis.get(key);
      console.log(`  ❌ ERREUR: ${key} = ${value}`);
      allClear = false;
    } else {
      console.log(`  ✅ OK: ${key} n'existe pas`);
    }
  }
  
  // 5. Scan final pour confirmer
  console.log('\n🔍 Scan final...\n');
  const remainingKeys = await redis.keys(`*${TELEGRAM_ID}*`);
  if (remainingKeys.length > 0) {
    console.log(`  ⚠️  ${remainingKeys.length} clés restantes avec telegram_id:`);
    remainingKeys.forEach(key => console.log(`    - ${key}`));
    allClear = false;
  } else {
    console.log('  ✅ Aucune clé restante avec telegram_id');
  }
  
  await redis.quit();
  
  console.log(`\n${allClear ? '✅ SUCCÈS TOTAL' : '❌ ÉCHEC - Clés persistent'}`);
  console.log('\n⚠️  IMPORTANT: Redémarrer le serveur pour vider le cache mémoire!\n');
  
  process.exit(allClear ? 0 : 1);
}

nuclearDelete().catch(err => {
  console.error('❌ ERREUR:', err);
  process.exit(1);
});

// Made with Bob
