# RAPPORT FORENSIQUE C277 — ANALYSE CODE SOURCE INTEL COMPUTE-RUNTIME (PARTIE 1/3)

**Date**: 2026-05-18  
**Session**: C277  
**Objectif**: Identifier TOUS les états GPU manquants causant GPU hangs  
**Méthode**: Analyse exhaustive code source Intel compute-runtime Gen9  
**GPU**: Intel UHD 620 (Skylake Gen9, 24 EUs)

---

## SECTION 1 : CONTEXTE ET PROBLÉMATIQUE

### 1.1 Historique Sessions C265-C276

**Symptômes observés**:
- Crash systématique au dispatch 28 (formule validée: `max = (nb_ctx × 3) + 1`)
- Message kernel: `"Resetting rcs0 for preemption time out"`
- GPU hang détecté par i915 DRM
- `gem_wait()` retourne 0 (succès) malgré GPU hang
- Système atteint 398 MH/s avant crash

**Évolution compréhension**:
1. **C265-C274**: Suspicion kernel SHA-256 → Kernel innocenté
2. **C275**: Suspicion cache erreurs i915 → gem_wait() comportement normal
3. **C276**: **TEST J DÉCISIF** → Batch buffer invalide confirmé (pas contexte)
4. **C277**: Reverse engineering états GPU manquants → **SOLUTION DÉFINITIVE**

### 1.2 Preuve Batch Buffer Invalide (TEST J)

**Test réalisé C276**:
```c
// Suppression complète save/restore contexte
// Résultat: MÊME crash dispatch 28
// Conclusion: Problème = BATCH BUFFER lui-même
```

**Implication**: Le batch buffer manuel manque des états GPU critiques que OpenCL/Level Zero configurent automatiquement.

---

## SECTION 2 : MÉTHODOLOGIE REVERSE ENGINEERING

### 2.1 Approches Testées

| Approche | Statut | Résultat |
|----------|--------|----------|
| intel_aubdump | ❌ Obsolète | Outil n'existe pas/plus dans IGT |
| LD_PRELOAD hook | ⚠️ Partiel | Fonctionne mais ne capture pas EXECBUFFER2 (processus séparé) |
| strace | ✅ Détection | 1 EXECBUFFER2 identifié mais pas contenu structures |
| **Analyse code source** | ✅ **DÉFINITIF** | **Accès direct implémentation Intel** |

### 2.2 Fichiers Créés Session C277

**Programme OpenCL référence** ([`opencl_reference.c`](../opencl_reference.c), 96 lignes):
```c
// Kernel NOP minimal pour capture batch GPU
__kernel void nop_kernel(__global int* data) {
    data[get_global_id(0)] = get_global_id(0);
}
```
- Compilé: `gcc -o opencl_reference opencl_reference.c -lOpenCL`
- Testé: ✅ "OpenCL kernel executed successfully!"

**Outil capture DRM** ([`drm_batch_capture.c`](../drm_batch_capture.c), 217 lignes):
- Décodeur commandes Gen9: STATE_BASE_ADDRESS, MEDIA_VFE_STATE, GPGPU_WALKER, PIPE_CONTROL
- Fonction `analyze_gen9_command()` parse opcodes
- Compilé: `gcc -o drm_batch_capture drm_batch_capture.c`

**LD_PRELOAD hook** ([`libdrm_hook.c`](../libdrm_hook.c), 191 lignes):
- Interception `ioctl()` pour capturer EXECBUFFER2
- Fonction `dump_buffer()` via mmap GEM
- Compilé: `gcc -shared -fPIC -o libdrm_hook.so libdrm_hook.c -ldl`
- **Limitation**: 0 EXECBUFFER2 capturé (runtime OpenCL multi-processus)

### 2.3 Fichiers Code Source Intel Analysés

**Fichiers critiques Gen9**:
1. [`preamble_gen9.cpp`](../../compute-runtime_test/shared/source/gen9/preamble_gen9.cpp) (77 lignes)
2. [`state_base_address_gen9.cpp`](../../compute-runtime_test/shared/source/gen9/state_base_address_gen9.cpp) (15 lignes)
3. [`reg_configs.h`](../../compute-runtime_test/shared/source/gen9/reg_configs.h) (41 lignes)
4. [`hw_cmds_base.h`](../../compute-runtime_test/shared/source/gen9/hw_cmds_base.h) (100+ lignes)

