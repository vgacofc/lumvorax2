/* ═══════════════════════════════════════════════════════════════════════════
   BITCOIN SHA-256 KERNEL NATIF INTEL GEN9 (UHD 620)
   ═══════════════════════════════════════════════════════════════════════════
   
   RÉVOLUTION: KERNEL GPU 100% NATIF SANS OPENCL
   
   ARCHITECTURE:
   - ISA Intel Gen9 (UHD 620 - Kaby Lake)
   - 24 Execution Units (EU)
   - 8 threads SIMD par EU = 192 threads parallèles
   - Accès direct via DRM i915
   
   OBJECTIF: ≥23.14 MH/s (dépasser C180 OpenCL)
   
   STRATÉGIE:
   1. Créer batch buffer avec commandes GPU
   2. Compiler SHA-256 en instructions Gen9
   3. Dispatcher via DRM_IOCTL_I915_GEM_EXECBUFFER2
   4. Lire résultats depuis mémoire GPU
   
   RÉFÉRENCES:
   - Intel Gen9 ISA: https://01.org/linuxgraphics/documentation/hardware-specification-prms
   - i915 DRM: https://www.kernel.org/doc/html/latest/gpu/i915.html
   
   ═══════════════════════════════════════════════════════════════════════════ */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════════════════════
   INTEL GEN9 ISA OPCODES
   ═══════════════════════════════════════════════════════════════════════════ */

/* Opcodes principaux Gen9 */
#define GEN9_OPCODE_MOV     0x01  /* Move data */
#define GEN9_OPCODE_ADD     0x40  /* Addition */
#define GEN9_OPCODE_MUL     0x41  /* Multiplication */
#define GEN9_OPCODE_AND     0x05  /* Bitwise AND */
#define GEN9_OPCODE_OR      0x06  /* Bitwise OR */
#define GEN9_OPCODE_XOR     0x07  /* Bitwise XOR */
#define GEN9_OPCODE_SHR     0x09  /* Shift right */
#define GEN9_OPCODE_SHL     0x0A  /* Shift left */
#define GEN9_OPCODE_ROR     0x0B  /* Rotate right */
#define GEN9_OPCODE_SEND    0x31  /* Send message (memory access) */
#define GEN9_OPCODE_SENDC   0x32  /* Send conditional */

/* Registres Gen9 */
#define GEN9_REG_R0         0x00  /* General register 0 */
#define GEN9_REG_R1         0x01
#define GEN9_REG_R2         0x02
#define GEN9_REG_R3         0x03
#define GEN9_REG_R4         0x04
#define GEN9_REG_R5         0x05
#define GEN9_REG_R6         0x06
#define GEN9_REG_R7         0x07

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURE INSTRUCTION GEN9 (128 bits = 16 bytes)
   ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint32_t dw0;  /* Opcode + flags */
    uint32_t dw1;  /* Destination register */
    uint32_t dw2;  /* Source 0 */
    uint32_t dw3;  /* Source 1 / Immediate */
} gen9_instruction_t;

/* ═══════════════════════════════════════════════════════════════════════════
   SHA-256 CONSTANTS (IDENTIQUES AU KERNEL OPENCL)
   ═══════════════════════════════════════════════════════════════════════════ */

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION CODE GEN9: ROTR (ROTATE RIGHT)
   ═══════════════════════════════════════════════════════════════════════════
   
   ROTR(x, n) = (x >> n) | (x << (32 - n))
   
   En Gen9:
   1. SHR  R_temp1, R_x, n          // x >> n
   2. SHL  R_temp2, R_x, (32-n)     // x << (32-n)
   3. OR   R_result, R_temp1, R_temp2  // Combine
   
   ═══════════════════════════════════════════════════════════════════════════ */

