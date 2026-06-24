// Copyright 2026 MDBAI Platform
// Chart Generator - Matplotlib Integration for Telegram Inline Charts

import { spawn } from 'child_process';
import { promises as fs } from 'fs';
import path from 'path';
import crypto from 'crypto';

/**
 * ChartGenerator - Génère des graphiques matplotlib pour analyses forensiques
 * 
 * Fonctionnalités:
 * - Distribution bytes (histogramme)
 * - Entropie temporelle (courbe)
 * - Timeline commits (graphe)
 * - Heatmap anomalies
 * - Comparaison versions (diff)
 */
class ChartGenerator {
    constructor(config = {}) {
        this.tempDir = config.tempDir || '/tmp/mdbai_charts';
        this.pythonPath = config.pythonPath || 'python3';
        this.maxChartSize = config.maxChartSize || 1024 * 1024; // 1MB
        this.chartTimeout = config.chartTimeout || 30000; // 30s
        this.dpi = config.dpi || 100;
        this.figsize = config.figsize || [10, 6];
        
        this._ensureTempDir();
    }

    /**
     * Crée le répertoire temporaire si nécessaire
     */
    async _ensureTempDir() {
        try {
            await fs.mkdir(this.tempDir, { recursive: true });
        } catch (error) {
            console.error('Failed to create temp directory:', error);
        }
    }

    /**
     * Génère un nom de fichier unique
     */
    _generateFilename(prefix = 'chart') {
        const timestamp = Date.now();
        const random = crypto.randomBytes(4).toString('hex');
        return `${prefix}_${timestamp}_${random}.png`;
    }