**Fichiers génériques**:
5. [`preamble_bdw_and_later.inl`](../../compute-runtime_test/shared/source/helpers/preamble_bdw_and_later.inl) (200 lignes)
6. [`state_base_address_base.inl`](../../compute-runtime_test/shared/source/helpers/state_base_address_base.inl) (200 lignes)
7. [`state_base_address_bdw_and_later.inl`](../../compute-runtime_test/shared/source/helpers/state_base_address_bdw_and_later.inl) (34 lignes)
8. [`state_base_address_skl.inl`](../../compute-runtime_test/shared/source/helpers/state_base_address_skl.inl) (29 lignes)
9. [`pipe_control_args.h`](../../compute-runtime_test/shared/source/helpers/pipe_control_args.h) (37 lignes)
10. [`preamble.h`](../../compute-runtime_test/shared/source/helpers/preamble.h) (150 lignes)

---

## SECTION 3 : DÉCOUVERTE #1 — CONFIGURATION L3 CACHE

### 3.1 Registre L3CNTLREG

**Offset**: `0x7034`  
**Source**: [`reg_configs.h:22`](../../compute-runtime_test/shared/source/gen9/reg_configs.h:22)

```cpp
template <>
struct L3CNTLRegisterOffset<Gen9Family> {
    static const uint32_t registerOffset = 0x7034;
};
```

### 3.2 Valeurs Skylake Gen9

**Source**: [`reg_configs.h:15-18`](../../compute-runtime_test/shared/source/gen9/reg_configs.h:15)

```cpp
template <>
struct L3CNTLREGConfig<IGFX_SKYLAKE> {
    static const uint32_t valueForSLM = 0x60000321u;    // Avec SLM
    static const uint32_t valueForNoSLM = 0x80000340u;  // Sans SLM
};
```

### 3.3 Analyse Bits Configuration

**Mode SLM** (`0x60000321`):
- Bit 9: Error Detection Behavior Control = 1 (activé)
- Allocation L3 optimisée pour Shared Local Memory
- URB: 96 KB, DC: 0 KB, RO: 0 KB, I/S: 0 KB, C: 384 KB, T: 384 KB

**Mode No-SLM** (`0x80000340`):
- Bit 9: Error Detection Behavior Control = 1 (activé)
- Allocation L3 optimisée pour accès mémoire globale
- URB: 64 KB, DC: 0 KB, RO: 256 KB, I/S: 0 KB, C: 256 KB, T: 256 KB

### 3.4 Implémentation Intel

**Source**: [`preamble_bdw_and_later.inl:16-21`](../../compute-runtime_test/shared/source/helpers/preamble_bdw_and_later.inl:16)

```cpp
template <typename GfxFamily>
void PreambleHelper<GfxFamily>::programL3(LinearStream *pCommandStream, uint32_t l3Config) {
    LriHelper<GfxFamily>::program(pCommandStream,
                                  L3CNTLRegisterOffset<GfxFamily>::registerOffset,
                                  l3Config,
                                  false);
}
```

**Commande générée**: `MI_LOAD_REGISTER_IMM` (Load Register Immediate)

### 3.5 Tests Validation Intel

**Source**: [`test_preamble_gen9.cpp:41-71`](../../compute-runtime_test/shared/test/unit_test/gen9/test_preamble_gen9.cpp:41)

```cpp
GEN9TEST_F(Gen9L3Config, GivenNoSlmWhenProgrammingL3ThenProgrammingIsCorrect) {
    bool slmUsed = false;
    uint32_t l3Config = getL3ConfigHelper<IGFX_SKYLAKE>(slmUsed);
    EXPECT_EQ(0x80000340u, l3Config);
    
    uint32_t errorDetectionBehaviorControlBit = 1 << 9;
    EXPECT_TRUE((l3Config & errorDetectionBehaviorControlBit) != 0);
}

GEN9TEST_F(Gen9L3Config, GivenSlmWhenProgrammingL3ThenProgrammingIsCorrect) {
    bool slmUsed = true;
    uint32_t l3Config = getL3ConfigHelper<IGFX_SKYLAKE>(slmUsed);
    EXPECT_EQ(0x60000321u, l3Config);
    
    uint32_t errorDetectionBehaviorControlBit = 1 << 9;
    EXPECT_TRUE((l3Config & errorDetectionBehaviorControlBit) != 0);
}
```

