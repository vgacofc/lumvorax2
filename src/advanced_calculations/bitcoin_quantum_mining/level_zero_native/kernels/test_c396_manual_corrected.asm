// Kernel C396 - SEND Manuel Corrigé (INDÉPENDANT OpenCL)
// Objectif: Corriger TOUS les facteurs identifiés en C395
// Basé sur: Documentation IGC + Analyse ISA OpenCL

// CORRECTIONS APPLIQUÉES:
// 1. SIMD1 → SIMD8 (minimum pour untyped surface write)
// 2. Payload scalaire → Payload vectorisé
// 3. Message type 0x1A (A64 scatter) → 0x09 (untyped surface write)
// 4. Registres r4+r6 → r5+r11 (convention OpenCL)
// 5. Message length aligné avec registres utilisés

.kernel test_c396_manual_corrected
.arch_gen9
.default_execution_size(8)
.default_register_type(:ud)

// Registres système
.decl V0000 v_type=G type=ud num_elts=8 alias=<r0, 0>
.decl V0001 v_type=G type=ud num_elts=8 alias=<r1, 0>
.decl V0002 v_type=G type=ud num_elts=8 alias=<r2, 0>
.decl V0003 v_type=G type=uq num_elts=1 alias=<r3, 0>  // CURBE: adresse output

// Registres de travail
.decl V0004 v_type=G type=ud num_elts=8 alias=<r4, 0>
.decl V0005 v_type=G type=ud num_elts=8 alias=<r5, 0>  // Address pour SEND
.decl V0011 v_type=G type=ud num_elts=8 alias=<r11, 0> // Payload pour SEND
.decl V0127 v_type=G type=ud num_elts=8 alias=<r127, 0> // EOT

.code

// Copier thread state dans r2
mov (8) r2.0<1>:ud r0.0<1;1,0>:ud

// Activer SEND capability
or (1) cr0.0<1>:ud cr0.0<0;1,0>:ud 0x4C0:uw

// Calculer GID (pour SIMD8, on a 8 threads)
// r1.0 contient local_id_x pour chaque thread
mov (8) r4.0<1>:ud r1.0<8;8,1>:uw

// Préparer adresse pour SEND (r5)
// Pour untyped surface write, on utilise offset en DWORDs
// offset = gid * 4 (bytes) = gid << 2
shl (8) r5.0<1>:ud r4.0<8;8,1>:ud 2:w

// Préparer payload vectorisé (r11)
// Chaque thread écrit 0xDEADBEEF
mov (8) r11.0<1>:ud 0xDEADBEEF:ud

// Sauvegarder r2 pour EOT
mov (8) r127.0<1>:ud r2.0<8;8,1>:ud

// ⭐ SEND CORRIGÉ - Untyped Surface Write
// Message Descriptor: 0x04025E09
//   [18:14] = 0x09 (UNTYPED_SURFACE_WRITE pour Gen7.5+)
//   [13:12] = 0x3 (DC1 category)
//   [11:8]  = 0x1 (SIMD mode - voir EU_DATA_PORT_UNTYPED_SURFACE_SIMD_16)
//   [7:0]   = 0x00 (BTI = 0, Surface State 0)
//   [28:25] = 0x02 (Message length = 2 registres: r5-r6)
//   [24:19] = 0x00 (Response length = 0, write-only)
//
// Extended Descriptor: 0x8C
//   [3:0] = 0xC (SFID = DC1 = 12)
//   [7:4] = 0x8 (autres flags)

sends (8|M0) null<1>:ud r5 r11 0x8C 0x04025E09

// End of thread
send (1|M0) null r127 0x27 0x02000010 {EOT}

.end_code
.end_kernel

