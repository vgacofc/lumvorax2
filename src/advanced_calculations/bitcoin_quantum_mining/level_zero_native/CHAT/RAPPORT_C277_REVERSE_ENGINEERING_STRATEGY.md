# RAPPORT C277 — STRATÉGIE REVERSE ENGINEERING BATCH BUFFER OPENCL

**Date**: 2026-05-18  
**Session**: C277  
**Objectif**: Identifier états GPU manquants par reverse engineering batch OpenCL

---

## 1. CONTEXTE SESSION

### Directive Utilisateur
> "Option 1 : Reverse engineer états GPU manquants (solution définitive) concentre toi uniquemn sur cela !!!!!!!!!!!!!!!!!!!!!!!"

### État Avancement Précédent (Session C276)
- ✅ **TEST J exécuté**: Context save/restore isolation
  - **Résultat CRITIQUE**: CAS A (destroy/recreate) = 9 dispatches OK MAIS 9 GPU hangs cachés
  - **Preuve**: Problème n'est PAS save/restore mais **batch buffer invalide**
  - **ecode GPU hang**: `9:1:f5d7ffff` (identique pour tous les hangs)

- ✅ **Rapport forensique ultra-complet**: 3 parties, 1108 lignes totales
  - Analyse ligne par ligne 1060 lignes logs
  - Métriques: 398 MH/s, crash dispatch 28, formule `(9×3)+1=28` validée
  - 15 GPU hangs détectés (dispatches 14-28)

- ✅ **Indépendance OpenCL**: 100% atteinte
  - Système 100% i915 DRM natif
  - Aucune dépendance OpenCL/Level Zero dans code production

### Cause Racine Identifiée
Le batch buffer créé manuellement est **incomplet** et manque états GPU critiques :
- ❌ PIPE_CONTROL (flush cache/TLB)
- ❌ STATE_BASE_ADDRESS complet
- ❌ Configuration L3 cache
- ❌ Thread dispatcher state
- ❌ Context image / save areas
- ❌ Scratch space (spill registers)
- ❌ URB configuration
- ❌ Scoreboard pointers

