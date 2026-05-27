// C406: Kernel NATIF Gen9 ISA minimal
// SEND descriptor: 0x04025E00 (stateful, BTI=0)

// Charger thread ID
mov (8|M0) r1.0<1>:ud r0.0<8;8,1>:ud

// nonce = thread_id
mov (1|M0) r2.0<1>:ud r1.0<0;1,0>:ud

// hash = nonce * 0xDEADBEEF (test simple)
mov (1|M0) r10.0<1>:ud 0xDEADBEEF:ud
mul (1|M0) r11.0<1>:ud r2.0<0;1,0>:ud r10.0<0;1,0>:ud

// leading_zeros = 4 (fixe pour test)
mov (1|M0) r12.0<1>:ud 4:ud

// result = (leading_zeros << 16) | nonce
shl (1|M0) r13.0<1>:ud r12.0<0;1,0>:ud 16:w
or (1|M0) r14.0<1>:ud r13.0<0;1,0>:ud r2.0<0;1,0>:ud

// Vectoriser SIMD16
mov (16|M0) r20.0<1>:ud r14.0<0;1,0>:ud

// Adresse: thread_id * 4
shl (16|M0) r30.0<1>:ud r1.0<8;8,1>:ud 2:w

// SEND STATEFUL 0x04025E00
sends (16|M0) null:w r30 r20 0x8C 0x04025E00

// EOT
send (1|M0) null r127 0x27 0x02000010 {EOT}
