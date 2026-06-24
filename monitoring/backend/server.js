const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const si = require('systeminformation');

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

const PORT = 3001;
const REFRESH_RATE = 100; // 100ms = 10 FPS

// État global
let benchmarkProcess = null;
let metricsHistory = {
    baseline: {
        tps: 485000,
        throughput: 485000000,
        latency: 0.0
    },
    current: {
        tps: 0,
        throughput: 0,
        latency: 0,
        cpu: 0,
        ram: 0,
        gpu: 0,
        disk_read: 0,
        disk_write: 0,
        errors: 0
    },
    hardware: {
        cpu_model: '',
        cpu_cores: 0,
        cpu_threads: 0,
        gpu_model: '',
        ram_total_gb: 0,
        disk_model: ''
    },
    simd: {
        avx2: false,
        sse42: false,
        aes_ni: false
    },
    history: {
        tps: [],
        latency: [],
        cpu: [],
        ram: []
    }
};

/**
 * Détecter capacités SIMD depuis /proc/cpuinfo
 */
async function detectSIMD() {
    try {
        const cpuInfo = fs.readFileSync('/proc/cpuinfo', 'utf8');
        const flagsLine = cpuInfo.split('\n').find(line => line.startsWith('flags'));
        
        if (flagsLine) {
            metricsHistory.simd.avx2 = flagsLine.includes('avx2');
            metricsHistory.simd.sse42 = flagsLine.includes('sse4_2');
            metricsHistory.simd.aes_ni = flagsLine.includes('aes');
        }
        
        console.log('[BACKEND] Capacités SIMD détectées:', metricsHistory.simd);
    } catch (error) {
        console.error('[BACKEND] Erreur détection SIMD:', error.message);
    }
}

/**
 * Lire métriques temps réel depuis CSV benchmark
 */
async function readRealtimeMetrics() {
    try {
        const metricsFile = path.join(__dirname, '..', '..', 'logs', 'forensic', 'realtime_metrics.csv');
        
        if (!fs.existsSync(metricsFile)) {
            return null;
        }
        
        const content = fs.readFileSync(metricsFile, 'utf8');
        const lines = content.trim().split('\n');
        
        if (lines.length < 2) {
            return null;
        }
        
        // Lire dernière ligne (données les plus récentes)
        const lastLine = lines[lines.length - 1];
        const parts = lastLine.split(',');
        
        // Format CSV: timestamp_ns,total_ticks,tps_current,tps_avg,throughput,latency_min_ns,latency_med_ns,latency_max_ns,errors,cpu_percent,ram_percent
        if (parts.length >= 11) {
            return {
                timestamp_ns: parts[0],
                total_ticks: parseInt(parts[1]),
                tps: parseFloat(parts[3]),  // POH ticks/s moyen
                tps_current: parseFloat(parts[2]),  // POH ticks/s instantané
                throughput: parseFloat(parts[4]),  // TPS RÉEL (vérifications/s)
                latency_min: parseFloat(parts[5]) / 1000000,  // ns → ms
                latency_med: parseFloat(parts[6]) / 1000000,  // ns → ms
                latency_max: parseFloat(parts[7]) / 1000000,  // ns → ms
                errors: parseInt(parts[8]),
                cpu: parseFloat(parts[9]),
                ram: parseFloat(parts[10])
            };
        }
        
        return null;
    } catch (error) {
        // Pas d'erreur si fichier n'existe pas encore (benchmark pas démarré)
        return null;
    }
}

/**
 * Collecter métriques système
 */
async function collectMetrics() {
    try {
        // Lire métriques temps réel depuis benchmark
        const realtimeData = await readRealtimeMetrics();
        
        if (realtimeData) {
            // Utiliser données réelles du benchmark
            metricsHistory.current.tps = Math.round(realtimeData.tps);
            metricsHistory.current.latency_min = realtimeData.latency_min || 0;
            metricsHistory.current.latency_med = realtimeData.latency_med || 0;
            metricsHistory.current.latency_max = realtimeData.latency_max || 0;
            metricsHistory.current.throughput = Math.round(realtimeData.throughput);
            metricsHistory.current.errors = realtimeData.errors;
            metricsHistory.current.cpu = Math.round(realtimeData.cpu);
            metricsHistory.current.ram = Math.round(realtimeData.ram);
        } else {
            // Fallback: métriques système si pas de benchmark actif
            const cpuLoad = await si.currentLoad();
            metricsHistory.current.cpu = Math.round(cpuLoad.currentLoad);
            
            const memData = await si.mem();
            metricsHistory.current.ram = Math.round((memData.used / memData.total) * 100);
            
            // Réinitialiser métriques PoH si pas de données
            metricsHistory.current.tps = 0;
            metricsHistory.current.latency_min = 0;
            metricsHistory.current.latency_med = 0;
            metricsHistory.current.latency_max = 0;
            metricsHistory.current.throughput = 0;
            metricsHistory.current.errors = 0;
        }
        
        // GPU (toujours depuis système)
        const gpuData = await si.graphics();
        if (gpuData.controllers && gpuData.controllers.length > 0) {
            metricsHistory.current.gpu = gpuData.controllers[0].utilizationGpu || 0;
        }
        
        // Disque I/O (toujours depuis système)
        const diskIO = await si.fsStats();
        metricsHistory.current.disk_read = (diskIO.rIO_sec / 1024 / 1024).toFixed(2);
        metricsHistory.current.disk_write = (diskIO.wIO_sec / 1024 / 1024).toFixed(2);
        
        // Historique pour graphiques
        metricsHistory.history.tps.push(metricsHistory.current.tps / 1000); // K
        metricsHistory.history.latency.push(metricsHistory.current.latency);
        metricsHistory.history.cpu.push(metricsHistory.current.cpu);
        metricsHistory.history.ram.push(metricsHistory.current.ram);
        
        // Limiter historique à 60 points
        const maxHistory = 60;
        if (metricsHistory.history.tps.length > maxHistory) {
            metricsHistory.history.tps.shift();
            metricsHistory.history.latency.shift();
            metricsHistory.history.cpu.shift();
            metricsHistory.history.ram.shift();
        }
        
        return metricsHistory;
    } catch (error) {
        console.error('[BACKEND] Erreur collecte métriques:', error.message);
        return metricsHistory;
    }
}