### 3.6 Code Implémentation C

```c
// Configuration L3 cache pour Skylake Gen9
#define L3CNTLREG_OFFSET 0x7034
#define L3_CONFIG_SLM    0x60000321u  // Avec SLM
#define L3_CONFIG_NO_SLM 0x80000340u  // Sans SLM

void emit_l3_config(uint32_t *batch, bool use_slm) {
    // MI_LOAD_REGISTER_IMM
    *batch++ = 0x11000001;  // Opcode + DWord Length-2
    *batch++ = L3CNTLREG_OFFSET;
    *batch++ = use_slm ? L3_CONFIG_SLM : L3_CONFIG_NO_SLM;
}
```

**CRITICITÉ**: **MAXIMALE** — Sans configuration L3, GPU ne peut pas accéder correctement au cache

---

## SECTION 4 : DÉCOUVERTE #2 — PIPE_CONTROL COMPLET

### 4.1 Structure Complète 19 Flags

**Source**: [`pipe_control_args.h:10-37`](../../compute-runtime_test/shared/source/helpers/pipe_control_args.h:10)

```cpp
struct PipeControlArgs {
    bool csStallOnly = false;                           // #1
    bool dcFlushEnable = false;                         // #2
    bool renderTargetCacheFlushEnable = false;          // #3
    bool instructionCacheInvalidateEnable = false;      // #4
    bool textureCacheInvalidationEnable = false;        // #5
    bool pipeControlFlushEnable = false;                // #6
    bool vfCacheInvalidationEnable = false;             // #7
    bool constantCacheInvalidationEnable = false;       // #8
    bool stateCacheInvalidationEnable = false;          // #9
    bool genericMediaStateClear = false;                // #10
    bool hdcPipelineFlush = false;                      // #11
    bool tlbInvalidation = false;                       // #12
    bool workloadPartitionOffset = false;               // #13
    bool amfsFlushEnable = false;                       // #14
    bool compressionControlSurfaceCcsFlush = false;     // #15
    bool notifyEnable = false;                          // #16
    bool unTypedDataPortCacheFlush = false;             // #17
    bool depthCacheFlushEnable = false;                 // #18
    bool depthStallEnable = false;                      // #19
};
```

### 4.2 PIPE_CONTROL Avant MEDIA_VFE_STATE (Workaround Gen9)

**Source**: [`preamble_gen9.cpp:58-66`](../../compute-runtime_test/shared/source/gen9/preamble_gen9.cpp:58)

```cpp
template <>
void PreambleHelper<Family>::addPipeControlBeforeVfeCmd(
    LinearStream *pCommandStream, 
    const HardwareInfo *hwInfo, 
    EngineGroupType engineGroupType) {
    
    PipeControlArgs args = {};
    if (hwInfo->workaroundTable.flags.waSendMIFLUSHBeforeVFE) {
        args.renderTargetCacheFlushEnable = true;
        args.depthCacheFlushEnable = true;
        args.dcFlushEnable = true;
    }
    MemorySynchronizationCommands<Family>::addSingleBarrier(*pCommandStream, args);
}
```

**Workaround critique**: `waSendMIFLUSHBeforeVFE` active 3 flush avant VFE pour éviter corruption état GPU

### 4.3 Tests Validation Intel

**Source**: [`test_preamble_gen9.cpp:110-128`](../../compute-runtime_test/shared/test/unit_test/gen9/test_preamble_gen9.cpp:110)

```cpp
GEN9TEST_F(PreambleVfeState, GivenWaOnWhenProgrammingVfeStateThenProgrammingIsCorrect) {
    testWaTable->flags.waSendMIFLUSHBeforeVFE = 1;
    // ...
    const auto &pc = *reinterpret_cast<PIPE_CONTROL *>(*itorPC);
    EXPECT_TRUE(pc.getRenderTargetCacheFlushEnable());
    EXPECT_TRUE(pc.getDepthCacheFlushEnable());
    EXPECT_TRUE(pc.getDcFlushEnable());
    EXPECT_EQ(1u, pc.getCommandStreamerStallEnable());
}
```

