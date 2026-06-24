#!/usr/bin/env node
/**
 * Test avec le vrai telegram_id trouvé dans Redis
 */

import 'dotenv/config';
import { getRedisClient } from './src/services/redis.service.js';
import { findUserByTelegram } from './src/services/user.service.js';

const REAL_TELEGRAM_ID = '7324485506'; // ID trouvé dans Redis

async function test() {
  console.log('\n=== TEST AVEC LE VRAI TELEGRAM_ID ===\n');
  
  const redis = getRedisClient();
  
  // 1. Vérifier l'index
  const indexKey = `mdbai:user:telegram:${REAL_TELEGRAM_ID}`;
  console.log(`Clé d'index: ${indexKey}`);
  const userId = await redis.get(indexKey);
  console.log(`User ID depuis index: ${userId}`);
  
  // 2. Récupérer les données utilisateur
  if (userId) {
    const userKey = `mdbai:user:${userId}`;
    console.log(`\nClé utilisateur: ${userKey}`);
    const userData = await redis.get(userKey);
    
    if (userData) {
      const user = JSON.parse(userData);
      console.log('\n📋 DONNÉES UTILISATEUR:');
      console.log(JSON.stringify(user, null, 2));
    }
  }
  
  // 3. Tester findUserByTelegram
  console.log('\n🔬 Test findUserByTelegram():');
  const user = await findUserByTelegram(REAL_TELEGRAM_ID);
  console.log('Résultat:', user ? 'UTILISATEUR TROUVÉ ✓' : 'null');
  
  if (user) {
    console.log('\nDétails:');
    console.log('- ID:', user.id);
    console.log('- Email:', user.email);
    console.log('- GitHub:', user.github_username);
    console.log('- Telegram ID:', user.telegram_id);
    console.log('- Statut:', user.status);
    console.log('- Créé le:', user.created_at);
  }
  
  process.exit(0);
}

test().catch(console.error);

// Made with Bob
