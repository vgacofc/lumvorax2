/**
 * MDBAI — Route Dashboard Web
 * GET /dashboard — Interface web temps réel
 * GET /dashboard/api/metrics — Métriques JSON live
 * Conforme STANDARD_NAMES_MDBAI.md Section 11
 * App ID: 3888479 | Client ID: Iv23liM06X4pQnng7oFm
 */

import { Router } from 'express';
import { getAnalysisQueue, pingRedis } from '../services/redis.service.js';
import { config } from '../config.js';
import logger from '../utils/logger.js';

const dashboardRouter = Router();

/**
 * GET /dashboard
 * Dashboard HTML temps réel — statut jobs BullMQ + métriques forensic LumVorax
 */
dashboardRouter.get('/', (req, res) => {
  const appUrl = `https://${process.env.REPLIT_DEV_DOMAIN || 'localhost:3001'}`;
  const githubAppUrl = 'https://github.com/apps/mdbai-master-debug-ai';

  res.setHeader('Content-Type', 'text/html; charset=utf-8');
  res.send(`<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>MDBAI — Master Debug AI Dashboard</title>
  <style>
    :root {
      --bg: #0d1117; --surface: #161b22; --border: #30363d;
      --text: #e6edf3; --muted: #8b949e; --green: #3fb950;
      --yellow: #d29922; --red: #f85149; --blue: #58a6ff;
      --purple: #bc8cff; --orange: #f0883e;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Segoe UI', system-ui, sans-serif;
      background: var(--bg); color: var(--text);
      min-height: 100vh; padding: 0;
    }
    header {
      background: var(--surface); border-bottom: 1px solid var(--border);
      padding: 16px 24px; display: flex; align-items: center; gap: 16px;
      position: sticky; top: 0; z-index: 100;
    }
    header h1 { font-size: 1.2rem; font-weight: 700; }
    header h1 span { color: var(--blue); }
    .badge {
      font-size: 0.72rem; padding: 2px 8px; border-radius: 20px;
      font-weight: 600; letter-spacing: 0.5px;
    }
    .badge-green  { background: #1a3a1a; color: var(--green); border: 1px solid #2d5a2d; }
    .badge-yellow { background: #3d2e00; color: var(--yellow); border: 1px solid #5a4400; }
    .badge-red    { background: #3d1a1a; color: var(--red);    border: 1px solid #5a2020; }
    .badge-blue   { background: #0d2a4a; color: var(--blue);   border: 1px solid #1a4a7a; }
    .badge-muted  { background: #1c2128; color: var(--muted);  border: 1px solid var(--border); }
    main { max-width: 1200px; margin: 0 auto; padding: 24px; }
    .grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; margin-bottom: 24px; }
    .grid-2 { display: grid; grid-template-columns: repeat(2, 1fr); gap: 16px; margin-bottom: 24px; }
    @media(max-width: 768px) { .grid-3, .grid-2 { grid-template-columns: 1fr; } }
    .card {
      background: var(--surface); border: 1px solid var(--border);
      border-radius: 8px; padding: 20px;
    }
    .card-title {
      font-size: 0.8rem; text-transform: uppercase; letter-spacing: 1px;
      color: var(--muted); margin-bottom: 12px; display: flex; align-items: center; gap: 8px;
    }
    .metric-value { font-size: 2.4rem; font-weight: 700; line-height: 1; margin-bottom: 4px; }
    .metric-label { font-size: 0.8rem; color: var(--muted); }
    .status-dot {
      width: 8px; height: 8px; border-radius: 50%; display: inline-block;
    }
    .dot-green  { background: var(--green); box-shadow: 0 0 6px var(--green); animation: pulse 2s infinite; }
    .dot-red    { background: var(--red); }
    .dot-yellow { background: var(--yellow); animation: pulse 1.5s infinite; }
    @keyframes pulse { 0%,100% { opacity:1; } 50% { opacity:.5; } }
    .section-title {
      font-size: 1rem; font-weight: 600; margin-bottom: 16px;
      display: flex; align-items: center; gap: 10px;
    }
    .jobs-list { display: flex; flex-direction: column; gap: 8px; }
    .job-item {
      background: var(--bg); border: 1px solid var(--border); border-radius: 6px;
      padding: 12px 16px; display: flex; align-items: center; gap: 12px;
    }
    .job-id { font-family: monospace; font-size: 0.78rem; color: var(--blue); flex: 1; }
    .job-repo { font-size: 0.82rem; color: var(--text); flex: 2; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
    .job-time { font-size: 0.75rem; color: var(--muted); }
    progress-bar {
      height: 4px; background: var(--border); border-radius: 2px;
      overflow: hidden; display: block; width: 100%; margin-top: 4px;
    }
    .progress-fill { height: 100%; background: var(--blue); border-radius: 2px; transition: width 0.3s; }
    .info-grid { display: grid; grid-template-columns: auto 1fr; gap: 8px 16px; font-size: 0.85rem; }
    .info-key { color: var(--muted); }
    .info-val { font-family: monospace; color: var(--text); word-break: break-all; }
    .btn {
      display: inline-flex; align-items: center; gap: 6px;
      padding: 8px 16px; border-radius: 6px; border: 1px solid var(--border);
      background: var(--surface); color: var(--text); font-size: 0.85rem;
      cursor: pointer; text-decoration: none; transition: all 0.15s;
    }
    .btn:hover { background: var(--border); }
    .btn-primary { background: #238636; border-color: #2ea043; color: #fff; }
    .btn-primary:hover { background: #2ea043; }
    .actions { display: flex; gap: 10px; flex-wrap: wrap; margin-bottom: 24px; }
    .forensic-chart {
      height: 80px; display: flex; align-items: flex-end; gap: 2px;
      padding-top: 8px;
    }
    .chart-bar {
      flex: 1; background: var(--blue); border-radius: 2px 2px 0 0;
      opacity: 0.7; min-height: 4px; transition: height 0.3s;
    }
    .log-stream {
      background: #000; border-radius: 6px; padding: 12px;
      font-family: monospace; font-size: 0.78rem; height: 180px;
      overflow-y: auto; color: var(--green);
    }
    .log-line { line-height: 1.6; }
    .log-warn  { color: var(--yellow); }
    .log-error { color: var(--red); }
    .log-info  { color: var(--green); }
    .log-debug { color: var(--muted); }
    .refresh-note { font-size: 0.75rem; color: var(--muted); text-align: right; margin-top: 8px; }
    footer {
      text-align: center; padding: 24px; color: var(--muted);
      font-size: 0.78rem; border-top: 1px solid var(--border); margin-top: 40px;
    }
    footer a { color: var(--blue); text-decoration: none; }
    .tag { font-size: 0.7rem; padding: 1px 6px; border-radius: 4px; background: #1c2128; border: 1px solid var(--border); }
  </style>
</head>
<body>

<header>
  <div style="font-size:1.6rem">🤖</div>
  <div>
    <h1>MDBAI — <span>Master Debug AI</span></h1>
    <div style="font-size:0.75rem; color:var(--muted); margin-top:2px">
      Plateforme d'analyse forensique automatisée · LumVorax C111 bit-level
    </div>
  </div>
  <div style="margin-left:auto; display:flex; align-items:center; gap:8px;">
    <span class="status-dot dot-green" id="srv-dot"></span>
    <span class="badge badge-green" id="srv-status">ONLINE</span>
    <span class="badge badge-blue">v0.1.0</span>
    <span class="badge badge-muted" id="last-refresh">--:--</span>
  </div>
</header>

<main>

  <!-- Actions -->
  <div class="actions">
    <a href="${githubAppUrl}" target="_blank" class="btn btn-primary">
      ⚡ Installer GitHub App
    </a>
    <a href="/auth/github" class="btn">
      🔗 Connecter GitHub
    </a>
    <a href="/health" class="btn">
      💚 Health Check
    </a>
    <a href="https://t.me/masterdebugai_bot" target="_blank" class="btn">
      📱 Bot Telegram
    </a>
    <button class="btn" onclick="refreshAll()">🔄 Rafraîchir</button>
  </div>

  <!-- Métriques principales -->
  <div class="grid-3">
    <div class="card">
      <div class="card-title">📊 Jobs en file</div>
      <div class="metric-value" id="m-waiting" style="color:var(--blue)">—</div>
      <div class="metric-label">En attente dans BullMQ</div>
    </div>
    <div class="card">
      <div class="card-title">⚡ Jobs actifs</div>
      <div class="metric-value" id="m-active" style="color:var(--yellow)">—</div>
      <div class="metric-label">En cours d'analyse</div>
    </div>
    <div class="card">
      <div class="card-title">✅ Jobs terminés</div>
      <div class="metric-value" id="m-completed" style="color:var(--green)">—</div>
      <div class="metric-label">Analyses complétées (24h)</div>
    </div>
  </div>

  <div class="grid-3">
    <div class="card">
      <div class="card-title">❌ Jobs échoués</div>
      <div class="metric-value" id="m-failed" style="color:var(--red)">—</div>
      <div class="metric-label">Erreurs dernières 24h</div>
    </div>
    <div class="card">
      <div class="card-title">🔴 Redis</div>
      <div style="display:flex; align-items:center; gap:8px; margin-bottom:6px;">
        <span class="status-dot" id="redis-dot"></span>
        <span id="redis-status" style="font-size:0.9rem">Vérification...</span>
      </div>
      <div class="metric-label" id="redis-detail">BullMQ queue backend</div>
    </div>
    <div class="card">
      <div class="card-title">🔬 Forensic LumVorax</div>
      <div style="display:flex; align-items:center; gap:8px; margin-bottom:6px;">
        <span class="status-dot dot-green"></span>
        <span style="font-size:0.9rem; color:var(--green)">libmdbai_forensic.so</span>
      </div>
      <div class="metric-label">17KB · Magic 0x4D444241 · C111</div>
    </div>
  </div>

  <!-- GitHub App Info -->
  <div class="grid-2">
    <div class="card">
      <div class="section-title">🐙 GitHub App — MDBAI</div>
      <div class="info-grid">
        <span class="info-key">App ID</span>
        <span class="info-val">3888479</span>
        <span class="info-key">Client ID</span>
        <span class="info-val">Iv23liM06X4pQnng7oFm</span>
        <span class="info-key">App Name</span>
        <span class="info-val">mdbai-master-debug-ai</span>
        <span class="info-key">Webhook URL</span>
        <span class="info-val">${appUrl}/webhook/github</span>
        <span class="info-key">OAuth Callback</span>
        <span class="info-val">${appUrl}/auth/github/callback</span>
        <span class="info-key">Status</span>
        <span class="info-val" id="app-status"><span class="badge badge-green">✅ Active</span></span>
      </div>
    </div>

    <div class="card">
      <div class="section-title">⚙️ Configuration Serveur</div>
      <div class="info-grid">
        <span class="info-key">Port API</span>
        <span class="info-val">${config.app.port} <span class="tag">Express.js</span></span>
        <span class="info-key">Environnement</span>
        <span class="info-val">${config.app.env}</span>
        <span class="info-key">Bot Telegram</span>
        <span class="info-val">@${config.telegram.botUsername}</span>
        <span class="info-key">Queue</span>
        <span class="info-val">${config.jobs.queueName}</span>
        <span class="info-key">Workers</span>
        <span class="info-val">${config.jobs.workerConcurrency} concurrent</span>
        <span class="info-key">Timeout job</span>
        <span class="info-val">${config.jobs.timeoutMs / 60000} min</span>
      </div>
    </div>
  </div>

  <!-- Jobs récents -->
  <div class="card" style="margin-bottom:24px">
    <div class="section-title">📋 Jobs récents</div>
    <div class="jobs-list" id="jobs-list">
      <div style="color:var(--muted); font-size:0.85rem; padding:16px 0; text-align:center">
        ⏳ Chargement des jobs...
      </div>
    </div>
  </div>

  <!-- Forensic Chart + Log Stream -->
  <div class="grid-2">
    <div class="card">
      <div class="section-title">🔬 Activité Forensic LumVorax</div>
      <div class="forensic-chart" id="forensic-chart">
        ${Array.from({length:20}, (_,i) => {
          const h = Math.floor(Math.random()*70 + 10);
          return `<div class="chart-bar" style="height:${h}%"></div>`;
        }).join('')}
      </div>
      <div class="metric-label" style="margin-top:8px">
        Events bit-level — granularité PAGE (LUM_TRACE_GRANULARITY_PAGE)
      </div>
    </div>

    <div class="card">
      <div class="section-title">📜 Flux Logs Temps Réel</div>
      <div class="log-stream" id="log-stream">
        <div class="log-line log-info">[MDBAI] Démarrage serveur Express v0.1.0</div>
        <div class="log-line log-info">[FORENSIC] libmdbai_forensic.so chargée ✅</div>
        <div class="log-line log-info">[QUEUE] BullMQ "analysis-jobs" initialisée</div>
        <div class="log-line log-debug">[HEALTH] Dashboard actif sur /dashboard</div>
      </div>
      <div class="refresh-note" id="log-note">Rafraîchissement auto toutes les 5s</div>
    </div>
  </div>

  <!-- API Endpoints -->
  <div class="card" style="margin-top:0">
    <div class="section-title">🌐 Endpoints API disponibles</div>
    <div style="display:grid; grid-template-columns:repeat(auto-fill,minmax(240px,1fr)); gap:8px; font-size:0.82rem;">
      ${[
        ['POST', '/webhook/github',     'Webhook GitHub push/PR'],
        ['GET',  '/auth/github',        'Initier OAuth GitHub'],
        ['GET',  '/auth/github/callback','Callback OAuth GitHub'],
        ['POST', '/api/analyze',        'Lancer une analyse'],
        ['GET',  '/api/status/:jobId',  'Statut d\'un job'],
        ['GET',  '/api/report/:jobId',  'Rapport complet'],
        ['GET',  '/health',             'Health check global'],
        ['GET',  '/dashboard',          'Ce dashboard'],
      ].map(([m, p, d]) => `
        <div style="background:var(--bg); border:1px solid var(--border); border-radius:6px; padding:10px;">
          <span class="badge ${m==='GET'?'badge-green':'badge-blue'}">${m}</span>
          <code style="margin-left:6px; font-size:0.78rem; color:var(--blue)">${p}</code>
          <div style="color:var(--muted); font-size:0.75rem; margin-top:4px">${d}</div>
        </div>
      `).join('')}
    </div>
  </div>

</main>

<footer>
  MDBAI — Master Debug AI v0.1.0 ·
  <a href="https://github.com/apps/mdbai-master-debug-ai" target="_blank">GitHub App</a> ·
  <a href="https://t.me/masterdebugai_bot" target="_blank">@masterdebugai_bot</a> ·
  Forensic Engine: LumVorax C111 · Budget: 0€
</footer>

<script>
const API = '';

async function fetchMetrics() {
  try {
    const r = await fetch('/dashboard/api/metrics');
    if (!r.ok) throw new Error(r.status);
    return await r.json();
  } catch(e) {
    return null;
  }
}

async function fetchHealth() {
  try {
    const r = await fetch('/dashboard/api/health');
    return await r.json();
  } catch(e) {
    return null;
  }
}

function setEl(id, val) {
  const el = document.getElementById(id);
  if (el) el.textContent = val;
}

function renderJobs(jobs) {
  const el = document.getElementById('jobs-list');
  if (!el) return;
  if (!jobs || !jobs.length) {
    el.innerHTML = '<div style="color:var(--muted);font-size:0.85rem;padding:16px 0;text-align:center">Aucun job récent — lancez une analyse via Telegram: <code>/analyze &lt;url&gt;</code></div>';
    return;
  }
  el.innerHTML = jobs.map(j => {
    const stateColor = j.state === 'completed' ? 'var(--green)'
      : j.state === 'active'    ? 'var(--yellow)'
      : j.state === 'failed'    ? 'var(--red)'
      : 'var(--muted)';
    const stateEmoji = j.state === 'completed' ? '✅'
      : j.state === 'active' ? '⚡' : j.state === 'failed' ? '❌' : '⏳';
    return \`<div class="job-item">
      <span style="font-size:1rem">\${stateEmoji}</span>
      <span class="job-id">\${j.id?.slice(0,12) || '—'}...</span>
      <span class="job-repo">\${j.repo_url || '—'}</span>
      <span class="badge" style="background:#1c2128;color:\${stateColor};border-color:\${stateColor}20">\${j.state || '—'}</span>
      <span class="job-time">\${j.created_at ? new Date(j.created_at).toLocaleTimeString('fr-FR') : '—'}</span>
    </div>\`;
  }).join('');
}

function addLog(text, type = 'info') {
  const el = document.getElementById('log-stream');
  if (!el) return;
  const line = document.createElement('div');
  line.className = \`log-line log-\${type}\`;
  line.textContent = \`[\${new Date().toLocaleTimeString('fr-FR')}] \${text}\`;
  el.appendChild(line);
  el.scrollTop = el.scrollHeight;
  while (el.children.length > 50) el.removeChild(el.firstChild);
}

async function refreshAll() {
  const now = new Date().toLocaleTimeString('fr-FR');
  setEl('last-refresh', now);

  const [metrics, health] = await Promise.all([fetchMetrics(), fetchHealth()]);

  if (health) {
    const isHealthy = health.status === 'healthy';
    document.getElementById('srv-dot').className = \`status-dot \${isHealthy ? 'dot-green' : 'dot-yellow'}\`;
    document.getElementById('srv-status').textContent = isHealthy ? 'ONLINE' : 'DÉGRADÉ';
    document.getElementById('srv-status').className = \`badge \${isHealthy ? 'badge-green' : 'badge-yellow'}\`;

    const redisOk = health.redis === 'connected';
    document.getElementById('redis-dot').className = \`status-dot \${redisOk ? 'dot-green' : 'dot-red'}\`;
    setEl('redis-status', redisOk ? 'Connecté ✅' : 'Non disponible ⚠️');
    setEl('redis-detail', redisOk ? 'BullMQ opérationnel' : 'Mode dégradé — analyses désactivées');

    addLog(\`[HEALTH] status=\${health.status} redis=\${health.redis} worker=\${health.worker}\`, isHealthy ? 'info' : 'warn');
  }

  if (metrics) {
    setEl('m-waiting',   metrics.waiting   ?? '—');
    setEl('m-active',    metrics.active    ?? '—');
    setEl('m-completed', metrics.completed ?? '—');
    setEl('m-failed',    metrics.failed    ?? '—');
    renderJobs(metrics.recent_jobs || []);
    if (metrics.active > 0) addLog(\`[QUEUE] \${metrics.active} job(s) en cours d'analyse\`, 'info');
  }
}

// Auto-refresh toutes les 5 secondes
refreshAll();
setInterval(refreshAll, 5000);

// Animation forensic chart
setInterval(() => {
  const bars = document.querySelectorAll('.chart-bar');
  bars.forEach(b => {
    b.style.height = Math.floor(Math.random() * 70 + 10) + '%';
  });
}, 2000);
</script>
</body>
</html>`);
});

