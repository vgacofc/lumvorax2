/* ═══════════════════════════════════════════════════════════════════════════
   BITCOIN SHA-256 KERNEL NATIF INTEL GEN9 - HEADER
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef BTC_SHA256_KERNEL_GEN9_NATIVE_H
#define BTC_SHA256_KERNEL_GEN9_NATIVE_H

#include <stdint.h>

/* Structure instruction Gen9 (128 bits) */
typedef struct {
    uint32_t dw0;  /* Opcode + flags */
    uint32_t dw1;  /* Destination register */
    uint32_t dw2;  /* Source 0 */
    uint32_t dw3;  /* Source 1 / Immediate */
} gen9_instruction_t;

/* Générer kernel SHA-256 en ISA Gen9 */
int gen9_generate_sha256_kernel(gen9_instruction_t* code_out, int max_instructions);

/* Dump kernel pour debug */
void gen9_dump_kernel(const gen9_instruction_t* code, int num_instructions);

#endif /* BTC_SHA256_KERNEL_GEN9_NATIVE_H */

// Made with Bob
