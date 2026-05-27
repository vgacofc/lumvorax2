/**
 * C279: Structures GPU Gen9 Intel
 * Référence: Intel compute-runtime + PRM Gen9
 */

#ifndef BTC_GEN9_STRUCTURES_H
#define BTC_GEN9_STRUCTURES_H

#include <stdint.h>

/* Gen9 Surface State (64 bytes) - Simplifié pour buffer RAW */
struct gen9_surface_state {
    uint32_t dw0;   /* Surface Type + Format */
    uint32_t dw1;   /* Base Address Low */
    uint32_t dw2;   /* Width */
    uint32_t dw3;   /* Height + Pitch */
    uint32_t dw4;   /* Depth + Min LOD */
    uint32_t dw5;   /* Mip Count + Min Array Element */
    uint32_t dw6;   /* Render Target View Extent */
    uint32_t dw7;   /* Resource Min LOD */
    uint32_t dw8;   /* Base Address High */
    uint32_t dw9;   /* Reserved */
    uint32_t dw10;  /* Reserved */
    uint32_t dw11;  /* Reserved */
    uint32_t dw12;  /* Reserved */
    uint32_t dw13;  /* Reserved */
    uint32_t dw14;  /* Reserved */
    uint32_t dw15;  /* Reserved */
} __attribute__((packed));

/* Gen9 Interface Descriptor (32 bytes) */
struct gen9_interface_descriptor {
    uint32_t dw0;   /* Kernel Start Pointer Low */
    uint32_t dw1;   /* Kernel Start Pointer High */
    uint32_t dw2;   /* Reserved */
    uint32_t dw3;   /* Sampler State Pointer */
    uint32_t dw4;   /* Binding Table Pointer */
    uint32_t dw5;   /* Constant URB Entry Read Length */
    uint32_t dw6;   /* Number of Threads + Barrier Enable */
    uint32_t dw7;   /* Shared Local Memory Size */
} __attribute__((packed));

/* Surface Type */
#define GEN9_SURFACE_BUFFER  4

/* Surface Format RAW (pas de format spécifique) */
#define GEN9_SURFACEFORMAT_RAW  0x1FF

#endif /* BTC_GEN9_STRUCTURES_H */

// Made with Bob
