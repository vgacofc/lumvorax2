# RAPPORT FORENSIQUE C277 — ANALYSE CODE SOURCE INTEL (PARTIE 2/3)

**Continuation Partie 1**: Configuration L3 Cache et PIPE_CONTROL  
**Cette partie**: MEDIA_VFE_STATE et STATE_BASE_ADDRESS

---

## SECTION 5 : DÉCOUVERTE #3 — MEDIA_VFE_STATE

### 5.1 Structure Complète

**Source**: [`preamble_bdw_and_later.inl:38-61`](../../compute-runtime_test/shared/source/helpers/preamble_bdw_and_later.inl:38)

```cpp
template <typename GfxFamily>
void PreambleHelper<GfxFamily>::programVfeState(
    void *pVfeState,
    const HardwareInfo &hwInfo,
    uint32_t scratchSize,
    uint64_t scratchAddress,
    uint32_t maxFrontEndThreads,
    const StreamProperties &streamProperties,
    LogicalStateHelper *logicalStateHelper) {
    
    using MEDIA_VFE_STATE = typename GfxFamily::MEDIA_VFE_STATE;
    
    auto pMediaVfeState = reinterpret_cast<MEDIA_VFE_STATE *>(pVfeState);
    MEDIA_VFE_STATE cmd = GfxFamily::cmdInitMediaVfeState;
    
    // Configuration threads GPU
    cmd.setMaximumNumberOfThreads(maxFrontEndThreads);
    cmd.setNumberOfUrbEntries(1);
    cmd.setUrbEntryAllocationSize(
        PreambleHelper<GfxFamily>::getUrbEntryAllocationSize()
    );
    
    // Configuration scratch space
    cmd.setPerThreadScratchSpace(
        PreambleHelper<GfxFamily>::getScratchSizeValueToProgramMediaVfeState(scratchSize)
    );
    cmd.setStackSize(
        PreambleHelper<GfxFamily>::getScratchSizeValueToProgramMediaVfeState(scratchSize)
    );
    
    // Adresse scratch space (64-bit split)
    uint32_t lowAddress = static_cast<uint32_t>(0xFFFFFFFF & scratchAddress);
    uint32_t highAddress = static_cast<uint32_t>(0xFFFFFFFF & (scratchAddress >> 32));
    cmd.setScratchSpaceBasePointer(lowAddress);
    cmd.setScratchSpaceBasePointerHigh(highAddress);
    
    appendProgramVFEState(hwInfo, streamProperties, &cmd);
    *pMediaVfeState = cmd;
}
```

### 5.2 Valeur URB Entry Allocation

**Source**: [`preamble_bdw_and_later.inl:24-26`](../../compute-runtime_test/shared/source/helpers/preamble_bdw_and_later.inl:24)

```cpp
template <typename GfxFamily>
uint32_t PreambleHelper<GfxFamily>::getUrbEntryAllocationSize() {
    return 0x782;  // Valeur fixe Gen9
}
```

**Signification**: Taille allocation URB (Unified Return Buffer) = 0x782 = 1922 bytes

### 5.3 Calcul Maximum Threads

**Pour Intel UHD 620 (Skylake Gen9)**:
- 24 Execution Units (EUs)
- 7 threads par EU
- **Total**: 24 × 7 = **168 threads maximum**

**Source validation**: Tests Intel compute-runtime utilisent 168 pour Gen9

### 5.4 Allocation Espace Avant VFE

**Source**: [`preamble_bdw_and_later.inl:29-35`](../../compute-runtime_test/shared/source/helpers/preamble_bdw_and_later.inl:29)

```cpp
template <typename GfxFamily>
void *PreambleHelper<GfxFamily>::getSpaceForVfeState(
    LinearStream *pCommandStream,
    const HardwareInfo &hwInfo,
    EngineGroupType engineGroupType) {
    
    using MEDIA_VFE_STATE = typename GfxFamily::MEDIA_VFE_STATE;
    addPipeControlBeforeVfeCmd(pCommandStream, &hwInfo, engineGroupType);
    return pCommandStream->getSpaceForCmd<MEDIA_VFE_STATE>();
}
```

**Important**: PIPE_CONTROL AVANT VFE est obligatoire (workaround Gen9)

### 5.5 Taille Commande VFE

