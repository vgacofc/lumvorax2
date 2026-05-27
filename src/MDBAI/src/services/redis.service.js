/**
 * MDBAI — Service Redis + BullMQ
 * Conforme STANDARD_NAMES_MDBAI.md Section 2 (REDIS_*), Section 6 (constantes)
 * Connexion Redis Cloud Europe-West1 via variables d'env — jamais de credentials en dur
 */

import { Queue, QueueEvents } from 'bullmq';
import Redis from 'ioredis';
import logger from '../utils/logger.js';
import { config } from '../config.js';
import { MdbaiError, ERR_REDIS_CONNECT } from '../utils/errors.js';

let redisClient = null;
let analysisQueue = null;
let queueEvents = null;

/**
 * Retourne la configuration Redis depuis env (jamais credentials hardcodés)
 */
function getRedisOptions() {
  if (config.redis.url) {
    return { url: config.redis.url };
  }
  const opts = {
    host: config.redis.host,
    port: config.redis.port,
    password: config.redis.password || undefined,
    maxRetriesPerRequest: null,
    enableReadyCheck: false,
    retryStrategy: (times) => {
      if (times > 5) return null;
      return Math.min(times * 500, 3000);
    },
  };
  if (config.redis.tls) opts.tls = config.redis.tls;
  return opts;
}

/**
 * Initialise le client Redis singleton
 */
export function getRedisClient() {
  if (redisClient) return redisClient;

  const opts = getRedisOptions();
  redisClient = opts.url ? new Redis(opts.url, {
    maxRetriesPerRequest: null,
    enableReadyCheck: false,
    retryStrategy: (t) => t > 5 ? null : Math.min(t * 500, 3000),
  }) : new Redis(opts);

  redisClient.on('connect',  () => logger.info('[REDIS] Connecté au serveur Redis Cloud'));
  redisClient.on('ready',    () => logger.info('[REDIS] Redis prêt'));
  redisClient.on('error',    (e) => logger.error('[REDIS] Erreur connexion', { error: e.message }));
  redisClient.on('close',    () => logger.warn('[REDIS] Connexion fermée'));
  redisClient.on('reconnecting', () => logger.warn('[REDIS] Reconnexion...'));

  return redisClient;
}

/**
 * Retourne la queue BullMQ analysis-jobs
 */
export function getAnalysisQueue() {
  if (analysisQueue) return analysisQueue;

  const connection = getRedisClient();
  analysisQueue = new Queue(config.jobs.queueName, {
    connection,
    defaultJobOptions: {
      attempts: config.jobs.retryCount,
      backoff: { type: 'exponential', delay: 2000 },
      removeOnComplete: { count: 100 },
      removeOnFail: { count: 50 },
      timeout: config.jobs.timeoutMs,
    },
  });

  analysisQueue.on('error', (e) => logger.error('[QUEUE] Erreur queue', { error: e.message }));
  logger.info(`[QUEUE] Queue "${config.jobs.queueName}" initialisée`);
  return analysisQueue;
}

/**
 * Retourne le QueueEvents pour écouter les événements de progression
 */
export function getQueueEvents() {
  if (queueEvents) return queueEvents;
  const connection = getRedisClient();
  queueEvents = new QueueEvents(config.jobs.queueName, { connection });
  return queueEvents;
}

/**
 * Enfile un job d'analyse dans BullMQ
 * @param {AnalysisJob} job - Job créé via job.model.js
 * @returns {Promise<Job>} Job BullMQ
 */
export async function enqueueAnalysisJob(job) {
  const queue = getAnalysisQueue();
  logger.info(`[QUEUE] Enqueue job ${job.id}`, { repo: job.repo_url });
  const bullJob = await queue.add('analyze', job, { jobId: job.id });
  logger.info(`[QUEUE] Job enqueued BullMQ id=${bullJob.id}`);
  return bullJob;
}

/**
 * Retourne l'état d'un job par son ID
 * @param {string} jobId
 * @returns {Promise<object|null>}
 */
export async function getJobStatus(jobId) {
  const queue = getAnalysisQueue();
  const job = await queue.getJob(jobId);
  if (!job) return null;

  const state = await job.getState();
  const progress = job.progress || 0;
  return {
    id: job.id,
    state,
    progress,
    data: job.data,
    returnvalue: job.returnvalue,
    failedReason: job.failedReason,
    timestamp: job.timestamp,
  };
}

/**
 * Stocke un résultat d'analyse dans Redis avec TTL 24h
 * @param {string} jobId
 * @param {AnalysisResult} result
 */
export async function storeJobResult(jobId, result) {
  const client = getRedisClient();
  const key = `mdbai:result:${jobId}`;
  await client.setex(key, 86400, JSON.stringify(result));
  logger.info(`[REDIS] Résultat stocké clé=${key} TTL=24h`);
}

/**
 * Récupère un résultat depuis Redis
 * @param {string} jobId
 * @returns {Promise<AnalysisResult|null>}
 */
export async function getJobResult(jobId) {
  const client = getRedisClient();
  const key = `mdbai:result:${jobId}`;
  const raw = await client.get(key);
  if (!raw) return null;
  return JSON.parse(raw);
}

/**
 * Vérifie la connexion Redis (health check)
 * @returns {Promise<boolean>}
 */
export async function pingRedis() {
  try {
    const client = getRedisClient();
    const res = await client.ping();
    return res === 'PONG';
  } catch (e) {
    logger.error('[REDIS] Ping échoué', { error: e.message });
    return false;
  }
}

/**
 * Ferme proprement les connexions Redis
 */
export async function closeRedis() {
  if (queueEvents)    { await queueEvents.close();    queueEvents   = null; }
  if (analysisQueue)  { await analysisQueue.close();  analysisQueue = null; }
  if (redisClient)    { await redisClient.quit();     redisClient   = null; }
  logger.info('[REDIS] Connexions fermées proprement');
}

export default {
  getRedisClient, getAnalysisQueue, getQueueEvents,
  enqueueAnalysisJob, getJobStatus, storeJobResult, getJobResult,
  pingRedis, closeRedis,
};
