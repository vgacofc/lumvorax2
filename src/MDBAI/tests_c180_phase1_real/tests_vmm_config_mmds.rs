// Tests pour vmm_config/mmds.rs (5 tests réels)
#[cfg(test)]
mod tests_c180_mmds_config {
    use super::*;
    use std::net::Ipv4Addr;

    #[test]
    fn test_mmds_config_new() {
        // Test création MmdsConfig avec valeurs par défaut
        let config = MmdsConfig {
            version: MmdsVersion::V1,
            network_interfaces: vec!["eth0".to_string()],
            ipv4_address: Some(Ipv4Addr::new(169, 254, 169, 254)),
            imds_compat: false,
        };
        
        assert_eq!(config.version(), MmdsVersion::V1);
        assert_eq!(config.network_interfaces().len(), 1);
        assert!(config.ipv4_addr().is_some());
    }

    #[test]
    fn test_mmds_config_version_getter() {
        // Test getter version()
        let config = MmdsConfig {
            version: MmdsVersion::V2,
            network_interfaces: vec![],
            ipv4_address: None,
            imds_compat: true,
        };
        
        assert_eq!(config.version(), MmdsVersion::V2);
    }

    #[test]
    fn test_mmds_config_network_interfaces_getter() {
        // Test getter network_interfaces()
        let interfaces = vec!["eth0".to_string(), "eth1".to_string()];
        let config = MmdsConfig {
            version: MmdsVersion::V1,
            network_interfaces: interfaces.clone(),
            ipv4_address: None,
            imds_compat: false,
        };
        
        assert_eq!(config.network_interfaces(), interfaces);
    }

    #[test]
    fn test_mmds_config_ipv4_addr_some() {
        // Test getter ipv4_addr() avec valeur
        let ip = Ipv4Addr::new(169, 254, 169, 254);
        let config = MmdsConfig {
            version: MmdsVersion::V1,
            network_interfaces: vec![],
            ipv4_address: Some(ip),
            imds_compat: false,
        };
        
        assert_eq!(config.ipv4_addr(), Some(ip));
    }

    #[test]
    fn test_mmds_config_ipv4_addr_none() {
        // Test getter ipv4_addr() sans valeur
        let config = MmdsConfig {
            version: MmdsVersion::V1,
            network_interfaces: vec![],
            ipv4_address: None,
            imds_compat: false,
        };
        
        assert_eq!(config.ipv4_addr(), None);
    }
}
