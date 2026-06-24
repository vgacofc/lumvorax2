import { getRedisClient } from './src/services/redis.service.js';

const telegramId = '7324485506'; // ID de VGACIA
const redis = getRedisClient();

console.log(`🔍 Recherche par telegram_id: ${telegramId}`);

const userId = await redis.get(`mdbai:user:telegram:${telegramId}`);
if (!userId) {
  console.log('⚠️  Aucun utilisateur trouvé');
  process.exit(0);
}

const userData = await redis.get(`mdbai:user:${userId}`);
const user = JSON.parse(userData);

console.log(`✅ Utilisateur trouvé:`);
console.log(`   - ID: ${user.id}`);
console.log(`   - Email: ${user.email || 'N/A'}`);
console.log(`   - Telegram: ${user.telegram_id}`);
console.log(`   - GitHub: ${user.github_login || 'N/A'}`);

console.log(`\n🗑️  Suppression...`);
await redis.del(`mdbai:user:${userId}`);
await redis.del(`mdbai:user:telegram:${telegramId}`);
if (user.email) await redis.del(`mdbai:user:email:${user.email}`);
if (user.github_login) await redis.del(`mdbai:user:github:${user.github_login}`);

console.log(`✅ Utilisateur supprimé!`);
process.exit(0);
