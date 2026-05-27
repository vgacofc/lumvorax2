# RAPPORT C378 - DIAGNOSTIC FINAL : GPU ÉCRIT DES ZÉROS MALGRÉ KERNEL CORRECT

**Date**: 2026-05-22  
**Système**: Intel UHD Graphics 620 (Gen9) - 100% i915 DRM Native  
**Contexte**: Test C377 avec kernel correct `test_c374_const.bin_kbl.bin`  
**Résultat**: ❌ `output[0] = 0x00000000` au lieu de `0xC374ABCD`  

---

## 📋 RÉSUMÉ EXÉCUTIF

### Travail Accompli
1. ✅ Rapport C288 lu et analysé
2. ✅ 447 lignes de logs forensiques analysées (C375)
3. ✅ Code C375 validé : extraction `.text` ELF fonctionnelle
4. ✅ Warnings compilation corrigés (C377)
5. ✅ Kernel correct copié : `tests/test_c374_const.bin_kbl.bin` → `kernels/test_add_gen9.bin`
6. ✅ Test C377 exécuté avec kernel correct
7. ❌ **Problème persiste** : GPU écrit toujours des zéros

### ROOT CAUSE ACTUEL
Le GPU **exécute** le kernel (memory_changed=64) mais **n'exécute PAS le code ISA correct**.

Hypothèses possibles :
1. **Instruction Base Address incorrecte** : GPU saute au mauvais offset
2. **Kernel non exécuté** : GPU exécute NOP ou code vide
3. **Bug i915 DRM** : Relocations kernel mal résolues
4. **Format ISA incompatible** : Code compilé pour mauvaise architecture

---

## 🔬 ANALYSE TECHNIQUE COMPLÈTE

### 1. Validation Kernel (✅ CORRECT)

#### Kernel Source
```c
__kernel void test_const(__global unsigned int* output) {
    output[0] = 0xC374ABCD;
}
```

#### Binaire ELF
```bash
$ ls -lh kernels/test_add_gen9.bin tests/test_c374_const.bin_kbl.bin
-rw-rw-r-- 1 lvx lvx 2.7K May 22 18:04 kernels/test_add_gen9.bin
-rw-rw-r-- 1 lvx lvx 2.7K May 22 17:40 tests/test_c374_const.bin_kbl.bin

$ md5sum kernels/test_add_gen9.bin tests/test_c374_const.bin_kbl.bin
5a1892d22c7f830a249bc5de04280629  kernels/test_add_gen9.bin
5a1892d22c7f830a249bc5de04280629  tests/test_c374_const.bin_kbl.bin
```
✅ **Fichiers identiques**

#### Section `.text` Extraite
```bash
$ hexdump -C /tmp/text_section.bin | grep "00000030"
00000030  01 00 00 00 2c 0e c0 20  00 00 00 00 cd ab 74 c3
                                                ^^^^^^^^^^
                                                0xC374ABCD !
```
✅ **Constante présente à offset 0x3C**

#### Code ISA Chargé
```
[21429.248214486] C375_ISA_DUMP: [0]=0x00600001 [1]=0x2040020c [2]=0x00200000 [3]=0x00000000
```
✅ **Code ISA chargé en mémoire GPU**

### 2. Configuration GPU (✅ CORRECT)

#### Surface States
```
Surface[0]: dw0=0x87fc0000 dw1=0x00001000 dw2=0x000fffff dw3=0x00000000  (Input)
Surface[1]: dw0=0x87fc0000 dw1=0x02627000 dw2=0x00000027 dw3=0x00000000  (Output)
```
✅ **Format Gen9 correct, DW1 patchés par i915, DW2 intacts**

#### Binding Table
```
BTI=0 → Surface 1 (Output) at offset 64
BTI=1 → Surface 0 (Input) at offset 0
```
✅ **Inversion correcte pour ocloc**

#### Interface Descriptor
```
dw0=0x00000000 (kernel_offset=0x00000000)
dw4=0x00000080 (binding_table=128)
dw6=0x00000100 (threads=256)
```
✅ **IDD correct, kernel_offset=0 relatif à Instruction Base Address**

#### Relocations
```
batch[ssh@48 dsh@56 ioh@64 kernel@72]
```
✅ **4 relocations STATE_BASE_ADDRESS configurées**

