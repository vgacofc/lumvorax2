// C424: Kernel SIMD1 MINIMAL - 1 thread, 1 lane, 1 write
// Objectif: Valider dataport write fonctionne RÉELLEMENT
// Basé sur Intel PRM Vol 2d Command Reference: Structures - Message Descriptor

// Hardcoder valeur test: 0xDEADBEEF
mov (1|M0) r10.0<1>:ud 0xDEADBEEF:ud

// Préparer header SEND (requis pour untyped surface write)
// r30 = header (1 GRF)
// DW0 = offset en bytes (0)
mov (1|M0) r30.0<1>:ud 0x00000000:ud

// Préparer data payload
// r20 = data (1 GRF minimum)
mov (1|M0) r20.0<1>:ud r10.0<0;1,0>:ud

// SEND untyped surface write
// Format: send (exec_size) dst src0 ex_desc desc
// exec_size = 1 (SIMD1)
// dst = null (pas de retour)
// src0 = r30 (header)
// ex_desc = 0x0 (pas d'extended descriptor pour untyped)
// desc = Message Descriptor pour untyped surface write:
//   [31:29] = 000 (reserved)
//   [28:25] = 1101 (SFID = Data Port Data Cache, 0xD)
//   [24:20] = 00001 (message length = 1 GRF header)
//   [19:14] = 000000 (reserved)
//   [13:12] = 00 (SIMD mode = SIMD8, mais 1 lane active)
//   [11:8]  = 1101 (message type = untyped surface write, 0xD)
//   [7:0]   = 00000001 (binding table index = 1)
// Descriptor = 0x0D010D01
send (1|M0) null:ud r30 0x0 0x0D010D01
