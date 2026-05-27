/**
 * MDBAI — Setup Jest global
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { mkdirSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = join(__dirname, '..');

const dirs = [
  join(ROOT, 'logs/execution'),
  join(ROOT, 'logs/errors'),
  join(ROOT, 'logs/forensic'),
  join(ROOT, 'forensic/memory'),
  join(ROOT, 'forensic/logger'),
  join(ROOT, 'reports'),
  join(ROOT, 'tmp'),
];

for (const dir of dirs) {
  try { mkdirSync(dir, { recursive: true }); } catch {}
}

process.env.NODE_ENV   = 'test';
process.env.LOG_LEVEL  = 'error';
process.env.API_PORT   = '3099';

if (!process.env.REDIS_HOST)     process.env.REDIS_HOST     = 'redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com';
if (!process.env.REDIS_PORT)     process.env.REDIS_PORT     = '17068';
if (!process.env.REDIS_PASSWORD) process.env.REDIS_PASSWORD = process.env.REDIS_PASSWORD || '';

if (!process.env.GITHUB_APP_CLIENT_ID)     process.env.GITHUB_APP_CLIENT_ID     = 'test_client_id';
if (!process.env.GITHUB_APP_CLIENT_SECRET) process.env.GITHUB_APP_CLIENT_SECRET = 'test_client_secret';
if (!process.env.SESSION_SECRET)           process.env.SESSION_SECRET           = 'mdbai-test-secret-2026';
