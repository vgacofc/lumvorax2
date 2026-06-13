# 🧪 GUIDE TEST MANUEL - Validation BUG#80

**Date**: 2026-06-08  
**Version**: 1.0.0  
**Objectif**: Valider correction BUG#80 (ForensicMonitoringService couches 6-7)

---

## ✅ PRÉREQUIS VALIDÉS

**Serveur MDBAI**: ✅ ACTIF
- URL: http://localhost:3001
- Status: healthy
- Redis: connected
- Telegram: active
- Worker: active
- Forensic lib: /home/lvx/LVX/lumvorax2/src/MDBAI/forensic/libmdbai_forensic.so

**Corrections Implémentées**:
- ✅ BUG#79: Chemin fichier Bob corrigé
- ✅ BUG#80: ForensicMonitoringService créé (368 lignes)
- ✅ 4 hooks forensic intégrés dans worker
- ✅ Détection anomalies 5 règles
- ✅ Logs signés HMAC-SHA256

---

## 📋 PROCÉDURE TEST MANUEL

### ÉTAPE 1: Préparer Monitoring (Terminal 1)

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Créer répertoire monitoring si absent
mkdir -p forensic/monitoring

# Surveiller logs serveur en temps réel
tail -f logs/server_restart_*.log
```

### ÉTAPE 2: Surveiller Fichiers Forensic (Terminal 2)

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Surveiller création fichiers forensic
watch -n 1 'ls -lhtr forensic/monitoring/ | tail -10'
```

### ÉTAPE 3: Lancer Analyse Test (Terminal 3)

**Option A: Via API directe**
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Test avec dépôt public simple
curl -X POST http://localhost:3001/api/analyze \
  -H "Content-Type: application/json" \
  -d '{
    "repo_url": "https://github.com/octocat/Hello-World",
    "branch": "master"
  }'
```

**Option B: Via Telegram**
```
1. Ouvrir Telegram
2. Chercher bot: @masterdebugai_bot
3. Envoyer: /analyze https://github.com/octocat/Hello-World
```

### ÉTAPE 4: Vérifier Logs Forensic (Terminal 4)

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Attendre que job démarre (5-10 secondes)
sleep 10

# Trouver dernier job ID
LAST_JOB=$(ls -t forensic/bob-tasks/ | head -1)
echo "Job ID: $LAST_JOB"

# Surveiller logs Bob
tail -f forensic/bob-tasks/$LAST_JOB/bob-execution.log
```

---

## 🔍 POINTS DE VALIDATION

### ✅ Validation 1: Snapshots Capturés

**Chercher dans logs serveur**:
```bash
grep "📸 Snapshot" logs/server_restart_*.log | tail -10
```

**Attendu**:
```
[WORKER] 📸 Snapshot Bob capturé pour détection anomalies
[WORKER] 📸 Snapshot rapport capturé — détection anomalies en cours...
[WORKER] 📸 Snapshot PR capturé
[WORKER] 📸 Snapshot final capturé
```

### ✅ Validation 2: Fichiers Forensic Générés

**Vérifier fichiers créés**:
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Lister fichiers monitoring
ls -lh forensic/monitoring/

# Attendu: 2 fichiers par job
# - {jobId}_monitoring.jsonl (logs événements)
# - {jobId}_report.json (rapport forensic)
```

**Exemple**:
```
-rw-r--r-- 1 lvx lvx  15K Jun  8 01:10 mdbai-abc123_monitoring.jsonl
-rw-r--r-- 1 lvx lvx  8.2K Jun  8 01:10 mdbai-abc123_report.json
```

### ✅ Validation 3: Logs Signés HMAC-SHA256

**Vérifier signatures dans monitoring.jsonl**:
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Trouver dernier fichier monitoring
LAST_MONITORING=$(ls -t forensic/monitoring/*_monitoring.jsonl | head -1)

# Afficher premier événement avec signature
head -1 $LAST_MONITORING | jq '.'
```

**Attendu**:
```json
{
  "timestamp": 1717804200000,
  "timestampISO": "2026-06-08T01:10:00.000Z",
  "jobId": "mdbai-abc123",
  "type": "INIT",
  "message": "Service forensic monitoring démarré",
  "data": {"layer": 6},
  "elapsedMs": 0,
  "signature": "a1b2c3d4e5f6789..." // ← HMAC-SHA256
}
```

### ✅ Validation 4: Détection Anomalies

