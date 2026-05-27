L0:
(W)     mov (8|M0)               r64.0<1>:ud   r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mov (1|M0)               r2.0<1>:uq    r5.0<0;1,0>:uq                  
(W)     mov (1|M0)               r3.2<1>:ud    0x20:uw                             
(W)     mov (1|M0)               r5.4<1>:uw    0x40:uw                             
(W)     mov (8|M0)               r11.0<1>:w    0x76543210:v                              
(W)     and (1|M0)               r6.0<1>:d     r64.5<0;1,0>:d    -1024:w              
(W)     mul (1|M0)               r6.1<1>:d     r6.2<0;1,0>:d     r64.1<0;1,0>:d  
(W)     mov (1|M0)               r58.3<1>:d    16:w                              
(W)     send (8|M0)              r7       r2      0xC            0x021D0AFF           // wr:1h+0, rd:1; hdc.dc1; a64 aligned oword block read x2
(W)     send (8|M0)              r8:d     r3      0xA            0x02184200           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x2
(W)     add (4|M0)               r9.0<1>:ud    r5.4<0;1,0>:uw    0x8080400:uv             
(W)     add (8|M0)               r11.8<1>:w    r11.0<8;8,1>:w    8:w              
(W)     mov (1|M0)               r2.2<1>:ud    0x100:uw                             
        add (16|M0)              r14.0<1>:d    r6.1<0;1,0>:d     r1.0<16;16,1>:uw
(W)     send (4|M0)              r10      r9      0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        shl (16|M0)              r12.0<1>:d    r11.0<16;16,1>:uw  2:w             
        add (16|M0)              r110.0<1>:d   r14.0<8;8,1>:d    r4.0<0;1,0>:d    {Compacted}
(W)     send (8|M0)              r4:d     r2      0xA            0x02184202           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x2
        add (16|M0)              r56.0<1>:d    r6.0<0;1,0>:d     r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r108.0<1>:d   r110.0<8;8,1>:d   r5.4<0;1,0>:d    {Compacted}
        add (16|M0)              r90.0<1>:d    r56.0<8;8,1>:d    64:w              
        add (16|M0)              r88.0<1>:d    r56.0<8;8,1>:d    128:w              
        add (16|M0)              r86.0<1>:d    r56.0<8;8,1>:d    192:w              
        add (16|M0)              r84.0<1>:d    r56.0<8;8,1>:d    256:w              
        add (16|M0)              r82.0<1>:d    r56.0<8;8,1>:d    320:w              
        add (16|M0)              r80.0<1>:d    r56.0<8;8,1>:d    384:w              
        add (16|M0)              r78.0<1>:d    r56.0<8;8,1>:d    448:w              
        add (16|M0)              r76.0<1>:d    r56.0<8;8,1>:d    512:w              
        add (16|M0)              r74.0<1>:d    r56.0<8;8,1>:d    576:w              
        add (16|M0)              r72.0<1>:d    r56.0<8;8,1>:d    640:w              
        add (16|M0)              r70.0<1>:d    r56.0<8;8,1>:d    704:w              
        add (16|M0)              r68.0<1>:d    r56.0<8;8,1>:d    768:w              
        add (16|M0)              r66.0<1>:d    r56.0<8;8,1>:d    832:w              
        add (16|M0)              r62.0<1>:d    r56.0<8;8,1>:d    896:w              
        add (16|M0)              r60.0<1>:d    r56.0<8;8,1>:d    960:w              
        mov (16|M0)              r16.0<1>:d    r7.0<0;1,0>:d                    {Compacted}
        mov (16|M0)              r18.0<1>:d    r7.1<0;1,0>:d                    {Compacted}
        mov (16|M0)              r20.0<1>:d    r7.2<0;1,0>:d                    {Compacted}
        mov (16|M0)              r22.0<1>:d    r7.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r56     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r24.0<1>:d    r7.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r90     r18     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r12.0<1>:d    r7.5<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r88     r20     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r14.0<1>:d    r7.6<0;1,0>:d                   
        sends (16|M0)            null:w   r86     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r26.0<1>:d    r7.7<0;1,0>:d                   
        sends (16|M0)            null:w   r84     r24     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r28.0<1>:d    r8.0<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r82     r12     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r30.0<1>:d    r8.1<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r80     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r32.0<1>:d    r8.2<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r78     r26     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r34.0<1>:d    r8.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r76     r28     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r36.0<1>:d    r8.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r74     r30     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r38.0<1>:d    r8.5<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r72     r32     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r40.0<1>:d    r8.6<0;1,0>:d                   
        sends (16|M0)            null:w   r70     r34     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r42.0<1>:d    r8.7<0;1,0>:d                   
        sends (16|M0)            null:w   r68     r36     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r30.0<1>:d    r8.6<0;1,0>:d                   
        sends (16|M0)            null:w   r66     r38     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r34.0<1>:d    r7.0<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r62     r40     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (2|M0)               r58.0<1>:d    r10.0<2;2,1>:d                  
        sends (16|M0)            null:w   r60     r42     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               r58.2<1>:d    r10.2<0;1,0>:d                  
L1024:
(W)     add (1|M0)               r2.0<1>:d     r58.3<0;1,0>:d    -15:w              
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    -7:w              
        shl (16|M0)              r16.0<1>:d    r30.0<8;8,1>:d    15:w              
        shr (16|M0)              r18.0<1>:ud   r30.0<8;8,1>:ud   17:w              
        shl (16|M0)              r20.0<1>:d    r30.0<8;8,1>:d    13:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        shr (16|M0)              r22.0<1>:ud   r30.0<8;8,1>:ud   19:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r30.0<8;8,1>:ud   10:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    64:w              
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r6.0<1>:d     r58.3<0;1,0>:d    -1:w              
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r7.1<1>:d     r58.3<0;1,0>:d    -14:w              
(W)     or (1|M0)                r5.2<1>:d     r58.3<0;1,0>:d    1:w              
(W)     mul (1|M0)               r7.0<1>:d     r6.0<0;1,0>:d     64:w              
        xor (16|M0)              r32.0<1>:d    r16.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
(W)     cmp (16|M0)   (lt)f0.1   null<1>:d     r58.3<0;1,0>:ud   0x34:uw             
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r16.0<1>:d    r56.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
(W)     or (1|M0)                r7.0<1>:d     r58.3<0;1,0>:d    2:w              
(W)     mul (1|M0)               r5.2<1>:d     r7.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r58.3<0;1,0>:d    -3:w              
        shl (16|M0)              r2.0<1>:d     r10.0<8;8,1>:d    25:w              
        shr (16|M0)              r24.0<1>:ud   r10.0<8;8,1>:ud   7:w              
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    14:w              
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   18:w              
        shr (16|M0)              r18.0<1>:ud   r10.0<8;8,1>:ud   3:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r32.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r10.0<1>:d    r7.1<0;1,0>:d     64:w              
(W)     add (1|M0)               r11.0<1>:d    r58.3<0;1,0>:d    -6:w              
(W)     add (1|M0)               r7.1<1>:d     r58.3<0;1,0>:d    -13:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r24.0<1>:d    r11.0<0;1,0>:d    64:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r10.0<0;1,0>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r24.0<0;1,0>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r22.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r20     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r14:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r2:w     r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r30.0<1>:d    r14.0<8;8,1>:d    15:w              
        shr (16|M0)              r32.0<1>:ud   r14.0<8;8,1>:ud   17:w              
        shl (16|M0)              r34.0<1>:d    r14.0<8;8,1>:d    13:w              
        shr (16|M0)              r36.0<1>:ud   r14.0<8;8,1>:ud   19:w              
        shl (16|M0)              r38.0<1>:d    r22.0<8;8,1>:d    25:w              
        shr (16|M0)              r40.0<1>:ud   r22.0<8;8,1>:ud   7:w              
        shl (16|M0)              r10.0<1>:d    r22.0<8;8,1>:d    14:w              
        shr (16|M0)              r24.0<1>:ud   r22.0<8;8,1>:ud   18:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r14.0<8;8,1>:ud   10:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r22.0<8;8,1>:ud   3:w              
        add (16|M0)              r14.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r30.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r8.0<1>:d     r7.1<0;1,0>:d     64:w              
(W)     add (1|M0)               r9.0<1>:d     r58.3<0;1,0>:d    -5:w              
(W)     add (1|M0)               r6.0<1>:d     r58.3<0;1,0>:d    -12:w              
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r42.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r8.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r24.0<1>:d    r9.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.1<1>:d     r58.3<0;1,0>:d    -2:w              
        add (16|M0)              r38.0<1>:d    r36.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r24.0<0;1,0>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r14     r38     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r10:w    r20     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r2:w     r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r34.0<1>:d    r10.0<8;8,1>:d    15:w              
        shr (16|M0)              r36.0<1>:ud   r10.0<8;8,1>:ud   17:w              
        shl (16|M0)              r38.0<1>:d    r10.0<8;8,1>:d    13:w              
        shr (16|M0)              r40.0<1>:ud   r10.0<8;8,1>:ud   19:w              
        shl (16|M0)              r42.0<1>:d    r2.0<8;8,1>:d     25:w              
        shr (16|M0)              r44.0<1>:ud   r2.0<8;8,1>:ud    7:w              
        shl (16|M0)              r8.0<1>:d     r2.0<8;8,1>:d     14:w              
        shr (16|M0)              r24.0<1>:ud   r2.0<8;8,1>:ud    18:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r10.0<8;8,1>:ud   10:w              
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r36.0<1>:ud   r2.0<8;8,1>:ud    3:w              
        add (16|M0)              r38.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r46.0<1>:d    r34.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r10.0<1>:d    r6.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r8.0<1>:d     r58.3<0;1,0>:d    -4:w              
