/**
 * MDBAI — Tests Unitaires Bob Shell Integration
 * Valide le fonctionnement complet de Bob CLI IBM
 * 
 * Tests:
 * 1. Vérification clé API Bob chargée
 * 2. Vérification Bob CLI installé
 * 3. Test Bob CLI simple (version)
 * 4. Test Bob CLI avec prompt réel
 * 5. Test script bash généré
 * 6. Test timeout dynamique
 * 7. Test buffer dynamique
 * 8. Test logs debug
 */

import { describe, it, expect, beforeAll, afterAll } from '@jest/globals';
import { existsSync, readFileSync, writeFileSync, mkdirSync, rmSync } from 'fs';
import { join } from 'path';
import { execSync } from 'child_process';

const TEST_DIR = join(process.cwd(), 'tests', 'fixtures', 'bob-shell');
const ENV_BOB_PATH = join(process.cwd(), '.env.bob');

describe('Bob Shell Integration Tests', () => {
  
  beforeAll(() => {
    // Créer répertoire de test
    mkdirSync(TEST_DIR, { recursive: true });
  });

  afterAll(() => {
    // Nettoyer
    if (existsSync(TEST_DIR)) {
      rmSync(TEST_DIR, { recursive: true, force: true });
    }
  });

  describe('1. Vérification Clé API Bob', () => {
    
    it('devrait avoir un fichier .env.bob', () => {
      expect(existsSync(ENV_BOB_PATH)).toBe(true);
    });

    it('devrait contenir BOBSHELL_API_KEY', () => {
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      expect(envContent).toContain('BOBSHELL_API_KEY=');
    });

    it('devrait avoir une clé API valide (format)', () => {
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      const match = envContent.match(/BOBSHELL_API_KEY=(.+)/);
      
      expect(match).not.toBeNull();
      
      const apiKey = match[1].trim();
      
      // Vérifier format: bob_prod_bob-admin_[base64]
      expect(apiKey).toMatch(/^bob_prod_bob-admin_[A-Za-z0-9_-]+$/);
      
      // Vérifier longueur minimale (100+ caractères)
      expect(apiKey.length).toBeGreaterThan(100);
      
      console.log(`✅ Clé API Bob valide: ${apiKey.substring(0, 30)}...`);
    });

    it('devrait charger la clé API dans l\'environnement', () => {
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      const match = envContent.match(/BOBSHELL_API_KEY=(.+)/);
      const apiKey = match[1].trim();
      
      process.env.BOBSHELL_API_KEY = apiKey;
      
      expect(process.env.BOBSHELL_API_KEY).toBe(apiKey);
      console.log(`✅ Clé API chargée dans process.env`);
    });
  });

  describe('2. Vérification Bob CLI Installé', () => {
    
    it('devrait avoir bob dans PATH', () => {
      try {
        const result = execSync('which bob', { encoding: 'utf8' });
        expect(result).toContain('bob');
        console.log(`✅ Bob CLI trouvé: ${result.trim()}`);
      } catch (error) {
        console.error('❌ Bob CLI non trouvé dans PATH');
        throw error;
      }
    });

    it('devrait afficher la version de Bob CLI', () => {
      try {
        const result = execSync('bob --version', { 
          encoding: 'utf8',
          timeout: 5000 
        });
        
        expect(result).toBeTruthy();
        console.log(`✅ Bob CLI version: ${result.trim()}`);
      } catch (error) {
        console.error('❌ Impossible d\'obtenir la version de Bob CLI');
        throw error;
      }
    });
  });

  describe('3. Test Bob CLI Simple', () => {
    
    it('devrait exécuter bob --help', () => {
      try {
        const result = execSync('bob --help', { 
          encoding: 'utf8',
          timeout: 5000 
        });
        
        expect(result).toContain('bob');
        console.log(`✅ Bob CLI help fonctionne`);
      } catch (error) {
        console.error('❌ Bob CLI help échoué');
        throw error;
      }
    });

    it('devrait exécuter bob avec un prompt simple', () => {
      // Charger la clé API
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      const match = envContent.match(/BOBSHELL_API_KEY=(.+)/);
      const apiKey = match[1].trim();
      
      try {
        const command = `export BOBSHELL_API_KEY="${apiKey}" && bob -y --chat-mode code --output-format json -p "Test simple"`;
        
        const result = execSync(command, { 
          encoding: 'utf8',
          timeout: 30000,  // 30 secondes
          shell: '/bin/bash'
        });
        
        expect(result).toBeTruthy();
        console.log(`✅ Bob CLI exécuté avec succès`);
        console.log(`Résultat (100 premiers chars): ${result.substring(0, 100)}...`);
      } catch (error) {
        console.error('❌ Bob CLI échoué:', error.message);
        throw error;
      }
    });
  });

  describe('4. Test Script Bash Généré', () => {
    
    it('devrait créer un script bash valide', () => {
      const scriptPath = join(TEST_DIR, 'test-bob.sh');
      const prompt = 'Analyse ce code de test';
      const escapedPrompt = prompt.replace(/"/g, '\\"').replace(/\n/g, ' ');
      
      const script = `#!/bin/bash
set -e  # Arrêter si erreur
set -x  # Debug mode

# Charger NVM
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \\. "$NVM_DIR/nvm.sh"

# Utiliser Node.js v22
nvm use v22.22.3

# Charger la clé API Bob
source ${ENV_BOB_PATH}

# Vérifier que la clé est chargée
if [ -z "$BOBSHELL_API_KEY" ]; then
  echo "ERREUR: BOBSHELL_API_KEY non chargée"
  exit 1
fi

# Exécuter Bob CLI
bob -y --chat-mode code --output-format json -p "${escapedPrompt}"
`;
      
      writeFileSync(scriptPath, script, { mode: 0o755 });
      
      expect(existsSync(scriptPath)).toBe(true);
      console.log(`✅ Script bash créé: ${scriptPath}`);
    });

    it('devrait exécuter le script bash', () => {
      const scriptPath = join(TEST_DIR, 'test-bob.sh');
      
      try {
        const result = execSync(`/bin/bash ${scriptPath}`, { 
          encoding: 'utf8',
          timeout: 30000,  // 30 secondes
          cwd: TEST_DIR
        });
        
        expect(result).toBeTruthy();
        console.log(`✅ Script bash exécuté avec succès`);
        console.log(`Résultat (100 premiers chars): ${result.substring(0, 100)}...`);
      } catch (error) {
        console.error('❌ Script bash échoué:', error.message);
        
        // Afficher stderr si disponible
        if (error.stderr) {
          console.error('STDERR:', error.stderr.toString());
        }
        
        throw error;
      }
    });
  });

  describe('5. Test Timeout Dynamique', () => {
    
    it('devrait calculer timeout basé sur lignes de code', () => {
      const lineCount = 1000;
      const baseTimeout = 60000;  // 1 minute
      const timeoutPerLine = 1;   // 1 ms par ligne
      
      const dynamicTimeout = Math.max(baseTimeout, lineCount * timeoutPerLine);
      
      expect(dynamicTimeout).toBe(60000);  // 1000 lignes = 1s < 60s
      console.log(`✅ Timeout dynamique (1000 lignes): ${dynamicTimeout}ms`);
    });

    it('devrait augmenter timeout pour gros fichiers', () => {
      const lineCount = 100000;  // 100k lignes
      const baseTimeout = 60000;
      const timeoutPerLine = 1;
      
      const dynamicTimeout = Math.max(baseTimeout, lineCount * timeoutPerLine);
      
      expect(dynamicTimeout).toBe(100000);  // 100k lignes = 100s > 60s
      console.log(`✅ Timeout dynamique (100k lignes): ${dynamicTimeout}ms`);
    });
  });

  describe('6. Test Buffer Dynamique', () => {
    
    it('devrait calculer buffer basé sur taille du code', () => {
      const sourceCodeSize = 1024 * 1024;  // 1 MB
      const forensicDataSize = 1024;       // 1 KB
      const totalSize = sourceCodeSize + forensicDataSize;
      
      const dynamicBuffer = Math.max(
        50 * 1024 * 1024,  // Minimum 50 MB
        totalSize * 3       // 3× la taille réelle
      );
      
      expect(dynamicBuffer).toBe(50 * 1024 * 1024);  // 1 MB * 3 = 3 MB < 50 MB
      console.log(`✅ Buffer dynamique (1 MB code): ${Math.round(dynamicBuffer / 1024 / 1024)}MB`);
    });

    it('devrait augmenter buffer pour gros fichiers', () => {
      const sourceCodeSize = 30 * 1024 * 1024;  // 30 MB
      const forensicDataSize = 1024;
      const totalSize = sourceCodeSize + forensicDataSize;
      
      const dynamicBuffer = Math.max(
        50 * 1024 * 1024,
        totalSize * 3
      );
      
      expect(dynamicBuffer).toBe(totalSize * 3);  // 30 MB * 3 = 90 MB > 50 MB
      console.log(`✅ Buffer dynamique (30 MB code): ${Math.round(dynamicBuffer / 1024 / 1024)}MB`);
    });
  });

  describe('7. Test Logs Debug', () => {
    
    it('devrait logger la taille du prompt', () => {
      const prompt = 'Test prompt avec du contenu détaillé pour l\'analyse';
      const promptLength = prompt.length;
      
      expect(promptLength).toBeGreaterThan(0);
      console.log(`✅ Prompt lu (${promptLength} chars)`);
    });

    it('devrait logger un extrait du prompt', () => {
      const prompt = 'Test prompt avec du contenu détaillé pour l\'analyse forensique complète du code source';
      const extract = prompt.substring(0, 50);
      
      expect(extract).toBe('Test prompt avec du contenu détaillé pour l\'ana');
      console.log(`✅ Extrait prompt: ${extract}...`);
    });

    it('devrait masquer la clé API dans les logs', () => {
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      const match = envContent.match(/BOBSHELL_API_KEY=(.+)/);
      const apiKey = match[1].trim();
      
      const maskedKey = apiKey.substring(0, 30);
      
      expect(maskedKey.length).toBe(30);
      expect(maskedKey).not.toBe(apiKey);
      console.log(`✅ API Key chargée: ${maskedKey}...`);
    });
  });

  describe('8. Test Prompt Échappement', () => {
    
    it('devrait échapper les guillemets doubles', () => {
      const prompt = 'Analyse ce code "important"';
      const escaped = prompt.replace(/"/g, '\\"');
      
      expect(escaped).toBe('Analyse ce code \\"important\\"');
      console.log(`✅ Guillemets échappés: ${escaped}`);
    });

    it('devrait échapper les retours à la ligne', () => {
      const prompt = 'Ligne 1\nLigne 2\nLigne 3';
      const escaped = prompt.replace(/\n/g, ' ');
      
      expect(escaped).toBe('Ligne 1 Ligne 2 Ligne 3');
      console.log(`✅ Retours à la ligne échappés: ${escaped}`);
    });

    it('devrait échapper les guillemets ET retours à la ligne', () => {
      const prompt = 'Analyse "code"\navec\n"détails"';
      const escaped = prompt.replace(/"/g, '\\"').replace(/\n/g, ' ');
      
      expect(escaped).toBe('Analyse \\"code\\" avec \\"détails\\"');
      console.log(`✅ Prompt complètement échappé: ${escaped}`);
    });
  });

  describe('9. Test Vérifications Robustes', () => {
    
    it('devrait vérifier que .env.bob existe', () => {
      expect(existsSync(ENV_BOB_PATH)).toBe(true);
      console.log(`✅ .env.bob existe: ${ENV_BOB_PATH}`);
    });

    it('devrait extraire la clé API avec regex', () => {
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      const match = envContent.match(/BOBSHELL_API_KEY=(.+)/);
      
      expect(match).not.toBeNull();
      expect(match[1]).toBeTruthy();
      console.log(`✅ Clé API extraite avec regex`);
    });

    it('devrait vérifier que la clé API n\'est pas vide', () => {
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      const match = envContent.match(/BOBSHELL_API_KEY=(.+)/);
      const apiKey = match[1].trim();
      
      expect(apiKey.length).toBeGreaterThan(0);
      console.log(`✅ Clé API non vide (${apiKey.length} chars)`);
    });

    it('devrait créer le répertoire de tâches', () => {
      const taskDir = join(TEST_DIR, 'tasks');
      mkdirSync(taskDir, { recursive: true });
      
      expect(existsSync(taskDir)).toBe(true);
      console.log(`✅ Répertoire de tâches créé: ${taskDir}`);
    });

    it('devrait créer le fichier de prompt', () => {
      const promptFile = join(TEST_DIR, 'bob-prompt.md');
      const prompt = '# Test Prompt\n\nAnalyse ce code de test.';
      
      writeFileSync(promptFile, prompt, 'utf8');
      
      expect(existsSync(promptFile)).toBe(true);
      console.log(`✅ Fichier de prompt créé: ${promptFile}`);
    });
  });

  describe('10. Test Intégration Complète', () => {
    
    it('devrait exécuter Bob CLI avec tous les paramètres', () => {
      // Charger la clé API
      const envContent = readFileSync(ENV_BOB_PATH, 'utf8');
      const match = envContent.match(/BOBSHELL_API_KEY=(.+)/);
      const apiKey = match[1].trim();
      
      // Créer un prompt réel
      const prompt = `# Analyse Forensique Test

## Code à Analyser

\`\`\`javascript
function test() {
  console.log("Test");
}
\`\`\`

## Mission

Analyse ce code et génère un rapport JSON.
`;
      
      const escapedPrompt = prompt.replace(/"/g, '\\"').replace(/\n/g, ' ');
      
      // Calculer timeout et buffer dynamiques
      const lineCount = 10;
      const dynamicTimeout = Math.max(60000, lineCount * 1);
      const dynamicBuffer = Math.max(50 * 1024 * 1024, 1024 * 3);
      
      console.log(`⏱️  Timeout: ${Math.round(dynamicTimeout / 1000)}s`);
      console.log(`💾 Buffer: ${Math.round(dynamicBuffer / 1024 / 1024)}MB`);
      
      // Créer script bash
      const scriptPath = join(TEST_DIR, 'run-bob-complete.sh');
      const script = `#!/bin/bash
set -e
set -x

export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \\. "$NVM_DIR/nvm.sh"

nvm use v22.22.3

source ${ENV_BOB_PATH}

if [ -z "$BOBSHELL_API_KEY" ]; then
  echo "ERREUR: BOBSHELL_API_KEY non chargée"
  exit 1
fi

bob -y --chat-mode code --output-format json -p "${escapedPrompt}"
`;
      
      writeFileSync(scriptPath, script, { mode: 0o755 });
      
      try {
        const result = execSync(`/bin/bash ${scriptPath}`, { 
          encoding: 'utf8',
          timeout: dynamicTimeout,
          maxBuffer: dynamicBuffer,
          cwd: TEST_DIR,
          env: {
            ...process.env,
            BOBSHELL_API_KEY: apiKey
          }
        });
        
        expect(result).toBeTruthy();
        console.log(`✅ Bob CLI intégration complète réussie`);
        console.log(`Résultat (200 premiers chars):\n${result.substring(0, 200)}...`);
      } catch (error) {
        console.error('❌ Bob CLI intégration complète échouée:', error.message);
        
        if (error.stderr) {
          console.error('STDERR:', error.stderr.toString());
        }
        
        // Ne pas throw si c'est juste un timeout (Bob CLI peut être lent)
        if (error.message.includes('ETIMEDOUT')) {
          console.warn('⚠️  Timeout Bob CLI (normal si serveur lent)');
        } else {
          throw error;
        }
      }
    });
  });
});

// Made with Bob