**Vérifier rapport forensic**:
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Trouver dernier rapport
LAST_REPORT=$(ls -t forensic/monitoring/*_report.json | head -1)

# Afficher résumé
cat $LAST_REPORT | jq '.summary'
```

**Attendu (si aucune anomalie)**:
```json
{
  "total_events": 12,
  "total_anomalies": 0,
  "critical_anomalies": 0,
  "high_anomalies": 0,
  "medium_anomalies": 0,
  "status": "CLEAN"
}
```

**Attendu (si anomalies détectées)**:
```json
{
  "total_events": 15,
  "total_anomalies": 2,
  "critical_anomalies": 2,
  "high_anomalies": 0,
  "medium_anomalies": 0,
  "status": "FRAUD_DETECTED"
}
```

### ✅ Validation 5: Alertes Anomalies Critiques

**Chercher alertes dans logs**:
```bash
grep "🚨 ANOMALIES CRITIQUES" logs/server_restart_*.log
```

**Si anomalies détectées, attendu**:
```
[WORKER] 🚨 ANOMALIES CRITIQUES DÉTECTÉES {
  count: 2,
  anomalies: [
    {
      type: 'SCORE_DIVERGENCE',
      severity: 'CRITICAL',
      message: 'Score Bob (23) vs Rapport (100) = divergence 77 points'
    }
  ]
}
```

### ✅ Validation 6: Overhead Performance

**Mesurer overhead CPU**:
```bash
# Avant analyse
top -b -n 1 | grep "node src/server.js"

# Pendant analyse (attendre 30s après démarrage job)
top -b -n 1 | grep "node src/server.js"

# Overhead attendu: <1% CPU supplémentaire
```

---

## 📊 RÉSULTATS ATTENDUS

### Scénario 1: Analyse Normale (Aucune Anomalie)

**Fichiers générés**:
- ✅ `forensic/monitoring/{jobId}_monitoring.jsonl` (~10-20 KB)
- ✅ `forensic/monitoring/{jobId}_report.json` (~5-10 KB)
- ✅ `forensic/bob-tasks/{jobId}/bob-analysis.json`
- ✅ `forensic/bob-tasks/{jobId}/bob-execution.log`

**Logs serveur**:
- ✅ "📸 Snapshot Bob capturé"
- ✅ "📸 Snapshot rapport capturé"
- ✅ "📸 Snapshot PR capturé"
- ✅ "📸 Snapshot final capturé"
- ✅ "📊 Rapport forensique généré"
- ✅ "✅ Aucune anomalie détectée"

**Rapport forensic**:
- ✅ `status: "CLEAN"`
- ✅ `total_anomalies: 0`
- ✅ `signature` présente (64 caractères hex)

### Scénario 2: Anomalies Détectées (Test BUG#79)

**Si score diverge >10 points**:
- ✅ Anomalie `SCORE_DIVERGENCE` détectée
- ✅ Severity: `CRITICAL`
- ✅ Alerte dans logs: "🚨 ANOMALIES CRITIQUES DÉTECTÉES"
- ✅ `status: "FRAUD_DETECTED"`

---

## 🐛 DÉPANNAGE

### Problème 1: Fichiers monitoring non créés

**Vérifier**:
```bash
# Répertoire existe ?
ls -ld forensic/monitoring/

# Permissions OK ?
ls -l forensic/

# Service importé ?
grep "ForensicMonitoringService" src/workers/analysis.worker.js
```

**Solution**:
```bash
# Créer répertoire si absent
mkdir -p forensic/monitoring
chmod 755 forensic/monitoring

# Redémarrer serveur
pkill -f "node src/server.js"
doppler run -- npm start &
```

### Problème 2: Snapshots non capturés

**Vérifier logs**:
```bash
grep "forensicMonitor" logs/server_restart_*.log
```

**Si aucun log**:
- Service non initialisé → Vérifier import ligne 30 worker
- Variable undefined → Vérifier déclaration ligne 67 worker

### Problème 3: Signatures invalides

**Vérifier secret HMAC**:
```bash
grep "FORENSIC_HMAC_SECRET" .env
```

**Si absent**:
```bash
echo 'FORENSIC_HMAC_SECRET=mdbai-forensic-secret-production-2026' >> .env
```

---

## 📝 CHECKLIST VALIDATION FINALE

Cocher après chaque test réussi:

- [ ] Serveur démarre sans erreur
- [ ] Analyse job lancée avec succès
- [ ] 4 snapshots capturés (Bob, Rapport, PR, Final)
- [ ] Fichier `{jobId}_monitoring.jsonl` créé
- [ ] Fichier `{jobId}_report.json` créé
- [ ] Logs contiennent signatures HMAC-SHA256
- [ ] Rapport forensic contient `summary.status`
- [ ] Détection anomalies fonctionne (si divergence)
- [ ] Alertes critiques affichées (si fraude)
- [ ] Overhead CPU <1%

---

## ✅ VALIDATION RÉUSSIE SI

**Tous les critères remplis**:
1. ✅ 4 snapshots capturés à chaque job
2. ✅ 2 fichiers forensic générés par job
3. ✅ Logs signés HMAC-SHA256 valides
4. ✅ Détection anomalies opérationnelle
5. ✅ Overhead performance <1% CPU
6. ✅ Aucune erreur dans logs serveur

**Résultat**: BUG#80 CORRIGÉ ET VALIDÉ ✅

---

## 📞 SUPPORT

**En cas de problème**:
1. Consulter logs: `tail -100 logs/server_restart_*.log`
2. Vérifier imports: `grep -n "ForensicMonitoring" src/workers/analysis.worker.js`
3. Tester service isolé: `node -e "import('./src/services/forensic-monitoring.service.js')"`

**Rapports disponibles**:
- `RAPPORT_CORRECTION_BUG80_FORENSIC_COUCHES_6_7.md` (673 lignes)
- `LEÇONS_APPRISES_MDBAI.md` v3.40.0 (231 leçons)

---

**FIN DU GUIDE**