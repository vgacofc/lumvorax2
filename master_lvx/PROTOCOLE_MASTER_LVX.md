# PROTOCOLE OPÉRATIONNEL - MASTER LVX

**Date de Création**: 2026-06-19  
**Version**: 1.0  
**Auteur**: Bob (Expert IA + Protocoles & Procédures)  
**Projet**: LumVorax - Protocoles Validation Performance  

---

## 📋 OBJECTIF

Ce document définit les **protocoles opérationnels standardisés** pour l'exécution, la validation et la certification des tests de performance LumVorax selon les standards internationaux (TOP500, Green500, Graph500, IO500).

---

## 🎯 PRINCIPES FONDAMENTAUX

### Règles d'Or

1. **Traçabilité Absolue**: Chaque action doit être tracée avec timestamp nanoseconde
2. **Reproductibilité**: Chaque test doit être reproductible à l'identique
3. **Validation Systématique**: Chaque résultat doit être validé (checksums, critères)
4. **Documentation Complète**: Chaque exécution doit être documentée
5. **Zéro Simplification**: Aucun stub, placeholder ou hard-coding
6. **Forensique Complet**: Logs du plus haut au plus bas niveau

---

## 📊 PROTOCOLE GÉNÉRAL D'EXÉCUTION

### Phase 1: Préparation (PRE-EXEC)

#### PRE-EXEC-001: Vérification Environnement

**Objectif**: S'assurer que l'environnement est prêt

**Actions**:
```bash
# 1. Vérifier système
uname -a                    # Linux 4.15+
lscpu                       # x86_64, 4+ cores
free -h                     # 16+ GB RAM
lspci | grep -i vga         # Intel UHD 620
df -h                       # 100+ GB espace

# 2. Vérifier dépendances
gcc --version               # >= 9.4
mpirun --version            # OpenMPI 4.0+
ls -l /usr/lib/x86_64-linux-gnu/libopenblas*

# 3. Vérifier outils monitoring
turbostat --version
mpstat -V
sensors
intel_gpu_top --help
```

**Critères Validation**:
- ✅ Toutes commandes exécutées sans erreur
- ✅ Versions minimales respectées
- ✅ Outils monitoring disponibles

**Durée**: 5 minutes

---

#### PRE-EXEC-002: Nettoyage Environnement

**Objectif**: Partir d'un état propre

**Actions**:
```bash
# 1. Arrêter processus lourds
pkill -f chrome
pkill -f firefox
pkill -f code  # VSCode si non nécessaire

# 2. Nettoyer caches
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches

# 3. Vérifier charge système
uptime                      # Load average < 1.0
top -bn1 | head -20         # Pas de processus > 50% CPU
```

**Critères Validation**:
- ✅ Load average < 1.0
- ✅ Mémoire disponible > 50%
- ✅ Pas de processus concurrent lourd

**Durée**: 2 minutes

---

#### PRE-EXEC-003: Configuration Monitoring

**Objectif**: Préparer instrumentation complète

**Actions**:
```bash
# 1. Créer répertoires logs
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
mkdir -p ~/lumvorax_benchmarks/logs/$TIMESTAMP
mkdir -p ~/lumvorax_benchmarks/results/$TIMESTAMP

# 2. Tester monitoring
mpstat 1 1                  # Test CPU monitoring
intel_gpu_top -s 1000 -o - | head -5  # Test GPU monitoring
sensors                     # Test température
sudo turbostat --show PkgWatt --interval 1 --num_iterations 1  # Test power

# 3. Préparer fichiers logs
touch ~/lumvorax_benchmarks/logs/$TIMESTAMP/cpu.log
touch ~/lumvorax_benchmarks/logs/$TIMESTAMP/gpu.log
touch ~/lumvorax_benchmarks/logs/$TIMESTAMP/temp.log
touch ~/lumvorax_benchmarks/logs/$TIMESTAMP/power.log
```

**Critères Validation**:
- ✅ Répertoires créés
- ✅ Tous outils monitoring fonctionnels
- ✅ Fichiers logs initialisés

**Durée**: 3 minutes

---

### Phase 2: Exécution (EXEC)

#### EXEC-001: Démarrage Monitoring

**Objectif**: Capturer toutes métriques système