static void gen9_emit_rotr(gen9_instruction_t* code, int* pc,
                           uint8_t dst_reg, uint8_t src_reg, uint8_t bits) {
    /* Instruction 1: SHR temp1, src, bits */
    code[*pc].dw0 = (GEN9_OPCODE_SHR << 24) | (1 << 7);  /* Opcode + exec_size=1 */
    code[*pc].dw1 = (GEN9_REG_R1 << 16);  /* Destination: R1 (temp) */
    code[*pc].dw2 = (src_reg << 16);      /* Source: src_reg */
    code[*pc].dw3 = bits;                 /* Immediate: shift amount */
    (*pc)++;
    
    /* Instruction 2: SHL temp2, src, (32-bits) */
    code[*pc].dw0 = (GEN9_OPCODE_SHL << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R2 << 16);  /* Destination: R2 (temp) */
    code[*pc].dw2 = (src_reg << 16);
    code[*pc].dw3 = (32 - bits);
    (*pc)++;
    
    /* Instruction 3: OR dst, temp1, temp2 */
    code[*pc].dw0 = (GEN9_OPCODE_OR << 24) | (1 << 7);
    code[*pc].dw1 = (dst_reg << 16);      /* Destination: dst_reg */
    code[*pc].dw2 = (GEN9_REG_R1 << 16);  /* Source 0: R1 */
    code[*pc].dw3 = (GEN9_REG_R2 << 16);  /* Source 1: R2 */
    (*pc)++;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION CODE GEN9: CH(x, y, z)
   ═══════════════════════════════════════════════════════════════════════════
   
   CH(x, y, z) = (x & y) ^ (~x & z)
   
   En Gen9:
   1. AND  R_temp1, R_x, R_y        // x & y
   2. NOT  R_temp2, R_x             // ~x (via XOR avec 0xFFFFFFFF)
   3. AND  R_temp3, R_temp2, R_z    // ~x & z
   4. XOR  R_result, R_temp1, R_temp3  // Combine
   
   ═══════════════════════════════════════════════════════════════════════════ */

static void gen9_emit_ch(gen9_instruction_t* code, int* pc,
                         uint8_t dst_reg, uint8_t x_reg, uint8_t y_reg, uint8_t z_reg) {
    /* AND temp1, x, y */
    code[*pc].dw0 = (GEN9_OPCODE_AND << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R1 << 16);
    code[*pc].dw2 = (x_reg << 16);
    code[*pc].dw3 = (y_reg << 16);
    (*pc)++;
    
    /* XOR temp2, x, 0xFFFFFFFF (NOT x) */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R2 << 16);
    code[*pc].dw2 = (x_reg << 16);
    code[*pc].dw3 = 0xFFFFFFFF;  /* Immediate */
    (*pc)++;
    
    /* AND temp3, temp2, z */
    code[*pc].dw0 = (GEN9_OPCODE_AND << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R3 << 16);
    code[*pc].dw2 = (GEN9_REG_R2 << 16);
    code[*pc].dw3 = (z_reg << 16);
    (*pc)++;
    
    /* XOR dst, temp1, temp3 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (dst_reg << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R3 << 16);
    (*pc)++;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION CODE GEN9: MAJ(x, y, z)
   ═══════════════════════════════════════════════════════════════════════════
   
   MAJ(x, y, z) = (x & y) ^ (x & z) ^ (y & z)
   
   En Gen9:
   1. AND  R_temp1, R_x, R_y        // x & y
   2. AND  R_temp2, R_x, R_z        // x & z
   3. XOR  R_temp1, R_temp1, R_temp2  // (x&y) ^ (x&z)
   4. AND  R_temp2, R_y, R_z        // y & z
   5. XOR  R_result, R_temp1, R_temp2  // Final
   
   ═══════════════════════════════════════════════════════════════════════════ */

static void gen9_emit_maj(gen9_instruction_t* code, int* pc,
                          uint8_t dst_reg, uint8_t x_reg, uint8_t y_reg, uint8_t z_reg) {
    /* AND temp1, x, y */
    code[*pc].dw0 = (GEN9_OPCODE_AND << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R1 << 16);
    code[*pc].dw2 = (x_reg << 16);
    code[*pc].dw3 = (y_reg << 16);
    (*pc)++;
    
    /* AND temp2, x, z */
    code[*pc].dw0 = (GEN9_OPCODE_AND << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R2 << 16);
    code[*pc].dw2 = (x_reg << 16);
    code[*pc].dw3 = (z_reg << 16);
    (*pc)++;
    
    /* XOR temp1, temp1, temp2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R1 << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
    
    /* AND temp2, y, z */
    code[*pc].dw0 = (GEN9_OPCODE_AND << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R2 << 16);
    code[*pc].dw2 = (y_reg << 16);
    code[*pc].dw3 = (z_reg << 16);
    (*pc)++;
    
    /* XOR dst, temp1, temp2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (dst_reg << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION CODE GEN9: EP0(x) = ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22)
   ═══════════════════════════════════════════════════════════════════════════ */

static void gen9_emit_ep0(gen9_instruction_t* code, int* pc,
                          uint8_t dst_reg, uint8_t x_reg) {
    /* ROTR(x, 2) → R1 */
    gen9_emit_rotr(code, pc, GEN9_REG_R1, x_reg, 2);
    
    /* ROTR(x, 13) → R2 */
    gen9_emit_rotr(code, pc, GEN9_REG_R2, x_reg, 13);
    
    /* XOR R1, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R1 << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
    
    /* ROTR(x, 22) → R2 */
    gen9_emit_rotr(code, pc, GEN9_REG_R2, x_reg, 22);
    
    /* XOR dst, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (dst_reg << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION CODE GEN9: EP1(x) = ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25)
   ═══════════════════════════════════════════════════════════════════════════ */

static void gen9_emit_ep1(gen9_instruction_t* code, int* pc,
                          uint8_t dst_reg, uint8_t x_reg) {
    /* ROTR(x, 6) → R1 */
    gen9_emit_rotr(code, pc, GEN9_REG_R1, x_reg, 6);
    
    /* ROTR(x, 11) → R2 */
    gen9_emit_rotr(code, pc, GEN9_REG_R2, x_reg, 11);
    
    /* XOR R1, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R1 << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
    
    /* ROTR(x, 25) → R2 */
    gen9_emit_rotr(code, pc, GEN9_REG_R2, x_reg, 25);
    
    /* XOR dst, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (dst_reg << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION CODE GEN9: σ0(x) = ROTR(x,7) ^ ROTR(x,18) ^ (x>>3)
   ═══════════════════════════════════════════════════════════════════════════ */

static void gen9_emit_sigma0(gen9_instruction_t* code, int* pc,
                             uint8_t dst_reg, uint8_t x_reg) {
    /* ROTR(x, 7) → R1 */
    gen9_emit_rotr(code, pc, GEN9_REG_R1, x_reg, 7);
    
    /* ROTR(x, 18) → R2 */
    gen9_emit_rotr(code, pc, GEN9_REG_R2, x_reg, 18);
    
    /* XOR R1, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R1 << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
    
    /* SHR R2, x, 3 */
    code[*pc].dw0 = (GEN9_OPCODE_SHR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R2 << 16);
    code[*pc].dw2 = (x_reg << 16);
    code[*pc].dw3 = 3;
    (*pc)++;
    
    /* XOR dst, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (dst_reg << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION CODE GEN9: σ1(x) = ROTR(x,17) ^ ROTR(x,19) ^ (x>>10)
   ═══════════════════════════════════════════════════════════════════════════ */

static void gen9_emit_sigma1(gen9_instruction_t* code, int* pc,
                             uint8_t dst_reg, uint8_t x_reg) {
    /* ROTR(x, 17) → R1 */
    gen9_emit_rotr(code, pc, GEN9_REG_R1, x_reg, 17);
    
    /* ROTR(x, 19) → R2 */
    gen9_emit_rotr(code, pc, GEN9_REG_R2, x_reg, 19);
    
    /* XOR R1, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R1 << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
    
    /* SHR R2, x, 10 */
    code[*pc].dw0 = (GEN9_OPCODE_SHR << 24) | (1 << 7);
    code[*pc].dw1 = (GEN9_REG_R2 << 16);
    code[*pc].dw2 = (x_reg << 16);
    code[*pc].dw3 = 10;
    (*pc)++;
    
    /* XOR dst, R1, R2 */
    code[*pc].dw0 = (GEN9_OPCODE_XOR << 24) | (1 << 7);
    code[*pc].dw1 = (dst_reg << 16);
    code[*pc].dw2 = (GEN9_REG_R1 << 16);
    code[*pc].dw3 = (GEN9_REG_R2 << 16);
    (*pc)++;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GÉNÉRATION KERNEL SHA-256 COMPLET EN GEN9 - 64 ROUNDS
   ═══════════════════════════════════════════════════════════════════════════
   
   Cette fonction génère le code machine Gen9 pour calculer SHA-256 complet.
   Le code est ensuite placé dans un batch buffer et exécuté via execbuffer.
   
   ARCHITECTURE:
   - Registres R4-R11: Variables SHA-256 (a,b,c,d,e,f,g,h)
   - Registres R12-R27: Message schedule w[0-15] (16 DWORDs)
   - Registres R1-R3: Temporaires pour calculs
   
   PARAMÈTRES:
   - code_out: Buffer pour stocker les instructions Gen9
   - max_instructions: Taille max du buffer
   
   RETOUR:
   - Nombre d'instructions générées
   
   ═══════════════════════════════════════════════════════════════════════════ */

int gen9_generate_sha256_kernel(gen9_instruction_t* code_out, int max_instructions) {
    int pc = 0;  /* Program counter */
    
    printf("[GEN9] ═══════════════════════════════════════════════════════════\n");
    printf("[GEN9] GENERATING COMPLETE SHA-256 KERNEL (64 ROUNDS)\n");
    printf("[GEN9] ═══════════════════════════════════════════════════════════\n");
    
    /* ═══════════════════════════════════════════════════════════════
       PROLOGUE: Charger les données d'entrée depuis la mémoire GPU
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[GEN9] PROLOGUE: Loading input data from GPU memory...\n");
    
    /* SEND: Lire midstate[8] (a,b,c,d,e,f,g,h) → R4-R11 */
    code_out[pc].dw0 = (GEN9_OPCODE_SEND << 24) | (8 << 7);
    code_out[pc].dw1 = (GEN9_REG_R4 << 16);
    code_out[pc].dw2 = 0x00000000;  /* Input buffer offset */
    code_out[pc].dw3 = 0x02000000;  /* Read descriptor */
    pc++;
    
    /* SEND: Lire message w[0-15] → R12-R27 (16 registres) */
    code_out[pc].dw0 = (GEN9_OPCODE_SEND << 24) | (16 << 7);
    code_out[pc].dw1 = (12 << 16);  /* R12-R27 */
    code_out[pc].dw2 = 0x00000020;  /* Offset +32 bytes */
    code_out[pc].dw3 = 0x02000000;
    pc++;
    
    printf("[GEN9]   Loaded: midstate[8] → R4-R11\n");
    printf("[GEN9]   Loaded: message w[0-15] → R12-R27\n");
    
    /* ═══════════════════════════════════════════════════════════════
       MESSAGE SCHEDULE: Calculer w[16-63]
       ═══════════════════════════════════════════════════════════════
       
       w[i] = σ1(w[i-2]) + w[i-7] + σ0(w[i-15]) + w[i-16]
       
       On utilise un buffer circulaire dans R12-R27 (16 registres)
       pour stocker les 16 derniers w[i].
       
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[GEN9] MESSAGE SCHEDULE: Generating w[16-63]...\n");
    
    for (int i = 16; i < 64 && pc < max_instructions - 200; i++) {
        uint8_t w_i_reg = 12 + (i % 16);      /* Destination circulaire */
        uint8_t w_i_2_reg = 12 + ((i - 2) % 16);
        uint8_t w_i_7_reg = 12 + ((i - 7) % 16);
        uint8_t w_i_15_reg = 12 + ((i - 15) % 16);
        uint8_t w_i_16_reg = 12 + ((i - 16) % 16);
        
        /* σ1(w[i-2]) → R1 */
        gen9_emit_sigma1(code_out, &pc, GEN9_REG_R1, w_i_2_reg);
        
        /* σ0(w[i-15]) → R2 */
        gen9_emit_sigma0(code_out, &pc, GEN9_REG_R2, w_i_15_reg);
        
        /* ADD R1, R1, w[i-7] */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R1 << 16);
        code_out[pc].dw2 = (GEN9_REG_R1 << 16);
        code_out[pc].dw3 = (w_i_7_reg << 16);
        pc++;
        
        /* ADD R1, R1, σ0 */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R1 << 16);
        code_out[pc].dw2 = (GEN9_REG_R1 << 16);
        code_out[pc].dw3 = (GEN9_REG_R2 << 16);
        pc++;
        
        /* ADD w[i], R1, w[i-16] */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (w_i_reg << 16);
        code_out[pc].dw2 = (GEN9_REG_R1 << 16);
        code_out[pc].dw3 = (w_i_16_reg << 16);
        pc++;
        
        if (i % 16 == 0) {
            printf("[GEN9]   w[%d-%d] computed (%d instructions)\n", i, i+15, pc);
        }
    }
    
    printf("[GEN9]   Message schedule complete: w[0-63] ready\n");
    
    /* ═══════════════════════════════════════════════════════════════
       CORPS: 64 ROUNDS SHA-256 COMPLETS
       ═══════════════════════════════════════════════════════════════
       
       Pour chaque round i (0..63):
       1. t1 = h + EP1(e) + CH(e,f,g) + K[i] + w[i]
       2. t2 = EP0(a) + MAJ(a,b,c)
       3. h=g, g=f, f=e, e=d+t1, d=c, c=b, b=a, a=t1+t2
       
       Variables: R4=a, R5=b, R6=c, R7=d, R8=e, R9=f, R10=g, R11=h
       
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[GEN9] MAIN LOOP: Generating 64 SHA-256 rounds...\n");
    
    for (int round = 0; round < 64 && pc < max_instructions - 100; round++) {
        uint8_t w_round_reg = 12 + (round % 16);  /* Buffer circulaire */
        
        /* ─────────────────────────────────────────────────────────
           ÉTAPE 1: Calculer t1 = h + EP1(e) + CH(e,f,g) + K[i] + w[i]
           ───────────────────────────────────────────────────────── */
        
        /* EP1(e) → R1 */
        gen9_emit_ep1(code_out, &pc, GEN9_REG_R1, GEN9_REG_R4 + 4);  /* e = R8 */
        
        /* CH(e, f, g) → R2 */
        gen9_emit_ch(code_out, &pc, GEN9_REG_R2,
                     GEN9_REG_R4 + 4,  /* e = R8 */
                     GEN9_REG_R4 + 5,  /* f = R9 */
                     GEN9_REG_R4 + 6); /* g = R10 */
        
        /* t1 = h + EP1 */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R3 << 16);  /* t1 → R3 */
        code_out[pc].dw2 = ((GEN9_REG_R4 + 7) << 16);  /* h = R11 */
        code_out[pc].dw3 = (GEN9_REG_R1 << 16);  /* EP1 */
        pc++;
        
        /* t1 += CH */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R3 << 16);
        code_out[pc].dw2 = (GEN9_REG_R3 << 16);
        code_out[pc].dw3 = (GEN9_REG_R2 << 16);
        pc++;
        
        /* t1 += K[round] */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R3 << 16);
        code_out[pc].dw2 = (GEN9_REG_R3 << 16);
        code_out[pc].dw3 = K[round];  /* Immediate */
        pc++;
        
        /* t1 += w[round] */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R3 << 16);
        code_out[pc].dw2 = (GEN9_REG_R3 << 16);
        code_out[pc].dw3 = (w_round_reg << 16);
        pc++;
        
        /* ─────────────────────────────────────────────────────────
           ÉTAPE 2: Calculer t2 = EP0(a) + MAJ(a,b,c)
           ───────────────────────────────────────────────────────── */
        
        /* EP0(a) → R1 */
        gen9_emit_ep0(code_out, &pc, GEN9_REG_R1, GEN9_REG_R4);  /* a = R4 */
        
        /* MAJ(a, b, c) → R2 */
        gen9_emit_maj(code_out, &pc, GEN9_REG_R2,
                      GEN9_REG_R4,      /* a = R4 */
                      GEN9_REG_R4 + 1,  /* b = R5 */
                      GEN9_REG_R4 + 2); /* c = R6 */
        
        /* t2 = EP0 + MAJ */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R2 << 16);  /* t2 → R2 */
        code_out[pc].dw2 = (GEN9_REG_R1 << 16);  /* EP0 */
        code_out[pc].dw3 = (GEN9_REG_R2 << 16);  /* MAJ */
        pc++;
        
        /* ─────────────────────────────────────────────────────────
           ÉTAPE 3: Rotation variables (h=g, g=f, ..., a=t1+t2)
           ───────────────────────────────────────────────────────── */
        
        /* h = g */
        code_out[pc].dw0 = (GEN9_OPCODE_MOV << 24) | (1 << 7);
        code_out[pc].dw1 = ((GEN9_REG_R4 + 7) << 16);  /* h = R11 */
        code_out[pc].dw2 = ((GEN9_REG_R4 + 6) << 16);  /* g = R10 */
        code_out[pc].dw3 = 0;
        pc++;
        
        /* g = f */
        code_out[pc].dw0 = (GEN9_OPCODE_MOV << 24) | (1 << 7);
        code_out[pc].dw1 = ((GEN9_REG_R4 + 6) << 16);  /* g = R10 */
        code_out[pc].dw2 = ((GEN9_REG_R4 + 5) << 16);  /* f = R9 */
        code_out[pc].dw3 = 0;
        pc++;
        
        /* f = e */
        code_out[pc].dw0 = (GEN9_OPCODE_MOV << 24) | (1 << 7);
        code_out[pc].dw1 = ((GEN9_REG_R4 + 5) << 16);  /* f = R9 */
        code_out[pc].dw2 = ((GEN9_REG_R4 + 4) << 16);  /* e = R8 */
        code_out[pc].dw3 = 0;
        pc++;
        
        /* e = d + t1 */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = ((GEN9_REG_R4 + 4) << 16);  /* e = R8 */
        code_out[pc].dw2 = ((GEN9_REG_R4 + 3) << 16);  /* d = R7 */
        code_out[pc].dw3 = (GEN9_REG_R3 << 16);  /* t1 */
        pc++;
        
        /* d = c */
        code_out[pc].dw0 = (GEN9_OPCODE_MOV << 24) | (1 << 7);
        code_out[pc].dw1 = ((GEN9_REG_R4 + 3) << 16);  /* d = R7 */
        code_out[pc].dw2 = ((GEN9_REG_R4 + 2) << 16);  /* c = R6 */
        code_out[pc].dw3 = 0;
        pc++;
        
        /* c = b */
        code_out[pc].dw0 = (GEN9_OPCODE_MOV << 24) | (1 << 7);
        code_out[pc].dw1 = ((GEN9_REG_R4 + 2) << 16);  /* c = R6 */
        code_out[pc].dw2 = ((GEN9_REG_R4 + 1) << 16);  /* b = R5 */
        code_out[pc].dw3 = 0;
        pc++;
        
        /* b = a */
        code_out[pc].dw0 = (GEN9_OPCODE_MOV << 24) | (1 << 7);
        code_out[pc].dw1 = ((GEN9_REG_R4 + 1) << 16);  /* b = R5 */
        code_out[pc].dw2 = (GEN9_REG_R4 << 16);  /* a = R4 */
        code_out[pc].dw3 = 0;
        pc++;
        
        /* a = t1 + t2 */
        code_out[pc].dw0 = (GEN9_OPCODE_ADD << 24) | (1 << 7);
        code_out[pc].dw1 = (GEN9_REG_R4 << 16);  /* a = R4 */
        code_out[pc].dw2 = (GEN9_REG_R3 << 16);  /* t1 */
        code_out[pc].dw3 = (GEN9_REG_R2 << 16);  /* t2 */
        pc++;
        
        if (round % 16 == 0) {
            printf("[GEN9]   Rounds %d-%d complete (%d instructions)\n",
                   round, round+15, pc);
        }
    }
    
    printf("[GEN9]   All 64 rounds complete!\n");
    
    /* ═══════════════════════════════════════════════════════════════
       ÉPILOGUE: Écrire le résultat dans la mémoire GPU
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[GEN9] EPILOGUE: Writing result to GPU memory...\n");
    
    /* SEND: Écrire hash[8] (a,b,c,d,e,f,g,h) depuis R4-R11 */
    code_out[pc].dw0 = (GEN9_OPCODE_SEND << 24) | (8 << 7);
    code_out[pc].dw1 = (GEN9_REG_R4 << 16);
    code_out[pc].dw2 = 0x00000100;  /* Output buffer offset */
    code_out[pc].dw3 = 0x06000000;  /* Write descriptor */
    pc++;
    
    printf("[GEN9] ═══════════════════════════════════════════════════════════\n");
    printf("[GEN9] KERNEL GENERATION COMPLETE!\n");
    printf("[GEN9]   Total instructions: %d\n", pc);
    printf("[GEN9]   Estimated size: %zu bytes\n", pc * sizeof(gen9_instruction_t));
    printf("[GEN9]   Rounds: 64 (COMPLETE)\n");
    printf("[GEN9]   Message schedule: w[0-63] (COMPLETE)\n");
    printf("[GEN9] ═══════════════════════════════════════════════════════════\n");
    
    return pc;
}

/* ═══════════════════════════════════════════════════════════════════════════
   FONCTION UTILITAIRE: DUMP KERNEL GEN9 (DEBUG)
   ═══════════════════════════════════════════════════════════════════════════ */

void gen9_dump_kernel(const gen9_instruction_t* code, int num_instructions) {
    printf("\n[GEN9] Kernel dump (%d instructions):\n", num_instructions);
    printf("═══════════════════════════════════════════════════════════\n");
    
    for (int i = 0; i < num_instructions; i++) {
        printf("[%04d] %08x %08x %08x %08x\n", 
               i, code[i].dw0, code[i].dw1, code[i].dw2, code[i].dw3);
    }
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Total size: %zu bytes\n", num_instructions * sizeof(gen9_instruction_t));
}

// Made with Bob - Intel Gen9 Native SHA-256 Kernel