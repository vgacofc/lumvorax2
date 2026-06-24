#!/usr/bin/env node
/**
 * Test génération Installation Token GitHub App
 * Vérifie que GITHUB_APP_ID + GITHUB_PRIVATE_KEY sont valides
 */

import { GitHubService } from './src/services/github.service.js';
import logger from './src/utils/logger.js';

async function testInstallationToken() {
  console.log('\n🔍 TEST INSTALLATION TOKEN GITHUB APP\n');
  console.log('═'.repeat(60));
  
  // Vérifier variables d'environnement
  const appId = process.env.GITHUB_APP_ID || process.env.MDBAI_APP_ID;
  const privateKey = process.env.GITHUB_PRIVATE_KEY || process.env.MDBAI_PRIVATE_KEY;
  
  console.log(`\n📋 Configuration:`);
  console.log(`   GITHUB_APP_ID: ${appId || '❌ MANQUANT'}`);
  console.log(`   GITHUB_PRIVATE_KEY: ${privateKey ? `✅ Présente (${privateKey.length} caractères)` : '❌ MANQUANTE'}`);
  
  if (!appId || !privateKey) {
    console.error('\n❌ ERREUR: Variables d\'environnement manquantes');
    process.exit(1);
  }
  
  // Test génération token
  console.log(`\n🔐 Test génération Installation Token...`);
  console.log(`   Dépôt cible: Vgactec/lvgacnatif`);
  
  try {
    const github = new GitHubService(null);
    const token = await github.getInstallationToken('Vgactec', 'lvgacnatif');
    
    console.log(`\n✅ SUCCESS: Installation Token généré`);
    console.log(`   Token: ${token.substring(0, 20)}...${token.substring(token.length - 10)}`);
    console.log(`   Longueur: ${token.length} caractères`);
    console.log(`   Préfixe: ${token.substring(0, 4)}`);
    
    if (token.startsWith('ghs_')) {
      console.log(`\n🎉 VALIDATION: Token valide (préfixe ghs_)`);
    } else {
      console.log(`\n⚠️  WARNING: Token ne commence pas par ghs_`);
    }
    
    console.log('\n' + '═'.repeat(60));
    console.log('✅ TEST RÉUSSI - Installation Token fonctionnel\n');
    process.exit(0);
    
  } catch (error) {
    console.error(`\n❌ ERREUR lors de la génération du token:`);
    console.error(`   Message: ${error.message}`);
    console.error(`   Stack: ${error.stack}`);
    console.log('\n' + '═'.repeat(60));
    console.log('❌ TEST ÉCHOUÉ\n');
    process.exit(1);
  }
}

testInstallationToken();

// Made with Bob
