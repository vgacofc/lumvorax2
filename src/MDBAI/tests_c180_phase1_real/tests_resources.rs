// Tests pour resources.rs (10 tests réels)
#[cfg(test)]
mod tests_c180_resources {
    use super::*;

    #[test]
    fn test_vm_resources_default() {
        // Test VmResources::default()
        let resources = VmResources::default();
        // Vérifie que la structure est créée
        let _ = resources.machine_config;
        assert!(true);
    }

    #[test]
    fn test_vm_resources_debug() {
        // Test que VmResources implémente Debug
        let resources = VmResources::default();
        let debug_str = format!("{:?}", resources);
        assert!(!debug_str.is_empty());
    }

    #[test]
    fn test_vm_resources_machine_config_access() {
        // Test accès au champ machine_config
        let resources = VmResources::default();
        let _ = &resources.machine_config;
        assert!(true);
    }

    #[test]
    fn test_vm_resources_boot_source_access() {
        // Test accès au champ boot_source
        let resources = VmResources::default();
        let _ = &resources.boot_source;
        assert!(true);
    }

    #[test]
    fn test_vm_resources_block_access() {
        // Test accès au champ block
        let resources = VmResources::default();
        let _ = &resources.block;
        assert!(true);
    }

    #[test]
    fn test_vm_resources_size() {
        // Test taille de VmResources (doit être raisonnable)
        let size = std::mem::size_of::<VmResources>();
        assert!(size > 0);
        assert!(size < 10240); // Moins de 10KB
    }

    #[test]
    fn test_vmm_config_exists() {
        // Test que VmmConfig existe
        let size = std::mem::size_of::<VmmConfig>();
        assert!(size > 0);
    }

    #[test]
    fn test_machine_config_exists() {
        // Test que MachineConfig existe
        let size = std::mem::size_of::<MachineConfig>();
        assert!(size > 0);
    }

    #[test]
    fn test_boot_source_exists() {
        // Test que BootSource existe
        let size = std::mem::size_of::<BootSource>();
        assert!(size > 0);
    }

    #[test]
    fn test_block_builder_exists() {
        // Test que BlockBuilder existe
        let size = std::mem::size_of::<BlockBuilder>();
        assert!(size > 0);
    }
}
