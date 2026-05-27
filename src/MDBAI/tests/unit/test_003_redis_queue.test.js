/**
 * TEST_003 — Redis + BullMQ Queue
 * CF-003: /analyze → Job enqueued
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 * Teste la configuration Redis sans connexion réelle
 */

import { describe, test, expect } from '@jest/globals';
import { createJob, JOB_STATUS_PENDING, JOB_STATUS_RUNNING,
  JOB_STATUS_COMPLETED, JOB_STATUS_FAILED, updateJobStatus, serializeJob } from '../../src/models/job.model.js';
import { config } from '../../src/config.js';

describe('TEST_003 — Redis Queue et Job Model', () => {

  test('T003-A: config.jobs.queueName = "analysis-jobs"', () => {
    expect(config.jobs.queueName).toBe('analysis-jobs');
  });

  test('T003-B: config.jobs.timeoutMs = 600000 (10 min)', () => {
    expect(config.jobs.timeoutMs).toBe(600000);
  });

  test('T003-C: config.jobs.workerConcurrency = 3', () => {
    expect(config.jobs.workerConcurrency).toBe(3);
  });

  test('T003-D: createJob() génère un job valide', () => {
    const job = createJob('https://github.com/user/repo', '12345678');
    expect(job.id).toMatch(/^mdbai:[0-9a-f-]{36}$/);
    expect(job.status).toBe(JOB_STATUS_PENDING);
    expect(job.repo_url).toBe('https://github.com/user/repo');
    expect(job.user_id).toBe('12345678');
    expect(job.attempts).toBe(0);
  });

  test('T003-E: updateJobStatus() → running', () => {
    const job = createJob('https://github.com/user/repo', '123');
    const running = updateJobStatus(job, JOB_STATUS_RUNNING);
    expect(running.status).toBe(JOB_STATUS_RUNNING);
    expect(running.started_at).not.toBeNull();
  });

  test('T003-F: updateJobStatus() → completed', () => {
    const job = createJob('https://github.com/user/repo', '123');
    const done = updateJobStatus(job, JOB_STATUS_COMPLETED, { result: { score: 95 } });
    expect(done.status).toBe(JOB_STATUS_COMPLETED);
    expect(done.completed_at).not.toBeNull();
    expect(done.result.score).toBe(95);
  });

  test('T003-G: serializeJob() masque le token', () => {
    const job = createJob('https://github.com/u/r', '123', 'secret_token_xyz');
    const serialized = JSON.parse(serializeJob(job));
    expect(serialized.github_token).toBe('[REDACTED]');
    expect(serialized.github_token).not.toContain('secret_token_xyz');
  });

  test('T003-H: createJob() jobs différents ont des IDs uniques', () => {
    const j1 = createJob('https://github.com/a/b', '1');
    const j2 = createJob('https://github.com/a/b', '1');
    expect(j1.id).not.toBe(j2.id);
  });

  test('T003-I: statuts constants définis correctement', () => {
    expect(JOB_STATUS_PENDING).toBe('pending');
    expect(JOB_STATUS_RUNNING).toBe('running');
    expect(JOB_STATUS_COMPLETED).toBe('completed');
    expect(JOB_STATUS_FAILED).toBe('failed');
  });
});
