/**
 * MDBAI — Configuration centralisée
 * Toutes les constantes depuis variables d'environnement (jamais en dur)
 * Conforme STANDARD_NAMES_MDBAI.md Section 2 + 6
 */

export const config = {
  app: {
    name: 'mdbai',
    version: '0.1.0',
    env: process.env.NODE_ENV || 'development',
    port: parseInt(process.env.API_PORT || '3001', 10),
    logLevel: process.env.LOG_LEVEL || 'debug',
    baseUrl: process.env.BASE_URL || `http://localhost:${process.env.API_PORT || '3001'}`,
  },

  telegram: {
    token: process.env.TELEGRAM_BOT_TOKEN || '',
    botUsername: 'masterdebugai_bot',
    polling: process.env.NODE_ENV !== 'production',
  },

  redis: {
    url: process.env.REDIS_URL || '',
    host: process.env.REDIS_HOST || 'localhost',
    port: parseInt(process.env.REDIS_PORT || '6379', 10),
    password: process.env.REDIS_PASSWORD || '',
    tls: process.env.NODE_ENV === 'production' ? {} : undefined,
  },

  github: {
    appId: process.env.GITHUB_APP_ID || process.env.MDBAI_APP_ID || '',
    clientId: process.env.GITHUB_APP_CLIENT_ID || process.env.MDBAI_CLIENT_ID || '',
    clientSecret: process.env.GITHUB_APP_CLIENT_SECRET || process.env.MDBAI_CLIENT_SECRET || '',
    privateKey: (process.env.GITHUB_PRIVATE_KEY || process.env.MDBAI_PRIVATE_KEY || '').replace(/\\n/g, '\n'),
    webhookSecret: process.env.GITHUB_WEBHOOK_SECRET || process.env.MDBAI_WEBHOOK_SECRET || '',
    callbackUrl: process.env.GITHUB_CALLBACK_URL ||
      `https://${process.env.REPLIT_DEV_DOMAIN || 'localhost:3001'}/auth/github/callback`,
    oauthScopes: ['repo', 'read:user'],
  },

  session: {
    secret: process.env.SESSION_SECRET || 'mdbai-dev-secret-change-in-prod',
  },

  email: {
    smtpHost: process.env.SMTP_HOST || 'smtp.gmail.com',
    smtpPort: parseInt(process.env.SMTP_PORT || '587', 10),
    smtpSecure: process.env.SMTP_SECURE === 'true',
    smtpUser: process.env.SMTP_USER || '',
    smtpPassword: process.env.SMTP_PASSWORD || '',
    fromAddress: process.env.EMAIL_FROM || 'noreply@mdbai.dev',
  },

  jobs: {
    queueName: 'analysis-jobs',
    timeoutMs: 600000,
    retryCount: 3,
    workerConcurrency: 3,
  },

  forensic: {
    libPath: process.env.MDBAI_FORENSIC_LIB_PATH ||
      new URL('../forensic/libmdbai_forensic.so', import.meta.url).pathname,
    logRotationMb: 20,
    granularity: 'page',
  },

  analysis: {
    maxRepoSizeMb: 500,
    execTimeoutMs: 300000,
    tmpDir: '/tmp/mdbai-analysis',
    supportedLanguages: ['nodejs', 'python', 'rust', 'go', 'c_cpp'],
  },

  pr: {
    branchPrefix: 'mdbai-analysis-',
    labels: ['mdbai', 'automated-analysis', 'forensic'],
    reportFilename: 'RAPPORT_MDBAI_',
  },
};

export function validateConfig() {
  const required = [
    ['TELEGRAM_BOT_TOKEN', config.telegram.token],
    ['REDIS_URL', config.redis.url],
  ];

  const missing = required
    .filter(([, val]) => !val)
    .map(([name]) => name);

  if (missing.length > 0) {
    const warn = `[MDBAI-CONFIG] ⚠️ Variables manquantes: ${missing.join(', ')} — mode dégradé`;
    console.warn(warn);
    return false;
  }
  return true;
}

export default config;
