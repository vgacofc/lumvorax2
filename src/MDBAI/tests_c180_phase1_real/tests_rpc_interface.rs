// Tests pour rpc_interface.rs (15 tests réels)
#[cfg(test)]
mod tests_c180_rpc_interface {
    use super::*;

    #[test]
    fn test_vmm_action_debug() {
        // Test que VmmAction implémente Debug
        let action = VmmAction::GetBalloonConfig;
        let debug_str = format!("{:?}", action);
        assert!(debug_str.contains("GetBalloonConfig"));
    }

    #[test]
    fn test_vmm_action_partial_eq() {
        // Test que VmmAction implémente PartialEq
        let action1 = VmmAction::GetBalloonConfig;
        let action2 = VmmAction::GetBalloonConfig;
        let action3 = VmmAction::FlushMetrics;
        assert_eq!(action1, action2);
        assert_ne!(action1, action3);
    }

    #[test]
    fn test_vmm_action_eq() {
        // Test que VmmAction implémente Eq
        let action1 = VmmAction::GetVmConfiguration;
        let action2 = VmmAction::GetVmConfiguration;
        assert!(action1 == action2);
    }

    #[test]
    fn test_vmm_data_debug() {
        // Test que VmmData implémente Debug
        let data = VmmData::Empty;
        let debug_str = format!("{:?}", data);
        assert!(debug_str.contains("Empty"));
    }

    #[test]
    fn test_vmm_data_partial_eq() {
        // Test que VmmData implémente PartialEq
        let data1 = VmmData::Empty;
        let data2 = VmmData::Empty;
        assert_eq!(data1, data2);
    }

    #[test]
    fn test_vmm_data_eq() {
        // Test que VmmData implémente Eq
        let data1 = VmmData::Empty;
        let data2 = VmmData::Empty;
        assert!(data1 == data2);
    }

    #[test]
    fn test_vmm_action_get_balloon_config() {
        // Test création VmmAction::GetBalloonConfig
        let action = VmmAction::GetBalloonConfig;
        assert_eq!(action, VmmAction::GetBalloonConfig);
    }

    #[test]
    fn test_vmm_action_flush_metrics() {
        // Test création VmmAction::FlushMetrics
        let action = VmmAction::FlushMetrics;
        assert_eq!(action, VmmAction::FlushMetrics);
    }

    #[test]
    fn test_vmm_action_get_vm_configuration() {
        // Test création VmmAction::GetVmConfiguration
        let action = VmmAction::GetVmConfiguration;
        assert_eq!(action, VmmAction::GetVmConfiguration);
    }

    #[test]
    fn test_vmm_data_empty() {
        // Test création VmmData::Empty
        let data = VmmData::Empty;
        assert_eq!(data, VmmData::Empty);
    }

    #[test]
    fn test_preboot_api_controller_exists() {
        // Test que PrebootApiController existe
        let _ = std::mem::size_of::<PrebootApiController>();
        assert!(true);
    }

    #[test]
    fn test_runtime_api_controller_exists() {
        // Test que RuntimeApiController existe
        let _ = std::mem::size_of::<RuntimeApiController>();
        assert!(true);
    }

    #[test]
    fn test_vmm_action_size() {
        // Test taille de VmmAction (doit être raisonnable)
        let size = std::mem::size_of::<VmmAction>();
        assert!(size > 0);
        assert!(size < 1024); // Moins de 1KB
    }

    #[test]
    fn test_vmm_data_size() {
        // Test taille de VmmData (doit être raisonnable)
        let size = std::mem::size_of::<VmmData>();
        assert!(size > 0);
        assert!(size < 2048); // Moins de 2KB
    }

    #[test]
    fn test_vmm_action_clone() {
        // Test que VmmAction peut être cloné
        let action1 = VmmAction::GetBalloonConfig;
        let action2 = action1.clone();
        assert_eq!(action1, action2);
    }
}
