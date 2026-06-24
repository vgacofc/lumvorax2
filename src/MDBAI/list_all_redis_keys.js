import { getRedisClient } from './src/services/redis.service.js';

const redis = getRedisClient();
console.log('🔍 Scan de TOUTES les clés Redis MDBAI...\n');

const keys = await redis.keys('mdbai:*');
console.log(`📊 Total: ${keys.length} clés trouvées\n`);

for (const key of keys) {
  const value = await redis.get(key);
  console.log(`🔑 ${key}`);
  if (value && value.length < 200) {
    console.log(`   → ${value}`);
  } else if (value) {
    const parsed = JSON.parse(value);
    console.log(`   → User: ${parsed.email || 'N/A'}, GitHub: ${parsed.github_login || 'N/A'}, Telegram: ${parsed.telegram_id || 'N/A'}`);
  }
  console.log('');
}

process.exit(0);
