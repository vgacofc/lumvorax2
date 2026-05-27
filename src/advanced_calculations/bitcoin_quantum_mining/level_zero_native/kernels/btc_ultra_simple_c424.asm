// C424: Kernel ULTRA-SIMPLE - 1 seul thread, 1 seule écriture
// Objectif: Valider que GPU peut écrire EN MÉMOIRE

// Hardcoder valeur: 0xDEADBEEF
mov (1|M0) r10.0<1>:ud 0xDEADBEEF:ud

// Préparer adresse: offset 0
mov (1|M0) r30.0<1>:ud 0:ud

// Préparer data
mov (1|M0) r20.0<1>:ud r10.0<0;1,0>:ud

// SEND avec SIMD8 (plus simple que SIMD16)
// Descriptor 0x04020100: SIMD8, untyped surface write
// CORRECTION: utiliser 'sends' au lieu de 'send' pour Gen9
sends (8|M0) null:w r30 r20 0x8C 0x04020100

// EOT
send (1|M0) null r127 0x27 0x02000010 {EOT}

; Made with Bob