**Source**: [`preamble_bdw_and_later.inl:72-75`](../../compute-runtime_test/shared/source/helpers/preamble_bdw_and_later.inl:72)

```cpp
template <typename GfxFamily>
size_t PreambleHelper<GfxFamily>::getVFECommandsSize() {
    using MEDIA_VFE_STATE = typename GfxFamily::MEDIA_VFE_STATE;
    return sizeof(MEDIA_VFE_STATE) + sizeof(PIPE_CONTROL);
}
```

**Taille totale**: MEDIA_VFE_STATE (9 DWords) + PIPE_CONTROL (6 DWords) = 15 DWords = 60 bytes

### 5.6 Code Implémentation C

```c
// Structure MEDIA_VFE_STATE Gen9 (9 DWords)
struct gen9_media_vfe_state {
    uint32_t dw0;  // Opcode
    uint32_t dw1;  // Scratch space pointer low
    uint32_t dw2;  // Scratch space pointer high + config
    uint32_t dw3;  // Max threads + URB entries
    uint32_t dw4;  // URB entry size + CURBE size
    uint32_t dw5;  // Scoreboard config
    uint32_t dw6;  // Scoreboard mask
    uint32_t dw7;  // Scoreboard type
    uint32_t dw8;  // Reserved
};

#define URB_ENTRY_ALLOCATION_SIZE 0x782
#define MAX_THREADS_UHD620        168

void emit_media_vfe_state(uint32_t *batch, 
                         uint64_t scratch_addr, 
                         uint32_t scratch_size) {
    struct gen9_media_vfe_state *vfe = (struct gen9_media_vfe_state *)batch;
    
    vfe->dw0 = 0x70000007;  // MEDIA_VFE_STATE opcode + length
    
    // Scratch space address (64-bit)
    vfe->dw1 = (uint32_t)(scratch_addr & 0xFFFFFFFF);
    vfe->dw2 = (uint32_t)(scratch_addr >> 32);
    
    // Scratch space size (per-thread et stack)
    if (scratch_size > 0) {
        uint32_t scratch_value = 0;
        // Calcul log2(scratch_size / 1024)
        uint32_t size_kb = scratch_size / 1024;
        while (size_kb > 1) {
            scratch_value++;
            size_kb >>= 1;
        }
        vfe->dw2 |= (scratch_value << 12);  // Per-thread scratch space
        vfe->dw2 |= (scratch_value << 16);  // Stack size
    }
    
    // Maximum threads + URB entries
    vfe->dw3 = MAX_THREADS_UHD620;      // Max threads
    vfe->dw3 |= (1 << 16);              // Number of URB entries = 1
    
    // URB entry allocation size
    vfe->dw4 = URB_ENTRY_ALLOCATION_SIZE;
    vfe->dw4 |= (0 << 16);              // CURBE allocation size = 0
    
    // Scoreboard (désactivé pour compute)
    vfe->dw5 = 0;
    vfe->dw6 = 0;
    vfe->dw7 = 0;
    vfe->dw8 = 0;
}
```

**CRITICITÉ**: **MAXIMALE** — Sans MEDIA_VFE_STATE correct, GPU ne peut pas exécuter threads

---

## SECTION 6 : DÉCOUVERTE #4 — STATE_BASE_ADDRESS

### 6.1 Implémentation Complète

**Source**: [`state_base_address_base.inl:33-106`](../../compute-runtime_test/shared/source/helpers/state_base_address_base.inl:33)

