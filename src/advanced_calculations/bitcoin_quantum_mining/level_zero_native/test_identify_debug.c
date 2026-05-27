#include <stdio.h>
#include <stdint.h>

uint32_t extract_bits(uint32_t value, uint32_t start, uint32_t end) {
    uint32_t mask = ((1U << (end - start + 1)) - 1) << start;
    return (value & mask) >> start;
}

int main() {
    uint32_t opcodes[] = {
        0x71000002,  // MEDIA_CURBE_LOAD
        0x72000002,  // MEDIA_INTERFACE_DESCRIPTOR_LOAD
        0x7A05000D,  // GPGPU_WALKER
        0x70040000,  // MEDIA_STATE_FLUSH
        0x70000007   // MEDIA_VFE_STATE (fonctionne)
    };
    
    for (int i = 0; i < 5; i++) {
        uint32_t opcode = opcodes[i];
        uint32_t client = extract_bits(opcode, 29, 31);
        uint32_t opcode_masked = (opcode >> 16) & 0x1FFF;
        
        printf("Opcode: 0x%08X\n", opcode);
        printf("  Client (bits 29-31): 0x%X (%u)\n", client, client);
        printf("  Opcode masked (bits 16-28): 0x%04X\n", opcode_masked);
        printf("\n");
    }
    
    return 0;
}
