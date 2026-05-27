// TEST C441 - KERNEL GEN9 ISA ASSEMBLEUR MINIMAL
// Écrire 0x12345678 dans output[0]
// Entrée: r8.0-r8.1 = adresse 64-bit du buffer

L0:
    // Copier adresse buffer dans r12
    (W) mov (1|M0) r12.0<1>:uq  r8.0<0;1,0>:uq
    
    // Valeur à écrire dans r14
    (W) mov (1|M0) r14.0<1>:ud  0x12345678:ud
    
    // Écriture A64 scatter
    (W) sends (1|M0) null:ud  r12  r14  0x4C  0x040681FF
    
    // End Of Thread
    (W) mov (8|M0) r127.0<1>:ud  r0.0<8;8,1>:ud
    (W) send (1|M0) null  r127  0x27  0x02000010  {EOT}

; Made with Bob
