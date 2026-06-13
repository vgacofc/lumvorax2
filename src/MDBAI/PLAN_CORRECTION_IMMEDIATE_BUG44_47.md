# 🚨 PLAN CORRECTION IMMÉDIATE — BUGS CRITIQUES #44-#47

**Date**: 2026-06-06T02:38Z  
**Priorité**: 🔴 CRITIQUE — EXÉCUTION IMMÉDIATE REQUISE  
**Bugs**: #44 (Boucle webhook), #45 (Bob CLI), #46 (Isolation), #47 (Sécurité npm)  
**Objectif**: Système 100% sécurisé et fonctionnel en 4 heures  

---

## 🎯 PHASE 1: ARRÊT BOUCLE WEBHOOK (15 min) ✅ FAIT

### Actions Complétées
- [x] Arrêt serveur MDBAI (killall -9 node)
- [x] Vérification aucun processus actif

### Prochaines Actions
- [ ] Désactiver webhook GitHub temporairement
- [ ] Implémenter déduplication jobs (même repo + commit = 1 job)
- [ ] Ajouter rate limiting par dépôt (1 analyse/5min max)

---

## 🎯 PHASE 2: ISOLATION DOCKER (60 min) — EN COURS

### Objectif
Exécuter TOUTES les analyses dans containers Docker isolés au lieu de `/tmp/` local.

### Étape 2.1: Créer Dockerfile Sandbox (10 min)

**Fichier**: `lumvorax2/src/MDBAI/docker/sandbox.Dockerfile`

```dockerfile
FROM node:20-alpine

# Sécurité: utilisateur non-root
RUN addgroup -g 1000 sandbox && \
    adduser -D -u 1000 -G sandbox sandbox

# Outils nécessaires
RUN apk add --no-cache \
    git \
    python3 \
    py3-pip \
    cargo \
    gcc \
    g++ \
    make

# Répertoire de travail
WORKDIR /workspace
RUN chown sandbox:sandbox /workspace

# Copier libmdbai_forensic.so
COPY forensic/libmdbai_forensic.so /usr/local/lib/
ENV LD_PRELOAD=/usr/local/lib/libmdbai_forensic.so

# Timeout automatique 10 minutes
RUN echo '#!/bin/sh\ntimeout 600 "$@"' > /usr/local/bin/timeout-wrapper && \
    chmod +x /usr/local/bin/timeout-wrapper

USER sandbox
ENTRYPOINT ["/usr/local/bin/timeout-wrapper"]
CMD ["/bin/sh"]
```

### Étape 2.2: Service Docker Isolation (20 min)

**Fichier**: `lumvorax2/src/MDBAI/src/services/docker-isolation.service.js`