/**
 * Collecter infos matériel (1× au démarrage)
 */
async function collectHardwareInfo() {
    try {
        // CPU
        const cpuData = await si.cpu();
        metricsHistory.hardware.cpu_model = `${cpuData.manufacturer} ${cpuData.brand}`;
        metricsHistory.hardware.cpu_cores = cpuData.physicalCores || cpuData.cores;
        metricsHistory.hardware.cpu_threads = cpuData.cores || cpuData.physicalCores * 2;
        
        // RAM
        const memData = await si.mem();
        metricsHistory.hardware.ram_total_gb = (memData.total / 1024 / 1024 / 1024).toFixed(1);
        
        // GPU
        const gpuData = await si.graphics();
        if (gpuData.controllers && gpuData.controllers.length > 0) {
            metricsHistory.hardware.gpu_model = gpuData.controllers[0].model;
        }
        
        // Disque
        const diskData = await si.diskLayout();
        if (diskData && diskData.length > 0) {
            metricsHistory.hardware.disk_model = diskData[0].name;
        }
        
        console.log('[BACKEND] Matériel détecté:', metricsHistory.hardware);
    } catch (error) {
        console.error('[BACKEND] Erreur collecte matériel:', error.message);
    }
}

// WebSocket connexion
io.on('connection', (socket) => {
    console.log(`[BACKEND] Client connecté: ${socket.id}`);
    
    // Envoyer capacités SIMD immédiatement
    socket.emit('simd_capabilities', metricsHistory.simd);
    
    // Envoyer infos matériel
    socket.emit('hardware_info', metricsHistory.hardware);
    
    // Événement: Démarrer benchmark
    socket.on('start_benchmark', (duration = 60) => {
        console.log(`[BACKEND] Démarrage benchmark ${duration}s demandé`);
        
        if (benchmarkProcess) {
            console.log('[BACKEND] Benchmark déjà en cours');
            socket.emit('benchmark_error', 'Benchmark déjà en cours');
            return;
        }
        
        // Lancer le benchmark C avec SIMD
        const benchmarkPath = path.join(__dirname, '..', '..', 'bench_c199_poh_simd');
        
        if (!fs.existsSync(benchmarkPath)) {
            console.error('[BACKEND] Exécutable benchmark introuvable:', benchmarkPath);
            socket.emit('benchmark_error', 'Exécutable benchmark introuvable');
            return;
        }
        
        benchmarkProcess = spawn(benchmarkPath, [duration.toString()], {
            cwd: path.join(__dirname, '..', '..'),
            stdio: ['ignore', 'pipe', 'pipe']
        });
        
        benchmarkProcess.stdout.on('data', (data) => {
            const output = data.toString().trim();
            console.log(`[BENCHMARK] ${output}`);
            io.emit('benchmark_output', output);
        });
        
        benchmarkProcess.stderr.on('data', (data) => {
            console.error(`[BENCHMARK ERROR] ${data.toString().trim()}`);
        });
        
        benchmarkProcess.on('close', (code) => {
            console.log(`[BACKEND] Benchmark terminé (code: ${code})`);
            benchmarkProcess = null;
            io.emit('benchmark_stopped', { code });
        });
        
        io.emit('benchmark_started', { duration });
    });
    
    // Événement: Arrêter benchmark
    socket.on('stop_benchmark', () => {
        console.log('[BACKEND] Arrêt benchmark demandé');
        
        if (benchmarkProcess) {
            benchmarkProcess.kill('SIGTERM');
            benchmarkProcess = null;
            io.emit('benchmark_stopped', { code: -1 });
        }
    });
    
    socket.on('disconnect', () => {
        console.log(`[BACKEND] Client déconnecté: ${socket.id}`);
    });
});

// Boucle collecte et broadcast métriques
setInterval(async () => {
    const metrics = await collectMetrics();
    io.emit('metrics_update', metrics);
}, REFRESH_RATE);

// Initialisation
(async () => {
    console.log('[BACKEND] Initialisation serveur WebSocket...');
    await detectSIMD();
    await collectHardwareInfo();
    
    server.listen(PORT, () => {
        console.log(`[BACKEND] Serveur WebSocket démarré sur http://localhost:${PORT}`);
        console.log(`[BACKEND] Refresh rate: ${REFRESH_RATE}ms (${1000/REFRESH_RATE} FPS)`);
    });
})();

// Made with Bob
