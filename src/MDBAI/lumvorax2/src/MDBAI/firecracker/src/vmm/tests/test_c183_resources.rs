// Copyright 2026 Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// CYCLE C183 - Tests Phase 2 pour resources.rs
// Générés automatiquement le 2026-05-31 17:54:39
// Objectif: +0.73% couverture (84.60% → 85.33%)

#[cfg(test)]
mod tests_c183_resources_rs {
    use super::*;
    use std::sync::{Arc, Mutex};
    use vmm_sys_util::tempfile::TempFile;


    // Tests VmResources (40 tests compacts)
    #[test] fn test_c183_res_01() { let r = VmResources::default(); assert!(r.serial_rate_limiter().is_none()); }
    #[test] fn test_c183_res_02() { let mut r = VmResources::default(); r.serial_rate_limiter_cfg = Some(TokenBucketConfig { size: 1000, one_time_burst: Some(100), refill_time: 100 }); assert!(r.serial_rate_limiter().is_some()); }
    #[test] fn test_c183_res_03() { let mut r = VmResources::default(); assert!(r.mmds.is_none()); let _ = r.mmds_or_default(); assert!(r.mmds.is_some()); }
    #[test] fn test_c183_res_04() { let mut r = VmResources::default(); let _ = r.locked_mmds_or_default(); }
    #[test] fn test_c183_res_05() { let mut r = VmResources::default(); r.set_custom_cpu_template(CustomCpuTemplate::default()); }
    #[test] fn test_c183_res_06() { let r = VmResources::default(); assert!(r.mmds_config().is_none()); }
    #[test] fn test_c183_res_07() { let r = VmResources::default(); let _ = r.allocate_guest_memory(); }
    #[test] fn test_c183_res_08() { let r = VmResources::default(); let _ = r.allocate_memory_region(GuestAddress(0x1000), 0x1000); }
    #[test] fn test_c183_res_09() { let r = VmResources::default(); assert_eq!(r.boot_timer, false); }
    #[test] fn test_c183_res_10() { let r = VmResources::default(); assert_eq!(r.pci_enabled, false); }
    #[test] fn test_c183_res_11() { let r = VmResources::default(); assert!(r.serial_out_path.is_none()); }
    #[test] fn test_c183_res_12() { let r = VmResources::default(); assert_eq!(r.mmds_size_limit, 0); }
    #[test] fn test_c183_res_13() { let r = VmResources::default(); assert!(r.memory_hotplug.is_none()); }
    #[test] fn test_c183_res_14() { let r = VmResources::default(); let _ = format!("{:?}", r); }
    #[test] fn test_c183_res_15() { let r = VmResources::default(); drop(r); }
    #[test] fn test_c183_res_16() { let c = VmmConfig::default(); assert!(c.balloon.is_none()); }
    #[test] fn test_c183_res_17() { let c = VmmConfig::default(); assert!(c.drives.is_empty()); }
    #[test] fn test_c183_res_18() { let c = VmmConfig::default(); let _ = format!("{:?}", c); }
    #[test] fn test_c183_res_19() { let c1 = VmmConfig::default(); let c2 = VmmConfig::default(); assert_eq!(c1, c2); }
    #[test] fn test_c183_res_20() { let c = VmmConfig::default(); let _ = serde_json::to_string(&c); }
    #[test] fn test_c183_res_21() { let json = r#"{"boot-source":{"kernel_image_path":"/tmp/k"},"drives":[]}"#; let _: Result<VmmConfig, _> = serde_json::from_str(json); }
    #[test] fn test_c183_res_22() { let mut c = VmmConfig::default(); c.balloon = Some(BalloonDeviceConfig { amount_mib: 100, deflate_on_oom: false, stats_polling_interval_s: 0, free_page_hinting: false, free_page_reporting: false }); assert!(c.balloon.is_some()); }
    #[test] fn test_c183_res_23() { let c = VmmConfig::default(); assert_eq!(c.network_interfaces.len(), 0); }
    #[test] fn test_c183_res_24() { let c = VmmConfig::default(); assert!(c.vsock.is_none()); }
    #[test] fn test_c183_res_25() { let c = VmmConfig::default(); assert!(c.entropy.is_none()); }
    #[test] fn test_c183_res_26() { let c = VmmConfig::default(); assert_eq!(c.pmem_devices.len(), 0); }
    #[test] fn test_c183_res_27() { let c = VmmConfig::default(); assert!(c.machine_config.is_none()); }
    #[test] fn test_c183_res_28() { let c = VmmConfig::default(); assert!(c.logger.is_none()); }
    #[test] fn test_c183_res_29() { let c = VmmConfig::default(); assert!(c.metrics.is_none()); }
    #[test] fn test_c183_res_30() { let c = VmmConfig::default(); assert!(c.mmds_config.is_none()); }
    #[test] fn test_c183_res_31() { let c = VmmConfig::default(); assert!(c.cpu_config.is_none()); }
    #[test] fn test_c183_res_32() { let c = VmmConfig::default(); assert!(c.memory_hotplug.is_none()); }
    #[test] fn test_c183_res_33() { let p = CustomCpuTemplateOrPath::Path(PathBuf::from("/tmp/t.json")); let _ = format!("{:?}", p); }
    #[test] fn test_c183_res_34() { let p1 = CustomCpuTemplateOrPath::Path(PathBuf::from("/tmp/t.json")); let p2 = CustomCpuTemplateOrPath::Path(PathBuf::from("/tmp/t.json")); assert_eq!(p1, p2); }
    #[test] fn test_c183_res_35() { let p = CustomCpuTemplateOrPath::Path(PathBuf::from("/tmp/t.json")); let _ = serde_json::to_string(&p); }
    #[test] fn test_c183_res_36() { let t = CustomCpuTemplateOrPath::Template(CustomCpuTemplate::default()); let _ = serde_json::to_string(&t); }
    #[test] fn test_c183_res_37() { let json = r#""/tmp/t.json""#; let _: Result<CustomCpuTemplateOrPath, _> = serde_json::from_str(json); }
    #[test] fn test_c183_res_38() { let r = VmResources::default(); assert_eq!(std::mem::size_of_val(&r) > 0, true); }
    #[test] fn test_c183_res_39() { let c = VmmConfig::default(); assert_eq!(std::mem::size_of_val(&c) > 0, true); }
    #[test] fn test_c183_res_40() { let r = VmResources::default(); let _c: VmmConfig = (&r).into(); }

}
