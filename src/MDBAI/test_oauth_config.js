import { config } from './src/config.js';
import { GitHubService } from './src/services/github.service.js';

console.log('=== VÉRIFICATION CONFIGURATION GITHUB OAUTH ===\n');

console.log('CLIENT_ID:', config.github.clientId || '❌ VIDE');
console.log('CLIENT_SECRET:', config.github.clientSecret ? config.github.clientSecret.substring(0,10) + '...' : '❌ VIDE');
console.log('CALLBACK_URL:', config.github.callbackUrl);

console.log('\n=== TEST URL OAUTH ===');
const github = new GitHubService();
const url = github.getAuthUrl('7324485506');
console.log(url);

console.log('\n=== VALIDATION ===');
console.log('✅ client_id présent:', url.includes('client_id=Iv23'));
console.log('✅ redirect_uri ngrok:', url.includes('headboard-romp-crust.ngrok-free.dev'));
console.log('✅ state telegram_id:', url.includes('state=7324485506'));

// Made with Bob