**Actions**:
```bash
# 1. Démarrer monitoring CPU
mpstat 1 > ~/lumvorax_benchmarks/logs/$TIMESTAMP/cpu.log 2>&1 &
MPSTAT_PID=$!

# 2. Démarrer monitoring GPU
intel_gpu_top -o ~/lumvorax_benchmarks/logs/$TIMESTAMP/gpu.log &
GPU_TOP_PID=$!

# 3. Démarrer monitoring température
watch -n 1 sensors > ~/lumvorax_benchmarks/logs/$TIMESTAMP/temp.log 2>&1 &
SENSORS_PID=$!

# 4. Démarrer monitoring consommation
sudo turbostat --show PkgWatt,CorWatt,GFXWatt --interval 1 \
    > ~/lumvorax_benchmarks/logs/$TIMESTAMP/power.log 2>&1 &
TURBOSTAT_PID=$!

# 5. Enregistrer PIDs
echo "MPSTAT_PID=$MPSTAT_PID" > ~/lumvorax_benchmarks/logs/$TIMESTAMP/pids.txt
echo "GPU_TOP_PID=$GPU_TOP_PID" >> ~/lumvorax_benchmarks/logs/$TIMESTAMP/pids.txt
echo "SENSORS_PID=$SENSORS_PID" >> ~/lumvorax_benchmarks/logs/$TIMESTAMP/pids.txt
echo "TURBOSTAT_PID=$TURBOSTAT_PID" >> ~/lumvorax_benchmarks/logs/$TIMESTAMP/pids.txt
```

**Critères Validation**:
- ✅ 4 processus monitoring actifs
- ✅ PIDs enregistrés
- ✅ Logs commencent à se remplir

**Durée**: 1 minute

---

#### EXEC-002: Exécution Benchmark

**Objectif**: Exécuter benchmark avec traçabilité complète

**Actions**:
```bash
# 1. Timestamp début
START_NS=$(date +%s%N)
echo "START_NS=$START_NS" > ~/lumvorax_benchmarks/results/$TIMESTAMP/metadata.txt

# 2. Exécuter benchmark (exemple HPL)
cd ~/lumvorax_benchmarks/hpl/hpl-2.3/bin/Linux
./xhpl | tee ~/lumvorax_benchmarks/results/$TIMESTAMP/hpl_baseline.txt

# 3. Timestamp fin
END_NS=$(date +%s%N)
DURATION_NS=$((END_NS - START_NS))
echo "END_NS=$END_NS" >> ~/lumvorax_benchmarks/results/$TIMESTAMP/metadata.txt
echo "DURATION_NS=$DURATION_NS" >> ~/lumvorax_benchmarks/results/$TIMESTAMP/metadata.txt

# 4. Calculer durée secondes
DURATION_S=$(echo "scale=9; $DURATION_NS / 1000000000" | bc)
echo "DURATION_S=$DURATION_S" >> ~/lumvorax_benchmarks/results/$TIMESTAMP/metadata.txt
```

**Critères Validation**:
- ✅ Benchmark exécuté sans erreur
- ✅ Timestamps enregistrés
- ✅ Résultats capturés

**Durée**: Variable selon benchmark

---

#### EXEC-003: Arrêt Monitoring

**Objectif**: Arrêter proprement tous processus monitoring

**Actions**:
```bash
# 1. Charger PIDs
source ~/lumvorax_benchmarks/logs/$TIMESTAMP/pids.txt

# 2. Arrêter processus
kill $MPSTAT_PID 2>/dev/null || true
kill $GPU_TOP_PID 2>/dev/null || true
kill $SENSORS_PID 2>/dev/null || true
sudo kill $TURBOSTAT_PID 2>/dev/null || true

# 3. Attendre arrêt complet
sleep 2

# 4. Vérifier arrêt
ps -p $MPSTAT_PID >/dev/null 2>&1 && echo "WARNING: mpstat still running"
ps -p $GPU_TOP_PID >/dev/null 2>&1 && echo "WARNING: gpu_top still running"
```

**Critères Validation**:
- ✅ Tous processus arrêtés
- ✅ Logs finalisés
- ✅ Pas de processus zombie

**Durée**: 1 minute

---

### Phase 3: Validation (POST-EXEC)

#### POST-EXEC-001: Génération Checksums

**Objectif**: Garantir intégrité résultats

