/*
 * INTEL STRUCTS MINIMAL - Structures Gen8 uniquement
 * 
 * Extrait de Intel Beignet intel_structs.h
 * License: LGPL 2.1
 * 
 * Contient UNIQUEMENT gen8_interface_descriptor_t pour Gen9
 * SANS macros BITFIELD problématiques
 */

#ifndef __INTEL_STRUCTS_MINIMAL_H__
#define __INTEL_STRUCTS_MINIMAL_H__

#include <stdint.h>

/* Gen8 Interface Descriptor (8 DWORDs = 32 bytes) - Utilisé pour Gen9 */
typedef struct gen8_interface_descriptor {
  uint32_t dw0;  /* Kernel Start Pointer [31:6] */
  uint32_t dw1;  /* Reserved */
  uint32_t dw2;  /* Reserved */
  uint32_t dw3;  /* Sampler State Pointer [31:5], Sampler Count [4:2] */
  uint32_t dw4;  /* Binding Table Pointer [31:5], Binding Table Entry Count [4:0] */
  uint32_t dw5;  /* CURBE Read Offset [31:16], CURBE Read Length [15:0] */
  uint32_t dw6;  /* Barrier Enable [31], Number of Threads [29:24], Shared Local Memory Size [20:16] */
  uint32_t dw7;  /* Cross-Thread Constant Data Read Length [7:0] */
} gen8_interface_descriptor_t;

#endif /* __INTEL_STRUCTS_MINIMAL_H__ */

// Made with Bob