**Conséquence**: GPU hang systématique au preemption timeout (scheduler attend états qui n'existent pas).

---

## 2. TRAVAIL SESSION C277

### 2.1 Programme OpenCL Référence Créé

**Fichier**: [`opencl_reference.c`](opencl_reference.c) (96 lignes)

**Objectif**: Programme OpenCL minimal pour capturer batch GPU réel

**Caractéristiques**:
- Kernel NOP ultra-simple (1 work-item)
- Exécution OpenCL standard via runtime Intel
- Permet capture trace DRM avec strace
- Base pour comparaison avec batch manuel

**Compilation et test**:
```bash
gcc -o opencl_reference opencl_reference.c -lOpenCL
./opencl_reference
# Output: "OpenCL kernel executed successfully!"
```

### 2.2 Outil Capture DRM Créé

**Fichier**: [`drm_batch_capture.c`](drm_batch_capture.c) (217 lignes)

**Fonctionnalités**:
1. **Capture trace strace**: Enregistre tous les ioctl DRM
2. **Analyse commandes Gen9**: Décode batch buffer
3. **Identification états GPU**: Parse structures (PIPE_CONTROL, STATE_BASE_ADDRESS, etc.)

**Commandes supportées**:
- `STATE_BASE_ADDRESS` (0x78000000)
- `STATE_SIP` (0x78010000)
- `3DSTATE_URB_*` (0x78050000)
- `MEDIA_VFE_STATE` (0x78100000)
- `MEDIA_CURBE_LOAD` (0x78110000)
- `MEDIA_INTERFACE_DESCRIPTOR_LOAD` (0x78120000)
- `GPGPU_WALKER` (0x71000000)
- `PIPE_CONTROL` (0x7A000000)
- `MI_BATCH_BUFFER_END` (0x05000000)

**Exécution**:
```bash
gcc -o drm_batch_capture drm_batch_capture.c
./drm_batch_capture ./opencl_reference
# Trace capturée dans /tmp/drm_trace.txt
```

### 2.3 Trace DRM Capturée

**Résultats capture**:
- **Fichier**: `/tmp/drm_trace.txt` (95 lignes)
- **EXECBUFFER2 détectés**: 1 (kernel OpenCL exécuté)
- **Séquence ioctl**:
  ```
  DRM_IOCTL_I915_GEM_EXECBUFFER2 (1×)
  DRM_IOCTL_I915_GEM_WAIT (multiple)
  DRM_IOCTL_GEM_CLOSE (cleanup)
  DRM_IOCTL_I915_GEM_CONTEXT_DESTROY (2×)
  ```

**Limitation identifiée**: strace ne capture pas le contenu des structures de données (batch buffer lui-même).

### 2.4 Recherche Outils Intel

**Outils testés**:
- ❌ `intel_aubdump`: Non disponible dans package `intel-gpu-tools`
- ✅ `intel-gpu-tools` installé: Version 1.28-1ubuntu2
- ✅ Outils disponibles: `intel_dump_decode`, `intel_gpu_overlay`, `gputop`, etc.

**Packages système**:
```
intel-gpu-tools (1.28-1ubuntu2)
intel-opencl-icd (23.43.27642.40-1ubuntu3)
libigc1 (1.0.15468.25-2ubuntu0.1)
libigdfcl1 (1.0.15468.25-2ubuntu0.1)
```

### 2.5 Recherche Code Source Runtime

**Tentative**: Analyse code source `compute-runtime_test` pour comprendre construction batch

**Fichiers identifiés**:
- Tests unitaires: `hw_helper_tests.cpp`, `test_preamble_xehp_and_later.cpp`
- Command stream: `command_stream_receiver_hw.h`, `preemption.h`
- Structures: Nombreux headers dans `shared/source/command_stream/`

**Limitation**: Code source très complexe, nécessite analyse approfondie.

---

## 3. STRATÉGIE REVERSE ENGINEERING

### 3.1 Approche Actuelle (Strace)

**Avantages**:
- ✅ Simple à utiliser
- ✅ Capture séquence ioctl complète
- ✅ Identifie EXECBUFFER2

**Limitations**:
- ❌ Ne capture pas contenu batch buffer
- ❌ Structures tronquées (strace -s limité)
- ❌ Pas d'accès mémoire GPU

### 3.2 Approches Alternatives

#### Option A: LD_PRELOAD Hook
**Principe**: Intercepter `ioctl()` avec bibliothèque custom

**Avantages**:
- ✅ Accès complet structures
- ✅ Dump batch buffer en mémoire
- ✅ Pas besoin root

**Implémentation**:
```c
// libdrm_hook.so
int ioctl(int fd, unsigned long request, void *argp) {
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *exec = argp;
        // Dump batch buffer via exec->buffers_ptr
        dump_batch_buffer(exec);
    }
    return real_ioctl(fd, request, argp);
}
```

#### Option B: GDB Memory Dump
**Principe**: Attacher gdb au processus OpenCL et dumper batch

**Commandes**:
```bash
gdb --pid $(pgrep opencl_reference)
(gdb) info proc mappings  # Trouver adresse batch
(gdb) dump binary memory /tmp/batch.bin 0x... 0x...
```

**Avantages**:
- ✅ Accès direct mémoire
- ✅ Pas besoin modifier code

**Limitations**:
- ❌ Timing critique (processus court)
- ❌ Nécessite identifier adresse batch

#### Option C: Analyse Code Source Runtime
**Principe**: Lire code `compute-runtime` pour comprendre construction batch

**Fichiers clés**:
- `command_stream_receiver_hw.cpp`: Soumission batch
- `preemption.cpp`: États preemption
- `hw_helper_*.cpp`: Helpers spécifiques Gen9

**Avantages**:
- ✅ Compréhension complète
- ✅ Documentation implicite

**Limitations**:
- ❌ Code très complexe (~500K lignes)
- ❌ Temps analyse important

#### Option D: Intel GPU Tools
**Principe**: Utiliser outils Intel existants

**Outils potentiels**:
- `intel_dump_decode`: Décode dumps GPU
- `intel_gpu_overlay`: Monitoring temps réel
- `i915-perf-recorder`: Enregistrement traces

**Statut**: À explorer davantage

---

## 4. PLAN ACTION IMMÉDIAT

### Phase 1: Capture Batch Buffer (EN COURS)

**Objectif**: Obtenir batch buffer OpenCL complet en hexadécimal

**Méthode recommandée**: LD_PRELOAD Hook (Option A)

**Étapes**:
1. ✅ Créer `opencl_reference.c` (FAIT)
2. ⏳ Créer `libdrm_hook.so` avec interception ioctl
3. ⏳ Exécuter `LD_PRELOAD=./libdrm_hook.so ./opencl_reference`
4. ⏳ Capturer batch buffer complet
5. ⏳ Sauvegarder dans `/tmp/opencl_batch.bin`

### Phase 2: Analyse Batch OpenCL

**Objectif**: Identifier TOUS les états GPU présents

**Outils**:
- `drm_batch_capture` (déjà créé)
- `hexdump -C /tmp/opencl_batch.bin`
- Analyse manuelle avec specs Gen9

**Livrables**:
- Liste complète commandes GPU
- Valeurs exactes registres
- Séquence d'initialisation

### Phase 3: Comparaison Batch Manuel vs OpenCL

**Objectif**: Identifier états manquants

**Méthode**:
1. Lister états batch OpenCL
2. Lister états batch manuel (actuel)
3. Diff et identifier manquants
4. Prioriser par criticité

**Livrables**:
- Tableau comparatif
- Liste états manquants critiques
- Plan implémentation

### Phase 4: Implémentation États Manquants

**Objectif**: Compléter batch buffer manuel

**Étapes**:
1. Implémenter PIPE_CONTROL complet
2. Implémenter STATE_BASE_ADDRESS complet
3. Implémenter configuration L3
4. Implémenter scratch space
5. Implémenter URB config
6. Tester après chaque ajout

**Critère succès**: 100+ dispatches sans GPU hang

---

## 5. ÉTATS GPU CRITIQUES ATTENDUS

### 5.1 PIPE_CONTROL (Synchronisation)

**Opcode**: `0x7A000000`

**Flags critiques**:
- CS Stall (bit 20)
- TLB Invalidate (bit 18)
- Instruction Cache Invalidate (bit 17)
- Texture Cache Invalidate (bit 16)
- Post-Sync Write (bit 14)

**Usage**: Avant/après chaque dispatch pour flush caches

### 5.2 STATE_BASE_ADDRESS (Pointeurs Base)

**Opcode**: `0x78000000`

**Champs critiques**:
- General State Base Address
- Surface State Base Address
- Dynamic State Base Address
- Indirect Object Base Address
- Instruction Base Address

**Usage**: Définir espaces mémoire GPU

### 5.3 MEDIA_VFE_STATE (Configuration Threads)

**Opcode**: `0x78100000`

**Champs critiques**:
- Scratch Space Base Pointer
- Maximum Number of Threads
- URB Entry Allocation Size
- CURBE Allocation Size
- Scoreboard Mask

**Usage**: Configuration compute threads

### 5.4 Configuration L3 Cache

**Méthode**: Via registres MMIO ou commandes MI_LOAD_REGISTER

**Paramètres**:
- L3 allocation (SLM vs URB vs DC vs RO)
- Cache policies
- Coherency settings

**Usage**: Optimisation performance mémoire

### 5.5 Scratch Space (Spill Registers)

**Allocation**: Via GEM buffer dédié

**Configuration**: Dans MEDIA_VFE_STATE

**Taille**: Dépend complexité kernel (SHA-256 = ~4KB par thread)

**Usage**: Stockage temporaire registres débordés

---

## 6. MÉTRIQUES SUCCÈS

### Critères Validation

**Niveau 1 - Basique**:
- ✅ Batch buffer compile sans erreur
- ✅ EXECBUFFER2 retourne 0
- ✅ gem_wait() retourne 0

**Niveau 2 - Fonctionnel**:
- ✅ Aucun GPU hang dans dmesg
- ✅ 10+ dispatches consécutifs OK
- ✅ Résultats calcul corrects

**Niveau 3 - Production**:
- ✅ 100+ dispatches sans crash
- ✅ Performance maintenue (>300 MH/s)
- ✅ Stabilité long terme (1h+)

### Métriques Performance

**Hash Rate**:
- Baseline CPU: 1.07 MH/s
- Actuel GPU: 398 MH/s (avant crash)
- Objectif: 400+ MH/s stable

**Latence Dispatch**:
- Actuel: 0.7-1.5 sec par dispatch
- Objectif: <1 sec stable

**Utilisation GPU**:
- Actuel: 24 EUs × 100% = 192 threads
- Objectif: Maintenir sans GPU hang

---

## 7. RISQUES ET LIMITATIONS

### Risques Techniques

**R1 - Complexité Batch Buffer**:
- **Probabilité**: Haute
- **Impact**: Moyen
- **Mitigation**: Approche incrémentale, test après chaque ajout

**R2 - États GPU Undocumented**:
- **Probabilité**: Moyenne
- **Impact**: Élevé
- **Mitigation**: Analyse code source runtime, forums Intel

**R3 - Limitation Hardware Gen9**:
- **Probabilité**: Faible
- **Impact**: Critique
- **Mitigation**: Validation sur autre GPU si nécessaire

### Limitations Connues

**L1 - Pas d'outil Intel aubdump**:
- Impact: Ralentit reverse engineering
- Workaround: LD_PRELOAD hook custom

**L2 - Documentation Gen9 incomplète**:
- Impact: Nécessite expérimentation
- Workaround: Analyse code source runtime

**L3 - Temps développement**:
- Impact: Solution non immédiate
- Workaround: Approche méthodique, tests incrémentaux

---

## 8. CONCLUSION SESSION C277

### Progrès Réalisés

✅ **Programme OpenCL référence créé** (96 lignes)  
✅ **Outil capture DRM créé** (217 lignes)  
✅ **Trace DRM capturée** (1 EXECBUFFER2 identifié)  
✅ **Stratégie reverse engineering définie**  
✅ **Plan action détaillé établi**

### Prochaines Étapes Critiques

1. **Créer LD_PRELOAD hook** pour capturer batch buffer complet
2. **Analyser batch OpenCL** avec outil decode
3. **Comparer avec batch manuel** et identifier manquants
4. **Implémenter états critiques** (PIPE_CONTROL, STATE_BASE_ADDRESS, etc.)
5. **Tester incrémentalement** après chaque ajout

### Estimation Temps

**Phase 1 (Capture)**: 1-2 heures  
**Phase 2 (Analyse)**: 2-3 heures  
**Phase 3 (Comparaison)**: 1 heure  
**Phase 4 (Implémentation)**: 4-6 heures  
**Total estimé**: 8-12 heures de développement

### Probabilité Succès

**Optimiste**: 80% (si batch OpenCL révèle tous états nécessaires)  
**Réaliste**: 60% (si quelques états undocumented)  
**Pessimiste**: 40% (si limitation hardware fondamentale)

---

## 9. RÉFÉRENCES

### Documentation Intel

- **PRM Gen9**: Programmer's Reference Manual Volume 2a/2b/2c (Command Reference)
- **Bspec**: Internal Intel specifications (accès limité)
- **compute-runtime**: Code source OpenCL/Level Zero Intel

### Fichiers Projet

- [`opencl_reference.c`](opencl_reference.c): Programme test OpenCL
- [`drm_batch_capture.c`](drm_batch_capture.c): Outil analyse batch
- [`/tmp/drm_trace.txt`]: Trace strace EXECBUFFER2
- [`RAPPORT_C276_PARTIE_*.md`](RAPPORT_FORENSIQUE_ULTRA_COMPLET_C276_PARTIE_1.md): Rapports forensiques session précédente

### Commandes Utiles

```bash
# Capture trace DRM
strace -e trace=ioctl -v -s 999999 ./opencl_reference 2>&1 | tee trace.txt

# Analyse batch buffer
hexdump -C /tmp/batch.bin | head -100

# Monitoring GPU
intel_gpu_top

# Logs kernel
sudo dmesg -w | grep i915
```

---

**FIN RAPPORT C277**

**Statut**: Stratégie définie, outils créés, prêt pour Phase 1 (Capture batch OpenCL)

**Prochaine action**: Créer `libdrm_hook.so` avec LD_PRELOAD pour capturer batch buffer complet