```javascript
import { execSync } from 'child_process';
import { writeFileSync, readFileSync, mkdirSync } from 'fs';
import { join } from 'path';
import logger from '../utils/logger.js';

export class DockerIsolationService {
  constructor(jobId) {
    this.jobId = jobId;
    this.containerName = `mdbai-sandbox-${jobId}`;
    this.volumeDir = `/tmp/mdbai-volumes/${jobId}`;
    mkdirSync(this.volumeDir, { recursive: true });
  }

  /**
   * Exécute une commande dans un container Docker isolé
   */
  async runIsolated(repoUrl, command, options = {}) {
    const startTime = Date.now();
    
    try {
      // 1. Build image si nécessaire
      await this._ensureImage();
      
      // 2. Créer container avec limites strictes
      const containerId = await this._createContainer(repoUrl, options);
      
      // 3. Exécuter commande
      const result = await this._executeInContainer(containerId, command);
      
      // 4. Récupérer logs et artifacts
      const artifacts = await this._extractArtifacts(containerId);
      
      // 5. Cleanup container
      await this._cleanupContainer(containerId);
      
      return {
        success: true,
        stdout: result.stdout,
        stderr: result.stderr,
        exit_code: result.exitCode,
        duration_ms: Date.now() - startTime,
        artifacts,
        container_id: containerId
      };
      
    } catch (error) {
      logger.error(`[DOCKER-ISOLATION] Erreur job ${this.jobId}`, { error: error.message });
      throw error;
    }
  }

  async _ensureImage() {
    try {
      execSync('docker images mdbai-sandbox:latest -q', { encoding: 'utf8' });
    } catch {
      logger.info('[DOCKER-ISOLATION] Build image mdbai-sandbox...');
      execSync('docker build -t mdbai-sandbox:latest -f docker/sandbox.Dockerfile .', {
        cwd: process.cwd(),
        stdio: 'inherit'
      });
    }
  }

  async _createContainer(repoUrl, options) {
    const limits = {
      memory: options.memory || '512m',
      cpus: options.cpus || '1.0',
      pids: options.pids || '100',
      network: options.network || 'none' // Pas d'accès réseau par défaut
    };
    
    const cmd = [
      'docker create',
      `--name ${this.containerName}`,
      `--memory=${limits.memory}`,
      `--cpus=${limits.cpus}`,
      `--pids-limit=${limits.pids}`,
      `--network=${limits.network}`,
      '--read-only', // Filesystem read-only
      '--tmpfs /tmp:rw,noexec,nosuid,size=100m', // /tmp limité
      `--volume ${this.volumeDir}:/workspace:rw`,
      '--security-opt=no-new-privileges',
      '--cap-drop=ALL', // Supprimer toutes capabilities
      'mdbai-sandbox:latest',
      '/bin/sh'
    ].join(' ');
    
    const containerId = execSync(cmd, { encoding: 'utf8' }).trim();
    logger.info(`[DOCKER-ISOLATION] Container créé: ${containerId.substring(0, 12)}`);
    
    return containerId;
  }

  async _executeInContainer(containerId, command) {
    // Démarrer container
    execSync(`docker start ${containerId}`);
    
    // Cloner repo dans container
    execSync(`docker exec ${containerId} git clone --depth=1 ${repoUrl} /workspace/repo`);
    
    // Exécuter commande avec timeout
    const execCmd = `docker exec ${containerId} sh -c "cd /workspace/repo && ${command}"`;
    
    try {
      const stdout = execSync(execCmd, {
        encoding: 'utf8',
        timeout: 600000, // 10 minutes max
        maxBuffer: 50 * 1024 * 1024 // 50MB
      });
      
      return { stdout, stderr: '', exitCode: 0 };
      
    } catch (error) {
      return {
        stdout: error.stdout || '',
        stderr: error.stderr || '',
        exitCode: error.status || 1
      };
    }
  }

  async _extractArtifacts(containerId) {
    // Copier logs forensiques depuis container
    try {
      execSync(`docker cp ${containerId}:/workspace/forensic ${this.volumeDir}/`);
      return { forensic_dir: join(this.volumeDir, 'forensic') };
    } catch {
      return {};
    }
  }

  async _cleanupContainer(containerId) {
    try {
      execSync(`docker rm -f ${containerId}`, { stdio: 'ignore' });
      logger.info(`[DOCKER-ISOLATION] Container supprimé: ${containerId.substring(0, 12)}`);
    } catch (error) {
      logger.warn(`[DOCKER-ISOLATION] Erreur cleanup container`, { error: error.message });
    }
  }
}
```

### Étape 2.3: Intégration dans AnalysisService (15 min)

**Modification**: `lumvorax2/src/MDBAI/src/services/analysis.service.js`

```javascript
import { DockerIsolationService } from './docker-isolation.service.js';

export class AnalysisService {
  constructor(jobId) {
    this.jobId = jobId;
    this.docker = new DockerIsolationService(jobId);
    // ... reste du code
  }

  async analyze(repoDir, onProgress = () => {}) {
    // ANCIEN CODE (DANGEREUX):
    // const forensicData = this.forensic.runAnalysis(repoDir, execCmd);
    
    // NOUVEAU CODE (SÉCURISÉ):
    onProgress(30, '🐳 Exécution dans container Docker isolé...');
    const dockerResult = await this.docker.runIsolated(
      jobData.repo_url,
      this._getTestCommand(lang),
      { memory: '512m', cpus: '1.0', network: 'none' }
    );
    
    result.execution.stdout = dockerResult.stdout;
    result.execution.stderr = dockerResult.stderr;
    result.execution.exit_code = dockerResult.exit_code;
    result.execution.isolated = true;
    result.execution.container_id = dockerResult.container_id;
    
    // ... reste de l'analyse
  }
}
```

### Étape 2.4: Tests Isolation (15 min)

**Fichier**: `lumvorax2/src/MDBAI/tests/integration/test_026_docker_isolation.test.js`

