#!/usr/bin/env node
/**
 * Script pour supprimer un utilisateur de Redis Cloud
 * Usage: node delete_user_redis.js <email>
 */

import { getRedisClient } from './src/services/redis.service.js';
import { findUserByEmail } from './src/services/user.service.js';
import logger from './src/utils/logger.js';

const email = process.argv[2];

if (!email) {
  console.error('❌ Usage: node delete_user_redis.js <email>');
  console.error('   Exemple: node delete_user_redis.js vgac42@gmail.com');
  process.exit(1);
}

async function deleteUser() {
  try {
    // Connexion Redis
    const redis = getRedisClient();
    
    // Recherche utilisateur
    console.log(`🔍 Recherche utilisateur: ${email}`);
    const user = await findUserByEmail(email);
    
    if (!user) {
      console.log(`⚠️  Utilisateur ${email} introuvable dans Redis`);
      process.exit(0);
    }
    
    console.log(`✅ Utilisateur trouvé:`);
    console.log(`   - ID: ${user.id}`);
    console.log(`   - Email: ${user.email}`);
    console.log(`   - Telegram ID: ${user.telegram_id || 'N/A'}`);
    console.log(`   - GitHub: ${user.github_login || 'N/A'}`);
    console.log(`   - Status: ${user.status}`);
    console.log(`   - Créé le: ${user.created_at}`);
    
    // Suppression de toutes les clés Redis associées
    console.log(`\n🗑️  Suppression des clés Redis...`);
    
    const keysToDelete = [
      `mdbai:user:${user.id}`,
      `mdbai:user:email:${user.email}`,
    ];
    
    if (user.telegram_id) {
      keysToDelete.push(`mdbai:user:telegram:${user.telegram_id}`);
    }
    
    if (user.github_login) {
      keysToDelete.push(`mdbai:user:github:${user.github_login}`);
    }
    
    if (user.phone) {
      keysToDelete.push(`mdbai:user:phone:${user.phone}`);
    }
    
    for (const key of keysToDelete) {
      const deleted = await redis.del(key);
      if (deleted) {
        console.log(`   ✅ Supprimé: ${key}`);
      } else {
        console.log(`   ⚠️  Clé inexistante: ${key}`);
      }
    }
    
    console.log(`\n✅ Utilisateur ${email} supprimé avec succès de Redis Cloud`);
    console.log(`\n💡 Vous pouvez maintenant recommencer l'inscription avec /register`);
    
    process.exit(0);
    
  } catch (error) {
    console.error(`❌ Erreur lors de la suppression:`, error.message);
    logger.error('[DELETE_USER] Erreur', { error: error.message, email });
    process.exit(1);
  }
}

deleteUser();

// Made with Bob