(W)     or (1|M0)                r3.0<1>:d     r58.3<0;1,0>:d    3:w              
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r46.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r56.0<8;8,1>:d    r10.0<0;1,0>:d   {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r8.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    -11:w              
        add (16|M0)              r42.0<1>:d    r40.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r54.0<1>:d    r56.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r3.0<0;1,0>:d     64:w              
        add (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    4:w              
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
        sends (16|M0)            null:w   r38     r42     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r18:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r46:w    r44     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r24:w    r54     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r48:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    15:w              
        shr (16|M0)              r34.0<1>:ud   r18.0<8;8,1>:ud   17:w              
        shl (16|M0)              r32.0<1>:d    r18.0<8;8,1>:d    13:w              
        shr (16|M0)              r36.0<1>:ud   r18.0<8;8,1>:ud   19:w              
        shl (16|M0)              r50.0<1>:d    r46.0<8;8,1>:d    25:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r52.0<1>:ud   r46.0<8;8,1>:ud   7:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r34.0<1>:d    r46.0<8;8,1>:d    14:w              
        shr (16|M0)              r36.0<1>:ud   r46.0<8;8,1>:ud   18:w              
        shr (16|M0)              r40.0<1>:ud   r18.0<8;8,1>:ud   10:w              
        or (16|M0)               r50.0<1>:d    r50.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r46.0<8;8,1>:ud   3:w              
        xor (16|M0)              r42.0<1>:d    r28.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r50.0<1>:d    r50.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r42.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r50.0<1>:d    r50.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r18.0<1>:d    r7.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r58.3<0;1,0>:d    5:w              
        add (16|M0)              r50.0<1>:d    r8.0<8;8,1>:d     r50.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r18.0<0;1,0>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r7.0<0;1,0>:d     64:w              
        add (16|M0)              r50.0<1>:d    r50.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
(W)     add (1|M0)               r7.0<1>:d     r58.3<0;1,0>:d    -8:w              
        sends (16|M0)            null:w   r10     r50     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r22:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r44     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r2.0<1>:d     r22.0<8;8,1>:d    15:w              
        shr (16|M0)              r36.0<1>:ud   r22.0<8;8,1>:ud   17:w              
        shl (16|M0)              r40.0<1>:d    r22.0<8;8,1>:d    13:w              
        shr (16|M0)              r42.0<1>:ud   r22.0<8;8,1>:ud   19:w              
        shl (16|M0)              r46.0<1>:d    r28.0<8;8,1>:d    25:w              
        shr (16|M0)              r48.0<1>:ud   r28.0<8;8,1>:ud   7:w              
        shl (16|M0)              r50.0<1>:d    r28.0<8;8,1>:d    14:w              
        shr (16|M0)              r18.0<1>:ud   r28.0<8;8,1>:ud   18:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r22.0<8;8,1>:ud   10:w              
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        or (16|M0)               r50.0<1>:d    r50.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r36.0<1>:ud   r28.0<8;8,1>:ud   3:w              
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
(W)     add (1|M0)               r2.0<1>:d     r58.3<0;1,0>:d    -10:w              
(W)     add (1|M0)               r6.0<1>:d     r58.3<0;1,0>:d    -9:w              
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r44.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r32.0<1>:d    r7.1<0;1,0>:d     64:w              
        add (16|M0)              r46.0<1>:d    r42.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r36.0<1>:d    r56.0<8;8,1>:d    r32.0<0;1,0>:d   {Compacted}
        add (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r22     r46     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r8:w     r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r40:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r42.0<1>:d    r8.0<8;8,1>:d     15:w              
        shr (16|M0)              r44.0<1>:ud   r8.0<8;8,1>:ud    17:w              
        shl (16|M0)              r46.0<1>:d    r8.0<8;8,1>:d     13:w              
        shr (16|M0)              r48.0<1>:ud   r8.0<8;8,1>:ud    19:w              
        shl (16|M0)              r2.0<1>:d     r28.0<8;8,1>:d    25:w              
        shr (16|M0)              r50.0<1>:ud   r28.0<8;8,1>:ud   7:w              
        shl (16|M0)              r52.0<1>:d    r28.0<8;8,1>:d    14:w              
        shr (16|M0)              r32.0<1>:ud   r28.0<8;8,1>:ud   18:w              
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r24.0<1>:ud   r8.0<8;8,1>:ud    10:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r50.0<8;8,1>:d   {Compacted}
        or (16|M0)               r52.0<1>:d    r52.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r44.0<1>:ud   r28.0<8;8,1>:ud   3:w              
        add (16|M0)              r46.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r52.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r42.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r8.0<1>:d     r6.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    6:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r44.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r36.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    7:w              
        add (16|M0)              r2.0<1>:d     r48.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r40.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r46     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r24:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r16.0<1>:d    r56.0<8;8,1>:d    r8.0<0;1,0>:d    {Compacted}
        send (16|M0)             r48:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r18.0<1>:d    r7.0<0;1,0>:d     64:w              
        send (16|M0)             r44:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r58.3<0;1,0>:d    8:w              
(W)     mul (1|M0)               r5.2<1>:d     r7.0<0;1,0>:d     64:w              
        shl (16|M0)              r28.0<1>:d    r24.0<8;8,1>:d    15:w              
        shr (16|M0)              r34.0<1>:ud   r24.0<8;8,1>:ud   17:w              
        shl (16|M0)              r36.0<1>:d    r24.0<8;8,1>:d    13:w              
        shr (16|M0)              r42.0<1>:ud   r24.0<8;8,1>:ud   19:w              
        shr (16|M0)              r2.0<1>:ud    r24.0<8;8,1>:ud   10:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r50.0<1>:d    r44.0<8;8,1>:d    25:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r52.0<1>:ud   r44.0<8;8,1>:ud   7:w              
        shl (16|M0)              r34.0<1>:d    r44.0<8;8,1>:d    14:w              
        shr (16|M0)              r42.0<1>:ud   r44.0<8;8,1>:ud   18:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r50.0<1>:d    r50.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r2.0<1>:ud    r44.0<8;8,1>:ud   3:w              
        xor (16|M0)              r50.0<1>:d    r50.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r56.0<8;8,1>:d    r18.0<0;1,0>:d   {Compacted}
        xor (16|M0)              r50.0<1>:d    r50.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r50.0<1>:d    r40.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        add (16|M0)              r50.0<1>:d    r50.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r8      r50     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r24:w    r46     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r28     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r20     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r2:w     r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r36.0<1>:d    r24.0<8;8,1>:d    15:w              
        shr (16|M0)              r40.0<1>:ud   r24.0<8;8,1>:ud   17:w              
        shl (16|M0)              r42.0<1>:d    r24.0<8;8,1>:d    13:w              
        shr (16|M0)              r44.0<1>:ud   r24.0<8;8,1>:ud   19:w              
        shl (16|M0)              r48.0<1>:d    r32.0<8;8,1>:d    25:w              
        shr (16|M0)              r46.0<1>:ud   r32.0<8;8,1>:ud   7:w              
        shl (16|M0)              r18.0<1>:d    r32.0<8;8,1>:d    14:w              
        shr (16|M0)              r50.0<1>:ud   r32.0<8;8,1>:ud   18:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r16.0<1>:ud   r24.0<8;8,1>:ud   10:w              
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r20.0<1>:ud   r32.0<8;8,1>:ud   3:w              
        add (16|M0)              r24.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r36.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r40.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        sends (16|M0)            null:w   r24     r48     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r42:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r18:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r28     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r34.0<1>:d    r42.0<8;8,1>:d    15:w              
        shr (16|M0)              r20.0<1>:ud   r42.0<8;8,1>:ud   17:w              
        shl (16|M0)              r36.0<1>:d    r42.0<8;8,1>:d    13:w              
        shr (16|M0)              r40.0<1>:ud   r42.0<8;8,1>:ud   19:w              
        shl (16|M0)              r2.0<1>:d     r16.0<8;8,1>:d    25:w              
        shr (16|M0)              r8.0<1>:ud    r16.0<8;8,1>:ud   7:w              
        shl (16|M0)              r44.0<1>:d    r16.0<8;8,1>:d    14:w              
        shr (16|M0)              r46.0<1>:ud   r16.0<8;8,1>:ud   18:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r42.0<8;8,1>:ud   10:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r20.0<1>:ud   r16.0<8;8,1>:ud   3:w              
        add (16|M0)              r36.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    9:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r34.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    10:w              
        add (16|M0)              r2.0<1>:d     r28.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r32.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r36     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r8:w     r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r40:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r14:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r42:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r16.0<1>:d    r8.0<8;8,1>:d     15:w              
        shr (16|M0)              r18.0<1>:ud   r8.0<8;8,1>:ud    17:w              
        shl (16|M0)              r20.0<1>:d    r8.0<8;8,1>:d     13:w              
        shr (16|M0)              r28.0<1>:ud   r8.0<8;8,1>:ud    19:w              
        shl (16|M0)              r32.0<1>:d    r40.0<8;8,1>:d    25:w              
        shr (16|M0)              r2.0<1>:ud    r40.0<8;8,1>:ud   7:w              
        shl (16|M0)              r24.0<1>:d    r40.0<8;8,1>:d    14:w              
        shr (16|M0)              r34.0<1>:ud   r40.0<8;8,1>:ud   18:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r8.0<8;8,1>:ud    10:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r40.0<8;8,1>:ud   3:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r58.3<0;1,0>:d    11:w              
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r16.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r8      r32     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r2:w     r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r12:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r20:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r24:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r34.0<1>:d    r2.0<8;8,1>:d     15:w              
        shr (16|M0)              r14.0<1>:ud   r2.0<8;8,1>:ud    17:w              
        shl (16|M0)              r16.0<1>:d    r2.0<8;8,1>:d     13:w              
        shr (16|M0)              r18.0<1>:ud   r2.0<8;8,1>:ud    19:w              
        shl (16|M0)              r28.0<1>:d    r12.0<8;8,1>:d    25:w              
        shr (16|M0)              r32.0<1>:ud   r12.0<8;8,1>:ud   7:w              
        shl (16|M0)              r36.0<1>:d    r12.0<8;8,1>:d    14:w              
        shr (16|M0)              r38.0<1>:ud   r12.0<8;8,1>:ud   18:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r2.0<8;8,1>:ud    10:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r12.0<8;8,1>:ud   3:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r34.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r34     r28     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r2:w     r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r10:w    r54     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r12:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r26.0<1>:d    r2.0<8;8,1>:d     15:w              
        shr (16|M0)              r20.0<1>:ud   r2.0<8;8,1>:ud    17:w              
        shl (16|M0)              r14.0<1>:d    r2.0<8;8,1>:d     13:w              
        shr (16|M0)              r18.0<1>:ud   r2.0<8;8,1>:ud    19:w              
        shl (16|M0)              r24.0<1>:d    r10.0<8;8,1>:d    25:w              
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   7:w              
        shl (16|M0)              r8.0<1>:d     r10.0<8;8,1>:d    14:w              
        shr (16|M0)              r32.0<1>:ud   r10.0<8;8,1>:ud   18:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r2.0<8;8,1>:ud    10:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r20.0<1>:ud   r10.0<8;8,1>:ud   3:w              
        add (16|M0)              r2.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r26.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r2      r24     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(~f0.1) break (16|M0)                        L6072                  L6072                
L6024:
        send (16|M0)             r30:w    r34     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r54     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r58.3<1>:d    r58.3<0;1,0>:d    12:w              
L6072:
        while (16|M0)                        L1024                                
L6088:
        mov (16|M0)              r44.0<1>:d    r4.0<0;1,0>:d                    {Compacted}
        mov (16|M0)              r48.0<1>:d    r4.1<0;1,0>:d                    {Compacted}
        mov (16|M0)              r50.0<1>:d    r4.2<0;1,0>:d                    {Compacted}
        mov (16|M0)              r42.0<1>:d    r4.3<0;1,0>:d                    {Compacted}
        mov (16|M0)              r40.0<1>:d    r4.4<0;1,0>:d                    {Compacted}
        mov (16|M0)              r36.0<1>:d    r4.5<0;1,0>:d                    {Compacted}
        mov (16|M0)              r38.0<1>:d    r4.6<0;1,0>:d                   
        mov (16|M0)              r46.0<1>:d    r4.7<0;1,0>:d                   
(W)     mov (1|M0)               r58.3<1>:d    0:w                              
L6184:
(W)     shl (1|M0)               r2.0<1>:d     r58.3<0;1,0>:d    2:w              
        shl (16|M0)              r12.0<1>:d    r40.0<8;8,1>:d    26:w              
(W)     mul (1|M0)               r2.1<1>:d     r58.3<0;1,0>:d    64:w              
        shr (16|M0)              r14.0<1>:ud   r40.0<8;8,1>:ud   6:w              
        shl (16|M0)              r16.0<1>:d    r40.0<8;8,1>:d    21:w              
(W)     mov (1|M0)               r3.2<1>:ud    r2.0<0;1,0>:ud                   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r40.0<8;8,1>:ud   11:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r20.0<1>:d    r40.0<8;8,1>:d    7:w              
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        shr (16|M0)              r22.0<1>:ud   r40.0<8;8,1>:ud   25:w              
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    ~r40.0<8;8,1>:d  {Compacted}
        and (16|M0)              r8.0<1>:d     r40.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r5.2<1>:d     r58.3<0;1,0>:d    1:w              
        shr (16|M0)              r30.0<1>:ud   r44.0<8;8,1>:ud   22:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r5.2<0;1,0>:d     64:w              
        shl (16|M0)              r24.0<1>:d    r44.0<8;8,1>:d    30:w              
        xor (16|M0)              r32.0<1>:d    r48.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r46.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r3.0<1>:d     r58.3<0;1,0>:d    2:w              
        xor (16|M0)              r34.0<1>:d    r44.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        and (16|M0)              r32.0<1>:d    r44.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r8.0<1>:ud    r44.0<8;8,1>:ud   2:w              
(W)     mul (1|M0)               r6.0<1>:d     r3.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r3.2<1>:d     r2.0<0;1,0>:d     16:w              
(W)     or (1|M0)                r7.4<1>:d     r58.3<0;1,0>:d    13:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r12.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        shl (16|M0)              r12.0<1>:d    r44.0<8;8,1>:d    19:w              
        add (16|M0)              r18.0<1>:d    r14.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r44.0<8;8,1>:ud   13:w              
        shl (16|M0)              r14.0<1>:d    r44.0<8;8,1>:d    10:w              
        add (16|M0)              r26.0<1>:d    r42.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r26.0<8;8,1>:d    21:w              
        shr (16|M0)              r20.0<1>:ud   r26.0<8;8,1>:ud   11:w              
        shl (16|M0)              r28.0<1>:d    r26.0<8;8,1>:d    26:w              
        shr (16|M0)              r16.0<1>:ud   r26.0<8;8,1>:ud   6:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        send (16|M0)             r20:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r26.0<8;8,1>:d    7:w              
        and (16|M0)              r16.0<1>:d    r48.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r26.0<8;8,1>:ud   25:w              
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r26.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r36.0<8;8,1>:d    ~r26.0<8;8,1>:d  {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r2.1<1>:d     r58.3<0;1,0>:d    3:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.1<1>:d     r58.3<0;1,0>:d    4:w              
        add (16|M0)              r28.0<1>:d    r38.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r34.0<1>:d    r8.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r2.1<0;1,0>:d     64:w              
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        add (16|M0)              r10.0<1>:d    r28.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r8.0<8;8,1>:ud    2:w              
        add (16|M0)              r18.0<1>:d    r50.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r24.0<1>:d    r18.0<8;8,1>:d    26:w              
        shr (16|M0)              r30.0<1>:ud   r18.0<8;8,1>:ud   6:w              
        shl (16|M0)              r22.0<1>:d    r18.0<8;8,1>:d    21:w              
        shr (16|M0)              r12.0<1>:ud   r18.0<8;8,1>:ud   11:w              
        shl (16|M0)              r32.0<1>:d    r18.0<8;8,1>:d    7:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r8.0<8;8,1>:d     19:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r8.0<8;8,1>:d     30:w              
        shr (16|M0)              r16.0<1>:ud   r18.0<8;8,1>:ud   25:w              
        and (16|M0)              r14.0<1>:d    r18.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r8.0<8;8,1>:ud    13:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r16.0<1>:d    r8.0<8;8,1>:d     10:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r8.0<8;8,1>:ud    22:w              
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        send (16|M0)             r28:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r40.0<8;8,1>:d    ~r18.0<8;8,1>:d  {Compacted}
        add (16|M0)              r24.0<1>:d    r36.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        send (16|M0)             r32:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r6.0<1>:d     r58.3<0;1,0>:d    5:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r44.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r6.0<0;1,0>:d     64:w              
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r10.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r20.0<8;8,1>:ud   2:w              
        add (16|M0)              r36.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r48.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r16.0<1>:d    r38.0<8;8,1>:d    26:w              
        shr (16|M0)              r14.0<1>:ud   r38.0<8;8,1>:ud   6:w              
        shl (16|M0)              r34.0<1>:d    r38.0<8;8,1>:d    21:w              
        shr (16|M0)              r10.0<1>:ud   r38.0<8;8,1>:ud   11:w              
        shl (16|M0)              r12.0<1>:d    r38.0<8;8,1>:d    7:w              
        shr (16|M0)              r22.0<1>:ud   r38.0<8;8,1>:ud   25:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r26.0<8;8,1>:d    ~r38.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r20.0<8;8,1>:d    30:w              
        shr (16|M0)              r22.0<1>:ud   r20.0<8;8,1>:ud   13:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r34.0<1>:d    r20.0<8;8,1>:d    19:w              
        xor (16|M0)              r28.0<1>:d    r8.0<8;8,1>:d     r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r40.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    10:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r40.0<1>:ud   r20.0<8;8,1>:ud   22:w              
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r20.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
(W)     or (1|M0)                r3.0<1>:d     r58.3<0;1,0>:d    6:w              
(W)     or (1|M0)                r3.2<1>:d     r2.0<0;1,0>:d     32:w              
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r16.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r8.0<8;8,1>:d     r44.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
(W)     or (1|M0)                r5.2<1>:d     r58.3<0;1,0>:d    7:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r44.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r28.0<1>:d    r16.0<8;8,1>:d    26:w              
        shl (16|M0)              r34.0<1>:d    r16.0<8;8,1>:d    21:w              
        shr (16|M0)              r40.0<1>:ud   r16.0<8;8,1>:ud   25:w              
        add (16|M0)              r24.0<1>:d    r36.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r36.0<1>:d    r16.0<8;8,1>:d    7:w              
        and (16|M0)              r12.0<1>:d    r16.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r24.0<8;8,1>:d    30:w              
        shr (16|M0)              r30.0<1>:ud   r24.0<8;8,1>:ud   2:w              
        shl (16|M0)              r32.0<1>:d    r24.0<8;8,1>:d    19:w              
        shr (16|M0)              r42.0<1>:ud   r24.0<8;8,1>:ud   13:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r16.0<8;8,1>:ud   6:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r46.0<1>:d    r24.0<8;8,1>:d    10:w              
        xor (16|M0)              r40.0<1>:d    r20.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r16.0<8;8,1>:ud   11:w              
        and (16|M0)              r42.0<1>:d    r18.0<8;8,1>:d    ~r16.0<8;8,1>:d  {Compacted}
        and (16|M0)              r40.0<1>:d    r24.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r24.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r34.0<1>:ud   r24.0<8;8,1>:ud   22:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        and (16|M0)              r36.0<1>:d    r20.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r28.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r10.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r24.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r12.0<8;8,1>:ud   2:w              
        shl (16|M0)              r34.0<1>:d    r12.0<8;8,1>:d    10:w              
        and (16|M0)              r42.0<1>:d    r12.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r28.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        shl (16|M0)              r28.0<1>:d    r12.0<8;8,1>:d    19:w              
        add (16|M0)              r32.0<1>:d    r30.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r12.0<8;8,1>:d    30:w              
        shr (16|M0)              r30.0<1>:ud   r12.0<8;8,1>:ud   13:w              
        add (16|M0)              r44.0<1>:d    r8.0<8;8,1>:d     r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r12.0<8;8,1>:ud   22:w              
        shl (16|M0)              r36.0<1>:d    r44.0<8;8,1>:d    21:w              
        send (16|M0)             r30:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r40.0<1>:ud   r44.0<8;8,1>:ud   11:w              
        shl (16|M0)              r28.0<1>:d    r44.0<8;8,1>:d    26:w              
        shr (16|M0)              r8.0<1>:ud    r44.0<8;8,1>:ud   6:w              
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r44.0<8;8,1>:d    7:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r44.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r3.0<0;1,0>:d     64:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r32.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r44.0<8;8,1>:ud   25:w              
        and (16|M0)              r32.0<1>:d    r38.0<8;8,1>:d    ~r44.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r42.0<1>:d    r12.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r22.0<8;8,1>:d    19:w              
        and (16|M0)              r42.0<1>:d    r22.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r18.0<1>:d    r22.0<8;8,1>:d    30:w              
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r22.0<8;8,1>:ud   2:w              
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
(W)     or (1|M0)                r2.1<1>:d     r58.3<0;1,0>:d    8:w              
(W)     mul (1|M0)               r5.2<1>:d     r2.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.1<1>:d     r58.3<0;1,0>:d    9:w              
        add (16|M0)              r8.0<1>:d     r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r22.0<8;8,1>:ud   13:w              
        shr (16|M0)              r30.0<1>:ud   r22.0<8;8,1>:ud   22:w              
        add (16|M0)              r34.0<1>:d    r20.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        send (16|M0)             r28:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r20.0<1>:d    r22.0<8;8,1>:d    10:w              
        shl (16|M0)              r40.0<1>:d    r34.0<8;8,1>:d    26:w              
        shr (16|M0)              r36.0<1>:ud   r34.0<8;8,1>:ud   6:w              
        shl (16|M0)              r14.0<1>:d    r34.0<8;8,1>:d    21:w              
        shr (16|M0)              r10.0<1>:ud   r34.0<8;8,1>:ud   11:w              
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r34.0<8;8,1>:ud   25:w              
        shl (16|M0)              r30.0<1>:d    r34.0<8;8,1>:d    7:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r12.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r16.0<8;8,1>:d    ~r34.0<8;8,1>:d  {Compacted}
        and (16|M0)              r10.0<1>:d    r34.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r8.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r40.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r28.0<1>:d    r14.0<8;8,1>:d    30:w              
        add (16|M0)              r18.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r26.0<1>:d    r18.0<8;8,1>:d    26:w              
        shr (16|M0)              r36.0<1>:ud   r18.0<8;8,1>:ud   6:w              
        shl (16|M0)              r32.0<1>:d    r18.0<8;8,1>:d    21:w              
        shr (16|M0)              r30.0<1>:ud   r18.0<8;8,1>:ud   11:w              
        shl (16|M0)              r38.0<1>:d    r18.0<8;8,1>:d    7:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r36.0<1>:d    r14.0<8;8,1>:d    19:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r14.0<8;8,1>:ud   2:w              
        shr (16|M0)              r20.0<1>:ud   r18.0<8;8,1>:ud   25:w              
        and (16|M0)              r10.0<1>:d    r18.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r14.0<8;8,1>:ud   13:w              
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r44.0<8;8,1>:d    ~r18.0<8;8,1>:d  {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        send (16|M0)             r32:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r20.0<1>:d    r14.0<8;8,1>:d    10:w              
        shr (16|M0)              r38.0<1>:ud   r14.0<8;8,1>:ud   22:w              
        add (16|M0)              r26.0<1>:d    r16.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r22.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        send (16|M0)             r38:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        and (16|M0)              r10.0<1>:d    r22.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        and (16|M0)              r16.0<1>:d    r14.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.2<1>:d     r58.3<0;1,0>:d    10:w              
(W)     or (1|M0)                r3.2<1>:d     r2.0<0;1,0>:d     48:w              
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r5.2<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.2<1>:d     r58.3<0;1,0>:d    12:w              
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r24.0<1>:d    r8.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r5.2<1>:d     r58.3<0;1,0>:d    14:w              
        shr (16|M0)              r36.0<1>:ud   r24.0<8;8,1>:ud   2:w              
        add (16|M0)              r20.0<1>:d    r26.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r24.0<8;8,1>:d    30:w              
        add (16|M0)              r10.0<1>:d    r12.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r16.0<1>:d    r10.0<8;8,1>:d    26:w              
        shr (16|M0)              r8.0<1>:ud    r10.0<8;8,1>:ud   6:w              
        shl (16|M0)              r28.0<1>:d    r10.0<8;8,1>:d    21:w              
        shr (16|M0)              r30.0<1>:ud   r10.0<8;8,1>:ud   11:w              
        shl (16|M0)              r40.0<1>:d    r10.0<8;8,1>:d    7:w              
        shr (16|M0)              r42.0<1>:ud   r10.0<8;8,1>:ud   25:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r12.0<1>:d    r10.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r34.0<8;8,1>:d    ~r10.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r42.0<1>:ud   r24.0<8;8,1>:ud   13:w              
        shl (16|M0)              r28.0<1>:d    r24.0<8;8,1>:d    19:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r14.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r40.0<1>:d    r24.0<8;8,1>:d    10:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r44.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r44.0<1>:ud   r24.0<8;8,1>:ud   22:w              
        and (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r16.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r8.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r14.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
(W)     or (1|M0)                r3.0<1>:d     r58.3<0;1,0>:d    11:w              
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        send (16|M0)             r26:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r22.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r16.0<1>:d    r12.0<8;8,1>:d    30:w              
        shr (16|M0)              r36.0<1>:ud   r12.0<8;8,1>:ud   2:w              
        shl (16|M0)              r38.0<1>:d    r12.0<8;8,1>:d    19:w              
        shr (16|M0)              r42.0<1>:ud   r12.0<8;8,1>:ud   13:w              
        shl (16|M0)              r28.0<1>:d    r20.0<8;8,1>:d    26:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r20.0<8;8,1>:ud   6:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r40.0<1>:d    r20.0<8;8,1>:d    21:w              
        shr (16|M0)              r36.0<1>:ud   r20.0<8;8,1>:ud   11:w              
        shr (16|M0)              r42.0<1>:ud   r20.0<8;8,1>:ud   25:w              
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r20.0<8;8,1>:d    7:w              
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r44.0<1>:d    r12.0<8;8,1>:d    10:w              
        and (16|M0)              r8.0<1>:d     r20.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r40.0<1>:ud   r12.0<8;8,1>:ud   22:w              
        and (16|M0)              r22.0<1>:d    r18.0<8;8,1>:d    ~r20.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        and (16|M0)              r42.0<1>:d    r12.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r38.0<1>:d    r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r34.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r12.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r30.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        and (16|M0)              r16.0<1>:d    r12.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r3.0<0;1,0>:d     64:w              
        shr (16|M0)              r34.0<1>:ud   r8.0<8;8,1>:ud    13:w              
        shl (16|M0)              r22.0<1>:d    r8.0<8;8,1>:d     30:w              
        shl (16|M0)              r40.0<1>:d    r8.0<8;8,1>:d     10:w              
        and (16|M0)              r44.0<1>:d    r8.0<8;8,1>:d     r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r28.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r28.0<1>:d    r8.0<8;8,1>:d     19:w              
        shr (16|M0)              r26.0<1>:ud   r8.0<8;8,1>:ud    2:w              
        add (16|M0)              r36.0<1>:d    r14.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        send (16|M0)             r34:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shr (16|M0)              r14.0<1>:ud   r8.0<8;8,1>:ud    22:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r36.0<8;8,1>:d    21:w              
        shr (16|M0)              r42.0<1>:ud   r36.0<8;8,1>:ud   11:w              
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r36.0<8;8,1>:ud   6:w              
        shl (16|M0)              r28.0<1>:d    r36.0<8;8,1>:d    26:w              
        shl (16|M0)              r16.0<1>:d    r36.0<8;8,1>:d    7:w              
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r36.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r8.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r36.0<8;8,1>:ud   25:w              
        and (16|M0)              r32.0<1>:d    r10.0<8;8,1>:d    ~r36.0<8;8,1>:d  {Compacted}
        and (16|M0)              r44.0<1>:d    r26.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r26.0<8;8,1>:d    19:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r18.0<1>:d    r26.0<8;8,1>:d    30:w              
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r26.0<8;8,1>:ud   2:w              
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r26.0<8;8,1>:ud   13:w              
        shr (16|M0)              r34.0<1>:ud   r26.0<8;8,1>:ud   22:w              
        add (16|M0)              r40.0<1>:d    r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r24.0<1>:d    r26.0<8;8,1>:d    10:w              
        shl (16|M0)              r42.0<1>:d    r40.0<8;8,1>:d    26:w              
        shr (16|M0)              r38.0<1>:ud   r40.0<8;8,1>:ud   6:w              
        send (16|M0)             r30:w    r28     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r22.0<1>:d    r40.0<8;8,1>:d    21:w              
        shr (16|M0)              r16.0<1>:ud   r40.0<8;8,1>:ud   11:w              
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r40.0<8;8,1>:d    7:w              
        shr (16|M0)              r34.0<1>:ud   r40.0<8;8,1>:ud   25:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r8.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r40.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r20.0<8;8,1>:d    ~r40.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r10.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        add (16|M0)              r14.0<1>:d    r42.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r22.0<8;8,1>:d    30:w              
        shr (16|M0)              r42.0<1>:ud   r22.0<8;8,1>:ud   13:w              
        add (16|M0)              r18.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r34.0<1>:d    r18.0<8;8,1>:d    21:w              
        shr (16|M0)              r32.0<1>:ud   r18.0<8;8,1>:ud   11:w              
        and (16|M0)              r12.0<1>:d    r18.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r36.0<8;8,1>:d    ~r18.0<8;8,1>:d  {Compacted}
        shl (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    26:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r22.0<8;8,1>:ud   2:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        shr (16|M0)              r38.0<1>:ud   r18.0<8;8,1>:ud   6:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r18.0<8;8,1>:d    7:w              
        send (16|M0)             r32:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shr (16|M0)              r16.0<1>:ud   r18.0<8;8,1>:ud   25:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r22.0<8;8,1>:d    19:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r16.0<1>:ud   r22.0<8;8,1>:ud   22:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r26.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r22.0<8;8,1>:d    10:w              
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        and (16|M0)              r34.0<1>:d    r22.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r20.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r16.0<1>:d    r7.4<0;1,0>:d     64:w              
(W)     or (1|M0)                r7.4<1>:d     r58.3<0;1,0>:d    15:w              
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r56.0<8;8,1>:d    r16.0<0;1,0>:d   {Compacted}
        and (16|M0)              r12.0<1>:d    r26.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r7.4<0;1,0>:d     64:w              
(W)     cmp (16|M0)   (lt)f0.0   null<1>:d     r7.4<0;1,0>:ud    0x3F:uw             
        send (16|M0)             r10:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r28.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r14.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r28.0<1>:d    r20.0<8;8,1>:d    19:w              
        add (16|M0)              r34.0<1>:d    r12.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    30:w              
        shr (16|M0)              r32.0<1>:ud   r20.0<8;8,1>:ud   13:w              
        add (16|M0)              r46.0<1>:d    r8.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r20.0<8;8,1>:ud   2:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r46.0<8;8,1>:d    26:w              
        shr (16|M0)              r30.0<1>:ud   r46.0<8;8,1>:ud   6:w              
        shl (16|M0)              r2.0<1>:d     r46.0<8;8,1>:d    21:w              
        shr (16|M0)              r42.0<1>:ud   r46.0<8;8,1>:ud   11:w              
        shl (16|M0)              r24.0<1>:d    r46.0<8;8,1>:d    7:w              
        shr (16|M0)              r16.0<1>:ud   r46.0<8;8,1>:ud   25:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r38.0<1>:d    r46.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        and (16|M0)              r30.0<1>:d    r40.0<8;8,1>:d    ~r46.0<8;8,1>:d  {Compacted}
        shl (16|M0)              r42.0<1>:d    r20.0<8;8,1>:d    10:w              
        xor (16|M0)              r16.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r2.0<1>:ud    r20.0<8;8,1>:ud   22:w              
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r20.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r36.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        add (16|M0)              r16.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        add (16|M0)              r42.0<1>:d    r34.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r14.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        send (16|M0)             r10:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r34.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r42.0<8;8,1>:d    30:w              
        shr (16|M0)              r28.0<1>:ud   r42.0<8;8,1>:ud   2:w              
        add (16|M0)              r38.0<1>:d    r26.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r2.0<1>:d     r42.0<8;8,1>:d    19:w              
        shr (16|M0)              r32.0<1>:ud   r42.0<8;8,1>:ud   13:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r38.0<8;8,1>:d    26:w              
        shr (16|M0)              r14.0<1>:ud   r38.0<8;8,1>:ud   6:w              
        shl (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    21:w              
        shr (16|M0)              r28.0<1>:ud   r38.0<8;8,1>:ud   11:w              
        shl (16|M0)              r16.0<1>:d    r38.0<8;8,1>:d    7:w              
        shr (16|M0)              r36.0<1>:ud   r38.0<8;8,1>:ud   25:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r38.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    ~r38.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r42.0<8;8,1>:d    10:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r2.0<1>:ud    r42.0<8;8,1>:ud   22:w              
        and (16|M0)              r34.0<1>:d    r42.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r40.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r42.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r50.0<1>:d    r8.0<8;8,1>:d     r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r42.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r2.0<1>:d     r50.0<8;8,1>:d    30:w              
        shr (16|M0)              r24.0<1>:ud   r50.0<8;8,1>:ud   2:w              
        shl (16|M0)              r26.0<1>:d    r50.0<8;8,1>:d    19:w              
        shr (16|M0)              r16.0<1>:ud   r50.0<8;8,1>:ud   13:w              
        shl (16|M0)              r34.0<1>:d    r50.0<8;8,1>:d    10:w              
        shr (16|M0)              r28.0<1>:ud   r50.0<8;8,1>:ud   22:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r50.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r32.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        add (16|M0)              r36.0<1>:d    r22.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        send (16|M0)             r12:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r48.0<1>:d    r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r16.0<1>:d    r36.0<8;8,1>:d    26:w              
        shr (16|M0)              r24.0<1>:ud   r36.0<8;8,1>:ud   6:w              
        shl (16|M0)              r26.0<1>:d    r36.0<8;8,1>:d    21:w              
        shr (16|M0)              r28.0<1>:ud   r36.0<8;8,1>:ud   11:w              
        shl (16|M0)              r14.0<1>:d    r36.0<8;8,1>:d    7:w              
        shr (16|M0)              r8.0<1>:ud    r36.0<8;8,1>:ud   25:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r2.0<1>:d     r36.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r10.0<1>:d    r46.0<8;8,1>:d    ~r36.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r48.0<8;8,1>:d    30:w              
        shr (16|M0)              r30.0<1>:ud   r48.0<8;8,1>:ud   2:w              
        shl (16|M0)              r32.0<1>:d    r48.0<8;8,1>:d    19:w              
        shr (16|M0)              r24.0<1>:ud   r48.0<8;8,1>:ud   13:w              
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r8.0<1>:d     r48.0<8;8,1>:d    10:w              
        xor (16|M0)              r26.0<1>:d    r50.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r10.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r48.0<8;8,1>:ud   22:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r18.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r50.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r48.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r16.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r20.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r10.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
(~f0.0) break (16|M0)                        L13392                  L13392                
L13376:
(W)     add (1|M0)               r58.3<1>:d    r58.3<0;1,0>:d    16:w              
L13392:
        while (16|M0)                        L6184                                
L13408:
        mov (16|M0)              r2.0<1>:d     r58.0<0;1,0>:d                   {Compacted}
        mov (16|M0)              r8.0<1>:d     r58.1<0;1,0>:d                   {Compacted}
        mov (16|M0)              r10.0<1>:d    r58.2<0;1,0>:d                   {Compacted}
        mov (16|M0)              r12.0<1>:d    -2147483648:d                              
        sends (16|M0)            null:w   r56     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r14.0<1>:d    0:w                              
        sends (16|M0)            null:w   r90     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r2.0<1>:d     640:w                              
        sends (16|M0)            null:w   r88     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r106.0<1>:d   r4.0<0;1,0>:d     r44.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r86     r108    0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r104.0<1>:d   r4.1<0;1,0>:d     r48.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r84     r12     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r102.0<1>:d   r4.2<0;1,0>:d     r50.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r82     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r100.0<1>:d   r4.3<0;1,0>:d     r42.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r80     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r98.0<1>:d    r4.4<0;1,0>:d     r40.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r78     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r96.0<1>:d    r4.5<0;1,0>:d     r36.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r76     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r94.0<1>:d    r4.6<0;1,0>:d     r38.0<8;8,1>:d  
        sends (16|M0)            null:w   r74     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r92.0<1>:d    r4.7<0;1,0>:d     r46.0<8;8,1>:d  
        sends (16|M0)            null:w   r72     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r34.0<1>:d    r58.0<0;1,0>:d                   {Compacted}
        sends (16|M0)            null:w   r70     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r30.0<1>:d    0:w                              
        sends (16|M0)            null:w   r68     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r66     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r62     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r60     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               r4.0<1>:d     16:w                              
L13856:
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -15:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     -7:w              
        shl (16|M0)              r16.0<1>:d    r30.0<8;8,1>:d    15:w              
        shr (16|M0)              r18.0<1>:ud   r30.0<8;8,1>:ud   17:w              
        shl (16|M0)              r20.0<1>:d    r30.0<8;8,1>:d    13:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        shr (16|M0)              r22.0<1>:ud   r30.0<8;8,1>:ud   19:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r30.0<8;8,1>:ud   10:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     64:w              
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r6.0<1>:d     r4.0<0;1,0>:d     -1:w              
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r7.0<1>:d     r6.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     1:w              
(W)     add (1|M0)               r7.1<1>:d     r4.0<0;1,0>:d     -14:w              
        xor (16|M0)              r32.0<1>:d    r16.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
(W)     add (1|M0)               r6.1<1>:d     r4.0<0;1,0>:d     -4:w              
(W)     add (1|M0)               r6.5<1>:d     r4.0<0;1,0>:d     -10:w              
        add (16|M0)              r16.0<1>:d    r56.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -13:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     -12:w              
(W)     cmp (16|M0)   (lt)f1.1   null<1>:d     r4.0<0;1,0>:ud    0x34:uw             
        shl (16|M0)              r2.0<1>:d     r10.0<8;8,1>:d    25:w              
        shr (16|M0)              r24.0<1>:ud   r10.0<8;8,1>:ud   7:w              
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    14:w              
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   18:w              
        shr (16|M0)              r18.0<1>:ud   r10.0<8;8,1>:ud   3:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r32.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r10.0<1>:d    r7.1<0;1,0>:d     64:w              
(W)     add (1|M0)               r10.1<1>:d    r4.0<0;1,0>:d     -6:w              
(W)     add (1|M0)               r7.1<1>:d     r4.0<0;1,0>:d     -9:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r56.0<8;8,1>:d    r10.0<0;1,0>:d   {Compacted}
(W)     mul (1|M0)               r11.0<1>:d    r10.1<0;1,0>:d    64:w              
(W)     mul (1|M0)               r10.0<1>:d    r7.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r10.1<1>:d    r4.0<0;1,0>:d     -5:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r7.0<1>:d     r6.1<0;1,0>:d     64:w              
(W)     add (1|M0)               r6.1<1>:d     r4.0<0;1,0>:d     5:w              
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r11.0<0;1,0>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r22.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r58.0<1>:d    r56.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -11:w              
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r20     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r14:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r38:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r40:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     3:w              
        shl (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    15:w              
        shr (16|M0)              r2.0<1>:ud    r14.0<8;8,1>:ud   17:w              
        shl (16|M0)              r26.0<1>:d    r14.0<8;8,1>:d    13:w              
        shr (16|M0)              r28.0<1>:ud   r14.0<8;8,1>:ud   19:w              
        shl (16|M0)              r42.0<1>:d    r38.0<8;8,1>:d    25:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r44.0<1>:ud   r38.0<8;8,1>:ud   7:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r2.0<1>:d     r38.0<8;8,1>:d    14:w              
        shr (16|M0)              r28.0<1>:ud   r38.0<8;8,1>:ud   18:w              
        shr (16|M0)              r30.0<1>:ud   r14.0<8;8,1>:ud   10:w              
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r38.0<8;8,1>:ud   3:w              
        xor (16|M0)              r32.0<1>:d    r24.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r10.1<0;1,0>:d    64:w              
(W)     or (1|M0)                r3.0<1>:d     r4.0<0;1,0>:d     2:w              
        add (16|M0)              r34.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r56.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r3.0<0;1,0>:d     64:w              
        add (16|M0)              r42.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r8      r42     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r42.0<1>:d    r56.0<8;8,1>:d    r10.0<0;1,0>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
        send (16|M0)             r22:w    r20     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r44:w    r42     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r14:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r46:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r26.0<1>:d    r22.0<8;8,1>:d    15:w              
        shr (16|M0)              r28.0<1>:ud   r22.0<8;8,1>:ud   17:w              
        shl (16|M0)              r30.0<1>:d    r22.0<8;8,1>:d    13:w              
        shr (16|M0)              r32.0<1>:ud   r22.0<8;8,1>:ud   19:w              
        shl (16|M0)              r48.0<1>:d    r44.0<8;8,1>:d    25:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r50.0<1>:ud   r44.0<8;8,1>:ud   7:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r28.0<1>:d    r44.0<8;8,1>:d    14:w              
        shr (16|M0)              r32.0<1>:ud   r44.0<8;8,1>:ud   18:w              
        shr (16|M0)              r34.0<1>:ud   r22.0<8;8,1>:ud   10:w              
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r44.0<8;8,1>:ud   3:w              
        xor (16|M0)              r38.0<1>:d    r26.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r38.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     -2:w              
        add (16|M0)              r48.0<1>:d    r40.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r10     r48     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r14:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r40:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r58     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r44:w    r42     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r26.0<1>:d    r14.0<8;8,1>:d    15:w              
        shr (16|M0)              r28.0<1>:ud   r14.0<8;8,1>:ud   17:w              
        shl (16|M0)              r30.0<1>:d    r14.0<8;8,1>:d    13:w              
        shr (16|M0)              r2.0<1>:ud    r14.0<8;8,1>:ud   19:w              
        shl (16|M0)              r46.0<1>:d    r40.0<8;8,1>:d    25:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r48.0<1>:ud   r40.0<8;8,1>:ud   7:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r28.0<1>:d    r40.0<8;8,1>:d    14:w              
        shr (16|M0)              r2.0<1>:ud    r40.0<8;8,1>:ud   18:w              
        shr (16|M0)              r32.0<1>:ud   r14.0<8;8,1>:ud   10:w              
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r14.0<1>:ud   r40.0<8;8,1>:ud   3:w              
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -3:w              
        xor (16|M0)              r34.0<1>:d    r26.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r26.0<1>:d    r7.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     6:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
        add (16|M0)              r36.0<1>:d    r34.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        add (16|M0)              r28.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     4:w              
        add (16|M0)              r46.0<1>:d    r36.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     mul (1|M0)               r5.2<1>:d     r6.5<0;1,0>:d     64:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
        add (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r22     r46     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r30:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r14:w    r28     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r28.0<1>:d    r56.0<8;8,1>:d    r26.0<0;1,0>:d   {Compacted}
        send (16|M0)             r50:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r48:w    r28     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r32.0<1>:d    r30.0<8;8,1>:d    15:w              
        shr (16|M0)              r34.0<1>:ud   r30.0<8;8,1>:ud   17:w              
        shl (16|M0)              r36.0<1>:d    r30.0<8;8,1>:d    13:w              
        shr (16|M0)              r40.0<1>:ud   r30.0<8;8,1>:ud   19:w              
        shr (16|M0)              r42.0<1>:ud   r30.0<8;8,1>:ud   10:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r52.0<1>:d    r48.0<8;8,1>:d    25:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r54.0<1>:ud   r48.0<8;8,1>:ud   7:w              
        shl (16|M0)              r34.0<1>:d    r48.0<8;8,1>:d    14:w              
        shr (16|M0)              r40.0<1>:ud   r48.0<8;8,1>:ud   18:w              
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r52.0<1>:d    r52.0<8;8,1>:d    r54.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r48.0<8;8,1>:ud   3:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r32.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r6.1<0;1,0>:d     64:w              
        xor (16|M0)              r52.0<1>:d    r52.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r46.0<1>:d    r44.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r44.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r52.0<1>:d    r52.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
(W)     add (1|M0)               r6.1<1>:d     r4.0<0;1,0>:d     -8:w              
(W)     add (1|M0)               r6.0<1>:d     r4.0<0;1,0>:d     7:w              
        add (16|M0)              r52.0<1>:d    r46.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        add (16|M0)              r52.0<1>:d    r52.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r14     r52     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r26:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r42:w    r40     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r46:w    r44     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r32.0<1>:d    r26.0<8;8,1>:d    15:w              
        shr (16|M0)              r34.0<1>:ud   r26.0<8;8,1>:ud   17:w              
        shl (16|M0)              r36.0<1>:d    r26.0<8;8,1>:d    13:w              
        shr (16|M0)              r30.0<1>:ud   r26.0<8;8,1>:ud   19:w              
        shr (16|M0)              r2.0<1>:ud    r26.0<8;8,1>:ud   10:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        send (16|M0)             r34:w    r28     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r42.0<8;8,1>:d    14:w              
        shl (16|M0)              r30.0<1>:d    r42.0<8;8,1>:d    25:w              
        add (16|M0)              r28.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r36.0<1>:ud   r42.0<8;8,1>:ud   7:w              
(W)     mul (1|M0)               r5.2<1>:d     r7.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     8:w              
        xor (16|M0)              r38.0<1>:d    r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r2.0<1>:ud    r42.0<8;8,1>:ud   18:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r42.0<8;8,1>:ud   3:w              
        add (16|M0)              r48.0<1>:d    r38.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r26.0<1>:d    r7.1<0;1,0>:d     64:w              
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r56.0<8;8,1>:d    r26.0<0;1,0>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r48.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r28     r30     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r44:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r38:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r42:w    r40     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r36.0<1>:d    r44.0<8;8,1>:d    15:w              
        shr (16|M0)              r2.0<1>:ud    r44.0<8;8,1>:ud   17:w              
        shl (16|M0)              r30.0<1>:d    r44.0<8;8,1>:d    13:w              
        shr (16|M0)              r46.0<1>:ud   r44.0<8;8,1>:ud   19:w              
        shl (16|M0)              r48.0<1>:d    r38.0<8;8,1>:d    25:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r50.0<1>:d    r38.0<8;8,1>:d    14:w              
        shr (16|M0)              r52.0<1>:ud   r38.0<8;8,1>:ud   18:w              
        shr (16|M0)              r2.0<1>:ud    r38.0<8;8,1>:ud   7:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r44.0<8;8,1>:ud   10:w              
        or (16|M0)               r50.0<1>:d    r50.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r16.0<1>:ud   r38.0<8;8,1>:ud   3:w              
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r26.0<1>:d    r6.1<0;1,0>:d     64:w              
(W)     mul (1|M0)               r5.2<1>:d     r6.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r6.0<1>:d     r4.0<0;1,0>:d     9:w              
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r36.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r56.0<8;8,1>:d    r26.0<0;1,0>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r40.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r30     r48     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r2:w     r28     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r20     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r36:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r40.0<1>:d    r2.0<8;8,1>:d     15:w              
        shr (16|M0)              r42.0<1>:ud   r2.0<8;8,1>:ud    17:w              
        shl (16|M0)              r44.0<1>:d    r2.0<8;8,1>:d     13:w              
        shr (16|M0)              r46.0<1>:ud   r2.0<8;8,1>:ud    19:w              
        shl (16|M0)              r28.0<1>:d    r34.0<8;8,1>:d    25:w              
        shr (16|M0)              r48.0<1>:ud   r34.0<8;8,1>:ud   7:w              
        shl (16|M0)              r26.0<1>:d    r34.0<8;8,1>:d    14:w              
        shr (16|M0)              r50.0<1>:ud   r34.0<8;8,1>:ud   18:w              
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r20.0<1>:ud   r2.0<8;8,1>:ud    10:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r34.0<8;8,1>:ud   3:w              
        add (16|M0)              r44.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r7.0<0;1,0>:d     64:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r40.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r42.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r44     r28     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r2:w     r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r20:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r26:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r16.0<1>:d    r2.0<8;8,1>:d     15:w              
        shr (16|M0)              r32.0<1>:ud   r2.0<8;8,1>:ud    17:w              
        shl (16|M0)              r40.0<1>:d    r2.0<8;8,1>:d     13:w              
        shr (16|M0)              r42.0<1>:ud   r2.0<8;8,1>:ud    19:w              
        shl (16|M0)              r28.0<1>:d    r20.0<8;8,1>:d    25:w              
        shr (16|M0)              r30.0<1>:ud   r20.0<8;8,1>:ud   7:w              
        shl (16|M0)              r36.0<1>:d    r20.0<8;8,1>:d    14:w              
        shr (16|M0)              r46.0<1>:ud   r20.0<8;8,1>:ud   18:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r2.0<8;8,1>:ud    10:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r20.0<8;8,1>:ud   3:w              
        add (16|M0)              r40.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r6.0<0;1,0>:d     64:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r16.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r38.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r40     r28     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r2:w     r44     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r20:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r26:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r30.0<1>:d    r2.0<8;8,1>:d     15:w              
        shr (16|M0)              r8.0<1>:ud    r2.0<8;8,1>:ud    17:w              
        shl (16|M0)              r32.0<1>:d    r2.0<8;8,1>:d     13:w              
        shr (16|M0)              r34.0<1>:ud   r2.0<8;8,1>:ud    19:w              
        shl (16|M0)              r28.0<1>:d    r16.0<8;8,1>:d    25:w              
        shr (16|M0)              r36.0<1>:ud   r16.0<8;8,1>:ud   7:w              
        shl (16|M0)              r38.0<1>:d    r16.0<8;8,1>:d    14:w              
        shr (16|M0)              r42.0<1>:ud   r16.0<8;8,1>:ud   18:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r2.0<8;8,1>:ud    10:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r16.0<8;8,1>:ud   3:w              
        add (16|M0)              r32.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     10:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r30.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     11:w              
        add (16|M0)              r28.0<1>:d    r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r32     r28     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r2:w     r40     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r26:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r8.0<1>:d     r2.0<8;8,1>:d     15:w              
        shr (16|M0)              r36.0<1>:ud   r2.0<8;8,1>:ud    17:w              
        shl (16|M0)              r16.0<1>:d    r2.0<8;8,1>:d     13:w              
        shr (16|M0)              r20.0<1>:ud   r2.0<8;8,1>:ud    19:w              
        shl (16|M0)              r28.0<1>:d    r22.0<8;8,1>:d    25:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r22.0<8;8,1>:ud   7:w              
        shl (16|M0)              r38.0<1>:d    r22.0<8;8,1>:d    14:w              
        shr (16|M0)              r36.0<1>:ud   r22.0<8;8,1>:ud   18:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r2.0<8;8,1>:ud    10:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r16.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r2.0<1>:ud    r22.0<8;8,1>:ud   3:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r8.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r28.0<1>:d    r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r34     r28     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r8:w     r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r10:w    r58     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r18:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r20.0<1>:d    r8.0<8;8,1>:d     15:w              
        shr (16|M0)              r22.0<1>:ud   r8.0<8;8,1>:ud    17:w              
        shl (16|M0)              r2.0<1>:d     r8.0<8;8,1>:d     13:w              
        shr (16|M0)              r12.0<1>:ud   r8.0<8;8,1>:ud    19:w              
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    25:w              
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   7:w              
        shl (16|M0)              r30.0<1>:d    r10.0<8;8,1>:d    14:w              
        shr (16|M0)              r32.0<1>:ud   r10.0<8;8,1>:ud   18:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r8.0<8;8,1>:ud    10:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r22.0<1>:ud   r10.0<8;8,1>:ud   3:w              
        add (16|M0)              r2.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r12.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r2      r26     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(~f1.1) break (16|M0)                        L18904                  L18904                
L18856:
        send (16|M0)             r30:w    r34     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r58     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     12:w              
L18904:
        while (16|M0)                        L13856                                
L18920:
        mov (16|M0)              r44.0<1>:d    r106.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r38.0<1>:d    r104.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r34.0<1>:d    r102.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r32.0<1>:d    r100.0<8;8,1>:d                  {Compacted}
        mov (16|M0)              r42.0<1>:d    r98.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r36.0<1>:d    r96.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r40.0<1>:d    r94.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r26.0<1>:d    r92.0<8;8,1>:d                   {Compacted}
(W)     mov (1|M0)               r4.0<1>:d     0:w                              
L19000:
(W)     shl (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     2:w              
        shl (16|M0)              r12.0<1>:d    r42.0<8;8,1>:d    26:w              
(W)     mul (1|M0)               r2.1<1>:d     r4.0<0;1,0>:d     64:w              
        shr (16|M0)              r14.0<1>:ud   r42.0<8;8,1>:ud   6:w              
        shl (16|M0)              r16.0<1>:d    r42.0<8;8,1>:d    21:w              
(W)     mov (1|M0)               r3.2<1>:ud    r2.0<0;1,0>:ud                   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r42.0<8;8,1>:ud   11:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r20.0<1>:d    r42.0<8;8,1>:d    7:w              
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        shr (16|M0)              r22.0<1>:ud   r42.0<8;8,1>:ud   25:w              
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r40.0<8;8,1>:d    ~r42.0<8;8,1>:d  {Compacted}
        and (16|M0)              r8.0<1>:d     r42.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     1:w              
        shr (16|M0)              r28.0<1>:ud   r44.0<8;8,1>:ud   2:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
        shr (16|M0)              r22.0<1>:ud   r44.0<8;8,1>:ud   13:w              
        shl (16|M0)              r16.0<1>:d    r44.0<8;8,1>:d    19:w              
        add (16|M0)              r12.0<1>:d    r26.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r44.0<8;8,1>:d    30:w              
        shr (16|M0)              r30.0<1>:ud   r44.0<8;8,1>:ud   22:w              
        shl (16|M0)              r20.0<1>:d    r44.0<8;8,1>:d    10:w              
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     2:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     4:w              
(W)     or (1|M0)                r3.2<1>:d     r2.0<0;1,0>:d     16:w              
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r6.0<1>:d     r4.0<0;1,0>:d     5:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     3:w              
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r12.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        add (16|M0)              r18.0<1>:d    r14.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r12.0<1>:d    r32.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r12.0<8;8,1>:d    26:w              
        shr (16|M0)              r22.0<1>:ud   r12.0<8;8,1>:ud   6:w              
        shl (16|M0)              r24.0<1>:d    r12.0<8;8,1>:d    21:w              
        shr (16|M0)              r28.0<1>:ud   r12.0<8;8,1>:ud   11:w              
        shl (16|M0)              r16.0<1>:d    r12.0<8;8,1>:d    7:w              
        shr (16|M0)              r20.0<1>:ud   r12.0<8;8,1>:ud   25:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r12.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r36.0<8;8,1>:d    ~r12.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r38.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r44.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r40.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        add (16|M0)              r16.0<1>:d    r18.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r20.0<1>:d    r16.0<8;8,1>:d    30:w              
        shr (16|M0)              r32.0<1>:ud   r16.0<8;8,1>:ud   2:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r14.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r34.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r8.0<8;8,1>:d     26:w              
        shr (16|M0)              r14.0<1>:ud   r8.0<8;8,1>:ud    6:w              
        shl (16|M0)              r30.0<1>:d    r8.0<8;8,1>:d     21:w              
        shr (16|M0)              r24.0<1>:ud   r8.0<8;8,1>:ud    11:w              
        shl (16|M0)              r18.0<1>:d    r8.0<8;8,1>:d     7:w              
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r16.0<8;8,1>:d    19:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r24.0<1>:ud   r16.0<8;8,1>:ud   13:w              
        shr (16|M0)              r26.0<1>:ud   r8.0<8;8,1>:ud    25:w              
        and (16|M0)              r28.0<1>:d    r8.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r32.0<1>:d    r42.0<8;8,1>:d    ~r8.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r16.0<8;8,1>:d    10:w              
        send (16|M0)             r34:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r16.0<8;8,1>:ud   22:w              
        xor (16|M0)              r18.0<1>:d    r44.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r32.0<1>:d    r44.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r36.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        add (16|M0)              r28.0<1>:d    r22.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r28.0<8;8,1>:ud   2:w              
        shl (16|M0)              r24.0<1>:d    r28.0<8;8,1>:d    19:w              
        shr (16|M0)              r32.0<1>:ud   r28.0<8;8,1>:ud   13:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r10.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r28.0<8;8,1>:d    30:w              
        shr (16|M0)              r34.0<1>:ud   r28.0<8;8,1>:ud   22:w              
        add (16|M0)              r14.0<1>:d    r38.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        shl (16|M0)              r26.0<1>:d    r14.0<8;8,1>:d    26:w              
        shr (16|M0)              r38.0<1>:ud   r14.0<8;8,1>:ud   6:w              
        send (16|M0)             r32:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r40.0<1>:d    r14.0<8;8,1>:d    21:w              
        shr (16|M0)              r30.0<1>:ud   r14.0<8;8,1>:ud   11:w              
        shl (16|M0)              r18.0<1>:d    r14.0<8;8,1>:d    7:w              
        shr (16|M0)              r20.0<1>:ud   r14.0<8;8,1>:ud   25:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r28.0<8;8,1>:d    10:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r16.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r34.0<1>:d    r12.0<8;8,1>:d    ~r14.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r38.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r16.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r42.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
(W)     or (1|M0)                r3.2<1>:d     r2.0<0;1,0>:d     32:w              
        add (16|M0)              r20.0<1>:d    r36.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r40.0<1>:d    r20.0<8;8,1>:d    19:w              
        shl (16|M0)              r34.0<1>:d    r20.0<8;8,1>:d    30:w              
        shr (16|M0)              r24.0<1>:ud   r20.0<8;8,1>:ud   2:w              
        shr (16|M0)              r42.0<1>:ud   r20.0<8;8,1>:ud   22:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r26.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r20.0<8;8,1>:ud   13:w              
        shl (16|M0)              r32.0<1>:d    r20.0<8;8,1>:d    10:w              
        add (16|M0)              r30.0<1>:d    r44.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        xor (16|M0)              r44.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r10.0<1>:d    r30.0<8;8,1>:d    26:w              
        shr (16|M0)              r36.0<1>:ud   r30.0<8;8,1>:ud   6:w              
        shl (16|M0)              r38.0<1>:d    r30.0<8;8,1>:d    21:w              
        shr (16|M0)              r22.0<1>:ud   r30.0<8;8,1>:ud   11:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        send (16|M0)             r36:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        and (16|M0)              r44.0<1>:d    r20.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r40.0<1>:ud   r30.0<8;8,1>:ud   25:w              
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r30.0<8;8,1>:d    7:w              
        and (16|M0)              r22.0<1>:d    r30.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r6.0<0;1,0>:d     64:w              
        add (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r6.0<1>:d     r4.0<0;1,0>:d     9:w              
        add (16|M0)              r24.0<1>:d    r18.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r8.0<8;8,1>:d     ~r30.0<8;8,1>:d  {Compacted}
        shl (16|M0)              r42.0<1>:d    r24.0<8;8,1>:d    30:w              
        add (16|M0)              r10.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r44.0<1>:ud   r24.0<8;8,1>:ud   2:w              
        shl (16|M0)              r40.0<1>:d    r24.0<8;8,1>:d    19:w              
        shr (16|M0)              r38.0<1>:ud   r24.0<8;8,1>:ud   13:w              
        add (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r24.0<8;8,1>:d    10:w              
        shr (16|M0)              r18.0<1>:ud   r24.0<8;8,1>:ud   22:w              
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r10.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r22.0<1>:d    r20.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r20.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r24.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     6:w              
        add (16|M0)              r34.0<1>:d    r16.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        send (16|M0)             r36:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r16.0<1>:d    r26.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
        shl (16|M0)              r38.0<1>:d    r34.0<8;8,1>:d    26:w              
        shr (16|M0)              r40.0<1>:ud   r34.0<8;8,1>:ud   6:w              
        shl (16|M0)              r18.0<1>:d    r34.0<8;8,1>:d    21:w              
        shr (16|M0)              r44.0<1>:ud   r34.0<8;8,1>:ud   11:w              
        shl (16|M0)              r10.0<1>:d    r34.0<8;8,1>:d    7:w              
        shr (16|M0)              r12.0<1>:ud   r34.0<8;8,1>:ud   25:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r34.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r14.0<8;8,1>:d    ~r34.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r42.0<1>:ud   r16.0<8;8,1>:ud   2:w              
        shl (16|M0)              r40.0<1>:d    r16.0<8;8,1>:d    30:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r16.0<8;8,1>:ud   13:w              
        shl (16|M0)              r18.0<1>:d    r16.0<8;8,1>:d    19:w              
        shl (16|M0)              r10.0<1>:d    r16.0<8;8,1>:d    10:w              
        add (16|M0)              r38.0<1>:d    r8.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r16.0<8;8,1>:ud   22:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        send (16|M0)             r26:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     7:w              
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     8:w              
(W)     mul (1|M0)               r5.2<1>:d     r2.1<0;1,0>:d     64:w              
(W)     mul (1|M0)               r2.1<1>:d     r6.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r6.0<1>:d     r4.0<0;1,0>:d     13:w              
        add (16|M0)              r32.0<1>:d    r38.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r24.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r36.0<8;8,1>:d    26:w              
        shr (16|M0)              r12.0<1>:ud   r36.0<8;8,1>:ud   6:w              
        shl (16|M0)              r42.0<1>:d    r36.0<8;8,1>:d    21:w              
        shr (16|M0)              r44.0<1>:ud   r36.0<8;8,1>:ud   11:w              
        shl (16|M0)              r8.0<1>:d     r36.0<8;8,1>:d    7:w              
        shr (16|M0)              r10.0<1>:ud   r36.0<8;8,1>:ud   25:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r36.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r30.0<8;8,1>:d    ~r36.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r16.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r42.0<1>:d    r24.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r14.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r32.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        shr (16|M0)              r40.0<1>:ud   r8.0<8;8,1>:ud    13:w              
        shl (16|M0)              r10.0<1>:d    r8.0<8;8,1>:d     30:w              
        add (16|M0)              r12.0<1>:d    r38.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r8.0<8;8,1>:ud    2:w              
        add (16|M0)              r18.0<1>:d    r20.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r18.0<8;8,1>:d    26:w              
        shr (16|M0)              r26.0<1>:ud   r18.0<8;8,1>:ud   6:w              
        shl (16|M0)              r38.0<1>:d    r18.0<8;8,1>:d    21:w              
        shr (16|M0)              r42.0<1>:ud   r18.0<8;8,1>:ud   11:w              
        shl (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    7:w              
        shr (16|M0)              r32.0<1>:ud   r18.0<8;8,1>:ud   25:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r8.0<8;8,1>:d     19:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r18.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r34.0<8;8,1>:d    ~r18.0<8;8,1>:d  {Compacted}
        shr (16|M0)              r32.0<1>:ud   r8.0<8;8,1>:ud    22:w              
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r8.0<8;8,1>:d     10:w              
        xor (16|M0)              r28.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r30.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        send (16|M0)             r40:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        and (16|M0)              r28.0<1>:d    r8.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
(W)     or (1|M0)                r3.0<1>:d     r4.0<0;1,0>:d     10:w              
(W)     or (1|M0)                r3.2<1>:d     r2.0<0;1,0>:d     48:w              
        add (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r20.0<1>:d    r14.0<8;8,1>:d    30:w              
        shr (16|M0)              r28.0<1>:ud   r14.0<8;8,1>:ud   13:w              
        shl (16|M0)              r30.0<1>:d    r14.0<8;8,1>:d    10:w              
        add (16|M0)              r42.0<1>:d    r22.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r14.0<8;8,1>:ud   2:w              
        xor (16|M0)              r40.0<1>:d    r8.0<8;8,1>:d     r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    19:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        shl (16|M0)              r32.0<1>:d    r26.0<8;8,1>:d    26:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r44.0<1>:ud   r26.0<8;8,1>:ud   6:w              
        shl (16|M0)              r46.0<1>:d    r26.0<8;8,1>:d    21:w              
        shr (16|M0)              r38.0<1>:ud   r26.0<8;8,1>:ud   11:w              
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        send (16|M0)             r24:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r10.0<1>:d    r26.0<8;8,1>:d    7:w              
        shr (16|M0)              r12.0<1>:ud   r26.0<8;8,1>:ud   25:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r38.0<1>:ud   r14.0<8;8,1>:ud   22:w              
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r26.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        and (16|M0)              r40.0<1>:d    r14.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r14.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r8.0<8;8,1>:d     r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r34.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r34.0<1>:d    r36.0<8;8,1>:d    ~r26.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r42.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r32.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        shl (16|M0)              r32.0<1>:d    r12.0<8;8,1>:d    19:w              
        shl (16|M0)              r34.0<1>:d    r12.0<8;8,1>:d    30:w              
        shr (16|M0)              r28.0<1>:ud   r12.0<8;8,1>:ud   2:w              
        and (16|M0)              r44.0<1>:d    r12.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r10.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r12.0<8;8,1>:ud   13:w              
        shr (16|M0)              r24.0<1>:ud   r12.0<8;8,1>:ud   22:w              
        add (16|M0)              r30.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r16.0<1>:d    r12.0<8;8,1>:d    10:w              
        shl (16|M0)              r38.0<1>:d    r30.0<8;8,1>:d    26:w              
        shr (16|M0)              r40.0<1>:ud   r30.0<8;8,1>:ud   6:w              
        shl (16|M0)              r42.0<1>:d    r30.0<8;8,1>:d    21:w              
        shr (16|M0)              r22.0<1>:ud   r30.0<8;8,1>:ud   11:w              
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        send (16|M0)             r40:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r30.0<8;8,1>:d    7:w              
        shr (16|M0)              r24.0<1>:ud   r30.0<8;8,1>:ud   25:w              
        and (16|M0)              r22.0<1>:d    r14.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r30.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r18.0<8;8,1>:d    ~r30.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r3.0<0;1,0>:d     64:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r36.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r20.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r24.0<1>:d    r28.0<8;8,1>:d    19:w              
        shr (16|M0)              r32.0<1>:ud   r28.0<8;8,1>:ud   13:w              
        shl (16|M0)              r34.0<1>:d    r28.0<8;8,1>:d    30:w              
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        shr (16|M0)              r42.0<1>:ud   r28.0<8;8,1>:ud   2:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r36.0<1>:d    r28.0<8;8,1>:d    10:w              
        shr (16|M0)              r22.0<1>:ud   r28.0<8;8,1>:ud   22:w              
        xor (16|M0)              r16.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     11:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     12:w              
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r2.1<0;1,0>:d     64:w              
        add (16|M0)              r10.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        send (16|M0)             r40:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r20.0<1>:d    r8.0<8;8,1>:d     r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r42.0<1>:d    r20.0<8;8,1>:d    26:w              
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r24.0<1>:ud   r20.0<8;8,1>:ud   6:w              
        shl (16|M0)              r22.0<1>:d    r20.0<8;8,1>:d    21:w              
        shr (16|M0)              r44.0<1>:ud   r20.0<8;8,1>:ud   11:w              
        add (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r8.0<1>:d     r20.0<8;8,1>:d    7:w              
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r16.0<1>:ud   r20.0<8;8,1>:ud   25:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r10.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        and (16|M0)              r32.0<1>:d    r26.0<8;8,1>:d    ~r20.0<8;8,1>:d  {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r20.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r34.0<1>:d    r38.0<8;8,1>:d    30:w              
        shr (16|M0)              r36.0<1>:ud   r38.0<8;8,1>:ud   2:w              
        xor (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r16.0<1>:ud   r38.0<8;8,1>:ud   13:w              
        shl (16|M0)              r22.0<1>:d    r38.0<8;8,1>:d    19:w              
        add (16|M0)              r42.0<1>:d    r18.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d    10:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r38.0<8;8,1>:ud   22:w              
        add (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r42.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        send (16|M0)             r32:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r22.0<1>:d    r38.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r42.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r14.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r36.0<1>:d    r40.0<8;8,1>:d    26:w              
        shr (16|M0)              r16.0<1>:ud   r40.0<8;8,1>:ud   6:w              
        shl (16|M0)              r42.0<1>:d    r40.0<8;8,1>:d    21:w              
        shr (16|M0)              r44.0<1>:ud   r40.0<8;8,1>:ud   11:w              
        shl (16|M0)              r8.0<1>:d     r40.0<8;8,1>:d    7:w              
        shr (16|M0)              r18.0<1>:ud   r40.0<8;8,1>:ud   25:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r40.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r30.0<8;8,1>:d    ~r40.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r42.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r26.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r24.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
        shl (16|M0)              r18.0<1>:d    r8.0<8;8,1>:d     30:w              
        shr (16|M0)              r26.0<1>:ud   r8.0<8;8,1>:ud    2:w              
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        shr (16|M0)              r34.0<1>:ud   r8.0<8;8,1>:ud    13:w              
        add (16|M0)              r16.0<1>:d    r36.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r42.0<1>:d    r8.0<8;8,1>:d     10:w              
        add (16|M0)              r10.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r10.0<8;8,1>:d    26:w              
        shr (16|M0)              r32.0<1>:ud   r10.0<8;8,1>:ud   6:w              
        shl (16|M0)              r36.0<1>:d    r10.0<8;8,1>:d    21:w              
        shr (16|M0)              r44.0<1>:ud   r10.0<8;8,1>:ud   11:w              
        shl (16|M0)              r22.0<1>:d    r10.0<8;8,1>:d    7:w              
        shr (16|M0)              r24.0<1>:ud   r10.0<8;8,1>:ud   25:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r12.0<1>:d    r10.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r20.0<8;8,1>:d    ~r10.0<8;8,1>:d  {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r8.0<8;8,1>:d     19:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r36.0<1>:ud   r8.0<8;8,1>:ud    22:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r30.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        send (16|M0)             r30:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        and (16|M0)              r22.0<1>:d    r38.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r8.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     15:w              
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.3<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W)     cmp (16|M0)   (lt)f1.0   null<1>:d     r5.2<0;1,0>:ud    0x3F:uw             
        add (16|M0)              r12.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r36.0<1>:d    r12.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r14.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        shl (16|M0)              r14.0<1>:d    r12.0<8;8,1>:d    30:w              
        add (16|M0)              r34.0<1>:d    r32.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r12.0<8;8,1>:d    19:w              
        shr (16|M0)              r32.0<1>:ud   r12.0<8;8,1>:ud   13:w              
        add (16|M0)              r46.0<1>:d    r28.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r12.0<8;8,1>:ud   2:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r46.0<8;8,1>:d    26:w              
        shr (16|M0)              r24.0<1>:ud   r46.0<8;8,1>:ud   6:w              
        shl (16|M0)              r16.0<1>:d    r46.0<8;8,1>:d    21:w              
        shr (16|M0)              r18.0<1>:ud   r46.0<8;8,1>:ud   11:w              
        shl (16|M0)              r2.0<1>:d     r46.0<8;8,1>:d    7:w              
        shr (16|M0)              r26.0<1>:ud   r46.0<8;8,1>:ud   25:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r24.0<1>:ud   r12.0<8;8,1>:ud   22:w              
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r18.0<1>:d    r12.0<8;8,1>:d    10:w              
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r8.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r6.0<0;1,0>:d     64:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r40.0<8;8,1>:d    ~r46.0<8;8,1>:d  {Compacted}
        add (16|M0)              r22.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
        and (16|M0)              r20.0<1>:d    r46.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        send (16|M0)             r28:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r3.0<1>:d     r4.0<0;1,0>:d     14:w              
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r3.0<0;1,0>:d     64:w              
        add (16|M0)              r42.0<1>:d    r34.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r14.0<1>:d    r42.0<8;8,1>:d    30:w              
        shr (16|M0)              r18.0<1>:ud   r42.0<8;8,1>:ud   2:w              
        shl (16|M0)              r30.0<1>:d    r42.0<8;8,1>:d    19:w              
        shr (16|M0)              r26.0<1>:ud   r42.0<8;8,1>:ud   13:w              
        shl (16|M0)              r20.0<1>:d    r42.0<8;8,1>:d    10:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r22.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r42.0<8;8,1>:ud   22:w              
        xor (16|M0)              r28.0<1>:d    r12.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r48.0<1>:d    r38.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r38.0<1>:d    r12.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r28.0<1>:d    r42.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r36.0<1>:d    r48.0<8;8,1>:d    21:w              
        shr (16|M0)              r24.0<1>:ud   r48.0<8;8,1>:ud   11:w              
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r48.0<8;8,1>:d    26:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        send (16|M0)             r24:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shr (16|M0)              r34.0<1>:ud   r48.0<8;8,1>:ud   6:w              
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r48.0<8;8,1>:d    7:w              
        shr (16|M0)              r26.0<1>:ud   r48.0<8;8,1>:ud   25:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r16.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r10.0<8;8,1>:d    ~r48.0<8;8,1>:d  {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r2.0<1>:d     r34.0<8;8,1>:d    30:w              
        shr (16|M0)              r14.0<1>:ud   r34.0<8;8,1>:ud   2:w              
        shl (16|M0)              r16.0<1>:d    r34.0<8;8,1>:d    19:w              
        shr (16|M0)              r18.0<1>:ud   r34.0<8;8,1>:ud   13:w              
        and (16|M0)              r20.0<1>:d    r48.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r56.0<8;8,1>:d    r5.3<0;1,0>:d    {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        send (16|M0)             r18:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shr (16|M0)              r28.0<1>:ud   r34.0<8;8,1>:ud   22:w              
        add (16|M0)              r32.0<1>:d    r40.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r34.0<8;8,1>:d    10:w              
        xor (16|M0)              r30.0<1>:d    r42.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r42.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r34.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r32.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r8.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r26.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r16.0<1>:d    r36.0<8;8,1>:d    26:w              
        shr (16|M0)              r24.0<1>:ud   r36.0<8;8,1>:ud   6:w              
        shl (16|M0)              r28.0<1>:d    r36.0<8;8,1>:d    21:w              
        shr (16|M0)              r32.0<1>:ud   r36.0<8;8,1>:ud   11:w              
        shl (16|M0)              r8.0<1>:d     r36.0<8;8,1>:d    7:w              
        shr (16|M0)              r20.0<1>:ud   r36.0<8;8,1>:ud   25:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r2.0<1>:d     r36.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r46.0<8;8,1>:d    ~r36.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r38.0<8;8,1>:d    30:w              
        shr (16|M0)              r26.0<1>:ud   r38.0<8;8,1>:ud   2:w              
        shl (16|M0)              r30.0<1>:d    r38.0<8;8,1>:d    19:w              
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r24.0<1>:ud   r38.0<8;8,1>:ud   13:w              
        shl (16|M0)              r20.0<1>:d    r38.0<8;8,1>:d    10:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r10.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r38.0<8;8,1>:ud   22:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r38.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
        mov (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d                   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r20.0<1>:d    r34.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r14.0<1>:d    r36.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r12.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r18.0<1>:d    r46.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r16.0<1>:d    r48.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r6.0<1>:d     r28.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r10.0<1>:d    r42.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r12.0<1>:d    r2.0<8;8,1>:d                    {Compacted}
        mov (16|M0)              r22.0<1>:d    r6.0<8;8,1>:d                    {Compacted}
(~f1.0) break (16|M0)                        L26312                  L26312                
L26256:
        mov (16|M0)              r32.0<1>:d    r42.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r44.0<1>:d    r6.0<8;8,1>:d                    {Compacted}
        mov (16|M0)              r40.0<1>:d    r48.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r26.0<1>:d    r46.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r42.0<1>:d    r2.0<8;8,1>:d                    {Compacted}
(W)     add (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     16:w              
L26312:
        while (16|M0)                        L19000                                
L26328:
        add (16|M0)              r36.0<1>:d    r106.0<8;8,1>:d   r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r104.0<8;8,1>:d   r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r102.0<8;8,1>:d   r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r100.0<8;8,1>:d   r10.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r56     r36     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r12.0<1>:d    r98.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r90     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r14.0<1>:d    r96.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r88     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r16.0<1>:d    r94.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r86     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r18.0<1>:d    r92.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r84     r12     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r20.0<1>:d    -2147483648:d                              
        sends (16|M0)            null:w   r82     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r22.0<1>:d    0:w                              
        sends (16|M0)            null:w   r80     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r2.0<1>:d     256:w                              
        sends (16|M0)            null:w   r78     r18     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r30.0<1>:d    0:w                              
        sends (16|M0)            null:w   r76     r20     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r74     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r72     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r70     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r68     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r66     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r62     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r60     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               r4.0<1>:d     16:w                              
L26728:
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -15:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     -7:w              
        shl (16|M0)              r16.0<1>:d    r30.0<8;8,1>:d    15:w              
        shr (16|M0)              r18.0<1>:ud   r30.0<8;8,1>:ud   17:w              
        shl (16|M0)              r20.0<1>:d    r30.0<8;8,1>:d    13:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        shr (16|M0)              r22.0<1>:ud   r30.0<8;8,1>:ud   19:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r30.0<8;8,1>:ud   10:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     64:w              
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r6.0<1>:d     r4.0<0;1,0>:d     -1:w              
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r7.1<1>:d     r4.0<0;1,0>:d     -14:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     1:w              
(W)     mul (1|M0)               r7.0<1>:d     r6.0<0;1,0>:d     64:w              
        xor (16|M0)              r32.0<1>:d    r16.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
(W)     cmp (16|M0)   (lt)f0.1   null<1>:d     r4.0<0;1,0>:ud    0x34:uw             
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r16.0<1>:d    r56.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -13:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     2:w              
        shl (16|M0)              r2.0<1>:d     r10.0<8;8,1>:d    25:w              
        shr (16|M0)              r24.0<1>:ud   r10.0<8;8,1>:ud   7:w              
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    14:w              
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   18:w              
        shr (16|M0)              r18.0<1>:ud   r10.0<8;8,1>:ud   3:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r32.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r10.0<1>:d    r7.1<0;1,0>:d     64:w              
(W)     add (1|M0)               r11.0<1>:d    r4.0<0;1,0>:d     -6:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r24.0<1>:d    r11.0<0;1,0>:d    64:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r10.0<0;1,0>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r24.0<0;1,0>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r22.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r36.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r20     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r14:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r2:w     r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r30.0<1>:d    r14.0<8;8,1>:d    15:w              
        shr (16|M0)              r32.0<1>:ud   r14.0<8;8,1>:ud   17:w              
        shl (16|M0)              r34.0<1>:d    r14.0<8;8,1>:d    13:w              
        shr (16|M0)              r36.0<1>:ud   r14.0<8;8,1>:ud   19:w              
        shl (16|M0)              r38.0<1>:d    r22.0<8;8,1>:d    25:w              
        shr (16|M0)              r40.0<1>:ud   r22.0<8;8,1>:ud   7:w              
        shl (16|M0)              r10.0<1>:d    r22.0<8;8,1>:d    14:w              
        shr (16|M0)              r24.0<1>:ud   r22.0<8;8,1>:ud   18:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r14.0<8;8,1>:ud   10:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r22.0<8;8,1>:ud   3:w              
        add (16|M0)              r14.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r42.0<1>:d    r30.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r8.0<1>:d     r7.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -4:w              
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r42.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r8.0<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -5:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     -11:w              
        add (16|M0)              r38.0<1>:d    r36.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        sends (16|M0)            null:w   r14     r38     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r10:w    r20     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r24:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r34.0<1>:d    r10.0<8;8,1>:d    15:w              
        shr (16|M0)              r36.0<1>:ud   r10.0<8;8,1>:ud   17:w              
        shl (16|M0)              r38.0<1>:d    r10.0<8;8,1>:d    13:w              
        shr (16|M0)              r40.0<1>:ud   r10.0<8;8,1>:ud   19:w              
        shl (16|M0)              r8.0<1>:d     r24.0<8;8,1>:d    25:w              
        shr (16|M0)              r42.0<1>:ud   r24.0<8;8,1>:ud   7:w              
        shl (16|M0)              r44.0<1>:d    r24.0<8;8,1>:d    14:w              
        shr (16|M0)              r2.0<1>:ud    r24.0<8;8,1>:ud   18:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r10.0<8;8,1>:ud   10:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r36.0<1>:ud   r24.0<8;8,1>:ud   3:w              
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r46.0<1>:d    r34.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
(W)     add (1|M0)               r2.1<1>:d     r4.0<0;1,0>:d     -12:w              
(W)     mul (1|M0)               r34.0<1>:d    r7.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.0<1>:d     r4.0<0;1,0>:d     3:w              
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r46.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
        add (16|M0)              r58.0<1>:d    r56.0<8;8,1>:d    r34.0<0;1,0>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -3:w              
        add (16|M0)              r8.0<1>:d     r40.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r24.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     4:w              
        add (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r10     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r18:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r58     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r36:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r8.0<1>:d     r18.0<8;8,1>:d    15:w              
        shr (16|M0)              r38.0<1>:ud   r18.0<8;8,1>:ud   17:w              
        shl (16|M0)              r40.0<1>:d    r18.0<8;8,1>:d    13:w              
        shr (16|M0)              r42.0<1>:ud   r18.0<8;8,1>:ud   19:w              
        shl (16|M0)              r44.0<1>:d    r32.0<8;8,1>:d    25:w              
        shr (16|M0)              r46.0<1>:ud   r32.0<8;8,1>:ud   7:w              
        shl (16|M0)              r48.0<1>:d    r32.0<8;8,1>:d    14:w              
        shr (16|M0)              r34.0<1>:ud   r32.0<8;8,1>:ud   18:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r18.0<8;8,1>:ud   10:w              
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r40.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r38.0<1>:ud   r32.0<8;8,1>:ud   3:w              
        add (16|M0)              r40.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r50.0<1>:d    r8.0<8;8,1>:d     r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r50.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r28.0<1>:d    r7.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -10:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     5:w              
        add (16|M0)              r44.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r56.0<8;8,1>:d    r28.0<0;1,0>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r40     r44     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r8:w     r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r34:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r36:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r2.0<1>:d     r8.0<8;8,1>:d     15:w              
        shr (16|M0)              r38.0<1>:ud   r8.0<8;8,1>:ud    17:w              
        shl (16|M0)              r42.0<1>:d    r8.0<8;8,1>:d     13:w              
        shr (16|M0)              r44.0<1>:ud   r8.0<8;8,1>:ud    19:w              
        shl (16|M0)              r46.0<1>:d    r22.0<8;8,1>:d    25:w              
        shr (16|M0)              r48.0<1>:ud   r22.0<8;8,1>:ud   7:w              
        shl (16|M0)              r50.0<1>:d    r22.0<8;8,1>:d    14:w              
        shr (16|M0)              r28.0<1>:ud   r22.0<8;8,1>:ud   18:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r24.0<1>:ud   r8.0<8;8,1>:ud    10:w              
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        or (16|M0)               r50.0<1>:d    r50.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r38.0<1>:ud   r22.0<8;8,1>:ud   3:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r7.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r3.0<1>:d     r4.0<0;1,0>:d     -2:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r32.0<1>:d    r3.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -9:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     6:w              
        add (16|M0)              r46.0<1>:d    r44.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r56.0<8;8,1>:d    r32.0<0;1,0>:d   {Compacted}
        add (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r8      r46     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r24:w    r40     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r38:w    r34     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r52:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     mul (1|M0)               r18.0<1>:d    r7.0<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.0<1>:d     r4.0<0;1,0>:d     -8:w              
        shl (16|M0)              r36.0<1>:d    r24.0<8;8,1>:d    15:w              
        shr (16|M0)              r42.0<1>:ud   r24.0<8;8,1>:ud   17:w              
        shl (16|M0)              r44.0<1>:d    r24.0<8;8,1>:d    13:w              
        shr (16|M0)              r46.0<1>:ud   r24.0<8;8,1>:ud   19:w              
        shl (16|M0)              r48.0<1>:d    r28.0<8;8,1>:d    25:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r50.0<1>:ud   r28.0<8;8,1>:ud   7:w              
        shl (16|M0)              r54.0<1>:d    r28.0<8;8,1>:d    14:w              
        shr (16|M0)              r42.0<1>:ud   r28.0<8;8,1>:ud   18:w              
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r2.0<1>:ud    r24.0<8;8,1>:ud   10:w              
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        or (16|M0)               r54.0<1>:d    r54.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r46.0<1>:ud   r28.0<8;8,1>:ud   3:w              
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r54.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r36.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r32.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r56.0<8;8,1>:d    r18.0<0;1,0>:d   {Compacted}
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     7:w              
        add (16|M0)              r48.0<1>:d    r34.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r2      r48     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r24:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r36:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r38:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r34.0<1>:d    r24.0<8;8,1>:d    15:w              
        shr (16|M0)              r42.0<1>:ud   r24.0<8;8,1>:ud   17:w              
        shl (16|M0)              r44.0<1>:d    r24.0<8;8,1>:d    13:w              
        shr (16|M0)              r46.0<1>:ud   r24.0<8;8,1>:ud   19:w              
        shl (16|M0)              r48.0<1>:d    r36.0<8;8,1>:d    25:w              
        shr (16|M0)              r50.0<1>:ud   r36.0<8;8,1>:ud   7:w              
        shl (16|M0)              r18.0<1>:d    r36.0<8;8,1>:d    14:w              
        shr (16|M0)              r52.0<1>:ud   r36.0<8;8,1>:ud   18:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r16.0<1>:ud   r24.0<8;8,1>:ud   10:w              
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r36.0<8;8,1>:ud   3:w              
        add (16|M0)              r24.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r16.0<1>:d    r7.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     8:w              
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r34.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r56.0<8;8,1>:d    r16.0<0;1,0>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r42.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r24     r48     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r18:w    r2      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r44:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r20     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r34.0<1>:d    r18.0<8;8,1>:d    15:w              
        shr (16|M0)              r42.0<1>:ud   r18.0<8;8,1>:ud   17:w              
        shl (16|M0)              r38.0<1>:d    r18.0<8;8,1>:d    13:w              
        shr (16|M0)              r46.0<1>:ud   r18.0<8;8,1>:ud   19:w              
        shl (16|M0)              r48.0<1>:d    r44.0<8;8,1>:d    25:w              
        shr (16|M0)              r2.0<1>:ud    r44.0<8;8,1>:ud   7:w              
        shl (16|M0)              r16.0<1>:d    r44.0<8;8,1>:d    14:w              
        shr (16|M0)              r50.0<1>:ud   r44.0<8;8,1>:ud   18:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r20.0<1>:ud   r18.0<8;8,1>:ud   10:w              
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r32.0<1>:ud   r44.0<8;8,1>:ud   3:w              
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     9:w              
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r34.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r42.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r18     r48     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r2:w     r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r20:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r16.0<1>:d    r2.0<8;8,1>:d     15:w              
        shr (16|M0)              r38.0<1>:ud   r2.0<8;8,1>:ud    17:w              
        shl (16|M0)              r44.0<1>:d    r2.0<8;8,1>:d     13:w              
        shr (16|M0)              r28.0<1>:ud   r2.0<8;8,1>:ud    19:w              
        shl (16|M0)              r34.0<1>:d    r14.0<8;8,1>:d    25:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r42.0<1>:ud   r14.0<8;8,1>:ud   7:w              
        shl (16|M0)              r46.0<1>:d    r14.0<8;8,1>:d    14:w              
        shr (16|M0)              r38.0<1>:ud   r14.0<8;8,1>:ud   18:w              
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r2.0<8;8,1>:ud    10:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r2.0<1>:ud    r14.0<8;8,1>:ud   3:w              
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r24.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     10:w              
        add (16|M0)              r34.0<1>:d    r24.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r16     r34     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r20:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r36:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r38.0<1>:d    r20.0<8;8,1>:d    15:w              
        shr (16|M0)              r14.0<1>:ud   r20.0<8;8,1>:ud   17:w              
        shl (16|M0)              r2.0<1>:d     r20.0<8;8,1>:d    13:w              
        shr (16|M0)              r24.0<1>:ud   r20.0<8;8,1>:ud   19:w              
        shl (16|M0)              r32.0<1>:d    r22.0<8;8,1>:d    25:w              
        shr (16|M0)              r34.0<1>:ud   r22.0<8;8,1>:ud   7:w              
        shl (16|M0)              r18.0<1>:d    r22.0<8;8,1>:d    14:w              
        shr (16|M0)              r42.0<1>:ud   r22.0<8;8,1>:ud   18:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r20.0<8;8,1>:ud   10:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r12.0<1>:ud   r22.0<8;8,1>:ud   3:w              
        add (16|M0)              r2.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     add (1|M0)               r5.2<1>:d     r4.0<0;1,0>:d     11:w              
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r38.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r14.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r2      r32     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r20:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r10:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r18:w    r40     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r24.0<1>:d    r20.0<8;8,1>:d    15:w              
        shr (16|M0)              r28.0<1>:ud   r20.0<8;8,1>:ud   17:w              
        shl (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    13:w              
        shr (16|M0)              r14.0<1>:ud   r20.0<8;8,1>:ud   19:w              
        shl (16|M0)              r34.0<1>:d    r10.0<8;8,1>:d    25:w              
        shr (16|M0)              r32.0<1>:ud   r10.0<8;8,1>:ud   7:w              
        shl (16|M0)              r16.0<1>:d    r10.0<8;8,1>:d    14:w              
        shr (16|M0)              r36.0<1>:ud   r10.0<8;8,1>:ud   18:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r20.0<8;8,1>:ud   10:w              
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   3:w              
        add (16|M0)              r36.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r24.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r14.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r36     r34     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r12:w    r2      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r20:w    r58     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r10:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r26.0<1>:d    r12.0<8;8,1>:d    15:w              
        shr (16|M0)              r18.0<1>:ud   r12.0<8;8,1>:ud   17:w              
        shl (16|M0)              r24.0<1>:d    r12.0<8;8,1>:d    13:w              
        shr (16|M0)              r14.0<1>:ud   r12.0<8;8,1>:ud   19:w              
        shl (16|M0)              r22.0<1>:d    r20.0<8;8,1>:d    25:w              
        shr (16|M0)              r28.0<1>:ud   r20.0<8;8,1>:ud   7:w              
        shl (16|M0)              r2.0<1>:d     r20.0<8;8,1>:d    14:w              
        shr (16|M0)              r32.0<1>:ud   r20.0<8;8,1>:ud   18:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r12.0<8;8,1>:ud   10:w              
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r20.0<8;8,1>:ud   3:w              
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r26.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r14.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r12     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(~f0.1) break (16|M0)                        L31776                  L31776                
L31728:
        send (16|M0)             r30:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r36:w    r58     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     12:w              
L31776:
        while (16|M0)                        L26728                                
L31792:
        mov (16|M0)              r44.0<1>:d    1779033703:d                              
        mov (16|M0)              r40.0<1>:d    -1150833019:d                              
        mov (16|M0)              r38.0<1>:d    1013904242:d                              
        mov (16|M0)              r48.0<1>:d    -1521486534:d                              
        mov (16|M0)              r42.0<1>:d    1359893119:d                              
        mov (16|M0)              r46.0<1>:d    -1694144372:d                              
        mov (16|M0)              r50.0<1>:d    528734635:d                              
        mov (16|M0)              r52.0<1>:d    1541459225:d                              
(W)     mov (1|M0)               r4.0<1>:d     0:w                              
L31936:
(W)     shl (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     2:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     1:w              
(W)     mul (1|M0)               r2.1<1>:d     r4.0<0;1,0>:d     64:w              
        shl (16|M0)              r16.0<1>:d    r42.0<8;8,1>:d    26:w              
        shr (16|M0)              r18.0<1>:ud   r42.0<8;8,1>:ud   6:w              
(W)     mov (1|M0)               r3.2<1>:ud    r2.0<0;1,0>:ud                   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r20.0<1>:d    r42.0<8;8,1>:d    21:w              
        shr (16|M0)              r22.0<1>:ud   r42.0<8;8,1>:ud   11:w              
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shr (16|M0)              r24.0<1>:ud   r42.0<8;8,1>:ud   25:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r8.0<1>:d     r42.0<8;8,1>:d    7:w              
        and (16|M0)              r26.0<1>:d    r50.0<8;8,1>:d    ~r42.0<8;8,1>:d  {Compacted}
        and (16|M0)              r12.0<1>:d    r42.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r34.0<1>:ud   r44.0<8;8,1>:ud   22:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r40.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     or (1|M0)                r7.4<1>:d     r4.0<0;1,0>:d     2:w              
(W)     or (1|M0)                r3.0<1>:d     r4.0<0;1,0>:d     3:w              
(W)     or (1|M0)                r6.0<1>:d     r4.0<0;1,0>:d     4:w              
        and (16|M0)              r36.0<1>:d    r44.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r52.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r7.4<0;1,0>:d     64:w              
(W)     mul (1|M0)               r5.2<1>:d     r3.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r3.2<1>:d     r2.0<0;1,0>:d     16:w              
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r16.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        add (16|M0)              r22.0<1>:d    r18.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r48.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r28.0<8;8,1>:d    26:w              
        shr (16|M0)              r20.0<1>:ud   r28.0<8;8,1>:ud   6:w              
        shl (16|M0)              r24.0<1>:d    r28.0<8;8,1>:d    21:w              
        shr (16|M0)              r8.0<1>:ud    r28.0<8;8,1>:ud   11:w              
        shl (16|M0)              r32.0<1>:d    r28.0<8;8,1>:d    7:w              
        shr (16|M0)              r26.0<1>:ud   r28.0<8;8,1>:ud   25:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r10.0<1>:d    r28.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r12.0<1>:d    r46.0<8;8,1>:d    ~r28.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r44.0<8;8,1>:d    30:w              
        shl (16|M0)              r32.0<1>:d    r44.0<8;8,1>:d    19:w              
        add (16|M0)              r30.0<1>:d    r50.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r44.0<8;8,1>:ud   2:w              
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r30.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r44.0<8;8,1>:ud   13:w              
        shl (16|M0)              r30.0<1>:d    r44.0<8;8,1>:d    10:w              
        add (16|M0)              r16.0<1>:d    r38.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r18.0<1>:d    r16.0<8;8,1>:d    26:w              
        shr (16|M0)              r20.0<1>:ud   r16.0<8;8,1>:ud   6:w              
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r24.0<1>:d    r16.0<8;8,1>:d    21:w              
        shr (16|M0)              r26.0<1>:ud   r16.0<8;8,1>:ud   11:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r40.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r44.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r22.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        and (16|M0)              r12.0<1>:d    r44.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r6.0<0;1,0>:d     64:w              
        shl (16|M0)              r14.0<1>:d    r10.0<8;8,1>:d    30:w              
        shr (16|M0)              r24.0<1>:ud   r10.0<8;8,1>:ud   2:w              
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    19:w              
        shr (16|M0)              r38.0<1>:ud   r10.0<8;8,1>:ud   13:w              
        shl (16|M0)              r32.0<1>:d    r10.0<8;8,1>:d    10:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        send (16|M0)             r24:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r34.0<1>:ud   r10.0<8;8,1>:ud   22:w              
        and (16|M0)              r30.0<1>:d    r10.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r16.0<8;8,1>:ud   25:w              
        send (16|M0)             r34:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r16.0<8;8,1>:d    7:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     5:w              
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r7.4<1>:d     r5.2<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     8:w              
        add (16|M0)              r20.0<1>:d    r8.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r16.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r42.0<8;8,1>:d    ~r16.0<8;8,1>:d  {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        add (16|M0)              r18.0<1>:d    r46.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     10:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        add (16|M0)              r22.0<1>:d    r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r40.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r26.0<8;8,1>:d    26:w              
        shr (16|M0)              r36.0<1>:ud   r26.0<8;8,1>:ud   6:w              
        shl (16|M0)              r12.0<1>:d    r26.0<8;8,1>:d    21:w              
        shr (16|M0)              r32.0<1>:ud   r26.0<8;8,1>:ud   11:w              
        shl (16|M0)              r38.0<1>:d    r26.0<8;8,1>:d    7:w              
        shr (16|M0)              r14.0<1>:ud   r26.0<8;8,1>:ud   25:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r26.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r28.0<8;8,1>:d    ~r26.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r42.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r42.0<1>:ud   r20.0<8;8,1>:ud   13:w              
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r8.0<1>:d     r20.0<8;8,1>:d    30:w              
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     send (4|M0)              r7:d     r3      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        add (16|M0)              r24.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r20.0<8;8,1>:ud   2:w              
        shl (16|M0)              r34.0<1>:d    r20.0<8;8,1>:d    19:w              
        add (16|M0)              r32.0<1>:d    r44.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r36.0<1>:d    r32.0<8;8,1>:d    26:w              
        shr (16|M0)              r12.0<1>:ud   r32.0<8;8,1>:ud   6:w              
        shl (16|M0)              r14.0<1>:d    r32.0<8;8,1>:d    21:w              
        shr (16|M0)              r38.0<1>:ud   r32.0<8;8,1>:ud   11:w              
        shl (16|M0)              r40.0<1>:d    r32.0<8;8,1>:d    7:w              
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r32.0<8;8,1>:ud   25:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    10:w              
        shr (16|M0)              r38.0<1>:ud   r20.0<8;8,1>:ud   22:w              
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r10.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r10.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r20.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r56.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        add (16|M0)              r36.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r20.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     7:w              
        add (16|M0)              r28.0<1>:d    r22.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        send (16|M0)             r22:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        and (16|M0)              r8.0<1>:d     r20.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
        shl (16|M0)              r30.0<1>:d    r28.0<8;8,1>:d    30:w              
        shr (16|M0)              r40.0<1>:ud   r28.0<8;8,1>:ud   2:w              
        shl (16|M0)              r42.0<1>:d    r28.0<8;8,1>:d    19:w              
        shr (16|M0)              r44.0<1>:ud   r28.0<8;8,1>:ud   13:w              
        shl (16|M0)              r34.0<1>:d    r28.0<8;8,1>:d    10:w              
        shr (16|M0)              r38.0<1>:ud   r28.0<8;8,1>:ud   22:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r12.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r56.0<8;8,1>:d    r7.4<0;1,0>:d    {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     or (1|M0)                r8.0<1>:d     r4.0<0;1,0>:d     6:w              
        and (16|M0)              r40.0<1>:d    r16.0<8;8,1>:d    ~r32.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        send (16|M0)             r34:w    r38     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     mul (1|M0)               r9.0<1>:d     r8.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     9:w              
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r9.0<0;1,0>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r24.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r32.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        send (16|M0)             r24:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r36.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        add (16|M0)              r42.0<1>:d    r18.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r10.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r44.0<8;8,1>:d    26:w              
        shr (16|M0)              r46.0<1>:ud   r44.0<8;8,1>:ud   6:w              
        shl (16|M0)              r48.0<1>:d    r44.0<8;8,1>:d    21:w              
        shr (16|M0)              r8.0<1>:ud    r44.0<8;8,1>:ud   11:w              
        shl (16|M0)              r12.0<1>:d    r44.0<8;8,1>:d    7:w              
        shr (16|M0)              r40.0<1>:ud   r44.0<8;8,1>:ud   25:w              
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r10.0<1>:d    r44.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r26.0<8;8,1>:d    ~r44.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r16.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r7.1<0;1,0>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r38.0<1>:ud   r14.0<8;8,1>:ud   2:w              
        add (16|M0)              r22.0<1>:d    r20.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r30.0<1>:d    r22.0<8;8,1>:d    26:w              
        shr (16|M0)              r36.0<1>:ud   r22.0<8;8,1>:ud   6:w              
        shl (16|M0)              r40.0<1>:d    r22.0<8;8,1>:d    21:w              
        shr (16|M0)              r12.0<1>:ud   r22.0<8;8,1>:ud   11:w              
        shl (16|M0)              r16.0<1>:d    r22.0<8;8,1>:d    7:w              
        shr (16|M0)              r18.0<1>:ud   r22.0<8;8,1>:ud   25:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r22.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r34.0<1>:d    r32.0<8;8,1>:d    ~r22.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r36.0<1>:d    r14.0<8;8,1>:d    30:w              
        shr (16|M0)              r18.0<1>:ud   r14.0<8;8,1>:ud   13:w              
        shl (16|M0)              r40.0<1>:d    r14.0<8;8,1>:d    19:w              
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r16.0<1>:d    r14.0<8;8,1>:d    10:w              
        add (16|M0)              r30.0<1>:d    r26.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r34.0<1>:d    r28.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r40.0<1>:d    r40.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r26.0<1>:ud   r14.0<8;8,1>:ud   22:w              
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r34.0<1>:d    r14.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r28.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r7.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r34.0<1>:d    r34.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r14.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r14.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r7.2<1>:d     r2.0<0;1,0>:d     48:w              
        add (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.1<1>:d     r4.0<0;1,0>:d     11:w              
        add (16|M0)              r24.0<1>:d    r42.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        send (16|M0)             r36:w    r34     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r18.0<1>:d    r24.0<8;8,1>:d    30:w              
        shr (16|M0)              r30.0<1>:ud   r24.0<8;8,1>:ud   2:w              
        shl (16|M0)              r38.0<1>:d    r24.0<8;8,1>:d    19:w              
        shr (16|M0)              r46.0<1>:ud   r24.0<8;8,1>:ud   13:w              
        shl (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    10:w              
        shr (16|M0)              r20.0<1>:ud   r24.0<8;8,1>:ud   22:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        and (16|M0)              r16.0<1>:d    r24.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r20.2<1>:d    r2.0<0;1,0>:d     32:w              
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
(W)     send (4|M0)              r21:d    r20     0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     12:w              
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        send (16|M0)             r10:w    r26     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r16.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r8.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r8.0<1>:d     r16.0<8;8,1>:d    26:w              
        shr (16|M0)              r18.0<1>:ud   r16.0<8;8,1>:ud   6:w              
        shl (16|M0)              r38.0<1>:d    r16.0<8;8,1>:d    21:w              
        shr (16|M0)              r40.0<1>:ud   r16.0<8;8,1>:ud   11:w              
        shl (16|M0)              r34.0<1>:d    r16.0<8;8,1>:d    7:w              
        shr (16|M0)              r42.0<1>:ud   r16.0<8;8,1>:ud   25:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r16.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r34.0<1>:d    r34.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r44.0<8;8,1>:d    ~r16.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r48.0<1>:ud   r30.0<8;8,1>:ud   2:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r32.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r7.3<0;1,0>:d    {Compacted}
        add (16|M0)              r18.0<1>:d    r8.0<8;8,1>:d     r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r46.0<1>:d    r40.0<8;8,1>:d    26:w              
        shr (16|M0)              r38.0<1>:ud   r40.0<8;8,1>:ud   6:w              
        shl (16|M0)              r42.0<1>:d    r40.0<8;8,1>:d    21:w              
        shr (16|M0)              r34.0<1>:ud   r40.0<8;8,1>:ud   11:w              
        shl (16|M0)              r32.0<1>:d    r40.0<8;8,1>:d    7:w              
        shr (16|M0)              r28.0<1>:ud   r40.0<8;8,1>:ud   25:w              
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r40.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r22.0<8;8,1>:d    ~r40.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r30.0<8;8,1>:d    30:w              
        shr (16|M0)              r28.0<1>:ud   r30.0<8;8,1>:ud   13:w              
        shl (16|M0)              r42.0<1>:d    r30.0<8;8,1>:d    19:w              
        xor (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r30.0<8;8,1>:d    10:w              
        add (16|M0)              r46.0<1>:d    r44.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r44.0<1>:ud   r30.0<8;8,1>:ud   22:w              
        add (16|M0)              r46.0<1>:d    r46.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r26.0<1>:d    r30.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r24.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r46.0<8;8,1>:d    r21.0<0;1,0>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r8.0<1>:d     r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r36.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r30.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r38.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r12.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r42.0<1>:d    r10.0<8;8,1>:d    10:w              
        shr (16|M0)              r14.0<1>:ud   r10.0<8;8,1>:ud   22:w              
        shl (16|M0)              r28.0<1>:d    r10.0<8;8,1>:d    30:w              
        shr (16|M0)              r36.0<1>:ud   r10.0<8;8,1>:ud   2:w              
        shl (16|M0)              r46.0<1>:d    r10.0<8;8,1>:d    19:w              
        or (16|M0)               r42.0<1>:d    r42.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        shr (16|M0)              r48.0<1>:ud   r10.0<8;8,1>:ud   13:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        and (16|M0)              r32.0<1>:d    r10.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        send (16|M0)             r38:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.1<0;1,0>:d     64:w              
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.2<1>:d     r4.0<0;1,0>:d     14:w              
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        send (16|M0)             r32:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r12.0<1>:d    r18.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r24.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r26.0<1>:d    r12.0<8;8,1>:d    30:w              
        shr (16|M0)              r36.0<1>:ud   r12.0<8;8,1>:ud   2:w              
        shl (16|M0)              r28.0<1>:d    r18.0<8;8,1>:d    26:w              
        shr (16|M0)              r42.0<1>:ud   r18.0<8;8,1>:ud   6:w              
        shr (16|M0)              r44.0<1>:ud   r18.0<8;8,1>:ud   11:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r36.0<1>:d    r18.0<8;8,1>:d    21:w              
        shl (16|M0)              r14.0<1>:d    r18.0<8;8,1>:d    7:w              
        shr (16|M0)              r46.0<1>:ud   r18.0<8;8,1>:ud   25:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r18.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r16.0<8;8,1>:d    ~r18.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r22.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r21.1<0;1,0>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r28.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r12.0<8;8,1>:d    10:w              
        add (16|M0)              r44.0<1>:d    r30.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r48.0<1>:d    r44.0<8;8,1>:d    26:w              
        shr (16|M0)              r36.0<1>:ud   r44.0<8;8,1>:ud   6:w              
        shl (16|M0)              r46.0<1>:d    r44.0<8;8,1>:d    21:w              
        shr (16|M0)              r14.0<1>:ud   r44.0<8;8,1>:ud   11:w              
        shl (16|M0)              r22.0<1>:d    r44.0<8;8,1>:d    7:w              
        shr (16|M0)              r24.0<1>:ud   r44.0<8;8,1>:ud   25:w              
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r44.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r40.0<8;8,1>:d    ~r44.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r36.0<1>:d    r12.0<8;8,1>:d    19:w              
        shr (16|M0)              r14.0<1>:ud   r12.0<8;8,1>:ud   13:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r24.0<1>:ud   r12.0<8;8,1>:ud   22:w              
        xor (16|M0)              r22.0<1>:d    r10.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r16.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r12.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r8.0<1>:d     r10.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r21.2<0;1,0>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r22.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r8.0<1>:d     r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r46.0<1>:d    r48.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r34.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r16.0<1>:d    r14.0<8;8,1>:d    30:w              
        shr (16|M0)              r28.0<1>:ud   r14.0<8;8,1>:ud   2:w              
        shl (16|M0)              r50.0<1>:d    r14.0<8;8,1>:d    19:w              
        shr (16|M0)              r32.0<1>:ud   r14.0<8;8,1>:ud   13:w              
        shl (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    10:w              
        shr (16|M0)              r30.0<1>:ud   r14.0<8;8,1>:ud   22:w              
        and (16|M0)              r36.0<1>:d    r14.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r50.0<1>:d    r50.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r36.0<1>:d    r36.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        add (16|M0)              r30.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.0<1>:d     r5.2<0;1,0>:d     64:w              
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        send (16|M0)             r24:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r42.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
(W)     send (4|M0)              r16:d    r7      0xA            0x02184002           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
(W)     or (1|M0)                r7.0<1>:d     r4.0<0;1,0>:d     15:w              
        shl (16|M0)              r26.0<1>:d    r22.0<8;8,1>:d    30:w              
        shr (16|M0)              r28.0<1>:ud   r22.0<8;8,1>:ud   2:w              
        shl (16|M0)              r32.0<1>:d    r22.0<8;8,1>:d    19:w              
        shr (16|M0)              r34.0<1>:ud   r22.0<8;8,1>:ud   13:w              
        shl (16|M0)              r50.0<1>:d    r22.0<8;8,1>:d    10:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        send (16|M0)             r28:w    r30     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r10.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r52.0<1>:ud   r22.0<8;8,1>:ud   22:w              
(W)     mul (1|M0)               r5.2<1>:d     r7.0<0;1,0>:d     64:w              
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r34.0<8;8,1>:d    26:w              
        shr (16|M0)              r36.0<1>:ud   r34.0<8;8,1>:ud   6:w              
        shl (16|M0)              r38.0<1>:d    r34.0<8;8,1>:d    21:w              
        shr (16|M0)              r42.0<1>:ud   r34.0<8;8,1>:ud   11:w              
        shl (16|M0)              r2.0<1>:d     r34.0<8;8,1>:d    7:w              
        shr (16|M0)              r8.0<1>:ud    r34.0<8;8,1>:ud   25:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r34.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        and (16|M0)              r30.0<1>:d    r18.0<8;8,1>:d    ~r34.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r50.0<1>:d    r50.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
(W)     cmp (16|M0)   (lt)f0.0   null<1>:d     r7.0<0;1,0>:ud    0x3F:uw             
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r40.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r14.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r40.0<1>:d    r22.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r21.3<0;1,0>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r32.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r14.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r12.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r48.0<1>:d    r42.0<8;8,1>:d    26:w              
        shr (16|M0)              r38.0<1>:ud   r42.0<8;8,1>:ud   6:w              
        shl (16|M0)              r8.0<1>:d     r42.0<8;8,1>:d    21:w              
        shr (16|M0)              r2.0<1>:ud    r42.0<8;8,1>:ud   11:w              
        shl (16|M0)              r10.0<1>:d    r42.0<8;8,1>:d    7:w              
        shr (16|M0)              r20.0<1>:ud   r42.0<8;8,1>:ud   25:w              
        or (16|M0)               r48.0<1>:d    r48.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r42.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r32.0<1>:d    r46.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r2.0<1>:d     r44.0<8;8,1>:d    ~r42.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r8.0<1>:d     r32.0<8;8,1>:d    30:w              
        shr (16|M0)              r12.0<1>:ud   r32.0<8;8,1>:ud   2:w              
        shl (16|M0)              r20.0<1>:d    r32.0<8;8,1>:d    19:w              
        shr (16|M0)              r24.0<1>:ud   r32.0<8;8,1>:ud   13:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r48.0<1>:d    r18.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r38.0<1>:d    r32.0<8;8,1>:d    10:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r22.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r32.0<8;8,1>:ud   22:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r48.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r2.0<1>:d     r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        and (16|M0)              r30.0<1>:d    r22.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r38.0<1>:d    r38.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r48.0<8;8,1>:d    r16.0<0;1,0>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r2.0<8;8,1>:d    {Compacted}
(W)     or (1|M0)                r2.0<1>:d     r4.0<0;1,0>:d     13:w              
        add (16|M0)              r52.0<1>:d    r14.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r36.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
        shl (16|M0)              r36.0<1>:d    r52.0<8;8,1>:d    21:w              
        shr (16|M0)              r38.0<1>:ud   r52.0<8;8,1>:ud   11:w              
        shl (16|M0)              r24.0<1>:d    r12.0<8;8,1>:d    30:w              
        add (16|M0)              r8.0<1>:d     r56.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        shr (16|M0)              r40.0<1>:ud   r12.0<8;8,1>:ud   2:w              
        shl (16|M0)              r46.0<1>:d    r12.0<8;8,1>:d    19:w              
        shr (16|M0)              r10.0<1>:ud   r12.0<8;8,1>:ud   13:w              
        send (16|M0)             r28:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r18.0<1>:d    r12.0<8;8,1>:d    10:w              
        shr (16|M0)              r20.0<1>:ud   r12.0<8;8,1>:ud   22:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r46.0<1>:d    r46.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r2.0<1>:d     r52.0<8;8,1>:d    7:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r20.0<1>:d    r52.0<8;8,1>:d    26:w              
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r52.0<8;8,1>:ud   25:w              
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r52.0<8;8,1>:ud   6:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        send (16|M0)             r30:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        and (16|M0)              r14.0<1>:d    r34.0<8;8,1>:d    ~r52.0<8;8,1>:d  {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r40.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r52.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        and (16|M0)              r40.0<1>:d    r12.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r40.0<1>:d    r40.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r44.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r12.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r16.1<0;1,0>:d   {Compacted}
        shl (16|M0)              r8.0<1>:d     r48.0<8;8,1>:d    30:w              
        shr (16|M0)              r36.0<1>:ud   r48.0<8;8,1>:ud   2:w              
        shl (16|M0)              r14.0<1>:d    r48.0<8;8,1>:d    19:w              
        shr (16|M0)              r38.0<1>:ud   r48.0<8;8,1>:ud   13:w              
        and (16|M0)              r24.0<1>:d    r48.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r36.0<8;8,1>:d   {Compacted}
        and (16|M0)              r36.0<1>:d    r12.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r20.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r20.0<1>:d    r48.0<8;8,1>:d    10:w              
        add (16|M0)              r50.0<1>:d    r22.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r22.0<1>:ud   r48.0<8;8,1>:ud   22:w              
        shl (16|M0)              r44.0<1>:d    r50.0<8;8,1>:d    26:w              
        shr (16|M0)              r10.0<1>:ud   r50.0<8;8,1>:ud   6:w              
        shl (16|M0)              r18.0<1>:d    r50.0<8;8,1>:d    21:w              
        shr (16|M0)              r46.0<1>:ud   r50.0<8;8,1>:ud   11:w              
        shl (16|M0)              r26.0<1>:d    r50.0<8;8,1>:d    7:w              
        shr (16|M0)              r28.0<1>:ud   r50.0<8;8,1>:ud   25:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r44.0<1>:d    r44.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r38.0<1>:d    r42.0<8;8,1>:d    ~r50.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r50.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r38.0<8;8,1>:d   {Compacted}
        add (16|M0)              r38.0<1>:d    r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r2.0<1>:d     r48.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r34.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        and (16|M0)              r8.0<1>:d     r48.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r38.0<8;8,1>:d    30:w              
        shr (16|M0)              r18.0<1>:ud   r38.0<8;8,1>:ud   2:w              
        add (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r56.0<8;8,1>:d    r5.2<0;1,0>:d    {Compacted}
        shl (16|M0)              r20.0<1>:d    r38.0<8;8,1>:d    19:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r38.0<8;8,1>:ud   13:w              
        send (16|M0)             r18:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r44.0<1>:d    r44.0<8;8,1>:d    r16.2<0;1,0>:d   {Compacted}
        shl (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    10:w              
        shr (16|M0)              r28.0<1>:ud   r38.0<8;8,1>:ud   22:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r26.0<1>:d    r44.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r2.0<1>:d     r38.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r46.0<1>:d    r32.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r20.0<1>:d    r46.0<8;8,1>:d    26:w              
        shr (16|M0)              r22.0<1>:ud   r46.0<8;8,1>:ud   6:w              
        shl (16|M0)              r28.0<1>:d    r46.0<8;8,1>:d    21:w              
        shr (16|M0)              r30.0<1>:ud   r46.0<8;8,1>:ud   11:w              
        add (16|M0)              r14.0<1>:d    r14.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r8.0<1>:d     r46.0<8;8,1>:d    7:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r2.0<1>:ud    r46.0<8;8,1>:ud   25:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r26.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r10.0<1>:d    r46.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r52.0<8;8,1>:d    ~r46.0<8;8,1>:d  {Compacted}
        shl (16|M0)              r24.0<1>:d    r40.0<8;8,1>:d    30:w              
        shr (16|M0)              r26.0<1>:ud   r40.0<8;8,1>:ud   2:w              
        shl (16|M0)              r32.0<1>:d    r40.0<8;8,1>:d    19:w              
        shr (16|M0)              r22.0<1>:ud   r40.0<8;8,1>:ud   13:w              
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r2.0<1>:d     r40.0<8;8,1>:d    10:w              
        xor (16|M0)              r28.0<1>:d    r38.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r40.0<8;8,1>:ud   22:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r42.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r38.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        and (16|M0)              r28.0<1>:d    r40.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r16.3<0;1,0>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r20.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r14.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
(~f0.0) break (16|M0)                        L39144                  L39144                
L39128:
(W)     add (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     16:w              
L39144:
        while (16|M0)                        L31936                                
L39160:
        add (16|M0)   (eq)f1.1   r4.0<1>:d     r44.0<8;8,1>:d    1779033703:d              
(~f1.1) goto (16|M0)                         L39256                  L39256                
L39192:
(W)     mov (1|M0)               r2.0<2>:b     0:w                              
(W)     mov (1|M0)               f1.1<1>:uw    0x0:uw                             
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
(f1.1)  goto (16|M0)                         L39256                  L40072                
L39256:
        join (16|M0)                         L40072                                
L39272:
        add (16|M0)   (eq)f1.0   r4.0<1>:d     r40.0<8;8,1>:d    -1150833019:d              
(~f1.0) goto (16|M0)                         L39368                  L39368                
L39304:
(W)     mov (1|M0)               r2.0<2>:b     32:w                              
(W)     mov (1|M0)               f1.0<1>:uw    0x0:uw                             
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
(f1.0)  goto (16|M0)                         L39368                  L40072                
L39368:
        join (16|M0)                         L40072                                
L39384:
        add (16|M0)   (eq)f0.1   r4.0<1>:d     r38.0<8;8,1>:d    1013904242:d              
(~f0.1) goto (16|M0)                         L39480                  L39480                
L39416:
(W)     mov (1|M0)               r2.0<2>:b     64:w                              
(W)     mov (1|M0)               f0.1<1>:uw    0x0:uw                             
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
(f0.1)  goto (16|M0)                         L39480                  L40072                
L39480:
        join (16|M0)                         L40072                                
L39496:
        add (16|M0)   (eq)f0.0   r4.0<1>:d     r48.0<8;8,1>:d    -1521486534:d              
(~f0.0) goto (16|M0)                         L39592                  L39592                
L39528:
(W)     mov (1|M0)               r2.0<2>:b     96:w                              
(W)     mov (1|M0)               f0.0<1>:uw    0x0:uw                             
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
(f0.0)  goto (16|M0)                         L39592                  L40072                
L39592:
        join (16|M0)                         L40072                                
L39608:
        add (16|M0)   (eq)f1.1   r4.0<1>:d     r42.0<8;8,1>:d    1359893119:d              
(~f1.1) goto (16|M0)                         L39704                  L39704                
L39640:
(W)     mov (1|M0)               r2.0<2>:b     -128:w                              
(W)     mov (1|M0)               f1.1<1>:uw    0x0:uw                             
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
(f1.1)  goto (16|M0)                         L39704                  L40072                
L39704:
        join (16|M0)                         L40072                                
L39720:
        add (16|M0)   (eq)f1.0   r4.0<1>:d     r46.0<8;8,1>:d    -1694144372:d              
(~f1.0) goto (16|M0)                         L39816                  L39816                
L39752:
(W)     mov (1|M0)               r2.0<2>:b     -96:w                              
(W)     mov (1|M0)               f1.0<1>:uw    0x0:uw                             
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
(f1.0)  goto (16|M0)                         L39816                  L40072                
L39816:
        join (16|M0)                         L40072                                
L39832:
        add (16|M0)   (eq)f0.1   r4.0<1>:d     r50.0<8;8,1>:d    528734635:d              
(~f0.1) goto (16|M0)                         L39928                  L39928                
L39864:
(W)     mov (1|M0)               r2.0<2>:b     -64:w                              
(W)     mov (1|M0)               f0.1<1>:uw    0x0:uw                             
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
(f0.1)  goto (16|M0)                         L39928                  L40072                
L39928:
        join (16|M0)                         L40072                                
L39944:
        add (16|M0)   (eq)f0.0   r4.0<1>:d     r52.0<8;8,1>:d    1541459225:d              
(f0.0)  goto (16|M0)                         L40024                  L40024                
L39976:
(W)     mov (1|M0)               f0.0<1>:uw    0x0:uw                             
        mov (16|M0)              r4.0<1>:d     16777216:d                              
(f0.0)  goto (16|M0)                         L40024                  L40128                
L40024:
        join (16|M0)                         L40072                                
L40040:
(W)     mov (1|M0)               r2.0<2>:b     -32:w                              
        mov (16|M0)              r6.0<1>:b     r2.0<0;1,0>:b                   
L40072:
        join (16|M0)                         L40128                                
L40088:
        lzd (16|M0)              r2.0<1>:ud    r4.0<8;8,1>:ud                   {Compacted}
        or (16|M0)               r8.0<1>:d     r6.0<8;8,1>:ub    r2.0<8;8,1>:d   
        shl (16|M0)              r4.0<1>:d     r8.0<8;8,1>:d     16:w              
L40128:
        join (16|M0)                         L40128                                
L40144:
        and (16|M0)              r2.0<1>:d     r108.0<8;8,1>:d   0xFFFF:uw             
        shl (16|M0)              r8.0<1>:d     r110.0<8;8,1>:d   2:w              
(W)     mov (8|M0)               r127.0<1>:ud  r64.0<8;8,1>:ud                  {Compacted}
        or (16|M0)               r2.0<1>:d     r4.0<8;8,1>:d     r2.0<8;8,1>:d    {Compacted}
        sends (16|M0)            null:w   r8      r2      0x8C            0x04025E01           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L40224:
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