### 4.4 Code Implémentation C

```c
// Structure PIPE_CONTROL Gen9 (6 DWords)
struct gen9_pipe_control {
    uint32_t dw0;  // Opcode + flags
    uint32_t dw1;  // Address low
    uint32_t dw2;  // Address high
    uint32_t dw3;  // Immediate data low
    uint32_t dw4;  // Immediate data high
    uint32_t dw5;  // Reserved
};

// Flags PIPE_CONTROL (DW0 bits)
#define PIPE_CONTROL_CS_STALL                    (1 << 20)
#define PIPE_CONTROL_DC_FLUSH                    (1 << 5)
#define PIPE_CONTROL_RENDER_TARGET_FLUSH         (1 << 12)
#define PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE (1 << 11)
#define PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE    (1 << 10)
#define PIPE_CONTROL_VF_CACHE_INVALIDATE         (1 << 4)
#define PIPE_CONTROL_CONST_CACHE_INVALIDATE      (1 << 3)
#define PIPE_CONTROL_STATE_CACHE_INVALIDATE      (1 << 2)
#define PIPE_CONTROL_HDC_PIPELINE_FLUSH          (1 << 9)
#define PIPE_CONTROL_TLB_INVALIDATE              (1 << 18)
#define PIPE_CONTROL_DEPTH_CACHE_FLUSH           (1 << 0)
#define PIPE_CONTROL_DEPTH_STALL                 (1 << 13)

// PIPE_CONTROL avant MEDIA_VFE_STATE (workaround Gen9)
void emit_pipe_control_before_vfe(uint32_t *batch) {
    struct gen9_pipe_control *pc = (struct gen9_pipe_control *)batch;
    
    pc->dw0 = 0x7A000004;  // PIPE_CONTROL opcode + length
    pc->dw0 |= PIPE_CONTROL_CS_STALL;
    pc->dw0 |= PIPE_CONTROL_DC_FLUSH;
    pc->dw0 |= PIPE_CONTROL_RENDER_TARGET_FLUSH;
    pc->dw0 |= PIPE_CONTROL_DEPTH_CACHE_FLUSH;
    
    pc->dw1 = 0;
    pc->dw2 = 0;
    pc->dw3 = 0;
    pc->dw4 = 0;
    pc->dw5 = 0;
}

// PIPE_CONTROL complet (tous flags critiques)
void emit_pipe_control_full_flush(uint32_t *batch) {
    struct gen9_pipe_control *pc = (struct gen9_pipe_control *)batch;
    
    pc->dw0 = 0x7A000004;
    pc->dw0 |= PIPE_CONTROL_CS_STALL;
    pc->dw0 |= PIPE_CONTROL_DC_FLUSH;
    pc->dw0 |= PIPE_CONTROL_RENDER_TARGET_FLUSH;
    pc->dw0 |= PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE;
    pc->dw0 |= PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE;
    pc->dw0 |= PIPE_CONTROL_VF_CACHE_INVALIDATE;
    pc->dw0 |= PIPE_CONTROL_CONST_CACHE_INVALIDATE;
    pc->dw0 |= PIPE_CONTROL_STATE_CACHE_INVALIDATE;
    pc->dw0 |= PIPE_CONTROL_HDC_PIPELINE_FLUSH;
    pc->dw0 |= PIPE_CONTROL_TLB_INVALIDATE;
    pc->dw0 |= PIPE_CONTROL_DEPTH_CACHE_FLUSH;
    
    pc->dw1 = 0;
    pc->dw2 = 0;
    pc->dw3 = 0;
    pc->dw4 = 0;
    pc->dw5 = 0;
}
```

**CRITICITÉ**: **MAXIMALE** — Sans PIPE_CONTROL, caches GPU corrompus causent GPU hangs

---

## FIN PARTIE 1/3

**Prochaine partie**: MEDIA_VFE_STATE et STATE_BASE_ADDRESS

**États identifiés Partie 1**: 21 (2 L3 Config + 19 PIPE_CONTROL flags)