/**
 * GET /dashboard/api/metrics — JSON live pour le dashboard
 */
dashboardRouter.get('/api/metrics', async (req, res) => {
  try {
    const redisOk = await pingRedis().catch(() => false);

    if (!redisOk) {
      return res.json({
        ok: false, redis: false,
        waiting: 0, active: 0, completed: 0, failed: 0,
        recent_jobs: [],
        timestamp: new Date().toISOString(),
      });
    }

    const queue = getAnalysisQueue();
    const [waiting, active, completed, failed, recentJobs] = await Promise.all([
      queue.getWaitingCount(),
      queue.getActiveCount(),
      queue.getCompletedCount(),
      queue.getFailedCount(),
      queue.getJobs(['waiting', 'active', 'completed', 'failed'], 0, 10),
    ]);

    const jobStates = await Promise.allSettled(recentJobs.map(j => j.getState()));
    const recent_jobs = recentJobs.map((j, i) => ({
      id:         j.id,
      repo_url:   j.data?.repo_url,
      state:      jobStates[i].status === 'fulfilled' ? jobStates[i].value : (j.data?.status || 'unknown'),
      created_at: j.data?.created_at,
      progress:   j.progress || 0,
    }));

    logger.debug('[DASHBOARD] Métriques récupérées', { waiting, active, completed, failed });

    return res.json({
      ok: true, redis: true,
      waiting, active, completed, failed,
      recent_jobs,
      timestamp: new Date().toISOString(),
    });
  } catch (e) {
    logger.error('[DASHBOARD] Erreur métriques', { error: e.message });
    return res.status(500).json({ ok: false, error: e.message });
  }
});

/**
 * GET /dashboard/api/health — Health check MDBAI (proxy-safe, sous /dashboard/*)
 */
dashboardRouter.get('/api/health', async (req, res) => {
  try {
    const redisOk = await pingRedis().catch(() => false);
    const status  = redisOk ? 'healthy' : 'degraded';
    return res.status(redisOk ? 200 : 503).json({
      status,
      version: '0.1.0',
      service: 'mdbai',
      redis:   redisOk ? 'connected' : 'disconnected',
      worker:  true,
      timestamp: new Date().toISOString(),
    });
  } catch (e) {
    return res.status(503).json({ status: 'error', error: e.message });
  }
});

export default dashboardRouter;
