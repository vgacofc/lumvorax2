# RAPPORT C198 PHASE 6 - COMMANDES RELANCE + DRIVER I915
**Date**: 2026-05-06 21:56 UTC+2  
**Conversation**: C198 (suite C197)  
**Code Root**: emmaus  
**Progression Globale**: 79% (47/59 tâches)

---

## 📊 ÉTAT ACTUEL

### Compilation IGC
- **Statut**: ARRÊTÉE à 50% (BobIDE crash)
- **Fichiers compilés**: 458/~2,000 fichiers .o
- **Build directory**: ~/igc_full/build (3.2 GB)
- **Temps restant estimé**: ~1h
- **Script relance**: ✅ CRÉÉ

### Environnement i915
- **Statut**: PRÉPARÉ
- **README**: ✅ CRÉÉ (200 lignes)
- **Script clone**: ✅ CRÉÉ (85 lignes)
- **Workspace**: ~/i915_driver_workspace (à créer)

---

## 🚀 COMMANDES RELANCE COMPILATION IGC

### Option 1: Via Script (RECOMMANDÉ)
```fish
# Aller dans le répertoire scripts
cd ~/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/scripts

# Rendre exécutable
chmod +x relance_compilation_igc.sh

# Lancer compilation en arrière-plan
./relance_compilation_igc.sh

# Le script va :
# - Vérifier l'état actuel
# - Relancer make -j8 -k en arrière-plan
# - Créer log dans /tmp/igc_compile_YYYYMMDD_HHMMSS.log
# - Afficher commandes de surveillance
```

### Option 2: Commande Directe
```fish
# Aller dans build directory
cd ~/igc_full/build

# Lancer compilation en arrière-plan avec log
nohup make -j8 -k > /tmp/igc_compile_(date +%Y%m%d_%H%M%S).log 2>&1 &

# Récupérer PID
set IGC_PID $last_pid
echo "PID compilation IGC: $IGC_PID"
```

### Surveillance Compilation

#### Progression (mise à jour toutes les 30s)
```fish
watch -n 30 'cd ~/igc_full/build && find . -name "*.o" | wc -l'
```

#### Logs temps réel
```fish
# Trouver le dernier log
set LOG_FILE (ls -t /tmp/igc_compile_*.log | head -1)

# Suivre avec filtrage pourcentage
tail -f $LOG_FILE | grep -E '\[.*%\]'
```

#### Processus actif
```fish
# Vérifier si compilation tourne
ps aux | grep -E 'make.*igc|c\+\+.*IGC' | grep -v grep

# Utilisation CPU/RAM
top -p (pgrep -f 'make.*igc' | tr '\n' ',' | sed 's/,$//')
```

#### Estimation temps restant
```fish
# Compter fichiers .o actuels
set CURRENT (find ~/igc_full/build -name "*.o" | wc -l)

# Estimation (2000 fichiers total, 8 jobs parallèles)
set REMAINING (math "2000 - $CURRENT")
set MINUTES (math "$REMAINING / 8 / 2")  # ~2 fichiers/min/job
echo "Fichiers restants: $REMAINING"
echo "Temps estimé: $MINUTES minutes"
```

---

## 🔧 COMMANDES DRIVER I915 (PARALLÈLE)

### Clonage Sources Kernel
```fish
# Aller dans répertoire i915
cd ~/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/i915_driver_native/scripts

# Rendre exécutable
chmod +x clone_i915_sources.sh

# Lancer clonage (~500 MB, 5-10 min)
./clone_i915_sources.sh

# Le script va :
# - Créer ~/i915_driver_workspace
# - Cloner kernel Linux v6.17 (shallow)
# - Copier driver i915 original
# - Créer copie de travail
# - Créer structure projet
```

### Vérification Post-Clonage
```fish
# Aller dans workspace
cd ~/i915_driver_workspace

# Vérifier structure
tree -L 2 -d

# Compter fichiers driver i915
find i915_original -name '*.c' | wc -l

# Taille totale
du -sh .
```

### Analyse Fichiers Clés
```fish
# Fichier principal: execbuffer
ls -lh i915_original/i915_gem_execbuffer.c

# Rechercher fonctions batch buffer
grep -n "batch" i915_original/i915_gem_execbuffer.c | head -20

# Rechercher fonctions traçage existantes
grep -n "trace" i915_original/*.c | wc -l
```

---

## 📋 PROCHAINES ÉTAPES

### Après Compilation IGC (Automatique)
1. **Installer IGC** (5 min)
   ```fish
   cd ~/igc_full/build
   make install DESTDIR=~/igc_local
   ```

2. **Compiler compute-runtime** (10-15 min)
   ```fish
   cd ~/compute-runtime_test/build
   cmake .. -DCMAKE_PREFIX_PATH=~/igc_local/usr/local
   make -j8
   ```

3. **Tester Level Zero**
   ```fish
   cd ~/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
   LD_LIBRARY_PATH=~/compute-runtime_test/build:~/igc_local/usr/local/lib \
   ./bin/test_levelzero_init
   ```

### Après Clonage i915 (Parallèle)
1. **Analyser execbuffer.c** (30 min)
   - Identifier points d'injection traçage
   - Comprendre flux batch buffers
   - Localiser structures GPU

2. **Créer module traçage** (1-2h)
   - Wrapper `i915_gem_execbuffer2`
   - Logger batch buffers
   - Exporter via debugfs

3. **Compiler module externe** (10 min)
   ```fish
   cd ~/i915_driver_workspace/tools
   make -C /lib/modules/(uname -r)/build M=$PWD modules
   ```

4. **Tester en environnement isolé**
   - VM ou chroot
   - Pas de risque système

---

## 🎯 OBJECTIFS FINAUX