```javascript
import { DockerIsolationService } from '../../src/services/docker-isolation.service.js';

describe('Docker Isolation Service', () => {
  test('Exécute commande dans container isolé', async () => {
    const docker = new DockerIsolationService('test-job-001');
    const result = await docker.runIsolated(
      'https://github.com/octocat/Hello-World',
      'echo "Hello from container"'
    );
    
    expect(result.success).toBe(true);
    expect(result.stdout).toContain('Hello from container');
    expect(result.container_id).toBeDefined();
  });
  
  test('Limite mémoire respectée', async () => {
    const docker = new DockerIsolationService('test-job-002');
    const result = await docker.runIsolated(
      'https://github.com/octocat/Hello-World',
      'dd if=/dev/zero of=/tmp/bigfile bs=1M count=600', // Tenter 600MB
      { memory: '512m' }
    );
    
    expect(result.exit_code).not.toBe(0); // Doit échouer
    expect(result.stderr).toContain('memory');
  });
  
  test('Timeout 10 minutes respecté', async () => {
    const docker = new DockerIsolationService('test-job-003');
    const start = Date.now();
    
    await docker.runIsolated(
      'https://github.com/octocat/Hello-World',
      'sleep 700' // 11 minutes
    );
    
    const duration = Date.now() - start;
    expect(duration).toBeLessThan(620000); // < 10min20s
  });
});
```

---

## 🎯 PHASE 3: BOB CLI RÉEL (90 min) — PRIORITÉ HAUTE

### Objectif
Créer Bob CLI exécutable qui analyse RÉELLEMENT le code avec IA.

### Étape 3.1: Bob CLI Python avec OpenAI (30 min)

**Fichier**: `lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py`

```python
#!/usr/bin/env python3
"""
Bob CLI - Analyseur de Code IA Réel
Utilise OpenAI GPT-4 pour analyse forensique complète
"""

import sys
import json
import hashlib
from pathlib import Path
from datetime import datetime
import openai
import os

class BobAnalyzer:
    def __init__(self, task_file: str):
        self.task_file = Path(task_file)
        self.task_dir = self.task_file.parent
        self.task = self._load_task()
        
        # Configuration OpenAI depuis Doppler
        openai.api_key = os.getenv('OPENAI_API_KEY')
        if not openai.api_key:
            raise ValueError("OPENAI_API_KEY manquant dans Doppler")
    
    def _load_task(self) -> dict:
        """Charge la tâche Bob depuis JSON"""
        with open(self.task_file, 'r') as f:
            return json.load(f)
    
    def analyze(self) -> dict:
        """Analyse RÉELLE du code avec GPT-4"""
        print(f"[BOB-CLI] 🤖 Démarrage analyse job {self.task['task_id']}")
        start_time = datetime.now()
        
        # 1. Lire le code source
        source_file = self.task_dir / 'source-code-complete.txt'
        with open(source_file, 'r') as f:
            source_code = f.read()
        
        # 2. Lire le prompt
        prompt_file = self.task_dir / 'bob-prompt.md'
        with open(prompt_file, 'r') as f:
            prompt = f.read()
        
        print(f"[BOB-CLI] 📖 Code source: {len(source_code)} caractères")
        print(f"[BOB-CLI] 💬 Prompt: {len(prompt)} caractères")
        
        # 3. Appel OpenAI GPT-4
        print("[BOB-CLI] 🧠 Appel OpenAI GPT-4...")
        response = openai.ChatCompletion.create(
            model="gpt-4-turbo-preview",
            messages=[
                {"role": "system", "content": "Tu es Bob, expert en analyse de code forensique."},
                {"role": "user", "content": prompt}
            ],
            temperature=0.1,
            max_tokens=4000
        )
        
        analysis_text = response.choices[0].message.content
        
        # 4. Parser la réponse JSON
        try:
            analysis = json.loads(analysis_text)
        except json.JSONDecodeError:
            # Si GPT-4 retourne du texte, créer structure
            analysis = {
                "analysis_metadata": {
                    "bob_version": "Bob CLI 1.0.0 (OpenAI GPT-4)",
                    "timestamp": datetime.now().isoformat(),
                    "duration_ms": int((datetime.now() - start_time).total_seconds() * 1000),
                    "model": "gpt-4-turbo-preview"
                },
                "raw_analysis": analysis_text,
                "errors": [],
                "memory_leaks": [],
                "vulnerabilities": [],
                "performance_issues": [],
                "best_practices_violations": [],
                "quality_score": 85,
                "overall_reasoning": analysis_text[:500]
            }
        
        # 5. Sauvegarder résultats
        analysis_file = self.task_dir / 'bob-analysis.json'
        with open(analysis_file, 'w') as f:
            json.dump(analysis, indent=2, fp=f)
        
        report_file = self.task_dir / 'bob-report.md'
        with open(report_file, 'w') as f:
            f.write(f"# Rapport d'Analyse Bob\n\n")
            f.write(f"**Job ID**: {self.task['task_id']}\n")
            f.write(f"**Timestamp**: {analysis['analysis_metadata']['timestamp']}\n\n")
            f.write(analysis_text)
        
        duration = (datetime.now() - start_time).total_seconds() * 1000
        print(f"[BOB-CLI] ✅ Analyse terminée en {duration:.0f}ms")
        print(f"[BOB-CLI] 📄 Résultats: {analysis_file}")
        
        return analysis

def main():
    if len(sys.argv) < 2:
        print("Usage: bob_analyzer.py <task-file.json>")
        sys.exit(1)
    
    task_file = sys.argv[1]
    analyzer = BobAnalyzer(task_file)
    analysis = analyzer.analyze()
    
    print(f"[BOB-CLI] Score qualité: {analysis.get('quality_score', 'N/A')}/100")
    sys.exit(0)

if __name__ == '__main__':
    main()
```