```cpp
template <typename GfxFamily>
void StateBaseAddressHelper<GfxFamily>::programStateBaseAddress(
    StateBaseAddressHelperArgs<GfxFamily> &args) {
    
    *args.stateBaseAddressCmd = GfxFamily::cmdInitStateBaseAddress;
    
    // Bindless Surface State Size
    const auto surfaceStateCount = getMaxBindlessSurfaceStates();
    args.stateBaseAddressCmd->setBindlessSurfaceStateSize(surfaceStateCount);
    
    if (args.useGlobalHeapsBaseAddress) {
        // Mode global heaps (simplifié)
        args.stateBaseAddressCmd->setDynamicStateBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setDynamicStateBufferSizeModifyEnable(true);
        args.stateBaseAddressCmd->setDynamicStateBaseAddress(args.globalHeapsBaseAddress);
        args.stateBaseAddressCmd->setDynamicStateBufferSize(MemoryConstants::pageSize64k);
        
        args.stateBaseAddressCmd->setSurfaceStateBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setSurfaceStateBaseAddress(args.globalHeapsBaseAddress);
        
        args.stateBaseAddressCmd->setBindlessSurfaceStateBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setBindlessSurfaceStateBaseAddress(args.globalHeapsBaseAddress);
    } else {
        // Mode heaps séparés
        if (args.dsh) {  // Dynamic State Heap
            args.stateBaseAddressCmd->setDynamicStateBaseAddressModifyEnable(true);
            args.stateBaseAddressCmd->setDynamicStateBufferSizeModifyEnable(true);
            args.stateBaseAddressCmd->setDynamicStateBaseAddress(args.dsh->getHeapGpuBase());
            args.stateBaseAddressCmd->setDynamicStateBufferSize(args.dsh->getHeapSizeInPages());
        }
        
        if (args.ssh) {  // Surface State Heap
            args.stateBaseAddressCmd->setSurfaceStateBaseAddressModifyEnable(true);
            args.stateBaseAddressCmd->setSurfaceStateBaseAddress(args.ssh->getHeapGpuBase());
        }
    }
    
    // Indirect Object Heap
    appendIohParameters(args);
    
    // Instruction Heap
    if (args.setInstructionStateBaseAddress) {
        args.stateBaseAddressCmd->setInstructionBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setInstructionBaseAddress(args.instructionHeapBaseAddress);
        args.stateBaseAddressCmd->setInstructionBufferSizeModifyEnable(true);
        args.stateBaseAddressCmd->setInstructionBufferSize(MemoryConstants::sizeOf4GBinPageEntities);
        
        auto resourceUsage = CacheSettingsHelper::getGmmUsageType(
            AllocationType::INTERNAL_HEAP, 
            DebugManager.flags.DisableCachingForHeaps.get(), 
            *args.gmmHelper->getHardwareInfo()
        );
        args.stateBaseAddressCmd->setInstructionMemoryObjectControlState(
            args.gmmHelper->getMOCS(resourceUsage)
        );
    }
    
    // General State Base Address (CRITIQUE pour stateless)
    if (args.setGeneralStateBaseAddress) {
        args.stateBaseAddressCmd->setGeneralStateBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setGeneralStateBufferSizeModifyEnable(true);
        args.stateBaseAddressCmd->setGeneralStateBaseAddress(
            args.gmmHelper->decanonize(args.generalStateBase)
        );
        args.stateBaseAddressCmd->setGeneralStateBufferSize(0xfffff);
    }
    
    // Stateless MOCS (CRITIQUE Gen9)
    if (DebugManager.flags.OverrideStatelessMocsIndex.get() != -1) {
        args.statelessMocsIndex = DebugManager.flags.OverrideStatelessMocsIndex.get();
    }
    args.statelessMocsIndex = args.statelessMocsIndex << 1;
    GmmHelper::applyMocsEncryptionBit(args.statelessMocsIndex);
    
    args.stateBaseAddressCmd->setStatelessDataPortAccessMemoryObjectControlState(
        args.statelessMocsIndex
    );
    
    appendStateBaseAddressParameters(args, overrideBindlessSurfaceStateBase);
}
```

### 6.2 Support Gen9 Spécifique

**Source**: [`hw_cmds_base.h:47-50`](../../compute-runtime_test/shared/source/gen9/hw_cmds_base.h:47)

```cpp
struct StateBaseAddressStateSupport {
    static constexpr bool globalAtomics = false;      // Pas supporté Gen9
    static constexpr bool statelessMocs = true;       // CRITIQUE Gen9
};
```

**Important**: Gen9 ne supporte PAS global atomics mais REQUIERT stateless MOCS

### 6.3 Bindless Surface State (Skylake)

**Source**: [`state_base_address_skl.inl:17-22`](../../compute-runtime_test/shared/source/helpers/state_base_address_skl.inl:17)