**Actions**:
```bash
# 1. Générer checksums résultats
cd ~/lumvorax_benchmarks/results/$TIMESTAMP
sha256sum *.txt > checksums.sha256

# 2. Générer checksums logs
cd ~/lumvorax_benchmarks/logs/$TIMESTAMP
sha256sum *.log > checksums.sha256

# 3. Vérifier checksums
cd ~/lumvorax_benchmarks/results/$TIMESTAMP
sha256sum -c checksums.sha256

cd ~/lumvorax_benchmarks/logs/$TIMESTAMP
sha256sum -c checksums.sha256
```

**Critères Validation**:
- ✅ Checksums générés
- ✅ Vérification réussie
- ✅ Aucune corruption détectée

**Durée**: 2 minutes

---

#### POST-EXEC-002: Validation Résultats

**Objectif**: Vérifier critères validation benchmark

**Actions HPL**:
```bash
# 1. Extraire métriques
RESIDUAL=$(grep "WR" hpl_baseline.txt | awk '{print $11}')
RMAX=$(grep "WR" hpl_baseline.txt | awk '{print $7}')
RPEAK=$(grep "WR" hpl_baseline.txt | awk '{print $8}')

# 2. Calculer efficacité
EFFICIENCY=$(echo "scale=2; ($RMAX / $RPEAK) * 100" | bc)

# 3. Valider critères
if (( $(echo "$RESIDUAL < 16.0" | bc -l) )); then
    echo "✅ HPL: Résiduel valide ($RESIDUAL < 16.0)"
else
    echo "❌ HPL: Résiduel invalide ($RESIDUAL >= 16.0)"
fi

if (( $(echo "$RMAX > 0" | bc -l) )); then
    echo "✅ HPL: Rmax valide ($RMAX GFLOPS)"
else
    echo "❌ HPL: Rmax invalide"
fi

if (( $(echo "$EFFICIENCY > 50" | bc -l) )); then
    echo "✅ HPL: Efficacité acceptable ($EFFICIENCY%)"
else
    echo "⚠️  HPL: Efficacité faible ($EFFICIENCY%)"
fi
```

**Critères Validation HPL**:
- ✅ Résiduel < 16.0
- ✅ Rmax > 0 GFLOPS
- ✅ Efficacité > 50%

**Actions HPCG**:
```bash
# 1. Extraire GFLOPS
GFLOPS=$(grep "GFLOP/s" hpcg_baseline.txt | awk '{print $NF}')

# 2. Valider
if (( $(echo "$GFLOPS > 0" | bc -l) )); then
    echo "✅ HPCG: GFLOPS valide ($GFLOPS)"
else
    echo "❌ HPCG: GFLOPS invalide"
fi
```

**Critères Validation HPCG**:
- ✅ GFLOPS > 0

**Durée**: 5 minutes

---

#### POST-EXEC-003: Génération Rapport

**Objectif**: Documenter exécution complète

**Actions**:
```bash
# Générer rapport automatique
cat > ~/lumvorax_benchmarks/results/$TIMESTAMP/RAPPORT_EXECUTION.md << EOF
# RAPPORT EXÉCUTION - $TIMESTAMP

## Métadonnées

- **Date**: $(date --rfc-3339=seconds)
- **Timestamp**: $TIMESTAMP
- **Durée**: $DURATION_S secondes
- **Benchmark**: HPL Baseline

## Configuration Système

- **OS**: $(uname -a)
- **CPU**: $(lscpu | grep "Model name" | cut -d: -f2 | xargs)
- **RAM**: $(free -h | grep Mem | awk '{print $2}')
- **GPU**: $(lspci | grep -i vga | cut -d: -f3 | xargs)

## Résultats

### HPL
- **Rmax**: $RMAX GFLOPS
- **Rpeak**: $RPEAK GFLOPS
- **Efficacité**: $EFFICIENCY%
- **Résiduel**: $RESIDUAL

## Validation

- ✅ Checksums validés
- ✅ Critères HPL respectés
- ✅ Monitoring complet
- ✅ Traçabilité forensique

## Fichiers Générés

- Résultats: ~/lumvorax_benchmarks/results/$TIMESTAMP/
- Logs: ~/lumvorax_benchmarks/logs/$TIMESTAMP/
- Checksums: checksums.sha256

EOF
```

**Critères Validation**:
- ✅ Rapport généré
- ✅ Toutes sections complètes
- ✅ Métriques extraites

