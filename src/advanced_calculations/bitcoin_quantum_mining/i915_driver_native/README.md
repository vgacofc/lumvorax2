# DRIVER I915 NATIF - NIVEAU 3 CONTRÔLE GPU
## Environnement Virtuel pour Modification Driver Kernel

**Date** : 2026-05-06  
**Objectif** : Attaquer le driver i915 en parallèle de Level Zero  
**Environnement** : Virtuel (n'affecte pas le système)

---

## 1. ARCHITECTURE

```
i915_driver_native/
├── README.md                    (ce fichier)
├── kernel_sources/              (sources kernel Linux)
│   └── drivers/gpu/drm/i915/   (driver i915)
├── patches/                     (nos modifications)
├── tools/                       (outils traçage)
├── vm/                          (config VM si nécessaire)
└── tests/                       (tests driver modifié)
```

---

## 2. OBJECTIFS NIVEAU 3 (DRIVER I915)

### 2.1 Traçage Batch Buffers
- Intercepter `drm_i915_gem_execbuffer2`
- Logger chaque commande GPU
- Analyser flux réel GPU

### 2.2 Forensic GPU
- Tracer allocations GEM
- Monitorer GTT (Graphics Translation Table)
- Capturer états GPU

### 2.3 Optimisations
- Bypass validations non critiques
- Optimiser chemins critiques
- Réduire overheads driver

---

## 3. APPROCHE SÉCURISÉE

### 3.1 Environnement Virtuel
- **VM QEMU/KVM** : Tester driver modifié sans risque
- **Namespace** : Isoler modifications
- **Chroot** : Environnement séparé

### 3.2 Pas de Modification Système
- Compilation hors-tree
- Module kernel externe
- Chargement manuel (insmod)

---

## 4. SOURCES NÉCESSAIRES

### 4.1 Kernel Linux
```bash
# Version actuelle système
uname -r  # 6.17

# Cloner sources correspondantes
git clone --depth 1 --branch v6.17 \
  https://github.com/torvalds/linux.git kernel_sources
```

### 4.2 Driver i915
```
kernel_sources/drivers/gpu/drm/i915/
├── i915_drv.c           (driver principal)
├── i915_gem.c           (GEM - Graphics Execution Manager)
├── i915_gem_execbuffer.c (execbuffer - CIBLE PRINCIPALE)
├── i915_debugfs.c       (debugfs)
└── ...
```

---

## 5. PLAN D'ATTAQUE

### Phase 1 : Reconnaissance (EN COURS)
- [x] Cloner sources kernel
- [ ] Identifier points d'entrée i915
- [ ] Analyser `i915_gem_execbuffer.c`
- [ ] Mapper flux GPU

### Phase 2 : Traçage Basique
- [ ] Ajouter printk dans execbuffer
- [ ] Logger batch buffers
- [ ] Compiler module externe
- [ ] Tester en VM

### Phase 3 : Forensic Avancé
- [ ] Dumper batch buffers complets
- [ ] Décoder commandes GPU
- [ ] Analyser ISA Gen9
- [ ] Corréler avec Level Zero

### Phase 4 : Optimisations
- [ ] Identifier bottlenecks
- [ ] Bypass validations
- [ ] Optimiser chemins critiques
- [ ] Benchmarker gains

---

## 6. OUTILS DÉVELOPPÉS

### 6.1 Traçage
- `i915_trace.c` : Module kernel traçage
- `batch_dumper.c` : Dump batch buffers
- `gem_monitor.c` : Monitor allocations GEM

### 6.2 Analyse
- `decode_batch.py` : Décodeur batch buffers
- `isa_analyzer.py` : Analyseur ISA Gen9
- `forensic_viewer.py` : Visualisation forensique

---

## 7. SÉCURITÉ

### 7.1 Risques
- ❌ Kernel panic si erreur
- ❌ Corruption GPU
- ❌ Freeze système

### 7.2 Mitigations
- ✅ Tests en VM uniquement
- ✅ Snapshots avant tests
- ✅ Pas de modification système
- ✅ Module externe chargeable/déchargeable

---

## 8. COMMANDES UTILES

### 8.1 Compilation Module Externe
```bash
# Compiler module i915 modifié
cd kernel_sources/drivers/gpu/drm/i915
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# Charger module (VM uniquement !)
sudo insmod i915.ko

# Décharger
sudo rmmod i915
```

### 8.2 Traçage
```bash
# Activer debugfs
sudo mount -t debugfs none /sys/kernel/debug

# Lire traces i915
sudo cat /sys/kernel/debug/dri/0/i915_gem_objects

# Tracer execbuffer
sudo trace-cmd record -e i915:i915_gem_request_add
```

### 8.3 Analyse
```bash
# Dumper batch buffer
sudo ./tools/batch_dumper /dev/dri/renderD128

# Décoder
python3 tools/decode_batch.py batch_dump.bin

# Analyser ISA
python3 tools/isa_analyzer.py batch_dump.bin
```

---

## 9. INTÉGRATION AVEC LEVEL ZERO

### 9.1 Corrélation
- Level Zero soumet commandes
- i915 traçage capture batch buffers
- Analyse corrélée Level Zero ↔ i915

### 9.2 Optimisation Conjointe
- Identifier overheads Level Zero
- Optimiser driver i915
- Mesurer gains combinés

---

## 10. PROCHAINES ÉTAPES

1. [ ] Cloner kernel Linux v6.17
2. [ ] Analyser `i915_gem_execbuffer.c`
3. [ ] Créer module traçage basique
4. [ ] Tester en environnement isolé
5. [ ] Intégrer avec Level Zero

---

**STATUT** : 🚧 EN PRÉPARATION  
**PRIORITÉ** : Parallèle à Level Zero  
**RISQUE** : Moyen (VM isolée)