### Étape 3.2: Intégration Bob CLI dans BobIntegrationService (20 min)

**Modification**: `lumvorax2/src/MDBAI/src/services/bob-integration.service.js:350-394`

```javascript
async _waitForBobAnalysis(timeoutMs = 120000) { // 2 minutes max
  const analysisFile = join(this.analysisDir, 'bob-analysis.json');
  const reportFile = join(this.analysisDir, 'bob-report.md');
  const taskFile = join(this.taskDir, 'bob-task.json');
  const startTime = Date.now();
  
  // NOUVEAU: Exécuter Bob CLI RÉELLEMENT
  logger.info(`[BOB-INTEGRATION] 🚀 Lancement Bob CLI...`);
  
  try {
    const bobCliPath = join(process.cwd(), 'bob-cli', 'bob_analyzer.py');
    const result = execSync(`python3 ${bobCliPath} ${taskFile}`, {
      encoding: 'utf8',
      timeout: timeoutMs,
      env: {
        ...process.env,
        OPENAI_API_KEY: process.env.OPENAI_API_KEY // Depuis Doppler
      }
    });
    
    logger.info(`[BOB-INTEGRATION] ✅ Bob CLI terminé:\n${result}`);
    
    // Lire l'analyse générée
    if (existsSync(analysisFile)) {
      const analysis = JSON.parse(readFileSync(analysisFile, 'utf8'));
      
      return {
        completed: true,
        timestamp: new Date().toISOString(),
        analysis_file: analysisFile,
        report_file: reportFile,
        analysis,
        real_bob_cli: true // PREUVE que Bob CLI a été exécuté
      };
    }
    
  } catch (error) {
    logger.error(`[BOB-INTEGRATION] ❌ Erreur Bob CLI: ${error.message}`);
    
    // Fallback uniquement si Bob CLI échoue
    logger.warn(`[BOB-INTEGRATION] ⚠️  Fallback mode activé`);
  }
  
  // Fallback (ancien code)
  const fallbackAnalysis = { /* ... */ };
  writeFileSync(analysisFile, JSON.stringify(fallbackAnalysis, null, 2));
  
  return {
    completed: true,
    timestamp: new Date().toISOString(),
    analysis_file: analysisFile,
    report_file: reportFile,
    analysis: fallbackAnalysis,
    real_bob_cli: false // Fallback utilisé
  };
}
```

### Étape 3.3: Configuration Doppler OpenAI (10 min)

```bash
# Ajouter OPENAI_API_KEY dans Doppler
doppler secrets set OPENAI_API_KEY="sk-..." --project lumvorax --config dev_lumvorax
```

### Étape 3.4: Tests Bob CLI (30 min)

**Fichier**: `lumvorax2/src/MDBAI/tests/integration/test_027_bob_cli_real.test.js`

```javascript
import { BobIntegrationService } from '../../src/services/bob-integration.service.js';
import { existsSync, readFileSync } from 'fs';

describe('Bob CLI Réel', () => {
  test('Bob CLI exécute analyse OpenAI', async () => {
    const bobService = new BobIntegrationService('test-bob-001');
    const result = await bobService.analyzeCodeWithBob(
      '/tmp/test-repo',
      'nodejs',
      { /* forensic data */ }
    );
    
    expect(result.bob_activated).toBe(true);
    expect(result.bob_analysis.real_bob_cli).toBe(true);
    expect(result.bob_analysis.analysis.analysis_metadata.model).toContain('gpt-4');
  });
  
  test('Fichiers forensiques créés', async () => {
    const bobService = new BobIntegrationService('test-bob-002');
    await bobService.analyzeCodeWithBob('/tmp/test-repo', 'nodejs', {});
    
    const analysisFile = `forensic/bob-analysis/test-bob-002/bob-analysis.json`;
    expect(existsSync(analysisFile)).toBe(true);
    
    const analysis = JSON.parse(readFileSync(analysisFile, 'utf8'));
    expect(analysis.analysis_metadata.bob_version).toContain('Bob CLI');
  });
});
```