### 3. Exécution GPU (✅ FONCTIONNE)

#### EXECBUFFER2
```
✅ Exécution réussie
Dispatches: 1
Total time: 0.002 sec
Avg dispatch: 1.576 ms
```
✅ **GPU exécute sans erreur**

#### Modification Mémoire
```
C361_BUFFER_DUMP_END: 64/64 values changed
```
✅ **GPU modifie bien la mémoire (pas un problème de cache)**

#### Résultat
```
[00] offset=0x0000 value=0x00000000 ← CHANGED
```
❌ **GPU écrit 0x00000000 au lieu de 0xC374ABCD**

---

## 🔍 HYPOTHÈSES & TESTS REQUIS

### Hypothèse #1 : Instruction Base Address Incorrecte

**Problème** : GPU saute au mauvais offset dans le kernel

**Test requis** :
```c
// Ajouter dans btc_gen9_execute() après EXECBUFFER2
LOG_EVENT(ctx, "C378_INSTRUCTION_BASE_GTT: 0x%016llx", 
         (unsigned long long)exec_objects[0].offset);
```

**Validation** : Vérifier que `exec_objects[0].offset` pointe vers le début du kernel

### Hypothèse #2 : Kernel Non Exécuté

**Problème** : GPU exécute des NOPs ou code vide

**Test requis** :
```c
// Modifier kernel pour écrire pattern reconnaissable
__kernel void test_pattern(__global unsigned int* output) {
    output[0] = 0xDEADBEEF;  // Pattern différent
    output[1] = 0xCAFEBABE;
    output[2] = 0x12345678;
}
```

**Validation** : Si pattern apparaît, kernel s'exécute

### Hypothèse #3 : Bug i915 Relocation Kernel

**Problème** : i915 ne résout pas correctement l'adresse du kernel

**Test requis** :
```bash
# Activer debug i915
echo 0x1f > /sys/module/drm/parameters/debug
dmesg | grep i915 | grep reloc
```

**Validation** : Vérifier messages d'erreur i915

### Hypothèse #4 : Format ISA Incompatible

**Problème** : Code compilé pour mauvaise architecture (SKL au lieu de KBL)

**Test requis** :
```bash
# Recompiler pour architecture exacte
ocloc compile -file test_const.cl -device kbl -output test_const_kbl_exact.bin
```

**Validation** : Comparer binaires

---

## 📊 COMPARAISON AVEC SYSTÈMES FONCTIONNELS

### Beignet (OpenCL)
- ✅ Utilise même pipeline i915 DRM
- ✅ Fonctionne sur Gen9
- ❓ Différences possibles :
  - Format kernel (SPIR-V vs ISA binaire)
  - Configuration IDD
  - Gestion relocations

### Intel Compute Runtime
- ✅ Référence officielle Intel
- ✅ Support Gen9 complet
- ❓ Différences possibles :
  - Commandes batch buffer supplémentaires
  - Configuration L3 cache
  - Synchronisation threads

---

## 🎯 RECOMMANDATIONS IMMÉDIATES

### Test #1 : Kernel Pattern Simple
```c
__kernel void test_pattern(__global unsigned int* output) {
    // Écrire pattern reconnaissable
    output[0] = 0xDEADBEEF;
    output[1] = 0xCAFEBABE;
    output[2] = 0x12345678;
    output[3] = 0xABCDEF01;
}
```

**Objectif** : Vérifier si GPU exécute vraiment le kernel

### Test #2 : Dump Instruction Base Address
```c
// Dans btc_gen9_execute() après EXECBUFFER2
uint64_t kernel_gtt = exec_objects[0].offset;
LOG_EVENT(ctx, "C378_KERNEL_GTT: 0x%016llx", (unsigned long long)kernel_gtt);

// Vérifier cohérence avec STATE_BASE_ADDRESS
LOG_EVENT(ctx, "C378_EXPECTED_KERNEL_ADDR: handle=%u presumed=0x%016llx actual=0x%016llx",
         ctx->kernel_bo, (unsigned long long)0, (unsigned long long)kernel_gtt);
```

**Objectif** : Confirmer que GPU sait où est le kernel