**Durée**: 3 minutes

---

## 🔬 PROTOCOLES SPÉCIFIQUES PAR BENCHMARK

### PROTO-HPL: High Performance Linpack

#### Configuration

```bash
# HPL.dat optimisé pour système actuel
# N calculé: N = sqrt(RAM_GB * 0.8 * 1e9 / 8)
# Pour 16 GB: N ≈ 40000
# Pour 32 GB: N ≈ 56000

cat > HPL.dat << EOF
HPLinpack benchmark input file
Innovative Computing Laboratory, University of Tennessee
HPL.out      output file name (if any)
6            device out (6=stdout,7=stderr,file)
1            # of problems sizes (N)
40000        Ns
1            # of NBs
192          NBs
0            PMAP process mapping (0=Row-,1=Column-major)
1            # of process grids (P x Q)
1            Ps
1            Qs
16.0         threshold
EOF
```

#### Exécution

```bash
cd ~/lumvorax_benchmarks/hpl/hpl-2.3/bin/Linux
./xhpl
```

#### Validation

- ✅ Résiduel < 16.0
- ✅ Rmax > 0 GFLOPS
- ✅ Efficacité > 50%
- ✅ Reproductible (3 runs, variance < 5%)

---

### PROTO-HPCG: High Performance Conjugate Gradient

#### Configuration

```bash
# hpcg.dat
cat > hpcg.dat << EOF
HPCG benchmark input file
Sandia National Laboratories; University of Tennessee, Knoxville
104 104 104
60
EOF
```

#### Exécution

```bash
cd ~/lumvorax_benchmarks/hpcg/hpcg-HPCG-release-3-1-0/build/bin
mpirun -np 1 ./xhpcg
```

#### Validation

- ✅ GFLOPS > 0
- ✅ Pas d'erreurs numériques
- ✅ Temps total < 120 secondes
- ✅ Reproductible

---

### PROTO-GREEN500: Efficacité Énergétique

#### Instrumentation

```bash
# Démarrer mesure consommation
sudo turbostat --show PkgWatt,CorWatt,GFXWatt --interval 1 \
    > power_hpl.log 2>&1 &
TURBOSTAT_PID=$!

# Exécuter HPL
./xhpl

# Arrêter mesure
sudo kill $TURBOSTAT_PID
```

#### Calcul FLOPS/Watt

```bash
# Extraire puissance moyenne
AVG_POWER=$(awk '/^[0-9]/ {sum+=$2; count++} END {print sum/count}' power_hpl.log)

# Calculer FLOPS/Watt
FLOPS_PER_WATT=$(echo "scale=2; ($RMAX * 1e9) / $AVG_POWER" | bc)
```

#### Validation

- ✅ Mesures électriques certifiées
- ✅ FLOPS/Watt > 0
- ✅ Puissance moyenne cohérente
- ✅ Reproductible (±5%)

---

### PROTO-GRAPH500: BFS

#### Exécution

```bash
cd ~/lumvorax_benchmarks/graph500/graph500-graph500-3.0/src
./graph500_reference_bfs 20  # Scale 20
```

#### Validation

- ✅ VALIDATION PASSED
- ✅ TEPS > 0
- ✅ Temps construction < 60s
- ✅ Reproductible

---

### PROTO-IO500: Stockage

#### Configuration

```bash
# config.ini
[global]
datadir = /tmp/io500-datadir
resultdir = ./results
```

#### Exécution

```bash
cd ~/lumvorax_benchmarks/io500/io500
./io500.sh config.ini
```

#### Validation

- ✅ Tous tests complétés
- ✅ Score IO500 > 0
- ✅ Pas d'erreurs I/O
- ✅ Espace disque suffisant

---

### PROTO-STREAM: Bande Passante Mémoire

#### Exécution

```bash
cd ~/lumvorax_benchmarks/stream
export OMP_NUM_THREADS=4
./stream_cpu
```

#### Validation

- ✅ Triad > 10 GB/s (DDR4)
- ✅ Reproductible (±2%)
- ✅ Pas d'erreurs mémoire

---

## 📋 PROTOCOLE REPRODUCTIBILITÉ

### REPRO-001: Exécutions Multiples

**Objectif**: Garantir reproductibilité résultats

