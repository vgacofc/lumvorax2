// Kernel C396: Test avec Message Descriptor OpenCL
// Objectif: Valider que descriptor 0x04025E00 préserve le payload
// Basé sur: Analyse comparative C395 OpenCL vs C391

.kernel test_c396_opencl_descriptor
.config
    .dims xyz
    .sgprsnum 16
    .vgprsnum 8
    .localsize 1,1,1
    .useargs
    .arg output, ulong*, global, const

.text
    // r0 = thread state (system values)
    // r3 = CURBE data (arguments from host)
    
    // Copier thread state dans r2
    mov (8) r2.0<1>:ud r0.0<1;1,0>:ud
    
    // Activer cr0 pour SEND
    or (1) cr0.0<1>:ud cr0.0<0;1,0>:ud 0x4C0:uw
    
    // Préparer adresse (r4) depuis CURBE
    // r3.0 contient l'adresse 64-bit du buffer output
    mov (1) r4.0<1>:uq r3.0<0;1,0>:uq
    
    // Préparer payload (r6) avec valeur magique
    mov (1) r6.0<1>:d 0xDEADBEEF:d
    
    // Sauvegarder r2 dans r127 pour EOT
    mov (8) r127.0<1>:ud r2.0<8;8,1>:ud
    
    // ⭐ SEND avec descriptor OpenCL (STATEFUL untyped surface write)
    // Message Descriptor: 0x04025E00
    //   - Message Type: 0x1E (untyped surface write)
    //   - Category: DC1 (0x3)
    //   - BTI: 0x00 (Surface State 0)
    //   - Message Length: 0x02 (r4-r5)
    //   - Response Length: 0x00 (write-only)
    sends (1) null<1>:ud r4 r6 0x8C 0x04025E00
    
    // End of thread
    send (1) null r127 0x27 0x02000010 {EOT}

.rodata
    // Metadata pour le compilateur
    .ascii "test_c396_opencl_descriptor"

; Made with Bob
