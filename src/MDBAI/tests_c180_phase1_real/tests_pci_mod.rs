// Tests pour pci/mod.rs (10 tests réels)
#[cfg(test)]
mod tests_c180_pci {
    use super::*;

    #[test]
    fn test_pci_sbdf_default() {
        // Test PciSBDF::default()
        let sbdf = PciSBDF::default();
        assert_eq!(sbdf.segment(), 0);
        assert_eq!(sbdf.bus(), 0);
        assert_eq!(sbdf.device(), 0);
        assert_eq!(sbdf.function(), 0);
    }

    #[test]
    fn test_pci_sbdf_segment_extraction() {
        // Test extraction segment depuis u32
        let value: u32 = 0x0001_0000; // segment = 1
        let sbdf = PciSBDF(value);
        assert_eq!(sbdf.segment(), 1);
    }

    #[test]
    fn test_pci_sbdf_bus_extraction() {
        // Test extraction bus depuis u32
        let value: u32 = 0x0000_FF00; // bus = 255
        let sbdf = PciSBDF(value);
        assert_eq!(sbdf.bus(), 255);
    }

    #[test]
    fn test_pci_sbdf_device_extraction() {
        // Test extraction device depuis u32
        let value: u32 = 0x0000_00F8; // device = 31 (5 bits)
        let sbdf = PciSBDF(value);
        assert_eq!(sbdf.device(), 31);
    }

    #[test]
    fn test_pci_sbdf_function_extraction() {
        // Test extraction function depuis u32
        let value: u32 = 0x0000_0007; // function = 7 (3 bits)
        let sbdf = PciSBDF(value);
        assert_eq!(sbdf.function(), 7);
    }

    #[test]
    fn test_pci_sbdf_combined() {
        // Test extraction combinée de tous les champs
        let value: u32 = 0x0001_FF1F; // segment=1, bus=255, device=3, function=7
        let sbdf = PciSBDF(value);
        assert_eq!(sbdf.segment(), 1);
        assert_eq!(sbdf.bus(), 255);
        assert_eq!(sbdf.device(), 3);
        assert_eq!(sbdf.function(), 7);
    }

    #[test]
    fn test_pci_sbdf_clone() {
        // Test que PciSBDF est Clone
        let sbdf1 = PciSBDF(0x1234);
        let sbdf2 = sbdf1.clone();
        assert_eq!(sbdf1.segment(), sbdf2.segment());
        assert_eq!(sbdf1.bus(), sbdf2.bus());
    }

    #[test]
    fn test_pci_sbdf_copy() {
        // Test que PciSBDF est Copy
        let sbdf1 = PciSBDF(0x5678);
        let sbdf2 = sbdf1; // Copy, pas move
        assert_eq!(sbdf1.segment(), sbdf2.segment());
    }

    #[test]
    fn test_pci_sbdf_partial_eq() {
        // Test que PciSBDF implémente PartialEq
        let sbdf1 = PciSBDF(0xABCD);
        let sbdf2 = PciSBDF(0xABCD);
        let sbdf3 = PciSBDF(0x1234);
        assert_eq!(sbdf1, sbdf2);
        assert_ne!(sbdf1, sbdf3);
    }

    #[test]
    fn test_pci_sbdf_eq() {
        // Test que PciSBDF implémente Eq
        let sbdf1 = PciSBDF(0xFFFF);
        let sbdf2 = PciSBDF(0xFFFF);
        assert!(sbdf1 == sbdf2);
    }
}