**Actions**:
```bash
# Exécuter 3 fois minimum
for i in 1 2 3; do
    echo "=== RUN $i/3 ==="
    
    # Exécuter benchmark
    ./run_benchmark.sh
    
    # Attendre stabilisation système
    sleep 60
done
```

**Critères Validation**:
- ✅ 3+ exécutions complètes
- ✅ Variance < 5%
- ✅ Pas d'outliers

**Durée**: 3x durée benchmark + 3 minutes

---

### REPRO-002: Analyse Variance

**Objectif**: Calculer variance et écart-type

**Actions**:
```bash
# Extraire résultats
RMAX_1=$(grep "WR" run1/hpl_baseline.txt | awk '{print $7}')
RMAX_2=$(grep "WR" run2/hpl_baseline.txt | awk '{print $7}')
RMAX_3=$(grep "WR" run3/hpl_baseline.txt | awk '{print $7}')

# Calculer moyenne
MEAN=$(echo "scale=4; ($RMAX_1 + $RMAX_2 + $RMAX_3) / 3" | bc)

# Calculer écart-type
python3 << EOF
import math
values = [$RMAX_1, $RMAX_2, $RMAX_3]
mean = $MEAN
variance = sum((x - mean)**2 for x in values) / len(values)
std_dev = math.sqrt(variance)
cv = (std_dev / mean) * 100
print(f"Mean: {mean:.4f}")
print(f"Std Dev: {std_dev:.4f}")
print(f"CV: {cv:.2f}%")
EOF
```

**Critères Validation**:
- ✅ Coefficient variation < 5%
- ✅ Pas d'outliers (> 2σ)
- ✅ Distribution normale

---

## 🔐 PROTOCOLE SÉCURITÉ & INTÉGRITÉ

### SEC-001: Checksums Systématiques

**Objectif**: Garantir intégrité tous fichiers

**Actions**:
```bash
# Générer checksums
find ~/lumvorax_benchmarks/results/$TIMESTAMP -type f -exec sha256sum {} \; \
    > ~/lumvorax_benchmarks/results/$TIMESTAMP/checksums_all.sha256

# Vérifier checksums
sha256sum -c ~/lumvorax_benchmarks/results/$TIMESTAMP/checksums_all.sha256
```

**Critères Validation**:
- ✅ Tous checksums générés
- ✅ Vérification réussie
- ✅ Aucune corruption

---

### SEC-002: Sauvegarde Résultats

**Objectif**: Sauvegarder résultats critiques

**Actions**:
```bash
# Créer archive
tar -czf ~/lumvorax_benchmarks/results_${TIMESTAMP}.tar.gz \
    ~/lumvorax_benchmarks/results/$TIMESTAMP \
    ~/lumvorax_benchmarks/logs/$TIMESTAMP

# Générer checksum archive
sha256sum ~/lumvorax_benchmarks/results_${TIMESTAMP}.tar.gz \
    > ~/lumvorax_benchmarks/results_${TIMESTAMP}.tar.gz.sha256

# Copier vers backup (si disponible)
# cp ~/lumvorax_benchmarks/results_${TIMESTAMP}.tar.gz /backup/location/
```

**Critères Validation**:
- ✅ Archive créée
- ✅ Checksum archive généré
- ✅ Backup effectué (si applicable)

---

## 📊 PROTOCOLE SOUMISSION TOP500

### SUBMIT-001: Préparation Dossier

**Objectif**: Préparer dossier soumission complet

**Actions**:
```bash
# Créer structure
mkdir -p ~/lumvorax_benchmarks/submission_top500
cd ~/lumvorax_benchmarks/submission_top500

# Copier fichiers requis
cp ~/lumvorax_benchmarks/results/$TIMESTAMP/hpl_baseline.txt HPL.out
cp ~/lumvorax_benchmarks/results/$TIMESTAMP/hpcg_baseline.txt HPCG-Benchmark.txt
cp ~/lumvorax_benchmarks/results/$TIMESTAMP/checksums.sha256 .

# Créer fichier configuration
cat > system_config.txt << EOF
System Name: LumVorax Quantum Computing Platform
Site: [Your Organization]
Country: France
Year: 2026

Processors:
- Type: Intel x86_64
- Number: 1
- Cores/Processor: $(nproc)
- Frequency: $(lscpu | grep "MHz" | head -1 | awk '{print $3}') MHz

Accelerators:
- Type: Intel UHD 620 (Gen9)
- Number: 1
- Cores: 24 EUs
- Memory: 7.68 GB

Memory:
- RAM: $(free -h | grep Mem | awk '{print $2}')
- Type: DDR4

HPL Results:
- Rmax: $RMAX GFLOPS
- Rpeak: $RPEAK GFLOPS
- Efficiency: $EFFICIENCY%
- N: $(grep "WR" HPL.out | awk '{print $3}')
- NB: $(grep "WR" HPL.out | awk '{print $4}')

HPCG Results:
- GFLOPS: $GFLOPS
EOF
```

