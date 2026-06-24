import Redis from 'ioredis';
import dotenv from 'dotenv';

dotenv.config();

const redis = new Redis(process.env.REDIS_URL);

async function checkUser() {
  try {
    console.log('🔍 Vérification utilisateur gabrielvgac@gmail.com...\n');
    
    // Chercher par telegram ID
    const telegramKey = 'mdbai:user:telegram:7324485506';
    const userId = await redis.get(telegramKey);
    console.log(`Clé Telegram: ${telegramKey}`);
    console.log(`User ID trouvé: ${userId || 'NULL'}\n`);
    
    if (userId) {
      // Récupérer l'utilisateur complet
      const userKey = `mdbai:user:${userId}`;
      const userData = await redis.get(userKey);
      
      if (userData) {
        const user = JSON.parse(userData);
        console.log('📦 Données utilisateur:');
        console.log(`  - ID: ${user.id}`);
        console.log(`  - Email: ${user.email}`);
        console.log(`  - Status: ${user.status}`);
        console.log(`  - Email vérifié: ${user.email_verified}`);
        console.log(`  - Telegram ID: ${user.telegram_id}`);
        console.log(`  - Créé le: ${new Date(user.created_at).toLocaleString('fr-FR')}`);
        
        if (user.status === 'pending') {
          console.log('\n⚠️  PROBLÈME: Status = pending (devrait être active)');
        } else {
          console.log('\n✅ Status correct');
        }
      }
    } else {
      console.log('❌ Utilisateur non trouvé dans Redis');
    }
    
    await redis.quit();
  } catch (error) {
    console.error('Erreur:', error.message);
    process.exit(1);
  }
}

checkUser();

// Made with Bob