```cpp
template <typename GfxFamily>
void StateBaseAddressHelper<GfxFamily>::appendStateBaseAddressParameters(
    StateBaseAddressHelperArgs<GfxFamily> &args,
    bool overrideBindlessSurfaceStateBase) {
    
    if (overrideBindlessSurfaceStateBase && args.ssh) {
        args.stateBaseAddressCmd->setBindlessSurfaceStateBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setBindlessSurfaceStateBaseAddress(
            args.ssh->getHeapGpuBase()
        );
        uint32_t size = uint32_t(args.ssh->getMaxAvailableSpace() / 64) - 1;
        args.stateBaseAddressCmd->setBindlessSurfaceStateSize(size);
    }
}
```

### 6.4 Taille Maximum Bindless

**Source**: [`state_base_address_skl.inl:26-28`](../../compute-runtime_test/shared/source/helpers/state_base_address_skl.inl:26)

```cpp
template <typename GfxFamily>
uint32_t StateBaseAddressHelper<GfxFamily>::getMaxBindlessSurfaceStates() {
    return (1 << 20) - 1;  // 1048575 surfaces maximum
}
```

### 6.5 Indirect Object Heap

**Source**: [`state_base_address_bdw_and_later.inl:17-29`](../../compute-runtime_test/shared/source/helpers/state_base_address_bdw_and_later.inl:17)

```cpp
template <typename GfxFamily>
void StateBaseAddressHelper<GfxFamily>::appendIohParameters(
    StateBaseAddressHelperArgs<GfxFamily> &args) {
    
    if (args.useGlobalHeapsBaseAddress) {
        args.stateBaseAddressCmd->setIndirectObjectBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setIndirectObjectBufferSizeModifyEnable(true);
        args.stateBaseAddressCmd->setIndirectObjectBaseAddress(args.indirectObjectHeapBaseAddress);
        args.stateBaseAddressCmd->setIndirectObjectBufferSize(MemoryConstants::sizeOf4GBinPageEntities);
    } else if (args.ioh) {
        args.stateBaseAddressCmd->setIndirectObjectBaseAddressModifyEnable(true);
        args.stateBaseAddressCmd->setIndirectObjectBufferSizeModifyEnable(true);
        args.stateBaseAddressCmd->setIndirectObjectBaseAddress(args.ioh->getHeapGpuBase());
        args.stateBaseAddressCmd->setIndirectObjectBufferSize(args.ioh->getHeapSizeInPages());
    }
}
```

### 6.6 Code Implémentation C

```c
// Structure STATE_BASE_ADDRESS Gen9 (16 DWords minimum)
struct gen9_state_base_address {
    uint32_t dw0;   // Opcode + length
    uint32_t dw1;   // General state base address low
    uint32_t dw2;   // General state base address high + MOCS
    uint32_t dw3;   // Surface state base address low
    uint32_t dw4;   // Surface state base address high + MOCS
    uint32_t dw5;   // Dynamic state base address low
    uint32_t dw6;   // Dynamic state base address high + MOCS
    uint32_t dw7;   // Indirect object base address low
    uint32_t dw8;   // Indirect object base address high + MOCS
    uint32_t dw9;   // Instruction base address low
    uint32_t dw10;  // Instruction base address high + MOCS
    uint32_t dw11;  // General state buffer size
    uint32_t dw12;  // Dynamic state buffer size
    uint32_t dw13;  // Indirect object buffer size
    uint32_t dw14;  // Instruction buffer size
    uint32_t dw15;  // Bindless surface state base address low
    uint32_t dw16;  // Bindless surface state base address high + size
};

#define MOCS_UNCACHED 0x0
#define MOCS_CACHED   0x1
#define MOCS_WB       0x2  // Write-back
#define MOCS_WT       0x3  // Write-through

#define MAX_BINDLESS_SURFACES ((1 << 20) - 1)

void emit_state_base_address(uint32_t *batch, 
                             uint64_t general_base,
                             uint64_t surface_base,
                             uint64_t dynamic_base,
                             uint64_t instruction_base,
                             uint32_t stateless_mocs) {
    struct gen9_state_base_address *sba = (struct gen9_state_base_address *)batch;
    
    sba->dw0 = 0x61000010;  // STATE_BASE_ADDRESS opcode + length (16 DWords)
    
    // General State Base Address (0 pour stateless access)
    sba->dw1 = (uint32_t)(general_base & 0xFFFFFFFF) | 0x1;  // Modify enable
    sba->dw2 = (uint32_t)(general_base >> 32);
    sba->dw2 |= (MOCS_CACHED << 4);  // MOCS index
    
    // Surface State Base Address
    sba->dw3 = (uint32_t)(surface_base & 0xFFFFFFFF) | 0x1;
    sba->dw4 = (uint32_t)(surface_base >> 32);
    sba->dw4 |= (MOCS_CACHED << 4);
    
    // Dynamic State Base Address
    sba->dw5 = (uint32_t)(dynamic_base & 0xFFFFFFFF) | 0x1;
    sba->dw6 = (uint32_t)(dynamic_base >> 32);
    sba->dw6 |= (MOCS_CACHED << 4);
    
    // Indirect Object Base Address
    sba->dw7 = 0x1;  // Modify enable only (optionnel)
    sba->dw8 = 0;
    
    // Instruction Base Address
    sba->dw9 = (uint32_t)(instruction_base & 0xFFFFFFFF) | 0x1;
    sba->dw10 = (uint32_t)(instruction_base >> 32);
    sba->dw10 |= (MOCS_CACHED << 4);
    
    // Buffer sizes (en pages 4KB)
    sba->dw11 = 0xfffff | 0x1;  // General: max + modify enable
    sba->dw12 = 0xfffff | 0x1;  // Dynamic: max + modify enable
    sba->dw13 = 0xfffff | 0x1;  // Indirect: max + modify enable
    sba->dw14 = 0xfffff | 0x1;  // Instruction: max + modify enable
    
    // Bindless Surface State Base Address
    sba->dw15 = (uint32_t)(surface_base & 0xFFFFFFFF) | 0x1;
    sba->dw16 = (uint32_t)(surface_base >> 32);
    sba->dw16 |= MAX_BINDLESS_SURFACES;  // Max bindless surfaces
    
    // Stateless Data Port Access MOCS (CRITIQUE Gen9)
    // Note: Doit être configuré dans un DWord supplémentaire selon version
    // Pour Gen9, généralement dans extension de la commande
}
```

