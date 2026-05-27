// C419: Kernel BARE-METAL Gen9 ISA - AUCUNE dépendance runtime
// Objectif: Valider pipeline GPU natif sans ABI OpenCL
// 
// Ce kernel:
// - N'utilise PAS get_global_id()
// - N'utilise PAS d'arguments kernel
// - N'utilise PAS de CURBE
// - N'utilise PAS de payload OpenCL
// - Écrit valeur HARDCODÉE à offset HARDCODÉ
//
// Si ce kernel produit des résultats:
// -> Pipeline GPU natif est VALIDÉ
// -> Problème C411-C418 était bien l'ABI runtime

// Hardcoder valeur magique: 0xDEADBEEF
mov (1|M0) r10.0<1>:ud 0xDEADBEEF:ud

// Préparer adresse: offset 0 (hardcodé)
// Pour SIMD16, on écrit 16 valeurs consécutives
mov (16|M0) r30.0<1>:ud 0:ud

// Préparer data: 16x la même valeur 0xDEADBEEF
mov (16|M0) r20.0<1>:ud r10.0<0;1,0>:ud

// SEND STATEFUL avec descriptor 0x04025E00
// Format: sends (exec_size) dst src0 src1 ex_desc desc
// - src0 = r30 (adresses)
// - src1 = r20 (data)
// - ex_desc = 0x8C (extended descriptor)
// - desc = 0x04025E00 (message descriptor)
sends (16|M0) null:w r30 r20 0x8C 0x04025E00

// EOT (End of Thread)
send (1|M0) null r127 0x27 0x02000010 {EOT}

