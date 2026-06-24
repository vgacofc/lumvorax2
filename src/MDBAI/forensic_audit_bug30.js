#!/usr/bin/env node
/**
 * AUDIT FORENSIQUE BUG #30 - Diagnostic complet
 * Objectif: Comprendre pourquoi l'utilisateur est détecté comme inscrit
 * alors que toutes les données Redis ont été supprimées
 */

import 'dotenv/config';
import Redis from 'ioredis';
import { getRedisClient } from './src/services/redis.service.js';
import { findUserByTelegram } from './src/services/user.service.js';
import logger from './src/utils/logger.js';

const TELEGRAM_ID_TEST = '7901234567'; // ID de test de l'utilisateur

console.log('\n=== AUDIT FORENSIQUE BUG #30 ===\n');

async function auditForensique() {
  try {
    // 1. VÉRIFIER LA CONFIGURATION REDIS
    console.log('📋 ÉTAPE 1: Configuration Redis');
    console.log('─────────────────────────────────');
    console.log('REDIS_URL depuis .env:', process.env.REDIS_URL ? '✓ Défini' : '✗ Manquant');
    console.log('URL complète:', process.env.REDIS_URL);
    console.log('');

    // 2. VÉRIFIER LA CONNEXION REDIS CLOUD
    console.log('🔌 ÉTAPE 2: Connexion Redis Cloud');
    console.log('─────────────────────────────────');
    const redisClient = getRedisClient();
    const pingResult = await redisClient.ping();
    console.log('Ping Redis Cloud:', pingResult);
    
    const info = await redisClient.info('server');
    const redisVersion = info.match(/redis_version:([^\r\n]+)/)?.[1];
    console.log('Version Redis:', redisVersion);
    console.log('');

    // 3. LISTER TOUTES LES CLÉS (PAS SEULEMENT mdbai:*)
    console.log('🔍 ÉTAPE 3: Scan TOUTES les clés Redis');
    console.log('─────────────────────────────────');
    
    let cursor = '0';
    let allKeys = [];
    do {
      const [newCursor, keys] = await redisClient.scan(cursor, 'COUNT', 1000);
      cursor = newCursor;
      allKeys = allKeys.concat(keys);
    } while (cursor !== '0');
    
    console.log(`Total de clés trouvées: ${allKeys.length}`);
    
    if (allKeys.length > 0) {
      console.log('\n📦 Clés par préfixe:');
      const prefixes = {};
      allKeys.forEach(key => {
        const prefix = key.split(':')[0];
        prefixes[prefix] = (prefixes[prefix] || 0) + 1;
      });
      Object.entries(prefixes).forEach(([prefix, count]) => {
        console.log(`  ${prefix}:* → ${count} clés`);
      });
      
      console.log('\n📝 Toutes les clés:');
      allKeys.forEach(key => console.log(`  - ${key}`));
    } else {
      console.log('⚠️  AUCUNE CLÉ TROUVÉE dans Redis Cloud!');
    }
    console.log('');

    // 4. CHERCHER SPÉCIFIQUEMENT LES CLÉS UTILISATEUR
    console.log('👤 ÉTAPE 4: Recherche clés utilisateur');
    console.log('─────────────────────────────────');
    
    const userPatterns = [
      'mdbai:user:*',
      'user:*',
      '*telegram*',
      `*${TELEGRAM_ID_TEST}*`
    ];
    
    for (const pattern of userPatterns) {
      const keys = await redisClient.keys(pattern);
      console.log(`Pattern "${pattern}": ${keys.length} clés`);
      if (keys.length > 0) {
        keys.forEach(key => console.log(`  - ${key}`));
      }
    }
    console.log('');

    // 5. TESTER findUserByTelegram AVEC LOGS
    console.log('🔬 ÉTAPE 5: Test findUserByTelegram()');
    console.log('─────────────────────────────────');
    console.log(`Recherche utilisateur avec telegram_id: ${TELEGRAM_ID_TEST}`);
    
    // Vérifier la clé d'index directement
    const indexKey = `mdbai:user:telegram:${TELEGRAM_ID_TEST}`;
    console.log(`Clé d'index cherchée: ${indexKey}`);
    const userId = await redisClient.get(indexKey);
    console.log(`Résultat GET direct: ${userId || 'null'}`);
    
    // Appeler la fonction du service
    const user = await findUserByTelegram(TELEGRAM_ID_TEST);
    console.log('Résultat findUserByTelegram():', user ? 'UTILISATEUR TROUVÉ ✓' : 'null');
    
    if (user) {
      console.log('\n⚠️  ANOMALIE DÉTECTÉE: Utilisateur trouvé!');
      console.log('Données utilisateur:', JSON.stringify(user, null, 2));
    }
    console.log('');

    // 6. VÉRIFIER SI UN REDIS LOCAL EXISTE
    console.log('🖥️  ÉTAPE 6: Vérification Redis local');
    console.log('─────────────────────────────────');
    try {
      const localRedis = new Redis({
        host: 'localhost',
        port: 6379,
        lazyConnect: true,
        retryStrategy: () => null,
      });
      
      await localRedis.connect();
      const localPing = await localRedis.ping();
      console.log('⚠️  REDIS LOCAL DÉTECTÉ!');
      console.log('Ping local:', localPing);
      
      // Compter les clés locales
      const localKeys = await localRedis.keys('*');
      console.log(`Clés dans Redis local: ${localKeys.length}`);
      
      if (localKeys.length > 0) {
        console.log('Clés locales:');
        localKeys.slice(0, 20).forEach(key => console.log(`  - ${key}`));
        if (localKeys.length > 20) {
          console.log(`  ... et ${localKeys.length - 20} autres`);
        }
      }
      
      await localRedis.quit();
    } catch (e) {
      console.log('✓ Pas de Redis local détecté (normal)');
    }
    console.log('');

    // 7. VÉRIFIER LES VARIABLES D'ENVIRONNEMENT DU PROCESSUS
    console.log('⚙️  ÉTAPE 7: Variables d\'environnement du processus');
    console.log('─────────────────────────────────');
    console.log('NODE_ENV:', process.env.NODE_ENV || 'non défini');
    console.log('REDIS_URL défini:', !!process.env.REDIS_URL);
    console.log('REDIS_HOST défini:', !!process.env.REDIS_HOST);
    console.log('REDIS_PORT défini:', !!process.env.REDIS_PORT);
    console.log('');

    // 8. DIAGNOSTIC FINAL
    console.log('🎯 DIAGNOSTIC FINAL');
    console.log('═════════════════════════════════');
    
    if (allKeys.length === 0) {
      console.log('✓ Redis Cloud est VIDE (comme attendu après suppression)');
    } else {
      console.log(`⚠️  Redis Cloud contient ${allKeys.length} clés`);
    }
    
    if (user) {
      console.log('❌ PROBLÈME: findUserByTelegram() retourne un utilisateur');
      console.log('');
      console.log('HYPOTHÈSES POSSIBLES:');
      console.log('1. Cache en mémoire dans le code (Map, Object)');
      console.log('2. Création automatique d\'utilisateur dans handleStart()');
      console.log('3. Données dans un autre namespace Redis');
      console.log('4. Connexion à un mauvais serveur Redis');
    } else {
      console.log('✓ findUserByTelegram() retourne null (comportement attendu)');
    }

  } catch (error) {
    console.error('\n❌ ERREUR DURANT L\'AUDIT:', error);
    console.error('Stack:', error.stack);
  } finally {
    process.exit(0);
  }
}

// Exécution
auditForensique().catch(console.error);

// Made with Bob
