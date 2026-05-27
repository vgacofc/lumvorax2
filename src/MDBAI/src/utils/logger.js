/**
 * MDBAI — Logger Winston structuré
 * Conforme STANDARD_NAMES_MDBAI.md Section 8 + prompt.txt Règle #1 traçabilité
 * Format logs: YYYY-MM-DD_HH-MM-SS_event.log
 */

import { createLogger, format, transports } from 'winston';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import { mkdirSync } from 'fs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const LOGS_BASE = join(__dirname, '../../logs');

const logsDirs = [
  join(LOGS_BASE, 'execution'),
  join(LOGS_BASE, 'errors'),
  join(LOGS_BASE, 'forensic'),
];

for (const dir of logsDirs) {
  try { mkdirSync(dir, { recursive: true }); } catch {}
}

const ts = () => new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);

const logger = createLogger({
  level: process.env.LOG_LEVEL || 'debug',
  format: format.combine(
    format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss.SSS' }),
    format.errors({ stack: true }),
    format.json()
  ),
  defaultMeta: { service: 'mdbai', version: '0.1.0' },
  transports: [
    new transports.Console({
      format: format.combine(
        format.colorize(),
        format.printf(({ timestamp, level, message, service, ...meta }) => {
          const extra = Object.keys(meta).length ? ` ${JSON.stringify(meta)}` : '';
          return `${timestamp} [${service}] ${level}: ${message}${extra}`;
        })
      ),
    }),
    new transports.File({
      filename: join(LOGS_BASE, 'errors', `${ts()}_error.log`),
      level: 'error',
      maxsize: 20 * 1024 * 1024,
      maxFiles: 5,
    }),
    new transports.File({
      filename: join(LOGS_BASE, 'execution', `${ts()}_execution.log`),
      maxsize: 20 * 1024 * 1024,
      maxFiles: 10,
    }),
  ],
});

export function createJobLogger(jobId) {
  return logger.child({ jobId });
}

export function createForensicLogger(jobId) {
  const forensicTransport = new transports.File({
    filename: join(LOGS_BASE, 'forensic', `${ts()}_forensic_${jobId}.log`),
    maxsize: 20 * 1024 * 1024,
    maxFiles: 5,
  });
  return createLogger({
    level: 'debug',
    format: format.combine(format.timestamp(), format.json()),
    defaultMeta: { service: 'mdbai-forensic', jobId },
    transports: [forensicTransport],
  });
}

export default logger;
