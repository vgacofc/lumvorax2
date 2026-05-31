# MAPPING COMPLET DES CORRECTIONS C172

## Types Réels Identifiés

### VmmActionError - Variantes Existantes
```rust
BalloonConfig(BalloonConfigError)
BalloonUpdate(VmmError)
BootSource(BootSourceConfigError)
CreateSnapshot(CreateSnapshotError)  // ✅ Pas "Snapshot"
ConfigureCpu(GuestConfigError)
DriveConfig(DriveError)
EntropyConfig(EntropyDeviceError)  // ✅ Pas "EntropyDevice"
PmemConfig(PmemConfigError)
MemoryHotplugConfig(MemoryHotplugConfigError)  // ✅ Pas "MemoryHotplug"
MemoryHotplugUpdate(VmmError)
InternalVmm(VmmError)  // ✅ Pas "Internal"
LoadSnapshot(LoadSnapshotError)
Logger(crate::logger::LoggerUpdateError)  // ✅ Pas "LoggerConfigError"
MachineConfig(MachineConfigError)  // ✅ Pas "VmConfigError"
Metrics(MetricsConfigError)
Mmds(data_store::MmdsDatastoreError)
MmdsConfig(MmdsConfigError)
MmdsLimitExceeded(data_store::MmdsDatastoreError)  // ✅ Prend MmdsDatastoreError, pas usize
NetworkConfig(NetworkInterfaceError)
NotSupported(String)
OperationNotSupportedPostBoot
OperationNotSupportedPreBoot
StartMicrovm(StartMicrovmError)
VsockConfig(VsockConfigError)
DeviceIdInUse
DeviceNotFound
CannotUnplugRootDevice
PciNotEnabled
PciManager(PciManagerError)
```

### VmmData - Variantes Existantes
```rust
BalloonConfig(BalloonDeviceConfig)
BalloonStats(BalloonStats)
Empty
FullVmConfig(VmmConfig)  // ✅ Pas "FullVmConfiguration"
MachineConfiguration(MachineConfig)
MmdsValue(serde_json::Value)
InstanceInformation(InstanceInfo)
VmmVersion(String)
VirtioMemStatus(VirtioMemStatus)  // ✅ Pas "MemoryHotplugSize"
HintingStatus(HintingStatus)  // ✅ Pas "FreePageHintingStatus"
```

### Signatures Fonctions
```rust
// ✅ CORRECT
fn mmds_patch_data(
    mut mmds: MutexGuard<'_, Mmds>,  // Pas Arc<Mutex<Mmds>>
    value: serde_json::Value,  // Pas de size_limit
) -> Result<VmmData, VmmActionError>

fn mmds_put_data(
    mut mmds: MutexGuard<'_, Mmds>,  // Pas Arc<Mutex<Mmds>>
    value: serde_json::Value,  // Pas de size_limit
) -> Result<VmmData, VmmActionError>
```

### MachineConfig Structure
```rust
pub struct MachineConfig {
    pub vcpu_count: u8,  // ✅ Pas Option<u8>
    pub mem_size_mib: usize,  // ✅ Pas Option<usize>
    pub smt: bool,  // ✅ Pas Option<bool>
    pub cpu_template: Option<CpuTemplateType>,
    pub track_dirty_pages: bool,  // ✅ Pas Option<bool>
    pub huge_pages: HugePageConfig,  // ✅ Field requis
}
```

## Corrections à Appliquer

### 1. Remplacements Types VmmActionError
```rust
// AVANT → APRÈS
VmmActionError::Snapshot(...) → VmmActionError::CreateSnapshot(...)
VmmActionError::EntropyDevice(...) → VmmActionError::EntropyConfig(...)
VmmActionError::MemoryHotplug(...) → VmmActionError::MemoryHotplugConfig(...)
VmmActionError::Internal(...) → VmmActionError::InternalVmm(...)
VmmActionError::Logger(LoggerConfigError::...) → VmmActionError::Logger(crate::logger::LoggerUpdateError::...)
VmmActionError::MachineConfig(VmConfigError::...) → VmmActionError::MachineConfig(MachineConfigError::...)
VmmActionError::MmdsLimitExceeded(51300) → VmmActionError::MmdsLimitExceeded(data_store::MmdsDatastoreError::DataStoreLimitExceeded)
```

### 2. Remplacements Types VmmData
```rust
// AVANT → APRÈS
VmmData::FullVmConfig(FullVmConfiguration::default()) → VmmData::FullVmConfig(VmmConfig::default())
VmmData::MemoryHotplugSize(...) → VmmData::VirtioMemStatus(...)
VmmData::FreePageHintingStatus(...) → VmmData::HintingStatus(...)
```

### 3. Corrections Appels Fonctions MMDS
```rust
// AVANT
let mmds = Arc::new(Mutex::new(Mmds::default()));
mmds_put_data(mmds.clone(), value, 1000)

// APRÈS
let mmds = Arc::new(Mutex::new(Mmds::default()));
mmds_put_data(mmds.lock().unwrap(), value)
```