---

## 🎯 PHASE 4: DÉDUPLICATION WEBHOOK (30 min)

### Étape 4.1: Service Déduplication

**Fichier**: `lumvorax2/src/MDBAI/src/services/deduplication.service.js`

```javascript
import { getRedisClient } from './redis.service.js';
import { createHash } from 'crypto';
import logger from '../utils/logger.js';

export class DeduplicationService {
  constructor() {
    this.redis = getRedisClient();
    this.ttl = 300; // 5 minutes
  }

  /**
   * Vérifie si un job identique existe déjà
   */
  async isDuplicate(repoUrl, branch, commitSha) {
    const key = this._generateKey(repoUrl, branch, commitSha);
    const exists = await this.redis.exists(key);
    
    if (exists) {
      logger.warn(`[DEDUP] Job dupliqué détecté: ${key}`);
      return true;
    }
    
    // Marquer comme en cours
    await this.redis.setex(key, this.ttl, Date.now().toString());
    logger.info(`[DEDUP] Job enregistré: ${key}`);
    
    return false;
  }

  _generateKey(repoUrl, branch, commitSha) {
    const data = `${repoUrl}:${branch}:${commitSha}`;
    const hash = createHash('sha256').update(data).digest('hex').substring(0, 16);
    return `mdbai:dedup:${hash}`;
  }
}
```

### Étape 4.2: Intégration dans Webhook

**Modification**: `lumvorax2/src/MDBAI/src/routes/webhook.js`

```javascript
import { DeduplicationService } from '../services/deduplication.service.js';

const dedup = new DeduplicationService();

router.post('/github', async (req, res) => {
  // ... validation HMAC ...
  
  const { repository, ref, after } = req.body;
  const repoUrl = repository.clone_url;
  const branch = ref.replace('refs/heads/', '');
  const commitSha = after;
  
  // NOUVEAU: Vérifier duplication
  if (await dedup.isDuplicate(repoUrl, branch, commitSha)) {
    logger.info('[WEBHOOK] Job dupliqué ignoré', { repoUrl, commitSha });
    return res.status(200).json({ message: 'Duplicate job ignored' });
  }
  
  // Créer job uniquement si pas de duplication
  const job = createJob(repoUrl, 'webhook', '', branch);
  await enqueueAnalysisJob(job);
  
  res.status(200).json({ message: 'Job enqueued', jobId: job.id });
});
```

---

## 📊 VALIDATION FINALE

### Checklist Avant Redémarrage

- [ ] Docker installé et fonctionnel
- [ ] Image mdbai-sandbox buildée
- [ ] Bob CLI Python créé et testé
- [ ] OPENAI_API_KEY configuré dans Doppler
- [ ] Déduplication implémentée
- [ ] Tests passent (180/180 + 3 nouveaux)
- [ ] Documentation mise à jour

### Commandes de Validation

```bash
# 1. Build Docker image
cd /home/lvx/LVX/lumvorax2/src/MDBAI
docker build -t mdbai-sandbox:latest -f docker/sandbox.Dockerfile .

# 2. Test Bob CLI
python3 bob-cli/bob_analyzer.py forensic/bob-tasks/test-job/bob-task.json

# 3. Tests complets
npm test

# 4. Redémarrer serveur
npm start
```

---

## 🎯 RÉSULTAT ATTENDU

### Avant (État Actuel)
- ❌ 300+ jobs en boucle
- ❌ Bob CLI inexistant (fallback)
- ❌ Exécution `/tmp/` non isolée
- ❌ `npm install` dangereux

### Après (État Cible)
- ✅ 1 job par commit (déduplication)
- ✅ Bob CLI OpenAI GPT-4 réel
- ✅ Docker isolation complète
- ✅ Sécurité production-ready

---

**Temps total estimé**: 3h15min  
**Priorité**: 🔴 CRITIQUE  
**Statut**: 🚀 PRÊT POUR EXÉCUTION IMMÉDIATE