    /**
     * Exécute un script Python matplotlib
     */
    async _executePythonScript(scriptPath, args = []) {
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                process.kill();
                reject(new Error('Chart generation timeout'));
            }, this.chartTimeout);

            const process = spawn(this.pythonPath, [scriptPath, ...args]);
            let stdout = '';
            let stderr = '';

            process.stdout.on('data', (data) => {
                stdout += data.toString();
            });

            process.stderr.on('data', (data) => {
                stderr += data.toString();
            });

            process.on('close', (code) => {
                clearTimeout(timeout);
                if (code === 0) {
                    resolve(stdout.trim());
                } else {
                    reject(new Error(`Python script failed: ${stderr}`));
                }
            });

            process.on('error', (error) => {
                clearTimeout(timeout);
                reject(error);
            });
        });
    }

    /**
     * Génère un histogramme de distribution des bytes
     * 
     * @param {Object} data - Données d'analyse
     * @param {Array<number>} data.byteDistribution - Distribution des bytes (256 valeurs)
     * @param {string} data.filename - Nom du fichier analysé
     * @returns {Promise<string>} - Chemin du fichier PNG généré
     */
    async generateByteDistribution(data) {
        const outputPath = path.join(this.tempDir, this._generateFilename('byte_dist'));
        
        const scriptContent = `
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import json
import sys

# Charger les données
data = json.loads(sys.argv[1])
distribution = data['byteDistribution']
filename = data['filename']

# Créer le graphique
fig, ax = plt.subplots(figsize=(${this.figsize[0]}, ${this.figsize[1]}))
x = np.arange(256)
ax.bar(x, distribution, color='steelblue', alpha=0.7, edgecolor='black', linewidth=0.5)

# Configuration
ax.set_xlabel('Byte Value (0x00-0xFF)', fontsize=12, fontweight='bold')
ax.set_ylabel('Frequency', fontsize=12, fontweight='bold')
ax.set_title(f'Byte Distribution Analysis\\n{filename}', fontsize=14, fontweight='bold')
ax.grid(True, alpha=0.3, linestyle='--')
ax.set_xlim(-1, 256)

# Annotations statistiques
mean_val = np.mean(distribution)
max_val = np.max(distribution)
max_idx = np.argmax(distribution)
ax.axhline(y=mean_val, color='red', linestyle='--', linewidth=2, label=f'Mean: {mean_val:.1f}')
ax.annotate(f'Peak: 0x{max_idx:02X}\\n({max_val:.0f})', 
            xy=(max_idx, max_val), xytext=(max_idx+20, max_val*1.1),
            arrowprops=dict(arrowstyle='->', color='red', lw=2),
            fontsize=10, fontweight='bold', color='red')

ax.legend(loc='upper right')
plt.tight_layout()
plt.savefig('${outputPath}', dpi=${this.dpi}, bbox_inches='tight')
plt.close()

print('${outputPath}')
`;

        const scriptPath = path.join(this.tempDir, 'byte_dist_script.py');
        await fs.writeFile(scriptPath, scriptContent);
        
        const result = await this._executePythonScript(scriptPath, [JSON.stringify(data)]);
        await fs.unlink(scriptPath);
        
        return result;
    }

    /**
     * Génère une courbe d'entropie temporelle
     * 
     * @param {Object} data - Données d'analyse
     * @param {Array<Object>} data.entropyTimeline - Timeline entropie [{timestamp, entropy, blockSize}]
     * @param {string} data.filename - Nom du fichier analysé
     * @returns {Promise<string>} - Chemin du fichier PNG généré
     */
    async generateEntropyTimeline(data) {
        const outputPath = path.join(this.tempDir, this._generateFilename('entropy'));
        
        const scriptContent = `
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import json
import sys
from datetime import datetime

# Charger les données
data = json.loads(sys.argv[1])
timeline = data['entropyTimeline']
filename = data['filename']

# Extraire les données
timestamps = [datetime.fromisoformat(t['timestamp'].replace('Z', '+00:00')) for t in timeline]
entropies = [t['entropy'] for t in timeline]
block_sizes = [t['blockSize'] for t in timeline]

# Créer le graphique avec deux axes Y
fig, ax1 = plt.subplots(figsize=(${this.figsize[0]}, ${this.figsize[1]}))

# Axe 1: Entropie
color = 'tab:blue'
ax1.set_xlabel('Time', fontsize=12, fontweight='bold')
ax1.set_ylabel('Entropy (bits)', fontsize=12, fontweight='bold', color=color)
line1 = ax1.plot(timestamps, entropies, color=color, linewidth=2, marker='o', 
                 markersize=4, label='Entropy')
ax1.tick_params(axis='y', labelcolor=color)
ax1.grid(True, alpha=0.3, linestyle='--')
ax1.set_ylim(0, 8.5)

# Axe 2: Block Size
ax2 = ax1.twinx()
color = 'tab:orange'
ax2.set_ylabel('Block Size (bytes)', fontsize=12, fontweight='bold', color=color)
line2 = ax2.plot(timestamps, block_sizes, color=color, linewidth=2, linestyle='--',
                 marker='s', markersize=4, alpha=0.7, label='Block Size')
ax2.tick_params(axis='y', labelcolor=color)

# Titre et légende
ax1.set_title(f'Entropy Timeline Analysis\\n{filename}', fontsize=14, fontweight='bold')
lines = line1 + line2
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper left')

# Rotation des labels de date
fig.autofmt_xdate()

plt.tight_layout()
plt.savefig('${outputPath}', dpi=${this.dpi}, bbox_inches='tight')
plt.close()

print('${outputPath}')
`;

        const scriptPath = path.join(this.tempDir, 'entropy_script.py');
        await fs.writeFile(scriptPath, scriptContent);
        
        const result = await this._executePythonScript(scriptPath, [JSON.stringify(data)]);
        await fs.unlink(scriptPath);
        
        return result;
    }

    /**
     * Génère une timeline de commits avec anomalies
     * 
     * @param {Object} data - Données d'analyse
     * @param {Array<Object>} data.commits - Commits [{hash, date, score, anomalies}]
     * @param {string} data.repository - Nom du repository
     * @returns {Promise<string>} - Chemin du fichier PNG généré
     */
    async generateCommitTimeline(data) {
        const outputPath = path.join(this.tempDir, this._generateFilename('commits'));
        
        const scriptContent = `
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import json
import sys
from datetime import datetime

# Charger les données
data = json.loads(sys.argv[1])
commits = data['commits']
repository = data['repository']

# Extraire les données
dates = [datetime.fromisoformat(c['date'].replace('Z', '+00:00')) for c in commits]
scores = [c['score'] for c in commits]
has_anomalies = [len(c.get('anomalies', [])) > 0 for c in commits]

# Créer le graphique
fig, ax = plt.subplots(figsize=(${this.figsize[0]}, ${this.figsize[1]}))

# Plot commits normaux
normal_dates = [d for d, a in zip(dates, has_anomalies) if not a]
normal_scores = [s for s, a in zip(scores, has_anomalies) if not a]
ax.scatter(normal_dates, normal_scores, c='green', s=100, alpha=0.6, 
           marker='o', label='Normal Commits', edgecolors='black', linewidth=1)

# Plot commits avec anomalies
anomaly_dates = [d for d, a in zip(dates, has_anomalies) if a]
anomaly_scores = [s for s, a in zip(scores, has_anomalies) if a]
ax.scatter(anomaly_dates, anomaly_scores, c='red', s=150, alpha=0.8,
           marker='X', label='Anomalies Detected', edgecolors='darkred', linewidth=2)

# Ligne de tendance
z = np.polyfit(range(len(dates)), scores, 2)
p = np.poly1d(z)
ax.plot(dates, p(range(len(dates))), "b--", alpha=0.5, linewidth=2, label='Trend')

# Configuration
ax.set_xlabel('Commit Date', fontsize=12, fontweight='bold')
ax.set_ylabel('Forensic Score', fontsize=12, fontweight='bold')
ax.set_title(f'Commit Timeline Analysis\\n{repository}', fontsize=14, fontweight='bold')
ax.grid(True, alpha=0.3, linestyle='--')
ax.legend(loc='upper left', fontsize=10)

# Rotation des labels de date
fig.autofmt_xdate()

plt.tight_layout()
plt.savefig('${outputPath}', dpi=${this.dpi}, bbox_inches='tight')
plt.close()

print('${outputPath}')
`;

        const scriptPath = path.join(this.tempDir, 'commits_script.py');
        await fs.writeFile(scriptPath, scriptContent);
        
        const result = await this._executePythonScript(scriptPath, [JSON.stringify(data)]);
        await fs.unlink(scriptPath);
        
        return result;
    }

    /**
     * Génère une heatmap d'anomalies
     * 
     * @param {Object} data - Données d'analyse
     * @param {Array<Array<number>>} data.heatmapData - Matrice 2D d'anomalies
     * @param {Array<string>} data.xLabels - Labels axe X
     * @param {Array<string>} data.yLabels - Labels axe Y
     * @param {string} data.title - Titre du graphique
     * @returns {Promise<string>} - Chemin du fichier PNG généré
     */
    async generateAnomalyHeatmap(data) {
        const outputPath = path.join(this.tempDir, this._generateFilename('heatmap'));
        
        const scriptContent = `
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import json
import sys

# Charger les données
data = json.loads(sys.argv[1])
heatmap_data = np.array(data['heatmapData'])
x_labels = data['xLabels']
y_labels = data['yLabels']
title = data['title']

# Créer le graphique
fig, ax = plt.subplots(figsize=(${this.figsize[0]}, ${this.figsize[1]}))

# Heatmap
im = ax.imshow(heatmap_data, cmap='YlOrRd', aspect='auto', interpolation='nearest')

# Colorbar
cbar = plt.colorbar(im, ax=ax)
cbar.set_label('Anomaly Score', fontsize=12, fontweight='bold')

# Labels
ax.set_xticks(np.arange(len(x_labels)))
ax.set_yticks(np.arange(len(y_labels)))
ax.set_xticklabels(x_labels, rotation=45, ha='right')
ax.set_yticklabels(y_labels)

# Titre
ax.set_title(title, fontsize=14, fontweight='bold', pad=20)

# Annotations
for i in range(len(y_labels)):
    for j in range(len(x_labels)):
        value = heatmap_data[i, j]
        if value > 0:
            text = ax.text(j, i, f'{value:.1f}',
                          ha="center", va="center", color="black" if value < 50 else "white",
                          fontsize=8, fontweight='bold')

plt.tight_layout()
plt.savefig('${outputPath}', dpi=${this.dpi}, bbox_inches='tight')
plt.close()

print('${outputPath}')
`;

        const scriptPath = path.join(this.tempDir, 'heatmap_script.py');
        await fs.writeFile(scriptPath, scriptContent);
        
        const result = await this._executePythonScript(scriptPath, [JSON.stringify(data)]);
        await fs.unlink(scriptPath);
        
        return result;
    }

    /**
     * Nettoie les fichiers temporaires anciens (>1h)
     */
    async cleanup() {
        try {
            const files = await fs.readdir(this.tempDir);
            const now = Date.now();
            const maxAge = 3600000; // 1 heure

            for (const file of files) {
                const filePath = path.join(this.tempDir, file);
                const stats = await fs.stat(filePath);
                
                if (now - stats.mtimeMs > maxAge) {
                    await fs.unlink(filePath);
                }
            }
        } catch (error) {
            console.error('Cleanup failed:', error);
        }
    }
}

// Export ES6
export default ChartGenerator;