### 4. Corrections MachineConfig
```rust
// AVANT
MachineConfig {
    vcpu_count: Some(2),
    mem_size_mib: Some(512),
    smt: Some(false),
    cpu_template: None,
    track_dirty_pages: Some(false),
}

// APRÈS
MachineConfig {
    vcpu_count: 2,
    mem_size_mib: 512,
    smt: false,
    cpu_template: None,
    track_dirty_pages: false,
    huge_pages: HugePageConfig::None,
}
```

### 5. Corrections Variantes Enum Spécifiques
```rust
// DriveError - À vérifier
DriveError::InvalidBlockDeviceID → Trouver variante réelle

// PmemConfigError - À vérifier
PmemConfigError::InvalidPmemDeviceID → Trouver variante réelle

// LoadSnapshotError - À vérifier
LoadSnapshotError::InvalidMemoryBackendType → Trouver variante réelle

// SnapshotError - À vérifier
SnapshotError::InvalidSnapshotVersion → crate::snapshot::SnapshotError::InvalidFormatVersion
```

## Tests à Créer (80+ tests complexes)

### Catégorie A: Tests VmmAction (30 tests)
- ConfigureBootSource avec paths valides/invalides
- ConfigureLogger avec différents niveaux
- ConfigureMetrics avec paths
- SetBalloonDevice avec configs variées
- SetVsockDevice avec CIDs différents
- SetMmdsConfiguration avec IPv4
- UpdateMachineConfiguration avec tous les champs
- StartMicroVm avec différents états
- Pause/Resume avec états
- CreateSnapshot avec types Full/Diff
- LoadSnapshot avec backends File/Uffd
- UpdateBalloon avec tailles
- UpdateNetworkInterface avec rate limiters
- UpdateBlockDevice avec rate limiters
- UpdatePmemDevice avec rate limiters

### Catégorie B: Tests VmmActionError (25 tests)
- Toutes les variantes avec From conversions
- BalloonConfig avec toutes les erreurs
- BootSource avec IO errors
- DriveConfig avec toutes les variantes
- EntropyConfig avec toutes les variantes
- MemoryHotplugConfig avec toutes les variantes
- Logger avec toutes les variantes
- MachineConfig avec toutes les variantes
- Mmds avec NotInitialized, DataStoreLimitExceeded
- MmdsLimitExceeded avec DataStoreLimitExceeded
- NetworkConfig avec toutes les variantes
- VsockConfig avec toutes les variantes
- CreateSnapshot avec toutes les variantes
- LoadSnapshot avec toutes les variantes

### Catégorie C: Tests VmmData (10 tests)
- BalloonConfig avec configs variées
- BalloonStats avec stats réelles
- FullVmConfig avec VmmConfig complet
- MachineConfiguration avec tous les champs
- MmdsValue avec JSON complexe
- VirtioMemStatus avec états
- HintingStatus avec états

### Catégorie D: Tests Helper Functions (10 tests)
- mmds_put_data avec données valides
- mmds_put_data avec limite dépassée
- mmds_patch_data avec patch valide
- mmds_patch_data avec MMDS non initialisé
- mmds_patch_data avec limite dépassée
- mmds_put_data avec JSON invalide
- mmds_patch_data avec JSON invalide
- mmds_put_data avec données vides
- mmds_patch_data avec patch vide
- mmds operations avec concurrence

### Catégorie E: Tests Integration (15 tests)
- PrebootApiController workflow complet
- RuntimeApiController workflow complet
- Transitions d'état PreBoot → PostBoot
- Opérations non supportées PreBoot
- Opérations non supportées PostBoot
- Device insertion séquentielle
- Device removal
- Configuration updates
- Snapshot create/restore
- MMDS operations complètes
- Rate limiter updates
- Error recovery
- Concurrent operations
- State persistence
- Rollback scenarios

## Timeline Exécution

1. **Vérifier variantes manquantes** (15 min)
   - DriveError
   - PmemConfigError
   - LoadSnapshotError
   - SnapshotError

2. **Créer tests Catégorie A** (45 min)
   - 30 tests VmmAction

3. **Créer tests Catégorie B** (45 min)
   - 25 tests VmmActionError

4. **Créer tests Catégorie C** (15 min)
   - 10 tests VmmData

5. **Créer tests Catégorie D** (15 min)
   - 10 tests Helper Functions

6. **Créer tests Catégorie E** (30 min)
   - 15 tests Integration

7. **Compilation et validation** (30 min)
   - Corriger erreurs restantes
   - Valider tous les tests passent

8. **Mesure couverture** (30 min)
   - cargo llvm-cov
   - Analyse rapport HTML
   - Validation impact +0.8%

**Total**: 3h30 (vs 4h initial)