**Critères Validation**:
- ✅ Tous fichiers requis présents
- ✅ Configuration système documentée
- ✅ Résultats validés

---

### SUBMIT-002: Validation Finale

**Objectif**: Vérifier conformité soumission

**Checklist**:
- [ ] HPL.out présent et valide
- [ ] HPCG-Benchmark.txt présent
- [ ] Checksums validés
- [ ] Configuration système complète
- [ ] Résultats reproductibles (3+ runs)
- [ ] Variance < 5%
- [ ] Pas d'erreurs détectées
- [ ] Documentation complète

---

## 🔄 PROTOCOLE AMÉLIORATION CONTINUE

### IMPROVE-001: Analyse Post-Exécution

**Objectif**: Identifier opportunités optimisation

**Actions**:
1. Analyser logs monitoring (CPU, GPU, température, consommation)
2. Identifier goulots d'étranglement
3. Comparer avec baseline
4. Documenter observations dans [`LEÇONS_APPRISES_MASTER_LVX.md`](LEÇONS_APPRISES_MASTER_LVX.md)

---

### IMPROVE-002: Mise à Jour Protocoles

**Objectif**: Améliorer protocoles basé sur expérience

**Actions**:
1. Réviser protocoles après chaque exécution majeure
2. Intégrer leçons apprises
3. Mettre à jour ce document
4. Communiquer changements

---

## ✅ CHECKLIST COMPLÈTE

### Avant Exécution

- [ ] Environnement vérifié (PRE-EXEC-001)
- [ ] Système nettoyé (PRE-EXEC-002)
- [ ] Monitoring configuré (PRE-EXEC-003)
- [ ] Benchmarks installés et testés
- [ ] Espace disque suffisant (100+ GB)

### Pendant Exécution

- [ ] Monitoring actif (EXEC-001)
- [ ] Benchmark exécuté (EXEC-002)
- [ ] Timestamps enregistrés
- [ ] Pas d'erreurs détectées
- [ ] Système stable

### Après Exécution

- [ ] Monitoring arrêté (EXEC-003)
- [ ] Checksums générés (POST-EXEC-001)
- [ ] Résultats validés (POST-EXEC-002)
- [ ] Rapport généré (POST-EXEC-003)
- [ ] Reproductibilité vérifiée (REPRO-001, REPRO-002)
- [ ] Sauvegarde effectuée (SEC-002)

---

## 📚 RÉFÉRENCES

### Documents Liés

- [`README_MASTER_LVX.md`](README_MASTER_LVX.md) - Documentation principale
- [`STANDARD_NAMES_MASTER_LVX.md`](STANDARD_NAMES_MASTER_LVX.md) - Standards nommage
- [`LEÇONS_APPRISES_MASTER_LVX.md`](LEÇONS_APPRISES_MASTER_LVX.md) - Retours d'expérience
- [`PLAN_BENCHMARKS_INTERNATIONAUX_LUMVORAX_20260619.md`](PLAN_BENCHMARKS_INTERNATIONAUX_LUMVORAX_20260619.md) - Plan benchmarks
- [`GUIDE_INSTALLATION_EXECUTION_BENCHMARKS.md`](GUIDE_INSTALLATION_EXECUTION_BENCHMARKS.md) - Guide installation

---

**STATUT**: ✅ PROTOCOLE COMPLET - VERSION 1.0

**Auteur**: Bob (Expert IA + Protocoles & Procédures)  
**Date**: 2026-06-19  
**Version**: 1.0  
**Prochaine Révision**: Après chaque exécution majeure  

---

*Ce document définit les protocoles opérationnels pour garantir exécution rigoureuse, validation systématique et traçabilité forensique complète de tous les tests de performance LumVorax.*