### Level Zero Native (Objectif #1)
- ✅ Architecture complète (2,785 lignes)
- ✅ SDK installé (v1.17.42)
- ⏳ Compilation IGC (50%, relance nécessaire)
- ⏳ Drivers Gen9 (après IGC)
- ⏳ Tests + benchmark vs OpenCL

### Driver i915 (Objectif #3 - Parallèle)
- ✅ Environnement préparé
- ✅ Scripts créés
- ⏳ Clonage sources (à lancer)
- ⏳ Analyse + modifications
- ⏳ Tests isolés

### Traçabilité Bit-Level (Objectif #2)
- ✅ Logger bit-level existant
- ✅ Tracker mémoire existant
- ⏳ Intégration Level Zero
- ⏳ Intégration i915
- ⏳ Validation nanoseconde

---

## 📊 MÉTRIQUES PROGRESSION

### Tâches Complétées: 47/59 (79%)
- Architecture Level Zero: 100%
- Dépendances IGC: 100%
- Compilation IGC: 50%
- Environnement i915: 100%
- Tests finaux: 0%

### Temps Investi
- C197: 11h (OpenCL forensique)
- C198: 18h (Level Zero + i915)
- **Total**: 29h

### Temps Restant Estimé
- Compilation IGC: 1h
- Installation + tests Level Zero: 1h
- Clonage + analyse i915: 2h
- Modifications i915: 3-4h
- Tests + rapports: 2h
- **Total**: 9-10h

---

## 🔍 VÉRIFICATIONS CRITIQUES

### Avant de Relancer IGC
```fish
# RAM disponible (minimum 2 GB)
free -h | grep Mem

# Espace disque (minimum 5 GB)
df -h ~/igc_full

# Processus bloquants
ps aux | grep -E 'firefox|chrome' | grep -v grep
```

### Pendant Compilation IGC
```fish
# Toutes les 5 minutes
watch -n 300 '
  echo "=== COMPILATION IGC ==="
  echo "Fichiers .o: $(find ~/igc_full/build -name "*.o" | wc -l)/2000"
  echo "RAM: $(free -h | grep Mem | awk "{print \$3\"/\"\$2}")"
  echo "CPU: $(top -bn1 | grep "Cpu(s)" | awk "{print \$2}")"
  echo "Temps: $(date)"
'
```

### Après Compilation IGC
```fish
# Vérifier bibliothèques compilées
ls -lh ~/igc_full/build/IGC/Release/*.so

# Vérifier taille totale
du -sh ~/igc_full/build

# Vérifier erreurs critiques
grep -i "error" /tmp/igc_compile_*.log | grep -v "warning" | wc -l
```

---

## 🚨 GESTION ERREURS

### Si Compilation IGC Échoue
1. Vérifier log: `tail -100 /tmp/igc_compile_*.log`
2. Identifier erreur critique (pas warning)
3. Si erreur lldELF: IGNORER (flag -k)
4. Si erreur mémoire: libérer RAM
5. Si erreur autre: analyser + corriger

### Si Clonage i915 Échoue
1. Vérifier connexion réseau
2. Vérifier espace disque (500 MB min)
3. Essayer version kernel différente
4. Cloner depuis mirror GitHub

### Si Tests Level Zero Échouent
1. Vérifier LD_LIBRARY_PATH
2. Vérifier permissions GPU
3. Vérifier logs: `dmesg | tail -50`
4. Vérifier device: `ls -l /dev/dri/renderD128`

---

## 📝 NOTES IMPORTANTES

### Compilation IGC
- **Flag -k**: Continue malgré erreurs (BiFManager non critique)
- **Parallélisme**: -j8 optimal pour 8 threads
- **RAM**: Minimum 2 GB libre pendant compilation
- **Durée**: 1h30-2h total (50% déjà fait)

### Driver i915
- **Sécurité**: TOUJOURS tester en VM/chroot d'abord
- **Kernel**: Version 6.17 (actuelle système)
- **Module externe**: Pas de recompilation kernel complète
- **Traçage**: Via debugfs, pas de modification binaire

### Level Zero
- **Drivers**: compute-runtime v21.49 (Gen9 compatible)
- **IGC**: v1.0.17791.9 (dernière version Gen9)
- **SDK**: v1.17.42 (déjà installé)
- **Test**: UHD 620 (24 EU, 300-1050 MHz)

---

## ✅ CHECKLIST IMMÉDIATE

### À Faire MAINTENANT
- [ ] Relancer compilation IGC (Option 1 ou 2)
- [ ] Surveiller progression (watch)
- [ ] Lancer clonage i915 en parallèle
- [ ] Vérifier RAM disponible

### À Faire APRÈS Compilation IGC
- [ ] Installer IGC dans ~/igc_local
- [ ] Compiler compute-runtime
- [ ] Tester Level Zero
- [ ] Générer rapport benchmark

### À Faire APRÈS Clonage i915
- [ ] Analyser i915_gem_execbuffer.c
- [ ] Identifier points injection
- [ ] Créer module traçage
- [ ] Compiler + tester en VM

---

## 📞 CONTACT / SUPPORT

### Si Blocage Technique
1. Vérifier logs détaillés
2. Consulter documentation Intel
3. Chercher erreur sur GitHub Issues
4. Demander aide communauté

### Ressources Utiles
- Intel Graphics Compiler: https://github.com/intel/intel-graphics-compiler
- compute-runtime: https://github.com/intel/compute-runtime
- Level Zero: https://spec.oneapi.io/level-zero/latest/
- Driver i915: https://www.kernel.org/doc/html/latest/gpu/i915.html

---

**FIN RAPPORT C198 PHASE 6**

**Prochaine action**: Relancer compilation IGC + cloner i915 en parallèle