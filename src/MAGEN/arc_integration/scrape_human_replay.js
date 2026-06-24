/**
 * MAGEN Phase 4.6.3 - Web Scraper pour Démonstrations HUMANS
 * Extrait les données de replay depuis arcprize.org
 */

const https = require('https');
const fs = require('fs');

const REPLAY_ID = '8aed7120-f7a9-45a1-837a-68bc7dc37a4f';
const OUTPUT_FILE = './human_replays/replay_' + REPLAY_ID + '.json';

console.log('🔍 SCRAPING HUMAN REPLAY DATA');
console.log('Replay ID:', REPLAY_ID);
console.log('URL:', `https://arcprize.org/replay/${REPLAY_ID}`);

// Télécharger la page HTML
https.get(`https://arcprize.org/replay/${REPLAY_ID}`, (res) => {
    let html = '';
    
    res.on('data', (chunk) => {
        html += chunk;
    });
    
    res.on('end', () => {
        console.log('\n✅ Page téléchargée:', html.length, 'bytes');
        
        // Extraire les données JSON embarquées dans les scripts Next.js
        const scriptMatches = html.match(/self\.__next_f\.push\(\[1,"([^"]+)"\]\)/g);
        
        if (!scriptMatches) {
            console.log('❌ Aucune donnée Next.js trouvée');
            return;
        }
        
        console.log('📦 Scripts Next.js trouvés:', scriptMatches.length);
        
        // Chercher les données de session
        let sessionData = null;
        let gameData = null;
        
        for (const match of scriptMatches) {
            const content = match.match(/self\.__next_f\.push\(\[1,"(.+)"\]\)/)[1];
            
            // Décoder les échappements
            const decoded = content
                .replace(/\\"/g, '"')
                .replace(/\\n/g, '\n')
                .replace(/\\t/g, '\t');
            
            // Chercher session_id
            if (decoded.includes('8aed7120-f7a9-45a1-837a-68bc7dc37a4f')) {
                console.log('\n🎯 Données de session trouvées!');
                console.log('Extrait:', decoded.substring(0, 200) + '...');
                sessionData = decoded;
            }
            
            // Chercher game_id, score, model
            if (decoded.includes('gameId') || decoded.includes('Score') || decoded.includes('Model')) {
                console.log('\n🎮 Données de jeu trouvées!');
                console.log('Extrait:', decoded.substring(0, 200) + '...');
                gameData = decoded;
            }
        }
        
        // Sauvegarder les données brutes
        const output = {
            replay_id: REPLAY_ID,
            url: `https://arcprize.org/replay/${REPLAY_ID}`,
            scraped_at: new Date().toISOString(),
            session_data: sessionData,
            game_data: gameData,
            html_size: html.length,
            scripts_found: scriptMatches.length
        };
        
        // Créer le dossier si nécessaire
        if (!fs.existsSync('./human_replays')) {
            fs.mkdirSync('./human_replays', { recursive: true });
        }
        
        fs.writeFileSync(OUTPUT_FILE, JSON.stringify(output, null, 2));
        console.log('\n💾 Données sauvegardées:', OUTPUT_FILE);
        
        // Afficher un résumé
        console.log('\n📊 RÉSUMÉ:');
        console.log('- HTML téléchargé:', html.length, 'bytes');
        console.log('- Scripts Next.js:', scriptMatches.length);
        console.log('- Session data:', sessionData ? 'OUI ✅' : 'NON ❌');
        console.log('- Game data:', gameData ? 'OUI ✅' : 'NON ❌');
        
        // Analyser plus en détail
        if (sessionData || gameData) {
            console.log('\n🔬 ANALYSE DÉTAILLÉE:');
            
            // Chercher des patterns JSON
            const jsonPattern = /\{[^{}]*"[^"]+"\s*:\s*[^{}]+\}/g;
            const jsonMatches = (sessionData + gameData).match(jsonPattern);
            
            if (jsonMatches) {
                console.log('- Objets JSON trouvés:', jsonMatches.length);
                jsonMatches.slice(0, 3).forEach((json, i) => {
                    console.log(`  ${i+1}.`, json.substring(0, 100) + '...');
                });
            }
        }
    });
    
}).on('error', (err) => {
    console.error('❌ Erreur:', err.message);
});

// Made with Bob
