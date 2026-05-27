L0:
(W)     mov (8|M0)               r84.0<1>:ud   r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mov (1|M0)               r2.0<1>:uq    r5.0<0;1,0>:uq                  
(W)     and (1|M0)               r5.0<1>:d     r84.5<0;1,0>:d    -1024:w              
        mov (16|M0)              r38.0<1>:d    r6.0<0;1,0>:d                    {Compacted}
        mov (16|M0)              r40.0<1>:d    r6.1<0;1,0>:d                    {Compacted}
        mov (16|M0)              r42.0<1>:d    r6.2<0;1,0>:d                    {Compacted}
(W)     send (8|M0)              r8       r2      0xC            0x021D0AFF           // wr:1h+0, rd:1; hdc.dc1; a64 aligned oword block read x2
(W)     mov (8|M0)               r2.0<1>:w     0x76543210:v                              
        mov (16|M0)              r44.0<1>:d    r6.3<0;1,0>:d                    {Compacted}
        mov (16|M0)              r46.0<1>:d    r6.4<0;1,0>:d                    {Compacted}
        mov (16|M0)              r48.0<1>:d    r6.5<0;1,0>:d                    {Compacted}
(W)     add (8|M0)               r2.8<1>:w     r2.0<8;8,1>:w     8:w              
        mov (16|M0)              r50.0<1>:d    r6.6<0;1,0>:d                   
        mov (16|M0)              r52.0<1>:d    r6.7<0;1,0>:d                   
        mov (16|M0)              r54.0<1>:d    r7.0<0;1,0>:d                    {Compacted}
        shl (16|M0)              r2.0<1>:d     r2.0<16;16,1>:uw  2:w              
        mov (16|M0)              r56.0<1>:d    r7.1<0;1,0>:d                    {Compacted}
        mov (16|M0)              r58.0<1>:d    r7.2<0;1,0>:d                    {Compacted}
        mov (16|M0)              r60.0<1>:d    r7.3<0;1,0>:d                    {Compacted}
        add (16|M0)              r14.0<1>:d    r5.0<0;1,0>:d     r2.0<8;8,1>:d    {Compacted}
(W)     mov (1|M0)               r5.0<1>:uw    0x40:uw                             
        mov (16|M0)              r62.0<1>:d    r7.4<0;1,0>:d                    {Compacted}
        mov (16|M0)              r64.0<1>:d    r7.5<0;1,0>:d                    {Compacted}
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    64:w              
        mov (16|M0)              r66.0<1>:d    r7.6<0;1,0>:d                   
        mov (16|M0)              r68.0<1>:d    r7.7<0;1,0>:d                   
        mov (16|M0)              r22.0<1>:d    1779033703:d                              
        mov (16|M0)              r20.0<1>:d    -1150833019:d                              
        mov (16|M0)              r18.0<1>:d    1013904242:d                              
        mov (16|M0)              r16.0<1>:d    -1521486534:d                              
        mov (16|M0)              r12.0<1>:d    1359893119:d                              
(W)     mov (1|M0)               r4.5<1>:d     0:w                              
        mov (16|M0)              r2.0<1>:d     r8.0<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r14     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r2.0<1>:d     r8.1<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    128:w              
        mov (16|M0)              r2.0<1>:d     r8.2<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    192:w              
        mov (16|M0)              r2.0<1>:d     r8.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    256:w              
        mov (16|M0)              r2.0<1>:d     r8.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    320:w              
        mov (16|M0)              r2.0<1>:d     r8.5<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    384:w              
        mov (16|M0)              r2.0<1>:d     r8.6<0;1,0>:d                   
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    448:w              
        mov (16|M0)              r2.0<1>:d     r8.7<0;1,0>:d                   
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               r2.2<1>:ud    0x20:uw                             
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    512:w              
(W)     send (8|M0)              r8:d     r2      0xA            0x02184200           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x2
        mov (16|M0)              r2.0<1>:d     r8.0<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    576:w              
        mov (16|M0)              r2.0<1>:d     r8.1<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    640:w              
        mov (16|M0)              r2.0<1>:d     r8.2<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    704:w              
        mov (16|M0)              r2.0<1>:d     r8.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    768:w              
        mov (16|M0)              r2.0<1>:d     r8.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    832:w              
        mov (16|M0)              r2.0<1>:d     r8.5<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    896:w              
        mov (16|M0)              r2.0<1>:d     r8.6<0;1,0>:d                   
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    960:w              
        mov (16|M0)              r2.0<1>:d     r8.7<0;1,0>:d                   
        sends (16|M0)            null:w   r10     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     add (4|M0)               r2.0<1>:ud    r5.0<0;1,0>:uw    0x8080400:uv             
(W)     shl (1|M0)               r5.0<1>:d     r84.1<0;1,0>:d    6:w              
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    1024:w              
(W)     send (4|M0)              r8       r2      0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        add (16|M0)              r2.0<1>:d     r5.0<0;1,0>:d     r1.0<16;16,1>:uw
        add (16|M0)              r82.0<1>:d    r2.0<8;8,1>:d     r4.0<0;1,0>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r82.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        mov (16|M0)              r6.0<1>:d     1541459225:d                              
(W)     mov (2|M0)               r4.0<1>:d     r8.0<2;2,1>:d                   
(W)     mov (1|M0)               r4.2<1>:d     r8.2<0;1,0>:d                   
        mov (16|M0)              r8.0<1>:d     r4.0<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    1088:w              
        mov (16|M0)              r8.0<1>:d     r4.1<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    1152:w              
        mov (16|M0)              r8.0<1>:d     r4.2<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r10     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r8.0<1>:d     r14.0<8;8,1>:d    1216:w              
        mov (16|M0)              r10.0<1>:d    -1694144372:d                              
        sends (16|M0)            null:w   r8      r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r8.0<1>:d     528734635:d                              