**CRITICITÉ**: **MAXIMALE** — Sans STATE_BASE_ADDRESS, GPU ne peut pas accéder à la mémoire

---

## SECTION 7 : RÉCAPITULATIF ÉTATS IDENTIFIÉS

### 7.1 Tableau Complet 25 États

| # | État GPU | Commande | Criticité | Partie |
|---|----------|----------|-----------|--------|
| 1 | L3 Config SLM | MI_LOAD_REGISTER_IMM | **CRITIQUE** | 1 |
| 2 | L3 Config No-SLM | MI_LOAD_REGISTER_IMM | **CRITIQUE** | 1 |
| 3-21 | PIPE_CONTROL (19 flags) | PIPE_CONTROL | **CRITIQUE** | 1 |
| 22 | MEDIA_VFE_STATE maxThreads | MEDIA_VFE_STATE | **CRITIQUE** | 2 |
| 23 | MEDIA_VFE_STATE scratchSpace | MEDIA_VFE_STATE | **CRITIQUE** | 2 |
| 24 | STATE_BASE_ADDRESS statelessMocs | STATE_BASE_ADDRESS | **CRITIQUE** | 2 |
| 25 | STATE_BASE_ADDRESS bindless | STATE_BASE_ADDRESS | Haute | 2 |

### 7.2 Ordre Séquence Batch Buffer

**Séquence obligatoire Intel**:
```
1. PIPELINE_SELECT (mode GPGPU)
2. MI_LOAD_REGISTER_IMM (L3 Config)          ← ÉTAT #1-2
3. STATE_BASE_ADDRESS (pointeurs mémoire)    ← ÉTAT #24-25
4. PIPE_CONTROL (flush avant VFE)            ← ÉTAT #3-21
5. MEDIA_VFE_STATE (configuration threads)   ← ÉTAT #22-23
6. PIPE_CONTROL (synchronisation post-VFE)   ← ÉTAT #3-21
7. MEDIA_INTERFACE_DESCRIPTOR_LOAD
8. GPGPU_WALKER (dispatch)
9. PIPE_CONTROL (flush final)                ← ÉTAT #3-21
10. MI_BATCH_BUFFER_END
```

---

## FIN PARTIE 2/3

**Prochaine partie**: Plan implémentation complet et tests validation

**États totaux identifiés**: **25 états GPU critiques**