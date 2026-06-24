import Redis from 'ioredis';
import dotenv from 'dotenv';

dotenv.config();

const redis = new Redis(process.env.REDIS_URL);

async function fixUser() {
  try {
    const userId = 'user-bb293c29-615f-44d6-bd2b-f011b493a25e';
    const userKey = `mdbai:user:${userId}`;
    
    console.log('🔧 Correction status utilisateur...\n');
    
    // Récupérer utilisateur
    const userData = await redis.get(userKey);
    if (!userData) {
      console.log('❌ Utilisateur non trouvé');
      await redis.quit();
      return;
    }
    
    const user = JSON.parse(userData);
    console.log('📦 Avant:');
    console.log(`  - Status: ${user.status}`);
    console.log(`  - Email vérifié: ${user.email_verified}`);
    
    // Mettre à jour
    user.status = 'active';
    user.email_verified = true;
    user.email_verification_code = null;
    user.email_verification_code_expires = null;
    
    await redis.set(userKey, JSON.stringify(user));
    
    console.log('\n✅ Après:');
    console.log(`  - Status: ${user.status}`);
    console.log(`  - Email vérifié: ${user.email_verified}`);
    
    console.log('\n🎉 Utilisateur activé avec succès!');
    console.log('⚠️  IMPORTANT: Redémarrer le serveur pour vider le cache mémoire');
    
    await redis.quit();
  } catch (error) {
    console.error('Erreur:', error.message);
    process.exit(1);
  }
}

fixUser();

// Made with Bob