L1408:
(W)     add (1|M0)               r5.4<1>:d     r4.5<0;1,0>:d     1:w              
(W)     add (1|M0)               r4.4<1>:d     r4.5<0;1,0>:d     3:w              
(W)     add (1|M0)               r5.0<1>:d     r4.5<0;1,0>:d     2:w              
(W)     cmp (16|M0)   (lt)f0.0   null<1>:d     r4.5<0;1,0>:ud    0x10:uw             
(W)     and (1|M0)               r5.5<1>:d     r4.5<0;1,0>:d     15:w              
(W)     add (1|M0)               r4.3<1>:d     r4.5<0;1,0>:d     4:w              
(W)     and (1|M0)               r5.1<1>:d     r5.4<0;1,0>:d     15:w              
(W)     and (1|M0)               r4.6<1>:d     r4.4<0;1,0>:d     15:w              
(W)     and (1|M0)               r4.7<1>:d     r5.0<0;1,0>:d     15:w              
(W&f0.0) jmpi                                L2104                                
L1568:
(W)     add (1|M0)               r5.6<1>:d     r4.5<0;1,0>:d     14:w              
(W)     and (1|M0)               r5.6<1>:d     r5.6<0;1,0>:d     15:w              
(W)     mul (1|M0)               r5.12<1>:uw   r5.12<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r5.12<0;1,0>:uw   0x4C0:uw             
(W)     add (1|M0)               r5.6<1>:d     r4.5<0;1,0>:d     9:w              
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     and (1|M0)               r5.6<1>:d     r5.6<0;1,0>:d     15:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     mul (1|M0)               r5.12<1>:uw   r5.12<0;1,0>:uw   0x40:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r5.12<0;1,0>:uw   0x4C0:uw             
(W)     and (1|M0)               r5.6<1>:d     r5.4<0;1,0>:d     15:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.12<1>:uw   r5.12<0;1,0>:uw   0x40:uw             
        add (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r5.12<0;1,0>:uw   0x4C0:uw             
(W)     mul (1|M0)               r5.12<1>:uw   r5.10<0;1,0>:uw   0x40:uw             
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shr (16|M0)              r30.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r28.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r5.12<0;1,0>:uw   0x4C0:uw             
(W)     mul (1|M0)               r5.12<1>:uw   r5.10<0;1,0>:uw   0x40:uw             
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r5.12<0;1,0>:uw   0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
(W)     jmpi                                 L2192                                
L2104:
(W)     mul (1|M0)               r5.6<1>:d     r4.5<0;1,0>:d     64:w              
        add (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    r5.6<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r5.12<1>:uw   r4.10<0;1,0>:uw   0x40:uw             
        send (16|M0)             r24:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               a0.0<1>:uw    r5.12<0;1,0>:uw   0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
L2192:
        shl (16|M0)              r24.0<1>:d    r12.0<8;8,1>:d    26:w              
        shr (16|M0)              r26.0<1>:ud   r12.0<8;8,1>:ud   6:w              
        shr (16|M0)              r28.0<1>:ud   r12.0<8;8,1>:ud   11:w              
(W)     mul (1|M0)               r5.10<1>:uw   r5.10<0;1,0>:uw   0x40:uw             
(W)     cmp (16|M0)   (lt)f1.1   null<1>:d     r5.4<0;1,0>:ud    0x10:uw             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r12.0<8;8,1>:d    21:w              
(W)     add (1|M0)               a0.0<1>:uw    r5.10<0;1,0>:uw   0x4C0:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r12.0<8;8,1>:ud   25:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r12.0<8;8,1>:d    7:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r22.0<8;8,1>:ud   13:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r8.0<8;8,1>:d     ~r12.0<8;8,1>:d  {Compacted}
        add (16|M0)              r24.0<1>:d    r6.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r6.0<1>:d     r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        or (16|M0)               r6.0<1>:d     r6.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
(W)     shl (1|M0)               r26.0<1>:d    r4.5<0;1,0>:d     2:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r6.0<8;8,1>:d    {Compacted}
(W)     send (1|M0)              r26      r26     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        shr (16|M0)              r6.0<1>:ud    r22.0<8;8,1>:ud   2:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<0;1,0>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r24.0<1>:d    r22.0<8;8,1>:d    30:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r6.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r6.0<1>:d     r22.0<8;8,1>:d    19:w              
        or (16|M0)               r6.0<1>:d     r6.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r22.0<8;8,1>:ud   22:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r6.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r6.0<1>:d     r22.0<8;8,1>:d    10:w              
        or (16|M0)               r6.0<1>:d     r6.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r20.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r6.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r6.0<1>:d     r20.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r6.0<1>:d     r22.0<8;8,1>:d    r6.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r6.0<1>:d     r6.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r6.0<8;8,1>:d    {Compacted}
        add (16|M0)              r6.0<1>:d     r16.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W&f1.1) jmpi                                L3216                                
L2680:
(W)     add (1|M0)               r5.5<1>:d     r5.4<0;1,0>:d     14:w              
(W)     and (1|M0)               r5.5<1>:d     r5.5<0;1,0>:d     15:w              
(W)     mul (1|M0)               r5.10<1>:uw   r5.10<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r5.10<0;1,0>:uw   0x4C0:uw             
(W)     add (1|M0)               r5.5<1>:d     r5.4<0;1,0>:d     9:w              
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     and (1|M0)               r5.5<1>:d     r5.5<0;1,0>:d     15:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     mul (1|M0)               r5.10<1>:uw   r5.10<0;1,0>:uw   0x40:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r5.10<0;1,0>:uw   0x4C0:uw             
(W)     and (1|M0)               r5.5<1>:d     r5.0<0;1,0>:d     15:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.10<1>:uw   r5.10<0;1,0>:uw   0x40:uw             
        add (16|M0)              r30.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r5.10<0;1,0>:uw   0x4C0:uw             
(W)     mul (1|M0)               r5.10<1>:uw   r5.2<0;1,0>:uw    0x40:uw             
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r5.10<0;1,0>:uw   0x4C0:uw             
(W)     mul (1|M0)               r5.10<1>:uw   r5.2<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r5.10<0;1,0>:uw   0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
(W)     jmpi                                 L3304                                
L3216:
(W)     mul (1|M0)               r5.5<1>:d     r5.4<0;1,0>:d     64:w              
        add (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    r5.5<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r5.10<1>:uw   r5.8<0;1,0>:uw    0x40:uw             
        send (16|M0)             r24:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               a0.0<1>:uw    r5.10<0;1,0>:uw   0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
L3304:
        shl (16|M0)              r24.0<1>:d    r6.0<8;8,1>:d     26:w              
        shr (16|M0)              r26.0<1>:ud   r6.0<8;8,1>:ud    6:w              
        shr (16|M0)              r28.0<1>:ud   r6.0<8;8,1>:ud    11:w              
(W)     mul (1|M0)               r5.2<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
(W)     cmp (16|M0)   (lt)f1.0   null<1>:d     r5.0<0;1,0>:ud    0x10:uw             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r6.0<8;8,1>:d     21:w              
(W)     add (1|M0)               a0.0<1>:uw    r5.2<0;1,0>:uw    0x4C0:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r6.0<8;8,1>:ud    25:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r6.0<8;8,1>:d     7:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r16.0<8;8,1>:ud   13:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    ~r6.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r6.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
(W)     shl (1|M0)               r26.0<1>:d    r5.4<0;1,0>:d     2:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     send (1|M0)              r26      r26     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        shr (16|M0)              r8.0<1>:ud    r16.0<8;8,1>:ud   2:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<0;1,0>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r24.0<1>:d    r16.0<8;8,1>:d    30:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r8.0<1>:d     r16.0<8;8,1>:d    19:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r16.0<8;8,1>:ud   22:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r8.0<1>:d     r16.0<8;8,1>:d    10:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r22.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r8.0<1>:d     r22.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r16.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r18.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W&f1.0) jmpi                                L4328                                
L3792:
(W)     add (1|M0)               r5.1<1>:d     r5.0<0;1,0>:d     14:w              
(W)     and (1|M0)               r5.1<1>:d     r5.1<0;1,0>:d     15:w              
(W)     mul (1|M0)               r5.2<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r5.2<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r5.1<1>:d     r5.0<0;1,0>:d     9:w              
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     and (1|M0)               r5.1<1>:d     r5.1<0;1,0>:d     15:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     mul (1|M0)               r5.2<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r5.2<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r5.1<1>:d     r4.4<0;1,0>:d     15:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
        add (16|M0)              r30.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r5.2<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r5.2<1>:uw    r4.14<0;1,0>:uw   0x40:uw             
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r5.2<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r5.2<1>:uw    r4.14<0;1,0>:uw   0x40:uw             
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r5.2<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
(W)     jmpi                                 L4416                                
L4328:
(W)     mul (1|M0)               r5.1<1>:d     r5.0<0;1,0>:d     64:w              
        add (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    r5.1<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r5.2<1>:uw    r5.0<0;1,0>:uw    0x40:uw             
        send (16|M0)             r24:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               a0.0<1>:uw    r5.2<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
L4416:
(W)     shl (1|M0)               r5.0<1>:d     r5.0<0;1,0>:d     2:w              
        shl (16|M0)              r24.0<1>:d    r8.0<8;8,1>:d     26:w              
        shr (16|M0)              r26.0<1>:ud   r8.0<8;8,1>:ud    6:w              
        shr (16|M0)              r28.0<1>:ud   r8.0<8;8,1>:ud    11:w              
(W)     send (1|M0)              r5       r5      0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.14<1>:uw   r4.14<0;1,0>:uw   0x40:uw             
(W)     cmp (16|M0)   (lt)f0.1   null<1>:d     r4.4<0;1,0>:ud    0x10:uw             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r8.0<8;8,1>:d     21:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.14<0;1,0>:uw   0x4C0:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r8.0<8;8,1>:ud    25:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r8.0<8;8,1>:d     7:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r18.0<8;8,1>:ud   13:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r12.0<8;8,1>:d    ~r8.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r10.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r8.0<8;8,1>:d     r6.0<8;8,1>:d    {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r18.0<8;8,1>:ud   2:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r5.0<0;1,0>:d    {Compacted}
        add (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r24.0<1>:d    r18.0<8;8,1>:d    30:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r18.0<8;8,1>:d    19:w              
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r18.0<8;8,1>:ud   22:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r18.0<8;8,1>:d    10:w              
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r16.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r16.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r18.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r20.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W&f0.1) jmpi                                L5440                                
L4904:
(W)     add (1|M0)               r4.7<1>:d     r4.4<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.7<1>:d     r4.7<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.14<1>:uw   r4.14<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.14<0;1,0>:uw   0x4C0:uw             
(W)     add (1|M0)               r4.7<1>:d     r4.4<0;1,0>:d     9:w              
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     and (1|M0)               r4.7<1>:d     r4.7<0;1,0>:d     15:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     mul (1|M0)               r4.14<1>:uw   r4.14<0;1,0>:uw   0x40:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.14<0;1,0>:uw   0x4C0:uw             
(W)     and (1|M0)               r4.7<1>:d     r4.3<0;1,0>:d     15:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.14<1>:uw   r4.14<0;1,0>:uw   0x40:uw             
        add (16|M0)              r30.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.14<0;1,0>:uw   0x4C0:uw             
(W)     mul (1|M0)               r4.14<1>:uw   r4.12<0;1,0>:uw   0x40:uw             
        shl (16|M0)              r24.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shr (16|M0)              r28.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.14<0;1,0>:uw   0x4C0:uw             
(W)     mul (1|M0)               r4.14<1>:uw   r4.12<0;1,0>:uw   0x40:uw             
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.14<0;1,0>:uw   0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
(W)     jmpi                                 L5528                                
L5440:
(W)     mul (1|M0)               r4.7<1>:d     r4.4<0;1,0>:d     64:w              
        add (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    r4.7<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r4.14<1>:uw   r4.8<0;1,0>:uw    0x40:uw             
        send (16|M0)             r24:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               a0.0<1>:uw    r4.14<0;1,0>:uw   0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r24.0<8;8,1>:d                  
L5528:
(W)     shl (1|M0)               r5.0<1>:d     r4.4<0;1,0>:d     2:w              
        shl (16|M0)              r24.0<1>:d    r10.0<8;8,1>:d    26:w              
        shr (16|M0)              r26.0<1>:ud   r10.0<8;8,1>:ud   6:w              
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   11:w              
(W)     send (1|M0)              r5       r5      0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.12<1>:uw   r4.12<0;1,0>:uw   0x40:uw             
(W)     cmp (16|M0)   (lt)f0.0   null<1>:d     r4.4<0;1,0>:ud    0x3F:uw             
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    21:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.12<0;1,0>:uw   0x4C0:uw             
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   25:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    7:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r20.0<8;8,1>:ud   13:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r6.0<8;8,1>:d     ~r10.0<8;8,1>:d  {Compacted}
        add (16|M0)              r24.0<1>:d    r12.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r12.0<1>:d    r10.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r20.0<8;8,1>:ud   2:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r5.0<0;1,0>:d    {Compacted}
        add (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r24.0<1>:d    r20.0<8;8,1>:d    30:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    19:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r20.0<8;8,1>:ud   22:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    10:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r18.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        and (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(~f0.0) break (16|M0)                        L6032                  L6032                
L6016:
(W)     mov (1|M0)               r4.5<1>:d     r4.3<0;1,0>:d                   
L6032:
        while (16|M0)                        L1408                                
L6048:
        add (16|M0)              r104.0<1>:d   r22.0<8;8,1>:d    1779033703:d              
        add (16|M0)              r118.0<1>:d   r20.0<8;8,1>:d    -1150833019:d              
        add (16|M0)              r116.0<1>:d   r18.0<8;8,1>:d    1013904242:d              
        add (16|M0)              r114.0<1>:d   r16.0<8;8,1>:d    -1521486534:d              
        add (16|M0)              r112.0<1>:d   r12.0<8;8,1>:d    1359893119:d              
        add (16|M0)              r110.0<1>:d   r10.0<8;8,1>:d    -1694144372:d              
        add (16|M0)              r108.0<1>:d   r8.0<8;8,1>:d     528734635:d              
        add (16|M0)              r106.0<1>:d   r6.0<8;8,1>:d     1541459225:d              
        mov (16|M0)              r12.0<1>:d    r2.0<8;8,1>:d                    {Compacted}
        mov (16|M0)              r14.0<1>:d    -2147483648:d                              
        mov (16|M0)              r24.0<1>:d    0:w                              
        mov (16|M0)              r26.0<1>:d    0:w                              
        mov (16|M0)              r28.0<1>:d    0:w                              
        mov (16|M0)              r30.0<1>:d    0:w                              
        mov (16|M0)              r32.0<1>:d    0:w                              
        mov (16|M0)              r34.0<1>:d    0:w                              
        mov (16|M0)              r36.0<1>:d    640:w                              
        mov (16|M0)              r22.0<1>:d    0:w                              
        mov (16|M0)              r20.0<1>:d    0:w                              
        mov (16|M0)              r18.0<1>:d    0:w                              
        mov (16|M0)              r16.0<1>:d    0:w                              
        mov (16|M0)              r74.0<1>:d    r118.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r76.0<1>:d    r116.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r10.0<1>:d    r4.2<0;1,0>:d                    {Compacted}
        mov (16|M0)              r80.0<1>:d    r114.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r8.0<1>:d     r4.1<0;1,0>:d                    {Compacted}
        mov (16|M0)              r72.0<1>:d    r112.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r6.0<1>:d     r4.0<0;1,0>:d                    {Compacted}
        mov (16|M0)              r78.0<1>:d    r110.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r2.0<1>:d     r104.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r86.0<1>:d    r108.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r70.0<1>:d    r106.0<8;8,1>:d                  {Compacted}
(W)     mov (1|M0)               r5.1<1>:d     0:w                              
L6480:
(W)     add (1|M0)               r5.5<1>:d     r5.1<0;1,0>:d     1:w              
(W)     add (1|M0)               r5.4<1>:d     r5.1<0;1,0>:d     2:w              
(W)     cmp (16|M0)   (lt)f1.1   null<1>:d     r5.1<0;1,0>:ud    0x10:uw             
(W)     add (1|M0)               r5.0<1>:d     r5.1<0;1,0>:d     3:w              
(W)     and (1|M0)               r5.6<1>:d     r5.1<0;1,0>:d     15:w              
(W)     add (1|M0)               r4.2<1>:d     r5.1<0;1,0>:d     4:w              
(W)     and (1|M0)               r5.7<1>:d     r5.5<0;1,0>:d     15:w              
(W)     and (1|M0)               r4.0<1>:d     r5.4<0;1,0>:d     15:w              
(W)     and (1|M0)               r4.1<1>:d     r5.0<0;1,0>:d     15:w              
(W&f1.1) jmpi                                L7176                                
L6640:
(W)     add (1|M0)               r4.3<1>:d     r5.1<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.3<1>:d     r5.1<0;1,0>:d     9:w              
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.3<1>:d     r5.5<0;1,0>:d     15:w              
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r94.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.12<0;1,0>:uw   0x40:uw             
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.12<0;1,0>:uw   0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r94.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r92.0<8;8,1>:d                  
(W)     jmpi                                 L7256                                
L7176:
(W)     mul (1|M0)               r4.6<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.6<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L7256:
(W)     shl (1|M0)               r85.0<1>:d    r5.1<0;1,0>:d     2:w              
        shl (16|M0)              r88.0<1>:d    r72.0<8;8,1>:d    26:w              
        shr (16|M0)              r90.0<1>:ud   r72.0<8;8,1>:ud   6:w              
        shl (16|M0)              r92.0<1>:d    r72.0<8;8,1>:d    21:w              
(W)     send (1|M0)              r85      r85     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.6<1>:uw    r5.12<0;1,0>:uw   0x40:uw             
(W)     cmp (16|M0)   (lt)f1.0   null<1>:d     r5.5<0;1,0>:ud    0x10:uw             
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r72.0<8;8,1>:ud   11:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r72.0<8;8,1>:ud   25:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r72.0<8;8,1>:d    7:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        and (16|M0)              r90.0<1>:d    r72.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r2.0<8;8,1>:d     19:w              
        add (16|M0)              r88.0<1>:d    r70.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        and (16|M0)              r70.0<1>:d    r86.0<8;8,1>:d    ~r72.0<8;8,1>:d  {Compacted}
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r70.0<1>:d    r2.0<8;8,1>:d     30:w              
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r2.0<8;8,1>:ud    2:w              
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r2.0<8;8,1>:ud    13:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r2.0<8;8,1>:ud    22:w              
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r2.0<8;8,1>:d     10:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        and (16|M0)              r90.0<1>:d    r74.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r74.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        and (16|M0)              r92.0<1>:d    r2.0<8;8,1>:d     r92.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r85.0<0;1,0>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        add (16|M0)              r98.0<1>:d    r80.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r96.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
(W&f1.0) jmpi                                L8280                                
L7744:
(W)     add (1|M0)               r4.3<1>:d     r5.5<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.3<1>:d     r5.5<0;1,0>:d     9:w              
        shl (16|M0)              r88.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shl (16|M0)              r80.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.3<1>:d     r5.4<0;1,0>:d     15:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r90.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.14<0;1,0>:uw   0x40:uw             
        shl (16|M0)              r88.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shl (16|M0)              r80.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.14<0;1,0>:uw   0x40:uw             
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r88.0<8;8,1>:d                  
(W)     jmpi                                 L8360                                
L8280:
(W)     mul (1|M0)               r4.6<1>:uw    r5.10<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.6<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.10<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L8360:
(W)     shl (1|M0)               r85.0<1>:d    r5.5<0;1,0>:d     2:w              
        shl (16|M0)              r70.0<1>:d    r98.0<8;8,1>:d    26:w              
        shr (16|M0)              r80.0<1>:ud   r98.0<8;8,1>:ud   6:w              
        shl (16|M0)              r88.0<1>:d    r98.0<8;8,1>:d    21:w              
(W)     send (1|M0)              r85      r85     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.6<1>:uw    r5.14<0;1,0>:uw   0x40:uw             
(W)     cmp (16|M0)   (lt)f0.1   null<1>:d     r5.4<0;1,0>:ud    0x10:uw             
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r80.0<1>:ud   r98.0<8;8,1>:ud   11:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r80.0<1>:ud   r98.0<8;8,1>:ud   25:w              
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r88.0<1>:d    r98.0<8;8,1>:d    7:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        and (16|M0)              r80.0<1>:d    r78.0<8;8,1>:d    ~r98.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r88.0<1>:d    r96.0<8;8,1>:d    19:w              
        add (16|M0)              r70.0<1>:d    r86.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        and (16|M0)              r86.0<1>:d    r98.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        or (16|M0)               r86.0<1>:d    r86.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r86.0<1>:ud   r96.0<8;8,1>:ud   2:w              
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r85.0<0;1,0>:d   {Compacted}
        add (16|M0)              r80.0<1>:d    r70.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r70.0<1>:d    r96.0<8;8,1>:d    30:w              
        add (16|M0)              r94.0<1>:d    r76.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r86.0<1>:ud   r96.0<8;8,1>:ud   13:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r86.0<1>:ud   r96.0<8;8,1>:ud   22:w              
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r88.0<1>:d    r96.0<8;8,1>:d    10:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        and (16|M0)              r86.0<1>:d    r2.0<8;8,1>:d     r74.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r2.0<8;8,1>:d     r74.0<8;8,1>:d   {Compacted}
        and (16|M0)              r88.0<1>:d    r96.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r80.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
(W&f0.1) jmpi                                L9384                                
L8848:
(W)     add (1|M0)               r4.3<1>:d     r5.4<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.3<1>:d     r5.4<0;1,0>:d     9:w              
        shl (16|M0)              r80.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shl (16|M0)              r76.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.3<1>:d     r5.0<0;1,0>:d     15:w              
        xor (16|M0)              r80.0<1>:d    r80.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r80.0<1>:d    r80.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r86.0<1>:d    r80.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        shl (16|M0)              r80.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shl (16|M0)              r76.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r80.0<1>:d    r80.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r80.0<1>:d    r80.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r80.0<1>:d    r86.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        add (16|M0)              r80.0<1>:d    r80.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r80.0<8;8,1>:d                  
(W)     jmpi                                 L9464                                
L9384:
(W)     mul (1|M0)               r4.6<1>:uw    r5.8<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.6<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.8<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L9464:
(W)     shl (1|M0)               r85.0<1>:d    r5.4<0;1,0>:d     2:w              
        shl (16|M0)              r70.0<1>:d    r94.0<8;8,1>:d    26:w              
        shr (16|M0)              r76.0<1>:ud   r94.0<8;8,1>:ud   6:w              
        shl (16|M0)              r80.0<1>:d    r94.0<8;8,1>:d    21:w              
(W)     send (1|M0)              r85      r85     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
(W)     cmp (16|M0)   (lt)f0.0   null<1>:d     r5.0<0;1,0>:ud    0x10:uw             
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r76.0<1>:ud   r94.0<8;8,1>:ud   11:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r76.0<1>:ud   r94.0<8;8,1>:ud   25:w              
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r80.0<1>:d    r94.0<8;8,1>:d    7:w              
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        and (16|M0)              r76.0<1>:d    r72.0<8;8,1>:d    ~r94.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r80.0<1>:d    r92.0<8;8,1>:d    19:w              
        add (16|M0)              r70.0<1>:d    r78.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        and (16|M0)              r78.0<1>:d    r94.0<8;8,1>:d    r98.0<8;8,1>:d   {Compacted}
        or (16|M0)               r78.0<1>:d    r78.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r78.0<1>:ud   r92.0<8;8,1>:ud   2:w              
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r85.0<0;1,0>:d   {Compacted}
        add (16|M0)              r76.0<1>:d    r70.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r70.0<1>:d    r92.0<8;8,1>:d    30:w              
        add (16|M0)              r90.0<1>:d    r74.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r78.0<1>:ud   r92.0<8;8,1>:ud   13:w              
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r78.0<1>:ud   r92.0<8;8,1>:ud   22:w              
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r80.0<1>:d    r92.0<8;8,1>:d    10:w              
        or (16|M0)               r80.0<1>:d    r80.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        and (16|M0)              r78.0<1>:d    r96.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r80.0<1>:d    r96.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        and (16|M0)              r80.0<1>:d    r92.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r80.0<1>:d    r80.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
(W&f0.0) jmpi                                L10488                                
L9952:
(W)     add (1|M0)               r4.0<1>:d     r5.0<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.0<1>:d     r5.0<0;1,0>:d     9:w              
        shl (16|M0)              r76.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shl (16|M0)              r74.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     and (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     15:w              
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        or (16|M0)               r74.0<1>:d    r74.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.0<1>:d     r4.2<0;1,0>:d     15:w              
        xor (16|M0)              r76.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r76.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r78.0<1>:d    r76.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.0<1>:uw    r4.2<0;1,0>:uw    0x40:uw             
        shl (16|M0)              r76.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shl (16|M0)              r74.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r74.0<1>:d    r74.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.0<1>:uw    r4.2<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r76.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r76.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r76.0<1>:d    r78.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        add (16|M0)              r76.0<1>:d    r76.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r76.0<8;8,1>:d                  
(W)     jmpi                                 L10568                                
L10488:
(W)     mul (1|M0)               r4.0<1>:uw    r5.0<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.0<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.0<1>:uw    r5.0<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L10568:
(W)     shl (1|M0)               r4.0<1>:d     r5.0<0;1,0>:d     2:w              
        shl (16|M0)              r70.0<1>:d    r90.0<8;8,1>:d    26:w              
        shr (16|M0)              r74.0<1>:ud   r90.0<8;8,1>:ud   6:w              
        shl (16|M0)              r76.0<1>:d    r90.0<8;8,1>:d    21:w              
(W)     send (1|M0)              r4       r4      0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     cmp (16|M0)   (lt)f1.1   null<1>:d     r5.0<0;1,0>:ud    0x3F:uw             
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r74.0<1>:ud   r90.0<8;8,1>:ud   11:w              
        mov (16|M0)              r78.0<1>:d    r90.0<8;8,1>:d                   {Compacted}
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r74.0<1>:ud   r90.0<8;8,1>:ud   25:w              
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r76.0<1>:d    r90.0<8;8,1>:d    7:w              
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        and (16|M0)              r74.0<1>:d    r90.0<8;8,1>:d    r94.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r76.0<1>:d    r88.0<8;8,1>:d    19:w              
        add (16|M0)              r70.0<1>:d    r72.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        and (16|M0)              r72.0<1>:d    r98.0<8;8,1>:d    ~r90.0<8;8,1>:d  {Compacted}
        or (16|M0)               r74.0<1>:d    r74.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r74.0<1>:ud   r88.0<8;8,1>:ud   2:w              
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r4.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r4.0<1>:uw    r4.2<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        add (16|M0)              r72.0<1>:d    r70.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r70.0<1>:d    r88.0<8;8,1>:d    30:w              
        add (16|M0)              r86.0<1>:d    r2.0<8;8,1>:d     r72.0<8;8,1>:d   {Compacted}
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r74.0<1>:ud   r88.0<8;8,1>:ud   13:w              
        mov (16|M0)              r2.0<1>:d     r96.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r80.0<1>:d    r86.0<8;8,1>:d                   {Compacted}
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r74.0<1>:ud   r88.0<8;8,1>:ud   22:w              
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r76.0<1>:d    r88.0<8;8,1>:d    10:w              
        or (16|M0)               r76.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        and (16|M0)              r74.0<1>:d    r92.0<8;8,1>:d    r96.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r76.0<1>:d    r92.0<8;8,1>:d    r96.0<8;8,1>:d   {Compacted}
        and (16|M0)              r76.0<1>:d    r88.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r76.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r74.0<1>:d    r94.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r76.0<1>:d    r88.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r100.0<1>:d   r72.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r72.0<1>:d    r92.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r70.0<1>:d    r98.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r102.0<1>:d   r100.0<8;8,1>:d                  {Compacted}
(~f1.1) break (16|M0)                        L11200                  L11200                
L11120:
        mov (16|M0)              r72.0<1>:d    r86.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r70.0<1>:d    r98.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r2.0<1>:d     r100.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r74.0<1>:d    r88.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r76.0<1>:d    r92.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r80.0<1>:d    r96.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r78.0<1>:d    r90.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r86.0<1>:d    r94.0<8;8,1>:d                   {Compacted}
(W)     mov (1|M0)               r5.1<1>:d     r4.2<0;1,0>:d                   
L11200:
        while (16|M0)                        L6480                                
L11216:
        add (16|M0)              r8.0<1>:d     r118.0<8;8,1>:d   r76.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r116.0<8;8,1>:d   r72.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r114.0<8;8,1>:d   r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r112.0<8;8,1>:d   r80.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r110.0<8;8,1>:d   r78.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r108.0<8;8,1>:d   r74.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r106.0<8;8,1>:d   r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r6.0<1>:d     r104.0<8;8,1>:d   r102.0<8;8,1>:d  {Compacted}
        mov (16|M0)              r22.0<1>:d    -2147483648:d                              
        mov (16|M0)              r24.0<1>:d    0:w                              
        mov (16|M0)              r26.0<1>:d    0:w                              
        mov (16|M0)              r28.0<1>:d    0:w                              
        mov (16|M0)              r30.0<1>:d    0:w                              
        mov (16|M0)              r32.0<1>:d    0:w                              
        mov (16|M0)              r34.0<1>:d    0:w                              
        mov (16|M0)              r36.0<1>:d    256:w                              
        mov (16|M0)              r86.0<1>:d    1541459225:d                              
        mov (16|M0)              r76.0<1>:d    1013904242:d                              
        mov (16|M0)              r72.0<1>:d    1359893119:d                              
        mov (16|M0)              r2.0<1>:d     528734635:d                              
        mov (16|M0)              r80.0<1>:d    1779033703:d                              
        mov (16|M0)              r78.0<1>:d    -1150833019:d                              
        mov (16|M0)              r74.0<1>:d    -1521486534:d                              
        mov (16|M0)              r70.0<1>:d    -1694144372:d                              
(W)     mov (1|M0)               r5.6<1>:d     0:w                              
L11552:
(W)     add (1|M0)               r5.5<1>:d     r5.6<0;1,0>:d     1:w              
(W)     add (1|M0)               r5.1<1>:d     r5.6<0;1,0>:d     2:w              
(W)     cmp (16|M0)   (lt)f1.0   null<1>:d     r5.6<0;1,0>:ud    0x10:uw             
(W)     add (1|M0)               r5.0<1>:d     r5.6<0;1,0>:d     3:w              
(W)     and (1|M0)               r5.7<1>:d     r5.6<0;1,0>:d     15:w              
(W)     add (1|M0)               r4.2<1>:d     r5.6<0;1,0>:d     4:w              
(W)     and (1|M0)               r5.4<1>:d     r5.5<0;1,0>:d     15:w              
(W)     and (1|M0)               r4.0<1>:d     r5.1<0;1,0>:d     15:w              
(W)     and (1|M0)               r4.1<1>:d     r5.0<0;1,0>:d     15:w              
(W&f1.0) jmpi                                L12248                                
L11712:
(W)     add (1|M0)               r4.3<1>:d     r5.6<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.3<1>:d     r5.6<0;1,0>:d     9:w              
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.3<1>:d     r5.5<0;1,0>:d     15:w              
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r94.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.14<0;1,0>:uw   0x40:uw             
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.14<0;1,0>:uw   0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r94.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r92.0<8;8,1>:d                  
(W)     jmpi                                 L12328                                
L12248:
(W)     mul (1|M0)               r4.6<1>:uw    r5.12<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.6<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.12<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L12328:
(W)     shl (1|M0)               r85.0<1>:d    r5.6<0;1,0>:d     2:w              
        shl (16|M0)              r88.0<1>:d    r72.0<8;8,1>:d    26:w              
        shr (16|M0)              r90.0<1>:ud   r72.0<8;8,1>:ud   6:w              
        shl (16|M0)              r92.0<1>:d    r72.0<8;8,1>:d    21:w              
(W)     send (1|M0)              r85      r85     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.6<1>:uw    r5.14<0;1,0>:uw   0x40:uw             
(W)     cmp (16|M0)   (lt)f0.1   null<1>:d     r5.5<0;1,0>:ud    0x10:uw             
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r72.0<8;8,1>:ud   11:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r72.0<8;8,1>:ud   25:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r72.0<8;8,1>:d    7:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        and (16|M0)              r90.0<1>:d    r72.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r80.0<8;8,1>:d    19:w              
        add (16|M0)              r88.0<1>:d    r86.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        and (16|M0)              r86.0<1>:d    r2.0<8;8,1>:d     ~r72.0<8;8,1>:d  {Compacted}
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r86.0<1>:ud   r80.0<8;8,1>:ud   2:w              
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r85.0<0;1,0>:d   {Compacted}
        add (16|M0)              r90.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r88.0<1>:d    r80.0<8;8,1>:d    30:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r86.0<1>:ud   r80.0<8;8,1>:ud   13:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r86.0<1>:ud   r80.0<8;8,1>:ud   22:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r80.0<8;8,1>:d    10:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        and (16|M0)              r86.0<1>:d    r78.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r78.0<8;8,1>:d    r76.0<8;8,1>:d   {Compacted}
        and (16|M0)              r92.0<1>:d    r80.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r86.0<8;8,1>:d   {Compacted}
        add (16|M0)              r86.0<1>:d    r74.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r74.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
(W&f0.1) jmpi                                L13352                                
L12816:
(W)     add (1|M0)               r4.3<1>:d     r5.5<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.3<1>:d     r5.5<0;1,0>:d     9:w              
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.3<1>:d     r5.1<0;1,0>:d     15:w              
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r94.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.8<0;1,0>:uw    0x40:uw             
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.8<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r94.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r92.0<8;8,1>:d                  
(W)     jmpi                                 L13432                                
L13352:
(W)     mul (1|M0)               r4.6<1>:uw    r5.10<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.6<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.10<0;1,0>:uw   0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L13432:
(W)     shl (1|M0)               r85.0<1>:d    r5.5<0;1,0>:d     2:w              
        shl (16|M0)              r88.0<1>:d    r86.0<8;8,1>:d    26:w              
        shr (16|M0)              r90.0<1>:ud   r86.0<8;8,1>:ud   6:w              
        shl (16|M0)              r92.0<1>:d    r86.0<8;8,1>:d    21:w              
(W)     send (1|M0)              r85      r85     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.6<1>:uw    r5.8<0;1,0>:uw    0x40:uw             
(W)     cmp (16|M0)   (lt)f0.0   null<1>:d     r5.1<0;1,0>:ud    0x10:uw             
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r86.0<8;8,1>:ud   11:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r86.0<8;8,1>:ud   25:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r86.0<8;8,1>:d    7:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        and (16|M0)              r90.0<1>:d    r86.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r74.0<8;8,1>:d    19:w              
        add (16|M0)              r88.0<1>:d    r2.0<8;8,1>:d     r88.0<8;8,1>:d   {Compacted}
        and (16|M0)              r2.0<1>:d     r70.0<8;8,1>:d    ~r86.0<8;8,1>:d  {Compacted}
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r2.0<1>:ud    r74.0<8;8,1>:ud   2:w              
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r85.0<0;1,0>:d   {Compacted}
        add (16|M0)              r90.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r88.0<1>:d    r74.0<8;8,1>:d    30:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r2.0<1>:ud    r74.0<8;8,1>:ud   13:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r2.0<1>:ud    r74.0<8;8,1>:ud   22:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r74.0<8;8,1>:d    10:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        and (16|M0)              r2.0<1>:d     r80.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r80.0<8;8,1>:d    r78.0<8;8,1>:d   {Compacted}
        and (16|M0)              r92.0<1>:d    r74.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r76.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r76.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
(W&f0.0) jmpi                                L14456                                
L13920:
(W)     add (1|M0)               r4.3<1>:d     r5.1<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.3<1>:d     r5.1<0;1,0>:d     9:w              
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     and (1|M0)               r4.3<1>:d     r4.3<0;1,0>:d     15:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.3<1>:d     r5.0<0;1,0>:d     15:w              
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.6<1>:uw    r4.6<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r94.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r88.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r94.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r92.0<1>:d    r92.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r92.0<8;8,1>:d                  
(W)     jmpi                                 L14536                                
L14456:
(W)     mul (1|M0)               r4.6<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.6<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.6<1>:uw    r5.2<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.6<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L14536:
(W)     shl (1|M0)               r85.0<1>:d    r5.1<0;1,0>:d     2:w              
        shl (16|M0)              r88.0<1>:d    r2.0<8;8,1>:d     26:w              
        shr (16|M0)              r90.0<1>:ud   r2.0<8;8,1>:ud    6:w              
        shl (16|M0)              r92.0<1>:d    r2.0<8;8,1>:d     21:w              
(W)     send (1|M0)              r85      r85     0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
(W)     cmp (16|M0)   (lt)f1.1   null<1>:d     r5.0<0;1,0>:ud    0x10:uw             
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r2.0<8;8,1>:ud    11:w              
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r2.0<8;8,1>:ud    25:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r2.0<8;8,1>:d     7:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        and (16|M0)              r90.0<1>:d    r72.0<8;8,1>:d    ~r2.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r92.0<1>:ud   r76.0<8;8,1>:ud   13:w              
        add (16|M0)              r88.0<1>:d    r70.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        and (16|M0)              r70.0<1>:d    r2.0<8;8,1>:d     r86.0<8;8,1>:d   {Compacted}
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r70.0<1>:ud   r76.0<8;8,1>:ud   2:w              
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r85.0<0;1,0>:d   {Compacted}
        add (16|M0)              r90.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r88.0<1>:d    r76.0<8;8,1>:d    30:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r70.0<1>:d    r76.0<8;8,1>:d    19:w              
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r92.0<1>:ud   r76.0<8;8,1>:ud   22:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r70.0<1>:d    r76.0<8;8,1>:d    10:w              
        or (16|M0)               r70.0<1>:d    r70.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        and (16|M0)              r92.0<1>:d    r74.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r70.0<1>:d    r74.0<8;8,1>:d    r80.0<8;8,1>:d   {Compacted}
        and (16|M0)              r70.0<1>:d    r76.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r70.0<1>:d    r70.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r70.0<8;8,1>:d   {Compacted}
        add (16|M0)              r70.0<1>:d    r78.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r78.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
(W&f1.1) jmpi                                L15560                                
L15024:
(W)     add (1|M0)               r4.0<1>:d     r5.0<0;1,0>:d     14:w              
(W)     and (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     15:w              
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     add (1|M0)               r4.0<1>:d     r5.0<0;1,0>:d     9:w              
        shl (16|M0)              r88.0<1>:d    r[a0.0]<1;1,0>:d  15:w              
        shr (16|M0)              r90.0<1>:ud   r[a0.0]<1;1,0>:ud  17:w             
        shr (16|M0)              r92.0<1>:ud   r[a0.0]<1;1,0>:ud  19:w             
(W)     and (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     15:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r90.0<1>:d    r[a0.0]<1;1,0>:d  13:w              
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r90.0<1>:ud   r[a0.0]<1;1,0>:ud  10:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     and (1|M0)               r4.0<1>:d     r4.2<0;1,0>:d     15:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r4.0<1>:uw    r4.0<0;1,0>:uw    0x40:uw             
        add (16|M0)              r90.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.0<1>:uw    r4.2<0;1,0>:uw    0x40:uw             
        shl (16|M0)              r88.0<1>:d    r[a0.0]<1;1,0>:d  25:w              
        shr (16|M0)              r92.0<1>:ud   r[a0.0]<1;1,0>:ud  7:w             
        shr (16|M0)              r94.0<1>:ud   r[a0.0]<1;1,0>:ud  18:w             
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r92.0<1>:d    r[a0.0]<1;1,0>:d  14:w              
        or (16|M0)               r92.0<1>:d    r92.0<8;8,1>:d    r94.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r92.0<1>:ud   r[a0.0]<1;1,0>:ud  3:w             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
(W)     mul (1|M0)               r4.0<1>:uw    r4.2<0;1,0>:uw    0x40:uw             
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r88.0<8;8,1>:d                  
(W)     jmpi                                 L15640                                
L15560:
(W)     mul (1|M0)               r4.0<1>:uw    r5.0<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.1<1>:uw    r4.0<0;1,0>:uw    0xC0:uw             
(W)     mul (1|M0)               r4.0<1>:uw    r5.0<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        mov (16|M0)              r[a0.0]<1>:d  r[a0.1]<1;1,0>:d                
L15640:
(W)     shl (1|M0)               r4.0<1>:d     r5.0<0;1,0>:d     2:w              
        shl (16|M0)              r88.0<1>:d    r70.0<8;8,1>:d    26:w              
        shr (16|M0)              r90.0<1>:ud   r70.0<8;8,1>:ud   6:w              
        shr (16|M0)              r92.0<1>:ud   r70.0<8;8,1>:ud   11:w              
(W)     send (1|M0)              r4       r4      0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     cmp (16|M0)   (lt)f1.0   null<1>:d     r5.0<0;1,0>:ud    0x3F:uw             
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r90.0<1>:d    r70.0<8;8,1>:d    21:w              
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r92.0<1>:ud   r70.0<8;8,1>:ud   25:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r90.0<1>:d    r70.0<8;8,1>:d    7:w              
        or (16|M0)               r90.0<1>:d    r90.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r92.0<1>:ud   r78.0<8;8,1>:ud   13:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        and (16|M0)              r90.0<1>:d    r86.0<8;8,1>:d    ~r70.0<8;8,1>:d  {Compacted}
        add (16|M0)              r88.0<1>:d    r72.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
        and (16|M0)              r72.0<1>:d    r70.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        or (16|M0)               r72.0<1>:d    r72.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r72.0<1>:ud   r78.0<8;8,1>:ud   2:w              
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r4.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r4.0<1>:uw    r4.2<0;1,0>:uw    0x40:uw             
(W)     add (1|M0)               a0.0<1>:uw    r4.0<0;1,0>:uw    0x4C0:uw             
        add (16|M0)              r90.0<1>:d    r88.0<8;8,1>:d    r[a0.0]<1;1,0>:d
        shl (16|M0)              r88.0<1>:d    r78.0<8;8,1>:d    30:w              
        or (16|M0)               r88.0<1>:d    r88.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r72.0<1>:d    r78.0<8;8,1>:d    19:w              
        or (16|M0)               r72.0<1>:d    r72.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r92.0<1>:ud   r78.0<8;8,1>:ud   22:w              
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r72.0<1>:d    r78.0<8;8,1>:d    10:w              
        or (16|M0)               r72.0<1>:d    r72.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        and (16|M0)              r92.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r72.0<1>:d    r76.0<8;8,1>:d    r74.0<8;8,1>:d   {Compacted}
        and (16|M0)              r72.0<1>:d    r78.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r72.0<1>:d    r72.0<8;8,1>:d    r92.0<8;8,1>:d   {Compacted}
        add (16|M0)              r88.0<1>:d    r88.0<8;8,1>:d    r72.0<8;8,1>:d   {Compacted}
        add (16|M0)              r72.0<1>:d    r80.0<8;8,1>:d    r90.0<8;8,1>:d   {Compacted}
        add (16|M0)              r80.0<1>:d    r90.0<8;8,1>:d    r88.0<8;8,1>:d   {Compacted}
(~f1.0) break (16|M0)                        L16144                  L16144                
L16128:
(W)     mov (1|M0)               r5.6<1>:d     r4.2<0;1,0>:d                   
L16144:
        while (16|M0)                        L11552                                
L16160:
        add (16|M0)              r18.0<1>:d    r2.0<8;8,1>:d     528734635:d              
        add (16|M0)              r6.0<1>:d     r80.0<8;8,1>:d    1779033703:d              
        add (16|M0)              r8.0<1>:d     r78.0<8;8,1>:d    -1150833019:d              
        add (16|M0)              r10.0<1>:d    r76.0<8;8,1>:d    1013904242:d              
        add (16|M0)              r12.0<1>:d    r74.0<8;8,1>:d    -1521486534:d              
        shl (16|M0)              r2.0<1>:d     r82.0<8;8,1>:d    5:w              
        add (16|M0)              r14.0<1>:d    r72.0<8;8,1>:d    1359893119:d              
        add (16|M0)              r16.0<1>:d    r70.0<8;8,1>:d    -1694144372:d              
        add (16|M0)              r20.0<1>:d    r86.0<8;8,1>:d    1541459225:d              
        sends (16|M0)            null:w   r2      r6      0x20C            0x04025003           // wr:2+8, rd:0; hdc.dc1; untyped surface write with xyzw
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     16:w              
(W)     mov (1|M0)               r5.0<1>:ud    0x1C:uw                             
        sends (16|M0)            null:w   r2      r14     0x20C            0x04025003           // wr:2+8, rd:0; hdc.dc1; untyped surface write with xyzw
(W)     send (1|M0)              r5       r5      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r20.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L16448                  L17728                
L16416:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L17728                  L17728                
L16448:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r20.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L16512                  L17712                
L16480:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L17712                  L17712                
L16512:
(W)     mov (1|M0)               r5.0<1>:ud    0x18:uw                             
(W)     send (1|M0)              r5       r5      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f1.1   null<1>:d     r18.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.1)  if (16|M0)                           L16608                  L17696                
L16576:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L17696                  L17696                
L16608:
        cmp (16|M0)   (lt)f1.0   null<1>:d     r18.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.0)  if (16|M0)                           L16672                  L17680                
L16640:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L17680                  L17680                
L16672:
(W)     mov (1|M0)               r5.0<1>:ud    0x14:uw                             
(W)     send (1|M0)              r5       r5      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r16.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L16768                  L17664                
L16736:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L17664                  L17664                
L16768:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r16.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L16832                  L17648                
L16800:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L17648                  L17648                
L16832:
(W)     mov (1|M0)               r5.0<1>:ud    0x10:uw                             
(W)     send (1|M0)              r5       r5      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f1.1   null<1>:d     r14.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.1)  if (16|M0)                           L16928                  L17632                
L16896:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L17632                  L17632                
L16928:
        cmp (16|M0)   (lt)f1.0   null<1>:d     r14.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.0)  if (16|M0)                           L16992                  L17616                
L16960:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L17616                  L17616                
L16992:
(W)     mov (1|M0)               r5.0<1>:ud    0xC:uw                             
(W)     send (1|M0)              r5       r5      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r12.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L17088                  L17600                
L17056:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L17600                  L17600                
L17088:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r12.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L17152                  L17584                
L17120:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L17584                  L17584                
L17152:
(W)     mov (1|M0)               r5.0<1>:ud    0x8:uw                             
(W)     send (1|M0)              r5       r5      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f1.1   null<1>:d     r10.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.1)  if (16|M0)                           L17248                  L17568                
L17216:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L17568                  L17568                
L17248:
        cmp (16|M0)   (lt)f1.0   null<1>:d     r10.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.0)  if (16|M0)                           L17312                  L17552                
L17280:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L17552                  L17552                
L17312:
(W)     mov (1|M0)               r5.0<1>:ud    0x4:uw                             
(W)     send (1|M0)              r5       r5      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r8.0<8;8,1>:ud    r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L17408                  L17536                
L17376:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L17536                  L17536                
L17408:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r8.0<8;8,1>:ud    r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L17472                  L17520                
L17440:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L17520                  L17520                
L17472:
(W)     mov (1|M0)               r2.0<1>:uq    r5.1<0;1,0>:uq                  
(W)     send (1|M0)              r2       r2:uq   0xC            0x041401FF           // wr:2+0, rd:1; hdc.dc1; a64 dword gathering read
        cmp (16|M0)   (le)f0.1   r2.0<1>:ud    r6.0<8;8,1>:ud    r2.0<0;1,0>:ud   {Compacted}
        mov (16|M0)              r6.0<1>:d     -r2.0<8;8,1>:d                   {Compacted}
L17520:
        endif (16|M0)                        L17536                                
L17536:
        endif (16|M0)                        L17552                                
L17552:
        endif (16|M0)                        L17568                                
L17568:
        endif (16|M0)                        L17584                                
L17584:
        endif (16|M0)                        L17600                                
L17600:
        endif (16|M0)                        L17616                                
L17616:
        endif (16|M0)                        L17632                                
L17632:
        endif (16|M0)                        L17648                                
L17648:
        endif (16|M0)                        L17664                                
L17664:
        endif (16|M0)                        L17680                                
L17680:
        endif (16|M0)                        L17696                                
L17696:
        endif (16|M0)                        L17712                                
L17712:
        endif (16|M0)                        L17728                                
L17728:
        endif (16|M0)                        L17744                                
L17744:
        shl (16|M0)              r2.0<1>:d     r82.0<8;8,1>:d    2:w              
(W)     mov (8|M0)               r112.0<1>:ud  r84.0<8;8,1>:ud                  {Compacted}
        sends (16|M0)            null:w   r2      r6      0x8C            0x04025E02           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               null<1>:ud    f0.0<0;1,0>:ud                  
(W)     send (1|M0)              null     r112    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L17816:
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
