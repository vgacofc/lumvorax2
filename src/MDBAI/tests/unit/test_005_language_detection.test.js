/**
 * TEST_005 — Détection de langage
 * CF-005: Dépendances installées / Langage détecté
 * Conforme STANDARD_NAMES_MDBAI.md Section 7
 */

import { describe, test, expect, beforeEach, afterEach } from '@jest/globals';
import { mkdirSync, writeFileSync, rmSync } from 'fs';
import { join } from 'path';
import { AnalysisService } from '../../src/services/analysis.service.js';
import {
  LANG_NODEJS, LANG_PYTHON, LANG_RUST, LANG_GO, LANG_C_CPP, LANG_UNKNOWN,
} from '../../src/models/result.model.js';

const TMP = '/tmp/mdbai_test_005';

describe('TEST_005 — Détection Langage', () => {
  let service;

  beforeEach(() => {
    service = new AnalysisService('test-005');
    mkdirSync(TMP, { recursive: true });
  });

  afterEach(() => {
    rmSync(TMP, { recursive: true, force: true });
  });

  test('T005-A: package.json → LANG_NODEJS', async () => {
    writeFileSync(join(TMP, 'package.json'), '{"name":"test"}');
    const lang = await service.detectLanguage(TMP);
    expect(lang).toBe(LANG_NODEJS);
  });

  test('T005-B: requirements.txt → LANG_PYTHON', async () => {
    writeFileSync(join(TMP, 'requirements.txt'), 'flask==2.0.0\n');
    const lang = await service.detectLanguage(TMP);
    expect(lang).toBe(LANG_PYTHON);
  });

  test('T005-C: Cargo.toml → LANG_RUST', async () => {
    writeFileSync(join(TMP, 'Cargo.toml'), '[package]\nname = "test"\n');
    const lang = await service.detectLanguage(TMP);
    expect(lang).toBe(LANG_RUST);
  });

  test('T005-D: go.mod → LANG_GO', async () => {
    writeFileSync(join(TMP, 'go.mod'), 'module test\ngo 1.21\n');
    const lang = await service.detectLanguage(TMP);
    expect(lang).toBe(LANG_GO);
  });

  test('T005-E: .c file → LANG_C_CPP', async () => {
    writeFileSync(join(TMP, 'main.c'), '#include <stdio.h>\n');
    const lang = await service.detectLanguage(TMP);
    expect(lang).toBe(LANG_C_CPP);
  });

  test('T005-F: répertoire vide → LANG_UNKNOWN', async () => {
    const emptyDir = join(TMP, 'empty');
    mkdirSync(emptyDir, { recursive: true });
    const lang = await service.detectLanguage(emptyDir);
    expect(lang).toBe(LANG_UNKNOWN);
  });

  test('T005-G: pyproject.toml → LANG_PYTHON', async () => {
    writeFileSync(join(TMP, 'pyproject.toml'), '[tool.poetry]\nname = "test"\n');
    const lang = await service.detectLanguage(TMP);
    expect(lang).toBe(LANG_PYTHON);
  });

  test('T005-H: langages connus sont tous dans STANDARD_NAMES', () => {
    const langs = [LANG_NODEJS, LANG_PYTHON, LANG_RUST, LANG_GO, LANG_C_CPP, LANG_UNKNOWN];
    expect(langs).toHaveLength(6);
    expect(langs).toContain('nodejs');
    expect(langs).toContain('python');
    expect(langs).toContain('rust');
    expect(langs).toContain('go');
    expect(langs).toContain('c_cpp');
    expect(langs).toContain('unknown');
  });
});
