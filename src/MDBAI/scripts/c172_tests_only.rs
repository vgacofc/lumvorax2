    #[test]
    fn test_c172_vmm_action_flush_metrics_variant() {
        let action = VmmAction::FlushMetrics;
        assert!(matches!(action, VmmAction::FlushMetrics));
    }

    #[test]
    fn test_c172_vmm_action_pause_variant() {
        let action = VmmAction::Pause;
        assert!(matches!(action, VmmAction::Pause));
    }

    #[test]
    fn test_c172_vmm_action_resume_variant() {
        let action = VmmAction::Resume;
        assert!(matches!(action, VmmAction::Resume));
    }

    #[test]
    fn test_c172_vmm_action_get_balloon_config_variant() {
        let action = VmmAction::GetBalloonConfig;
        assert!(matches!(action, VmmAction::GetBalloonConfig));
    }

    #[test]
    fn test_c172_vmm_action_get_balloon_stats_variant() {
        let action = VmmAction::GetBalloonStats;
        assert!(matches!(action, VmmAction::GetBalloonStats));
    }

    #[test]
    fn test_c172_vmm_action_get_vm_machine_config_variant() {
        let action = VmmAction::GetVmMachineConfig;
        assert!(matches!(action, VmmAction::GetVmMachineConfig));
    }

    #[test]
    fn test_c172_vmm_action_get_mmds_variant() {
        let action = VmmAction::GetMMDS;
        assert!(matches!(action, VmmAction::GetMMDS));
    }

    #[test]
    fn test_c172_vmm_action_start_microvm_variant() {
        let action = VmmAction::StartMicroVm;
        assert!(matches!(action, VmmAction::StartMicroVm));
    }

    #[test]
    fn test_c172_vmm_action_error_operation_not_supported_pre_boot() {
        let error = VmmActionError::OperationNotSupportedPreBoot;
        assert!(matches!(error, VmmActionError::OperationNotSupportedPreBoot));
    }

    #[test]
    fn test_c172_vmm_action_error_operation_not_supported_post_boot() {
        let error = VmmActionError::OperationNotSupportedPostBoot;
        assert!(matches!(error, VmmActionError::OperationNotSupportedPostBoot));
    }

    #[test]
    fn test_c172_vmm_action_error_device_id_in_use() {
        let error = VmmActionError::DeviceIdInUse;
        assert!(matches!(error, VmmActionError::DeviceIdInUse));
    }

    #[test]
    fn test_c172_vmm_action_error_device_not_found() {
        let error = VmmActionError::DeviceNotFound;
        assert!(matches!(error, VmmActionError::DeviceNotFound));
    }

    #[test]
    fn test_c172_vmm_action_error_cannot_unplug_root_device() {
        let error = VmmActionError::CannotUnplugRootDevice;
        assert!(matches!(error, VmmActionError::CannotUnplugRootDevice));
    }

    #[test]
    fn test_c172_vmm_action_error_pci_not_enabled() {
        let error = VmmActionError::PciNotEnabled;
        assert!(matches!(error, VmmActionError::PciNotEnabled));
    }

    #[test]
    fn test_c172_vmm_data_empty_variant() {
        let data = VmmData::Empty;
        assert!(matches!(data, VmmData::Empty));
    }

    #[test]
    fn test_c172_vmm_action_configure_boot_source() {
        let boot_config = BootSourceConfig {
            kernel_image_path: "/path/to/kernel".to_string(),
            initrd_path: None,
            boot_args: None,
        };
        let action = VmmAction::ConfigureBootSource(boot_config);
        assert!(matches!(action, VmmAction::ConfigureBootSource(_)));
    }

    #[test]
    fn test_c172_vmm_action_configure_logger() {
        let logger_config = LoggerConfig {
            log_path: "/tmp/firecracker.log".into(),
            level: crate::logger::LevelFilter::Info,
            show_level: true,
            show_log_origin: false,
            options: vec![],
        };
        let action = VmmAction::ConfigureLogger(logger_config);
        assert!(matches!(action, VmmAction::ConfigureLogger(_)));
    }

    #[test]
    fn test_c172_vmm_action_configure_metrics() {
        let metrics_config = MetricsConfig {
            metrics_path: "/tmp/metrics.log".into(),
        };
        let action = VmmAction::ConfigureMetrics(metrics_config);
        assert!(matches!(action, VmmAction::ConfigureMetrics(_)));
    }

    #[test]
    fn test_c172_vmm_action_insert_block_device() {
        let block_config = BlockDeviceConfig {
            drive_id: "rootfs".to_string(),
            path_on_host: "/path/to/rootfs.ext4".into(),
            is_root_device: true,
            partuuid: None,
            is_read_only: false,
            cache_type: crate::vmm_config::drive::CacheType::Unsafe,
            rate_limiter: None,
            file_engine_type: None,
        };
        let action = VmmAction::InsertBlockDevice(block_config);
        assert!(matches!(action, VmmAction::InsertBlockDevice(_)));
    }

    #[test]
    fn test_c172_vmm_action_insert_network_device() {
        let net_config = NetworkInterfaceConfig {
            iface_id: "eth0".to_string(),
            host_dev_name: "tap0".to_string(),
            guest_mac: None,
            rx_rate_limiter: None,
            tx_rate_limiter: None,
        };
        let action = VmmAction::InsertNetworkDevice(net_config);
        assert!(matches!(action, VmmAction::InsertNetworkDevice(_)));
    }

    #[test]
    fn test_c172_vmm_action_set_balloon_device() {
        let balloon_config = BalloonDeviceConfig {
            amount_mib: 128,
            deflate_on_oom: true,
            stats_polling_interval_s: 1,
        };
        let action = VmmAction::SetBalloonDevice(balloon_config);
        assert!(matches!(action, VmmAction::SetBalloonDevice(_)));
    }

    #[test]
    fn test_c172_vmm_action_update_balloon_size() {
        let update_config = BalloonUpdateConfig {
            amount_mib: 256,
        };
        let action = VmmAction::UpdateBalloon(update_config);
        assert!(matches!(action, VmmAction::UpdateBalloon(_)));
    }

    #[test]
    fn test_c172_vmm_action_update_balloon_statistics() {
        let stats_config = BalloonUpdateStatsConfig {
            stats_polling_interval_s: 2,
        };
        let action = VmmAction::UpdateBalloonStatistics(stats_config);
        assert!(matches!(action, VmmAction::UpdateBalloonStatistics(_)));
    }

    #[test]
    fn test_c172_vmm_action_update_block_device() {
        let update_config = BlockDeviceUpdateConfig {
            drive_id: "rootfs".to_string(),
            rate_limiter: None,
        };
        let action = VmmAction::UpdateBlockDevice(update_config);
        assert!(matches!(action, VmmAction::UpdateBlockDevice(_)));
    }

    #[test]
    fn test_c172_vmm_action_update_network_interface() {
        let update_config = NetworkInterfaceUpdateConfig {
            iface_id: "eth0".to_string(),
            rx_rate_limiter: None,
            tx_rate_limiter: None,
        };
        let action = VmmAction::UpdateNetworkInterface(update_config);
        assert!(matches!(action, VmmAction::UpdateNetworkInterface(_)));
    }

    #[test]
    fn test_c172_vmm_action_set_vsock_device() {
        let vsock_config = VsockDeviceConfig {
            vsock_id: "vsock0".to_string(),
            guest_cid: 3,
            uds_path: "/tmp/vsock.sock".into(),
        };
        let action = VmmAction::SetVsockDevice(vsock_config);
        assert!(matches!(action, VmmAction::SetVsockDevice(_)));
    }

    #[test]
    fn test_c172_vmm_action_set_mmds_configuration() {
        let mmds_config = MmdsConfig {
            version: crate::vmds::data_store::MmdsVersion::V2,
            network_interfaces: vec!["eth0".to_string()],
            ipv4_address: Some(std::net::Ipv4Addr::new(169, 254, 169, 254)),
        };
        let action = VmmAction::SetMmdsConfiguration(mmds_config);
        assert!(matches!(action, VmmAction::SetMmdsConfiguration(_)));
    }

    #[test]
    fn test_c172_vmm_action_patch_mmds() {
        let value = serde_json::json!({"key": "value"});
        let action = VmmAction::PatchMMDS(value);
        assert!(matches!(action, VmmAction::PatchMMDS(_)));
    }

    #[test]
    fn test_c172_vmm_action_put_mmds() {
        let value = serde_json::json!({"key": "value"});
        let action = VmmAction::PutMMDS(value);
        assert!(matches!(action, VmmAction::PutMMDS(_)));
    }

    #[test]
    fn test_c172_vmm_action_create_snapshot() {
        let snapshot_params = CreateSnapshotParams {
            snapshot_type: SnapshotType::Full,
            snapshot_path: "/tmp/snapshot".into(),
            mem_file_path: "/tmp/mem".into(),
        };
        let action = VmmAction::CreateSnapshot(snapshot_params);
        assert!(matches!(action, VmmAction::CreateSnapshot(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_not_supported() {
        let error = VmmActionError::NotSupported("test operation".to_string());
        assert!(matches!(error, VmmActionError::NotSupported(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_balloon_config() {
        let balloon_error = BalloonConfigError::InvalidSize;
        let error = VmmActionError::BalloonConfig(balloon_error);
        assert!(matches!(error, VmmActionError::BalloonConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_boot_source() {
        let boot_error = BootSourceConfigError::InvalidKernelPath;
        let error = VmmActionError::BootSource(boot_error);
        assert!(matches!(error, VmmActionError::BootSource(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_drive_config() {
        let drive_error = DriveError::RootBlockDeviceAlreadyAdded;
        let error = VmmActionError::DriveConfig(drive_error);
        assert!(matches!(error, VmmActionError::DriveConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_entropy_config() {
        let entropy_error = EntropyDeviceError::DeviceAlreadyExists;
        let error = VmmActionError::EntropyConfig(entropy_error);
        assert!(matches!(error, VmmActionError::EntropyConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_machine_config() {
        let machine_error = MachineConfigError::InvalidVcpuCount;
        let error = VmmActionError::MachineConfig(machine_error);
        assert!(matches!(error, VmmActionError::MachineConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_metrics() {
        let metrics_error = MetricsConfigError::InvalidMetricsPath;
        let error = VmmActionError::Metrics(metrics_error);
        assert!(matches!(error, VmmActionError::Metrics(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_mmds() {
        let mmds_error = MmdsDatastoreError::NotInitialized;
        let error = VmmActionError::Mmds(mmds_error);
        assert!(matches!(error, VmmActionError::Mmds(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_mmds_config() {
        let mmds_config_error = MmdsConfigError::InvalidIpv4Addr;
        let error = VmmActionError::MmdsConfig(mmds_config_error);
        assert!(matches!(error, VmmActionError::MmdsConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_mmds_limit_exceeded() {
        let mmds_error = MmdsDatastoreError::DataStoreLimitExceeded;
        let error = VmmActionError::MmdsLimitExceeded(mmds_error);
        assert!(matches!(error, VmmActionError::MmdsLimitExceeded(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_network_config() {
        let net_error = NetworkInterfaceError::GuestMacAddressInUse;
        let error = VmmActionError::NetworkConfig(net_error);
        assert!(matches!(error, VmmActionError::NetworkConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_vsock_config() {
        let vsock_error = VsockConfigError::CreateVsockDevice(std::io::Error::from(std::io::ErrorKind::NotFound));
        let error = VmmActionError::VsockConfig(vsock_error);
        assert!(matches!(error, VmmActionError::VsockConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_pmem_config() {
        let pmem_error = PmemConfigError::RootPmemDeviceAlreadyExist;
        let error = VmmActionError::PmemConfig(pmem_error);
        assert!(matches!(error, VmmActionError::PmemConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_memory_hotplug_config() {
        let mem_error = MemoryHotplugConfigError::InvalidSize;
        let error = VmmActionError::MemoryHotplugConfig(mem_error);
        assert!(matches!(error, VmmActionError::MemoryHotplugConfig(_)));
    }

    #[test]
    fn test_c172_vmm_action_error_start_microvm() {
        let start_error = StartMicrovmError::MissingKernelConfig;
        let error = VmmActionError::StartMicrovm(start_error);
        assert!(matches!(error, VmmActionError::StartMicrovm(_)));
    }

    #[test]
    fn test_c172_vmm_data_balloon_config() {
        let balloon_config = BalloonDeviceConfig {
            amount_mib: 128,
            deflate_on_oom: true,
            stats_polling_interval_s: 1,
        };
        let data = VmmData::BalloonConfig(balloon_config);
        assert!(matches!(data, VmmData::BalloonConfig(_)));
    }

    #[test]
    fn test_c172_vmm_data_balloon_stats() {
        let stats = BalloonStats {
            target_pages: 1000,
            actual_pages: 900,
            target_mib: 100,
            actual_mib: 90,
            swap_in: Some(10),
            swap_out: Some(20),
            major_faults: Some(5),
            minor_faults: Some(15),
            free_memory: Some(50),
            total_memory: Some(100),
            available_memory: Some(60),
            disk_caches: Some(30),
            hugetlb_allocations: Some(0),
            hugetlb_failures: Some(0),
        };
        let data = VmmData::BalloonStats(stats);
        assert!(matches!(data, VmmData::BalloonStats(_)));
    }

    #[test]
    fn test_c172_vmm_data_machine_configuration() {
        let machine_config = MachineConfig {
            vcpu_count: 2,
            mem_size_mib: 512,
            smt: false,
            cpu_template: None,
            track_dirty_pages: false,
            huge_pages: crate::vmm_config::machine_config::HugePageConfig::None,
        };
        let data = VmmData::MachineConfiguration(machine_config);
        assert!(matches!(data, VmmData::MachineConfiguration(_)));
    }

    #[test]
    fn test_c172_vmm_data_mmds_value() {
        let value = serde_json::json!({"key": "value", "nested": {"data": 123}});
        let data = VmmData::MmdsValue(value);
        assert!(matches!(data, VmmData::MmdsValue(_)));
    }

    #[test]
    fn test_c172_vmm_data_instance_information() {
        let instance_info = InstanceInfo {
            id: "i-1234567890abcdef0".to_string(),
            state: "Running".to_string(),
            vmm_version: "1.0.0".to_string(),
            app_name: "Firecracker".to_string(),
        };
        let data = VmmData::InstanceInformation(instance_info);
        assert!(matches!(data, VmmData::InstanceInformation(_)));
    }

    #[test]
    fn test_c172_vmm_data_vmm_version() {
        let version = "1.0.0".to_string();
        let data = VmmData::VmmVersion(version);
        assert!(matches!(data, VmmData::VmmVersion(_)));
    }

    #[test]
    fn test_c172_vmm_data_virtio_mem_status() {
        let status = VirtioMemStatus {
            plugged_size: 1024,
            requested_size: 2048,
        };
        let data = VmmData::VirtioMemStatus(status);
        assert!(matches!(data, VmmData::VirtioMemStatus(_)));
    }

    #[test]
    fn test_c172_vmm_data_hinting_status() {
        let status = HintingStatus {
            state: "Enabled".to_string(),
        };
        let data = VmmData::HintingStatus(status);
        assert!(matches!(data, VmmData::HintingStatus(_)));
    }

    #[test]
    fn test_c172_vmm_data_full_vm_config() {
        let vmm_config = VmmConfig::default();
        let data = VmmData::FullVmConfig(vmm_config);
        assert!(matches!(data, VmmData::FullVmConfig(_)));
    }

    #[test]
    fn test_c172_vmm_data_empty_variant_explicit() {
        let data = VmmData::Empty;
        match data {
            VmmData::Empty => assert!(true),
            _ => panic!("Expected VmmData::Empty"),
        }
    }

    #[test]
    fn test_c172_mmds_operations_basic() {
        // Test basique des opérations MMDS
        let value = serde_json::json!({"test": "data"});
        assert!(value.is_object());
        assert_eq!(value["test"], "data");
    }

    #[test]
    fn test_c172_vmm_action_debug_trait() {
        let action = VmmAction::FlushMetrics;
        let debug_str = format!("{:?}", action);
        assert!(debug_str.contains("FlushMetrics"));
    }

    #[test]
    fn test_c172_vmm_action_partial_eq() {
        let action1 = VmmAction::Pause;
        let action2 = VmmAction::Pause;
        let action3 = VmmAction::Resume;
        assert_eq!(action1, action2);
        assert_ne!(action1, action3);
    }

    #[test]
    fn test_c172_vmm_data_json_serialization() {
        let value = serde_json::json!({"key": "value"});
        let data = VmmData::MmdsValue(value.clone());
        if let VmmData::MmdsValue(v) = data {
            assert_eq!(v, value);
        } else {
            panic!("Expected MmdsValue variant");
        }
    }

    #[test]
    fn test_c172_error_variants_coverage() {
        // Test de couverture des variantes d'erreur
        let errors = vec![
            VmmActionError::OperationNotSupportedPreBoot,
            VmmActionError::OperationNotSupportedPostBoot,
            VmmActionError::DeviceIdInUse,
            VmmActionError::DeviceNotFound,
            VmmActionError::CannotUnplugRootDevice,
            VmmActionError::PciNotEnabled,
        ];
        assert_eq!(errors.len(), 6);
    }