### Test #3 : Comparer avec Beignet
```bash
# Installer Beignet
sudo apt install beignet-opencl-icd

# Tester même kernel avec Beignet
clinfo | grep "Device Name"
# Compiler et exécuter test_const.cl avec Beignet
```

**Objectif** : Valider que le hardware fonctionne

### Test #4 : Debug i915
```bash
# Activer debug complet i915
echo 0x1f > /sys/module/drm/parameters/debug

# Relancer test
./bin/test_gen9_native test_c378

# Analyser logs kernel
dmesg | tail -100 | grep -E "i915|drm|gem"
```

**Objectif** : Identifier erreurs i915 cachées

---

## 📈 ÉTAT D'AVANCEMENT

### Complété
- [x] Lecture rapport C288
- [x] Analyse forensique 447 lignes (C375)
- [x] Validation code C375 (extraction .text ELF)
- [x] Correction warnings compilation (C377)
- [x] Copie kernel correct
- [x] Test C377 exécuté
- [x] Validation kernel binaire (constante présente)
- [x] Validation configuration GPU (Surface States, IDD, Relocations)

### En Cours
- [ ] Diagnostic ROOT CAUSE final (GPU écrit zéros)
- [ ] Tests hypothèses (Instruction Base Address, Pattern, i915 debug)

### Bloqué
- [ ] Vérifier `output[0] = 0xC374ABCD` ❌
- [ ] Test kernel Bitcoin SHA-256 complet
- [ ] Mesurer hashrate réel
- [ ] Valider `near_miss_gpu > 0 bits`

---

## 🔧 OUTILS DE DEBUG REQUIS

### 1. Intel GPU Tools
```bash
sudo apt install intel-gpu-tools
intel_gpu_top  # Monitoring GPU en temps réel
intel_gpu_dump # Dump état GPU
```

### 2. Mesa Debug
```bash
export INTEL_DEBUG=bat,vs,wm,cs
export MESA_DEBUG=1
```

### 3. GDB avec GPU
```bash
gdb --args ./bin/test_gen9_native test_c378
(gdb) break btc_gen9_execute
(gdb) run
```

### 4. Strace i915
```bash
strace -e ioctl ./bin/test_gen9_native test_c378 2>&1 | grep DRM
```

---

## 💡 PISTES ALTERNATIVES

### Option A : Utiliser Beignet comme Référence
- Compiler kernel avec Beignet
- Extraire binaire ISA généré
- Comparer avec notre binaire
- Identifier différences

### Option B : Utiliser Intel Compute Runtime
- Étudier code source compute-runtime
- Comparer construction batch buffer
- Identifier commandes manquantes

### Option C : Simplifier au Maximum
- Kernel encore plus simple (NOP + EOT)
- Vérifier si GPU exécute au moins ça
- Ajouter instructions progressivement

### Option D : Contacter Intel
- Ouvrir issue sur compute-runtime GitHub
- Demander aide debug Gen9 native
- Partager logs forensiques

---

## 📝 CONCLUSION

### Situation Actuelle
Le système est **techniquement correct** :
- ✅ Code C375 fonctionne (extraction .text ELF)
- ✅ Kernel correct chargé (constante 0xC374ABCD présente)
- ✅ Configuration GPU correcte (Surface States, IDD, Relocations)
- ✅ GPU exécute (memory_changed=64)
- ❌ **Mais GPU écrit des zéros au lieu de la constante**

### Problème Fondamental
Le GPU **n'exécute PAS le code ISA du kernel**, soit parce que :
1. Il saute au mauvais offset (Instruction Base Address)
2. Il exécute du code vide/NOP
3. i915 ne résout pas correctement les relocations
4. Le format ISA est incompatible

### Prochaine Étape Critique
**TEST ABSOLU #5** : Dump Instruction Base Address après EXECBUFFER2
```c
LOG_EVENT(ctx, "C378_KERNEL_GTT_FINAL: 0x%016llx", 
         (unsigned long long)exec_objects[0].offset);
```

Si `exec_objects[0].offset == 0`, alors **ROOT CAUSE = Relocation kernel non résolue par i915**.

---

**FIN DU RAPPORT C378**

**Auteur** : Bob (LLM Assistant)  
**Date** : 2026-05-22  
**Version** : 1.0  
**Statut** : 🔴 BLOQUÉ - Nécessite debug GPU avancé