L0:
(W)     mov (8|M0)               r78.0<1>:ud   r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mov (8|M0)               r2.0<1>:w     0xD951C840:uv                             
(W)     add (1|M0)               r3.0<1>:d     r6.6<0;1,0>:d     32:w              
(W)     mov (8|M0)               r2.8<1>:w     0xFFFF0000:uv                             
(W)     add (1|M0)               r13.0<1>:d    r6.6<0;1,0>:d     64:w              
(W)     mov (2|M0)               r5.8<1>:w     0x40:uv                             
(W)     add (1|M0)               r15.0<1>:d    r6.6<0;1,0>:d     72:w              
(W)     mov (1|M0)               r4.5<1>:d     16:w                              
(W)     add (8|M0)               r8.0<1>:uq    r5.0<0;1,0>:uq    r2.0<8;8,1>:w   
(W)     add (8|M0)               r10.0<1>:ud   r3.0<0;1,0>:ud    r2.0<8;8,1>:w   
(W)     add (2|M0)               r14.0<1>:ud   r13.0<0;1,0>:ud   r5.8<2;2,1>:w   
(W)     send (1|M0)              r17      r15     0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     add (8|M0)               r8.0<1>:uq    r8.0<4;4,1>:uq    r2.8<8;8,1>:w   
(W)     add (8|M0)               r10.0<1>:ud   r10.0<8;8,1>:ud   r2.8<8;8,1>:w   
(W)     send (2|M0)              r16      r14     0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     send (8|M0)              r11      r8:uq   0xC            0x041408FF           // wr:2+0, rd:1; hdc.dc1; a64 byte gathering read 32b
(W)     send (8|M0)              r12      r10     0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     mov (8|M0)               r8.0<1>:w     0x76543210:v                              
(W)     add (8|M0)               r8.8<1>:w     r8.0<8;8,1>:w     8:w              
        shl (16|M0)              r18.0<1>:d    r8.0<16;16,1>:uw  2:w              
(W)     mov (1|M0)               r4.14<2>:b    r16.0<0;1,0>:b                  
(W)     mov (1|M0)               r4.12<2>:b    r16.1<0;1,0>:b                  
(W)     mov (1|M0)               r4.10<2>:b    r16.2<0;1,0>:b                  
(W)     mov (1|M0)               r6.1<1>:d     r11.0<0;1,0>:ub                 
(W)     mov (1|M0)               r3.0<1>:d     r11.1<0;1,0>:ub                 
(W)     mov (1|M0)               r2.0<1>:d     r11.5<0;1,0>:ub                 
(W)     mov (1|M0)               r3.1<1>:d     r11.4<0;1,0>:ub                 
(W)     mov (1|M0)               r2.1<1>:d     r11.8<0;1,0>:ub                 
(W)     mov (1|M0)               r2.2<1>:d     r11.9<0;1,0>:ub                 
(W)     mov (1|M0)               r2.3<1>:d     r11.12<0;1,0>:ub                
(W)     mov (1|M0)               r2.4<1>:d     r11.13<0;1,0>:ub                
(W)     mov (1|M0)               r3.2<1>:d     r11.21<0;1,0>:ub                
(W)     mov (1|M0)               r2.7<1>:d     r11.20<0;1,0>:ub                
(W)     mov (1|M0)               r5.6<1>:d     r12.5<0;1,0>:ub                 
(W)     mov (1|M0)               r9.1<1>:d     r11.2<0;1,0>:ub                 
(W)     mov (1|M0)               r5.7<1>:d     r12.8<0;1,0>:ub                 
(W)     shl (1|M0)               r7.7<1>:d     r6.1<0;1,0>:d     24:w              
(W)     shl (1|M0)               r9.0<1>:d     r3.0<0;1,0>:d     16:w              
(W)     mov (1|M0)               r2.5<1>:d     r11.16<0;1,0>:ub                
(W)     mov (1|M0)               r3.3<1>:d     r11.24<0;1,0>:ub                
(W)     mov (1|M0)               r2.6<1>:d     r11.17<0;1,0>:ub                
(W)     mov (1|M0)               r3.4<1>:d     r11.25<0;1,0>:ub                
(W)     mov (1|M0)               r3.5<1>:d     r11.28<0;1,0>:ub                
(W)     mov (1|M0)               r5.5<1>:d     r12.4<0;1,0>:ub                 
(W)     mov (1|M0)               r3.6<1>:d     r11.29<0;1,0>:ub                
(W)     mov (1|M0)               r6.2<1>:d     r12.9<0;1,0>:ub                 
(W)     mov (1|M0)               r9.4<1>:d     r11.6<0;1,0>:ub                 
(W)     shl (1|M0)               r9.3<1>:d     r2.0<0;1,0>:d     16:w              
(W)     mov (1|M0)               r6.3<1>:d     r12.12<0;1,0>:ub                
(W)     shl (1|M0)               r9.2<1>:d     r3.1<0;1,0>:d     24:w              
(W)     mov (1|M0)               r6.4<1>:d     r12.13<0;1,0>:ub                
(W)     mov (1|M0)               r9.7<1>:d     r11.10<0;1,0>:ub                
(W)     shl (1|M0)               r9.5<1>:d     r2.1<0;1,0>:d     24:w              
(W)     mov (1|M0)               r6.5<1>:d     r12.16<0;1,0>:ub                
(W)     shl (1|M0)               r9.6<1>:d     r2.2<0;1,0>:d     16:w              
(W)     mov (1|M0)               r6.7<1>:d     r12.17<0;1,0>:ub                
(W)     shl (1|M0)               r10.0<1>:d    r2.3<0;1,0>:d     24:w              
(W)     mov (1|M0)               r3.7<1>:d     r12.0<0;1,0>:ub                 
(W)     mov (1|M0)               r10.2<1>:d    r11.14<0;1,0>:ub                
(W)     mov (1|M0)               r5.4<1>:d     r12.1<0;1,0>:ub                 
(W)     shl (1|M0)               r10.1<1>:d    r2.4<0;1,0>:d     16:w              
(W)     shl (1|M0)               r3.0<1>:d     r2.7<0;1,0>:d     24:w              
(W)     shl (1|M0)               r10.5<1>:d    r3.2<0;1,0>:d     16:w              
(W)     shl (1|M0)               r2.1<1>:d     r5.6<0;1,0>:d     16:w              
(W)     mov (1|M0)               r7.3<1>:d     r12.20<0;1,0>:ub                
(W)     shl (1|M0)               r2.3<1>:d     r5.7<0;1,0>:d     24:w              
(W)     mov (1|M0)               r7.4<1>:d     r12.21<0;1,0>:ub                
(W)     mov (1|M0)               r7.5<1>:d     r12.24<0;1,0>:ub                
(W)     or (1|M0)                r5.6<1>:d     r7.7<0;1,0>:d     r9.0<0;1,0>:d   
(W)     mov (1|M0)               r7.6<1>:d     r12.25<0;1,0>:ub                
(W)     shl (1|M0)               r5.7<1>:d     r9.1<0;1,0>:d     8:w              
(W)     mov (1|M0)               r10.4<1>:d    r11.18<0;1,0>:ub                
(W)     mov (1|M0)               r13.1<1>:d    r11.26<0;1,0>:ub                
(W)     mov (1|M0)               r10.6<1>:d    r11.22<0;1,0>:ub                
(W)     mov (1|M0)               r13.4<1>:d    r11.30<0;1,0>:ub                
(W)     shl (1|M0)               r6.1<1>:d     r2.5<0;1,0>:d     24:w              
(W)     shl (1|M0)               r10.7<1>:d    r3.3<0;1,0>:d     24:w              
(W)     shl (1|M0)               r13.0<1>:d    r3.4<0;1,0>:d     16:w              
(W)     shl (1|M0)               r10.3<1>:d    r2.6<0;1,0>:d     16:w              
(W)     shl (1|M0)               r13.2<1>:d    r3.5<0;1,0>:d     24:w              
(W)     shl (1|M0)               r2.0<1>:d     r5.5<0;1,0>:d     24:w              
(W)     shl (1|M0)               r13.3<1>:d    r3.6<0;1,0>:d     16:w              
(W)     shl (1|M0)               r2.4<1>:d     r6.2<0;1,0>:d     16:w              
(W)     or (1|M0)                r14.1<1>:d    r9.2<0;1,0>:d     r9.3<0;1,0>:d   
(W)     shl (1|M0)               r6.2<1>:d     r9.4<0;1,0>:d     8:w              
(W)     shl (1|M0)               r2.7<1>:d     r6.4<0;1,0>:d     16:w              
(W)     shl (1|M0)               r6.4<1>:d     r9.7<0;1,0>:d     8:w              
(W)     shl (1|M0)               r2.6<1>:d     r6.3<0;1,0>:d     24:w              
(W)     or (1|M0)                r6.3<1>:d     r9.5<0;1,0>:d     r9.6<0;1,0>:d   
(W)     mov (1|M0)               r2.2<1>:d     r12.6<0;1,0>:ub                 
(W)     shl (1|M0)               r3.1<1>:d     r6.5<0;1,0>:d     24:w              
(W)     shl (1|M0)               r13.5<1>:d    r3.7<0;1,0>:d     24:w              
(W)     shl (1|M0)               r3.2<1>:d     r6.7<0;1,0>:d     16:w              
(W)     shl (1|M0)               r13.6<1>:d    r5.4<0;1,0>:d     16:w              
(W)     or (1|M0)                r14.4<1>:d    r3.0<0;1,0>:d     r10.5<0;1,0>:d  
(W)     or (1|M0)                r6.5<1>:d     r10.0<0;1,0>:d    r10.1<0;1,0>:d  
(W)     mov (1|M0)               r14.0<1>:d    r12.14<0;1,0>:ub                
(W)     shl (1|M0)               r6.7<1>:d     r10.2<0;1,0>:d    8:w              
(W)     mov (1|M0)               r2.5<1>:d     r12.10<0;1,0>:ub                
(W)     shl (1|M0)               r3.4<1>:d     r7.3<0;1,0>:d     24:w              
(W)     shl (1|M0)               r3.5<1>:d     r7.4<0;1,0>:d     16:w              
(W)     shl (1|M0)               r5.4<1>:d     r7.6<0;1,0>:d     16:w              
(W)     shl (1|M0)               r3.7<1>:d     r7.5<0;1,0>:d     24:w              
(W)     or (1|M0)                r10.5<1>:d    r5.6<0;1,0>:d     r5.7<0;1,0>:d   
(W)     shl (1|M0)               r14.3<1>:d    r10.4<0;1,0>:d    8:w              
(W)     mov (1|M0)               r5.5<1>:d     r12.26<0;1,0>:ub                
(W)     shl (1|M0)               r14.5<1>:d    r10.6<0;1,0>:d    8:w              
(W)     mov (1|M0)               r3.6<1>:d     r12.22<0;1,0>:ub                
(W)     or (1|M0)                r14.2<1>:d    r6.1<0;1,0>:d     r10.3<0;1,0>:d  
(W)     or (1|M0)                r9.1<1>:d     r2.0<0;1,0>:d     r2.1<0;1,0>:d   
(W)     or (1|M0)                r7.3<1>:d     r10.7<0;1,0>:d    r13.0<0;1,0>:d  
(W)     shl (1|M0)               r7.4<1>:d     r13.1<0;1,0>:d    8:w              
(W)     shl (1|M0)               r7.6<1>:d     r13.4<0;1,0>:d    8:w              
(W)     mov (1|M0)               r13.7<1>:d    r12.2<0;1,0>:ub                 
(W)     or (1|M0)                r7.5<1>:d     r13.2<0;1,0>:d    r13.3<0;1,0>:d  
(W)     and (1|M0)               r10.3<1>:d    r78.5<0;1,0>:d    -1024:w              
(W)     or (1|M0)                r2.0<1>:d     r14.1<0;1,0>:d    r6.2<0;1,0>:d   
(W)     shl (1|M0)               r9.2<1>:d     r2.2<0;1,0>:d     8:w              
(W)     or (1|M0)                r2.1<1>:d     r6.3<0;1,0>:d     r6.4<0;1,0>:d   
(W)     mul (1|M0)               r10.4<1>:d    r7.0<0;1,0>:d     r78.1<0;1,0>:d  
(W)     or (1|M0)                r2.2<1>:d     r6.5<0;1,0>:d     r6.7<0;1,0>:d   
(W)     mov (1|M0)               r3.3<1>:d     r12.18<0;1,0>:ub                
(W)     shl (1|M0)               r9.4<1>:d     r14.0<0;1,0>:d    8:w              
(W)     or (1|M0)                r3.0<1>:d     r2.6<0;1,0>:d     r2.7<0;1,0>:d   
(W)     shl (1|M0)               r9.3<1>:d     r2.5<0;1,0>:d     8:w              
(W)     or (1|M0)                r6.1<1>:d     r2.3<0;1,0>:d     r2.4<0;1,0>:d   
(W)     or (1|M0)                r5.7<1>:d     r10.5<0;1,0>:d    r11.3<0;1,0>:ub 
(W)     or (1|M0)                r9.7<1>:d     r3.4<0;1,0>:d     r3.5<0;1,0>:d   
(W)     or (1|M0)                r10.1<1>:d    r3.7<0;1,0>:d     r5.4<0;1,0>:d   
(W)     shl (1|M0)               r10.2<1>:d    r5.5<0;1,0>:d     8:w              
(W)     or (1|M0)                r2.6<1>:d     r7.5<0;1,0>:d     r7.6<0;1,0>:d   
(W)     shl (1|M0)               r10.0<1>:d    r3.6<0;1,0>:d     8:w              
(W)     or (1|M0)                r2.5<1>:d     r7.3<0;1,0>:d     r7.4<0;1,0>:d   
(W)     or (1|M0)                r2.3<1>:d     r14.2<0;1,0>:d    r14.3<0;1,0>:d  
(W)     or (1|M0)                r7.7<1>:d     r13.5<0;1,0>:d    r13.6<0;1,0>:d  
(W)     or (1|M0)                r2.4<1>:d     r14.4<0;1,0>:d    r14.5<0;1,0>:d  
(W)     shl (1|M0)               r9.0<1>:d     r13.7<0;1,0>:d    8:w              
        add (16|M0)              r38.0<1>:d    r10.3<0;1,0>:d    r18.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r8.1<1>:d     r2.0<0;1,0>:d     r11.7<0;1,0>:ub 
(W)     or (1|M0)                r9.5<1>:d     r3.1<0;1,0>:d     r3.2<0;1,0>:d   
(W)     or (1|M0)                r8.2<1>:d     r2.1<0;1,0>:d     r11.11<0;1,0>:ub
        add (16|M0)              r20.0<1>:d    r10.4<0;1,0>:d    r1.0<16;16,1>:uw
(W)     or (1|M0)                r7.3<1>:d     r2.2<0;1,0>:d     r11.15<0;1,0>:ub
(W)     shl (1|M0)               r9.6<1>:d     r3.3<0;1,0>:d     8:w              
(W)     or (1|M0)                r8.0<1>:d     r3.0<0;1,0>:d     r9.4<0;1,0>:d    {Compacted}
        mov (16|M0)              r14.0<1>:d    r5.7<0;1,0>:d                   
(W)     or (1|M0)                r3.2<1>:d     r6.1<0;1,0>:d     r9.3<0;1,0>:d   
(W)     or (1|M0)                r5.6<1>:d     r10.1<0;1,0>:d    r10.2<0;1,0>:d  
(W)     or (1|M0)                r7.4<1>:d     r2.3<0;1,0>:d     r11.19<0;1,0>:ub
(W)     or (1|M0)                r5.5<1>:d     r9.7<0;1,0>:d     r10.0<0;1,0>:d  
(W)     or (1|M0)                r3.3<1>:d     r2.6<0;1,0>:d     r11.31<0;1,0>:ub
(W)     or (1|M0)                r6.1<1>:d     r2.4<0;1,0>:d     r11.23<0;1,0>:ub
(W)     or (1|M0)                r3.0<1>:d     r2.5<0;1,0>:d     r11.27<0;1,0>:ub
(W)     or (1|M0)                r2.7<1>:d     r7.7<0;1,0>:d     r9.0<0;1,0>:d   
        add (16|M0)              r80.0<1>:d    r38.0<8;8,1>:d    64:w              
        mov (16|M0)              r10.0<1>:d    r8.1<0;1,0>:d                    {Compacted}
(W)     or (1|M0)                r3.1<1>:d     r9.1<0;1,0>:d     r9.2<0;1,0>:d   
        add (16|M0)              r76.0<1>:d    r38.0<8;8,1>:d    128:w              
        mov (16|M0)              r18.0<1>:d    r8.2<0;1,0>:d                    {Compacted}
        add (16|M0)              r74.0<1>:d    r38.0<8;8,1>:d    192:w              
        add (16|M0)              r82.0<1>:d    r20.0<8;8,1>:d    r4.0<0;1,0>:d    {Compacted}
        mov (16|M0)              r22.0<1>:d    r7.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r38     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r72.0<1>:d    r38.0<8;8,1>:d    256:w              
        mov (16|M0)              r20.0<1>:d    r7.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r80     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     or (1|M0)                r3.4<1>:d     r2.7<0;1,0>:d     r12.3<0;1,0>:ub 
        add (16|M0)              r70.0<1>:d    r38.0<8;8,1>:d    320:w              
(W)     or (1|M0)                r5.4<1>:d     r9.5<0;1,0>:d     r9.6<0;1,0>:d   
        mov (16|M0)              r14.0<1>:d    r6.1<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r76     r18     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     or (1|M0)                r6.4<1>:d     r8.0<0;1,0>:d     r12.15<0;1,0>:ub
        add (16|M0)              r68.0<1>:d    r38.0<8;8,1>:d    384:w              
(W)     or (1|M0)                r6.2<1>:d     r3.1<0;1,0>:d     r12.7<0;1,0>:ub 
        mov (16|M0)              r8.0<1>:d     r3.0<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r74     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     or (1|M0)                r6.3<1>:d     r3.2<0;1,0>:d     r12.11<0;1,0>:ub
        add (16|M0)              r66.0<1>:d    r38.0<8;8,1>:d    448:w              
        mov (16|M0)              r10.0<1>:d    r3.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r72     r20     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r64.0<1>:d    r38.0<8;8,1>:d    512:w              
        mov (16|M0)              r18.0<1>:d    r3.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r70     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     or (1|M0)                r6.5<1>:d     r5.4<0;1,0>:d     r12.19<0;1,0>:ub
        add (16|M0)              r62.0<1>:d    r38.0<8;8,1>:d    576:w              
        mov (16|M0)              r20.0<1>:d    r6.2<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r68     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r60.0<1>:d    r38.0<8;8,1>:d    640:w              
        mov (16|M0)              r14.0<1>:d    r6.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r66     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r42.0<1>:d    r38.0<8;8,1>:d    704:w              
        mov (16|M0)              r8.0<1>:d     r6.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r64     r18     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r40.0<1>:d    r38.0<8;8,1>:d    768:w              
        mov (16|M0)              r2.0<1>:d     r6.5<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r62     r20     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     or (1|M0)                r6.7<1>:d     r5.5<0;1,0>:d     r12.23<0;1,0>:ub
        sends (16|M0)            null:w   r60     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r36.0<1>:d    r38.0<8;8,1>:d    832:w              
        sends (16|M0)            null:w   r42     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     or (1|M0)                r7.5<1>:d     r5.6<0;1,0>:d     r12.27<0;1,0>:ub
        sends (16|M0)            null:w   r40     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               r2.0<1>:d     r12.28<0;1,0>:ub                
(W)     mov (1|M0)               r3.2<1>:d     r12.30<0;1,0>:ub                
(W)     mov (1|M0)               r2.1<1>:d     r12.29<0;1,0>:ub                
        mov (16|M0)              r10.0<1>:d    r6.7<0;1,0>:d                   
        add (16|M0)              r46.0<1>:d    r38.0<8;8,1>:d    896:w              
(W)     shl (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     24:w              
(W)     shl (1|M0)               r5.5<1>:d     r3.2<0;1,0>:d     8:w              
(W)     shl (1|M0)               r3.1<1>:d     r2.1<0;1,0>:d     16:w              
        sends (16|M0)            null:w   r36     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r8.0<1>:d     r7.5<0;1,0>:d                    {Compacted}
        add (16|M0)              r44.0<1>:d    r38.0<8;8,1>:d    960:w              
(W)     or (1|M0)                r5.4<1>:d     r3.0<0;1,0>:d     r3.1<0;1,0>:d   
        add (16|M0)              r48.0<1>:d    r82.0<8;8,1>:d    r6.0<0;1,0>:d    {Compacted}
        sends (16|M0)            null:w   r46     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               r4.8<2>:b     r16.3<0;1,0>:b                  
(W)     mov (1|M0)               r4.18<2>:b    r16.4<0;1,0>:b                  
(W)     or (1|M0)                r6.1<1>:d     r5.4<0;1,0>:d     r5.5<0;1,0>:d   
(W)     mov (1|M0)               r4.16<2>:b    r16.5<0;1,0>:b                  
(W)     mov (1|M0)               r4.6<2>:b     r16.6<0;1,0>:b                  
(W)     mov (1|M0)               r4.4<2>:b     r16.7<0;1,0>:b                  
(W)     mov (4|M0)               r4.0<1>:b     r17.0<4;4,1>:b                  
(W)     or (1|M0)                r7.3<1>:d     r6.1<0;1,0>:d     r12.31<0;1,0>:ub
        mov (16|M0)              r10.0<1>:d    r7.3<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r44     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
L3488:
(W)     add (1|M0)               r2.0<1>:d     r4.5<0;1,0>:d     -2:w              
(W)     add (1|M0)               r3.1<1>:d     r4.5<0;1,0>:d     -15:w              
(W)     add (1|M0)               r6.1<1>:d     r4.5<0;1,0>:d     -7:w              
(W)     add (1|M0)               r18.0<1>:d    r4.5<0;1,0>:d     -16:w              
(W)     cmp (16|M0)   (lt)f1.1   null<1>:d     r4.5<0;1,0>:ud    0x3F:uw             
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r5.4<1>:d     r3.1<0;1,0>:d     64:w              
(W)     mul (1|M0)               r7.3<1>:d     r6.1<0;1,0>:d     64:w              
(W)     mul (1|M0)               r2.0<1>:d     r18.0<0;1,0>:d    64:w              
        add (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r38.0<8;8,1>:d    r5.4<0;1,0>:d    {Compacted}
        add (16|M0)              r16.0<1>:d    r38.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
        add (16|M0)              r22.0<1>:d    r38.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r20:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r24:w    r22     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     mul (1|M0)               r5.4<1>:d     r4.5<0;1,0>:d     64:w              
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    15:w              
        shr (16|M0)              r8.0<1>:ud    r10.0<8;8,1>:ud   17:w              
        shl (16|M0)              r28.0<1>:d    r10.0<8;8,1>:d    13:w              
        shr (16|M0)              r12.0<1>:ud   r10.0<8;8,1>:ud   19:w              
        shl (16|M0)              r30.0<1>:d    r14.0<8;8,1>:d    25:w              
        shr (16|M0)              r32.0<1>:ud   r14.0<8;8,1>:ud   7:w              
        shl (16|M0)              r16.0<1>:d    r14.0<8;8,1>:d    14:w              
        shr (16|M0)              r18.0<1>:ud   r14.0<8;8,1>:ud   18:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r2.0<1>:ud    r10.0<8;8,1>:ud   10:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r14.0<8;8,1>:ud   3:w              
        add (16|M0)              r28.0<1>:d    r38.0<8;8,1>:d    r5.4<0;1,0>:d    {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r22.0<1>:d    r26.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r22.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r12.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r34.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r28     r24     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(~f1.1) break (16|M0)                        L4048                  L4048                
L4032:
(W)     add (1|M0)               r4.5<1>:d     r4.5<0;1,0>:d     1:w              
L4048:
        while (16|M0)                        L3488                                
L4064:
        mov (16|M0)              r34.0<1>:d    1779033703:d                              
        mov (16|M0)              r58.0<1>:d    -1150833019:d                              
        mov (16|M0)              r56.0<1>:d    1013904242:d                              
        mov (16|M0)              r54.0<1>:d    -1521486534:d                              
        mov (16|M0)              r32.0<1>:d    1359893119:d                              
        mov (16|M0)              r52.0<1>:d    -1694144372:d                              
        mov (16|M0)              r50.0<1>:d    528734635:d                              
        mov (16|M0)              r6.0<1>:d     1541459225:d                              
(W)     mov (1|M0)               r4.5<1>:d     0:w                              
L4208:
(W)     shl (1|M0)               r2.0<1>:d     r4.5<0;1,0>:d     2:w              
        shl (16|M0)              r12.0<1>:d    r32.0<8;8,1>:d    26:w              
(W)     mul (1|M0)               r2.1<1>:d     r4.5<0;1,0>:d     64:w              
        shr (16|M0)              r14.0<1>:ud   r32.0<8;8,1>:ud   6:w              
        shl (16|M0)              r16.0<1>:d    r32.0<8;8,1>:d    21:w              
(W)     send (1|M0)              r3       r2      0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        shr (16|M0)              r18.0<1>:ud   r32.0<8;8,1>:ud   11:w              
        add (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r20.0<1>:d    r32.0<8;8,1>:d    7:w              
        shr (16|M0)              r22.0<1>:ud   r32.0<8;8,1>:ud   25:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r50.0<8;8,1>:d    ~r32.0<8;8,1>:d  {Compacted}
        shl (16|M0)              r26.0<1>:d    r34.0<8;8,1>:d    30:w              
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r28.0<1>:ud   r34.0<8;8,1>:ud   2:w              
        shl (16|M0)              r30.0<1>:d    r34.0<8;8,1>:d    19:w              
        shr (16|M0)              r14.0<1>:ud   r34.0<8;8,1>:ud   13:w              
        and (16|M0)              r8.0<1>:d     r32.0<8;8,1>:d    r52.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r18.0<1>:ud   r34.0<8;8,1>:ud   22:w              
        shl (16|M0)              r16.0<1>:d    r34.0<8;8,1>:d    10:w              
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r20.0<1>:d    r58.0<8;8,1>:d    r56.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r6.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r58.0<8;8,1>:d    r56.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r34.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     cmp (16|M0)   (lt)f1.0   null<1>:d     r4.5<0;1,0>:ud    0x3F:uw             
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r22.0<1>:d    r58.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r16.0<1>:d    r52.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r20.0<1>:d    r56.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r10.0<1>:d    r50.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r2.0<1>:d     r54.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        add (16|M0)              r6.0<1>:d     r14.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r18.0<1>:d    r2.0<8;8,1>:d                    {Compacted}
        mov (16|M0)              r24.0<1>:d    r6.0<8;8,1>:d                    {Compacted}
(~f1.0) break (16|M0)                        L4824                  L4824                
L4744:
        mov (16|M0)              r54.0<1>:d    r56.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r56.0<1>:d    r58.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r58.0<1>:d    r34.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r34.0<1>:d    r6.0<8;8,1>:d                    {Compacted}
        mov (16|M0)              r6.0<1>:d     r50.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r50.0<1>:d    r52.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r52.0<1>:d    r32.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r32.0<1>:d    r2.0<8;8,1>:d                    {Compacted}
(W)     add (1|M0)               r4.5<1>:d     r4.5<0;1,0>:d     1:w              
L4824:
        while (16|M0)                        L4208                                
L4840:
(W)     mov (1|M0)               r2.0<1>:d     r4.14<0;1,0>:ub                 
(W)     mov (1|M0)               r3.1<1>:d     r4.12<0;1,0>:ub                 
(W)     mov (1|M0)               r6.2<1>:d     r4.10<0;1,0>:ub                 
(W)     mov (1|M0)               r9.0<1>:d     r4.18<0;1,0>:ub                 
(W)     mov (1|M0)               r8.1<1>:d     r4.1<0;1,0>:ub                  
        add (16|M0)              r50.0<1>:d    r10.0<8;8,1>:d    1541459225:d              
(W)     shl (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     24:w              
(W)     shl (1|M0)               r5.4<1>:d     r3.1<0;1,0>:d     16:w              
(W)     shl (1|M0)               r7.3<1>:d     r6.2<0;1,0>:d     8:w              
(W)     mov (1|M0)               r2.0<1>:d     r4.16<0;1,0>:ub                 
(W)     shl (1|M0)               r12.0<1>:d    r9.0<0;1,0>:d     24:w              
(W)     mov (1|M0)               r2.1<1>:d     r4.2<0;1,0>:ub                  
(W)     shl (1|M0)               r9.0<1>:d     r8.1<0;1,0>:d     16:w              
        add (16|M0)              r52.0<1>:d    r16.0<8;8,1>:d    528734635:d              
(W)     or (1|M0)                r6.1<1>:d     r3.0<0;1,0>:d     r5.4<0;1,0>:d   
(W)     shl (1|M0)               r12.1<1>:d    r2.0<0;1,0>:d     16:w              
(W)     mov (1|M0)               r3.0<1>:d     r4.6<0;1,0>:ub                  
        add (16|M0)              r88.0<1>:d    r34.0<8;8,1>:d    -1150833019:d              
        add (16|M0)              r90.0<1>:d    r24.0<8;8,1>:d    1779033703:d              
(W)     or (1|M0)                r8.0<1>:d     r6.1<0;1,0>:d     r7.3<0;1,0>:d   
(W)     mov (1|M0)               r7.3<1>:d     r4.0<0;1,0>:ub                  
(W)     or (1|M0)                r13.0<1>:d    r12.0<0;1,0>:d    r12.1<0;1,0>:d   {Compacted}
(W)     shl (1|M0)               r5.4<1>:d     r3.0<0;1,0>:d     8:w              
        add (16|M0)              r86.0<1>:d    r22.0<8;8,1>:d    1013904242:d              
        add (16|M0)              r84.0<1>:d    r20.0<8;8,1>:d    -1521486534:d              
(W)     or (1|M0)                r4.2<1>:d     r8.0<0;1,0>:d     r4.8<0;1,0>:ub  
(W)     shl (1|M0)               r8.0<1>:d     r7.3<0;1,0>:d     24:w              
(W)     or (1|M0)                r14.0<1>:d    r13.0<0;1,0>:d    r5.4<0;1,0>:d    {Compacted}
        add (16|M0)              r56.0<1>:d    r18.0<8;8,1>:d    1359893119:d              
        add (16|M0)              r54.0<1>:d    r32.0<8;8,1>:d    -1694144372:d              
        mov (16|M0)              r10.0<1>:d    r4.2<0;1,0>:d                    {Compacted}
(W)     or (1|M0)                r2.0<1>:d     r8.0<0;1,0>:d     r9.0<0;1,0>:d    {Compacted}
(W)     or (1|M0)                r6.1<1>:d     r14.0<0;1,0>:d    r4.4<0;1,0>:ub  
        mov (16|M0)              r14.3<4>:ub   r48.0<4;1,0>:ub                 
        mov (16|M0)              r8.0<1>:d     -2147483648:d                              
        sends (16|M0)            null:w   r38     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     shl (1|M0)               r10.0<1>:d    r2.1<0;1,0>:d     8:w              
        mov (16|M0)              r16.0<1>:d    r6.1<0;1,0>:d                    {Compacted}
        mov (16|M0)              r14.0<4>:ub   r48.3<4;1,0>:ub                 
        mov (16|M0)              r14.2<4>:ub   r48.1<4;1,0>:ub                 
(W)     or (1|M0)                r3.0<1>:d     r2.0<0;1,0>:d     r10.0<0;1,0>:d   {Compacted}
        mov (16|M0)              r14.1<4>:ub   r48.2<4;1,0>:ub                 
        sends (16|M0)            null:w   r80     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r16.0<1>:d    0:w                              
(W)     or (1|M0)                r5.4<1>:d     r3.0<0;1,0>:d     r4.3<0;1,0>:ub  
        mov (16|M0)              r2.0<1>:d     640:w                              
        mov (16|M0)              r34.0<1>:d    r4.2<0;1,0>:d                    {Compacted}
(W)     mov (1|M0)               r4.0<1>:d     16:w                              
        mov (16|M0)              r12.0<1>:d    r5.4<0;1,0>:d                    {Compacted}
        sends (16|M0)            null:w   r76     r12     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r74     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r72     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r70     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r68     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r66     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r64     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r62     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r60     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r42     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r40     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r36     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r46     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r44     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
L5784:
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -2:w              
(W)     add (1|M0)               r3.1<1>:d     r4.0<0;1,0>:d     -15:w              
(W)     add (1|M0)               r6.1<1>:d     r4.0<0;1,0>:d     -7:w              
(W)     cmp (16|M0)   (lt)f0.1   null<1>:d     r4.0<0;1,0>:ud    0x3F:uw             
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r5.4<1>:d     r3.1<0;1,0>:d     64:w              
(W)     mul (1|M0)               r7.3<1>:d     r6.1<0;1,0>:d     64:w              
        add (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r32.0<1>:d    r38.0<8;8,1>:d    r5.4<0;1,0>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r38.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r5.4<1>:d     r4.0<0;1,0>:d     64:w              
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r12:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r18.0<1>:d    r10.0<8;8,1>:d    15:w              
        shr (16|M0)              r20.0<1>:ud   r10.0<8;8,1>:ud   17:w              
        shl (16|M0)              r22.0<1>:d    r10.0<8;8,1>:d    13:w              
        shr (16|M0)              r24.0<1>:ud   r10.0<8;8,1>:ud   19:w              
        shl (16|M0)              r2.0<1>:d     r12.0<8;8,1>:d    25:w              
        shr (16|M0)              r8.0<1>:ud    r12.0<8;8,1>:ud   7:w              
        shl (16|M0)              r26.0<1>:d    r12.0<8;8,1>:d    14:w              
        shr (16|M0)              r28.0<1>:ud   r12.0<8;8,1>:ud   18:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r10.0<8;8,1>:ud   10:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r20.0<1>:ud   r12.0<8;8,1>:ud   3:w              
        add (16|M0)              r22.0<1>:d    r38.0<8;8,1>:d    r5.4<0;1,0>:d    {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r30.0<1>:d    r18.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r30.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r24.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r34.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r22     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(~f0.1) break (16|M0)                        L6304                  L6304                
L6272:
        send (16|M0)             r34:w    r32     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     1:w              
L6304:
        while (16|M0)                        L5784                                
L6320:
        mov (16|M0)              r22.0<1>:d    r4.2<0;1,0>:d                    {Compacted}
        mov (16|M0)              r34.0<1>:d    r90.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r32.0<1>:d    r88.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r30.0<1>:d    r86.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r28.0<1>:d    r84.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r26.0<1>:d    r56.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r24.0<1>:d    r54.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r6.0<1>:d     r52.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r18.0<1>:d    r50.0<8;8,1>:d                   {Compacted}
(W)     mov (1|M0)               r5.0<1>:d     0:w                              
L6408:
        shl (16|M0)              r2.0<1>:d     r26.0<8;8,1>:d    26:w              
        shr (16|M0)              r8.0<1>:ud    r26.0<8;8,1>:ud   6:w              
        shl (16|M0)              r10.0<1>:d    r26.0<8;8,1>:d    21:w              
        shr (16|M0)              r12.0<1>:ud   r26.0<8;8,1>:ud   11:w              
        shl (16|M0)              r14.0<1>:d    r26.0<8;8,1>:d    7:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
(W)     shl (1|M0)               r8.0<1>:d     r5.0<0;1,0>:d     2:w              
        shr (16|M0)              r16.0<1>:ud   r26.0<8;8,1>:ud   25:w              
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r20.0<1>:d    r6.0<8;8,1>:d     ~r26.0<8;8,1>:d  {Compacted}
(W)     send (1|M0)              r9       r8      0xA            0x02110804           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r34.0<8;8,1>:d    30:w              
        shr (16|M0)              r16.0<1>:ud   r34.0<8;8,1>:ud   2:w              
(W)     cmp (16|M0)   (lt)f0.0   null<1>:d     r5.0<0;1,0>:ud    0x3F:uw             
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r14.0<1>:d    r34.0<8;8,1>:d    19:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r16.0<1>:d    r24.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r2.0<1>:d     r18.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        and (16|M0)              r18.0<1>:d    r26.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r20.0<1>:ud   r34.0<8;8,1>:ud   22:w              
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r18.0<1>:d    r34.0<8;8,1>:d    10:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r20.0<1>:d    r26.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r9.0<0;1,0>:d    {Compacted}
        and (16|M0)              r8.0<1>:d     r32.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r2.0<8;8,1>:d     r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r34.0<8;8,1>:ud   13:w              
        xor (16|M0)              r2.0<1>:d     r32.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        and (16|M0)              r2.0<1>:d     r34.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        mov (16|M0)              r22.0<1>:d    r32.0<8;8,1>:d                   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r14.0<1>:d    r28.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r28.0<1>:d    r34.0<8;8,1>:d                   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r18.0<1>:d    r6.0<8;8,1>:d                    {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r48.0<1>:d    r10.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        mov (16|M0)              r10.0<1>:d    r30.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r12.0<1>:d    r14.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r58.0<1>:d    r48.0<8;8,1>:d                   {Compacted}
(~f0.0) break (16|M0)                        L7040                  L7040                
L6920:
(W)     add (1|M0)               r5.0<1>:d     r5.0<0;1,0>:d     1:w              
        mov (16|M0)              r28.0<1>:d    r30.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r18.0<1>:d    r6.0<8;8,1>:d                    {Compacted}
        mov (16|M0)              r30.0<1>:d    r32.0<8;8,1>:d                   {Compacted}
(W)     mul (1|M0)               r2.0<1>:d     r5.0<0;1,0>:d     64:w              
        mov (16|M0)              r6.0<1>:d     r24.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r32.0<1>:d    r34.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r24.0<1>:d    r26.0<8;8,1>:d                   {Compacted}
        add (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d    r2.0<0;1,0>:d    {Compacted}
        mov (16|M0)              r34.0<1>:d    r48.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r26.0<1>:d    r14.0<8;8,1>:d                   {Compacted}
        send (16|M0)             r22:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
L7040:
        while (16|M0)                        L6408                                
L7056:
        add (16|M0)              r32.0<1>:d    r90.0<8;8,1>:d    r58.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r88.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        add (16|M0)              r8.0<1>:d     r86.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r84.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r38     r32     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r12.0<1>:d    r56.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r80     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r14.0<1>:d    r54.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r76     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r16.0<1>:d    r52.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r74     r10     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        add (16|M0)              r18.0<1>:d    r50.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r72     r12     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r20.0<1>:d    -2147483648:d                              
        sends (16|M0)            null:w   r70     r14     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r22.0<1>:d    0:w                              
        sends (16|M0)            null:w   r68     r16     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r2.0<1>:d     256:w                              
        sends (16|M0)            null:w   r66     r18     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        mov (16|M0)              r30.0<1>:d    0:w                              
        sends (16|M0)            null:w   r64     r20     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r62     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r60     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r42     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r40     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r36     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r46     r22     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M0)            null:w   r44     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               r4.0<1>:d     16:w                              
L7456:
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -15:w              
(W)     add (1|M0)               r5.4<1>:d     r4.0<0;1,0>:d     -7:w              
        shl (16|M0)              r16.0<1>:d    r30.0<8;8,1>:d    15:w              
        shr (16|M0)              r18.0<1>:ud   r30.0<8;8,1>:ud   17:w              
        shl (16|M0)              r20.0<1>:d    r30.0<8;8,1>:d    13:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.1<1>:d     r5.4<0;1,0>:d     64:w              
        shr (16|M0)              r22.0<1>:ud   r30.0<8;8,1>:ud   19:w              
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r5.4<1>:d     r4.0<0;1,0>:d     64:w              
        add (16|M0)              r8.0<1>:d     r38.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r38.0<8;8,1>:d    r6.1<0;1,0>:d    {Compacted}
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
(W)     add (1|M0)               r6.1<1>:d     r4.0<0;1,0>:d     -1:w              
        send (16|M0)             r10:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r16.0<1>:d    r16.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r30.0<8;8,1>:ud   10:w              
        add (16|M0)              r20.0<1>:d    r38.0<8;8,1>:d    r5.4<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r7.4<1>:d     r4.0<0;1,0>:d     -14:w              
(W)     mul (1|M0)               r7.3<1>:d     r6.1<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.4<1>:d     r4.0<0;1,0>:d     1:w              
        xor (16|M0)              r30.0<1>:d    r16.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r38.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.1<1>:d     r5.4<0;1,0>:d     64:w              
(W)     add (1|M0)               r7.3<1>:d     r4.0<0;1,0>:d     -13:w              
(W)     or (1|M0)                r5.4<1>:d     r4.0<0;1,0>:d     2:w              
        shl (16|M0)              r2.0<1>:d     r10.0<8;8,1>:d    25:w              
        shr (16|M0)              r24.0<1>:ud   r10.0<8;8,1>:ud   7:w              
        shl (16|M0)              r26.0<1>:d    r10.0<8;8,1>:d    14:w              
        shr (16|M0)              r28.0<1>:ud   r10.0<8;8,1>:ud   18:w              
        shr (16|M0)              r18.0<1>:ud   r10.0<8;8,1>:ud   3:w              
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r22.0<1>:d    r30.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r10.0<1>:d    r7.4<0;1,0>:d     64:w              
(W)     add (1|M0)               r11.0<1>:d    r4.0<0;1,0>:d     -6:w              
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r38.0<8;8,1>:d    r10.0<0;1,0>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r22.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r22.0<1>:d    r11.0<0;1,0>:d    64:w              
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    r22.0<0;1,0>:d   {Compacted}
        sends (16|M0)            null:w   r20     r2      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r14:w    r12     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r18:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r2:w     r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r26:w    r8      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r28.0<1>:d    r14.0<8;8,1>:d    15:w              
        shr (16|M0)              r30.0<1>:ud   r14.0<8;8,1>:ud   17:w              
        shl (16|M0)              r32.0<1>:d    r14.0<8;8,1>:d    13:w              
        shr (16|M0)              r34.0<1>:ud   r14.0<8;8,1>:ud   19:w              
        shl (16|M0)              r12.0<1>:d    r18.0<8;8,1>:d    25:w              
        shr (16|M0)              r36.0<1>:ud   r18.0<8;8,1>:ud   7:w              
        shl (16|M0)              r10.0<1>:d    r18.0<8;8,1>:d    14:w              
        shr (16|M0)              r22.0<1>:ud   r18.0<8;8,1>:ud   18:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r8.0<1>:ud    r14.0<8;8,1>:ud   10:w              
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r18.0<8;8,1>:ud   3:w              
        add (16|M0)              r14.0<1>:d    r38.0<8;8,1>:d    r6.1<0;1,0>:d    {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r28.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r8.0<1>:d     r7.3<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.1<1>:d     r5.4<0;1,0>:d     64:w              
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r24.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r18.0<1>:d    r38.0<8;8,1>:d    r8.0<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -5:w              
        add (16|M0)              r12.0<1>:d    r34.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r24.0<1>:d    r38.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
        sends (16|M0)            null:w   r14     r12     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r10:w    r20     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r22:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r28:w    r24     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r26:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r30.0<1>:d    r10.0<8;8,1>:d    15:w              
        shr (16|M0)              r12.0<1>:ud   r10.0<8;8,1>:ud   17:w              
        shl (16|M0)              r32.0<1>:d    r10.0<8;8,1>:d    13:w              
        shr (16|M0)              r34.0<1>:ud   r10.0<8;8,1>:ud   19:w              
        shl (16|M0)              r8.0<1>:d     r22.0<8;8,1>:d    25:w              
        shr (16|M0)              r20.0<1>:ud   r22.0<8;8,1>:ud   7:w              
        shl (16|M0)              r36.0<1>:d    r22.0<8;8,1>:d    14:w              
        shr (16|M0)              r2.0<1>:ud    r22.0<8;8,1>:ud   18:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r16.0<1>:ud   r10.0<8;8,1>:ud   10:w              
        or (16|M0)               r8.0<1>:d     r8.0<8;8,1>:d     r20.0<8;8,1>:d   {Compacted}
        or (16|M0)               r36.0<1>:d    r36.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        xor (16|M0)              r30.0<1>:d    r30.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r22.0<8;8,1>:ud   3:w              
(W)     add (1|M0)               r3.1<1>:d     r4.0<0;1,0>:d     -4:w              
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r36.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r30.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r36.0<1>:d    r38.0<8;8,1>:d    r6.1<0;1,0>:d    {Compacted}
(W)     add (1|M0)               r2.0<1>:d     r4.0<0;1,0>:d     -12:w              
        xor (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
(W)     mul (1|M0)               r7.3<1>:d     r3.1<0;1,0>:d     64:w              
(W)     mul (1|M0)               r3.0<1>:d     r2.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r2.0<1>:d     r4.0<0;1,0>:d     3:w              
        add (16|M0)              r8.0<1>:d     r34.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r16.0<1>:d    r38.0<8;8,1>:d    r7.3<0;1,0>:d    {Compacted}
        add (16|M0)              r6.0<1>:d     r38.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r2.1<1>:d     r2.0<0;1,0>:d     64:w              
        add (16|M0)              r8.0<1>:d     r8.0<8;8,1>:d     r26.0<8;8,1>:d   {Compacted}
(W)     cmp (16|M0)   (lt)f1.1   null<1>:d     r2.0<0;1,0>:ud    0x3F:uw             
        sends (16|M0)            null:w   r36     r8      0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        send (16|M0)             r10:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r12:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r16:w    r6      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r30:w    r18     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        shl (16|M0)              r20.0<1>:d    r10.0<8;8,1>:d    15:w              
        shr (16|M0)              r22.0<1>:ud   r10.0<8;8,1>:ud   17:w              
        shl (16|M0)              r24.0<1>:d    r10.0<8;8,1>:d    13:w              
        shr (16|M0)              r26.0<1>:ud   r10.0<8;8,1>:ud   19:w              
        shl (16|M0)              r32.0<1>:d    r16.0<8;8,1>:d    25:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r34.0<1>:ud   r16.0<8;8,1>:ud   7:w              
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r16.0<8;8,1>:d    14:w              
        shr (16|M0)              r26.0<1>:ud   r16.0<8;8,1>:ud   18:w              
        shr (16|M0)              r8.0<1>:ud    r10.0<8;8,1>:ud   10:w              
        xor (16|M0)              r20.0<1>:d    r20.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r38.0<8;8,1>:d    r2.1<0;1,0>:d    {Compacted}
        xor (16|M0)              r14.0<1>:d    r20.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        shr (16|M0)              r8.0<1>:ud    r16.0<8;8,1>:ud   3:w              
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r14.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r8.0<8;8,1>:d    {Compacted}
        add (16|M0)              r32.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        sends (16|M0)            null:w   r10     r32     0x8C            0x04025EFF           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(~f1.1) break (16|M0)                        L9320                  L9320                
L9272:
        send (16|M0)             r30:w    r36     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r32:w    r6      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
(W)     add (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     4:w              
L9320:
        while (16|M0)                        L7456                                
L9336:
        mov (16|M0)              r50.0<1>:d    1779033703:d                              
        mov (16|M0)              r48.0<1>:d    -1150833019:d                              
        mov (16|M0)              r34.0<1>:d    1013904242:d                              
        mov (16|M0)              r46.0<1>:d    -1521486534:d                              
        mov (16|M0)              r44.0<1>:d    1359893119:d                              
        mov (16|M0)              r42.0<1>:d    -1694144372:d                              
        mov (16|M0)              r40.0<1>:d    528734635:d                              
        mov (16|M0)              r36.0<1>:d    1541459225:d                              
(W)     mov (1|M0)               r4.0<1>:d     0:w                              
L9480:
(W)     shl (1|M0)               r2.2<1>:d     r4.0<0;1,0>:d     2:w              
(W)     mul (1|M0)               r3.0<1>:d     r4.0<0;1,0>:d     64:w              
(W)     or (1|M0)                r5.4<1>:d     r4.0<0;1,0>:d     1:w              
        shl (16|M0)              r18.0<1>:d    r44.0<8;8,1>:d    26:w              
        shr (16|M0)              r20.0<1>:ud   r44.0<8;8,1>:ud   6:w              
(W)     send (4|M0)              r8:d     r2      0xA            0x02184004           // wr:1h+0, rd:1; hdc.dc0; oword aligned block read x1
        add (16|M0)              r10.0<1>:d    r38.0<8;8,1>:d    r3.0<0;1,0>:d    {Compacted}
(W)     mul (1|M0)               r6.1<1>:d     r5.4<0;1,0>:d     64:w              
        shl (16|M0)              r22.0<1>:d    r44.0<8;8,1>:d    21:w              
        shr (16|M0)              r24.0<1>:ud   r44.0<8;8,1>:ud   11:w              
        send (16|M0)             r12:w    r10     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r14.0<1>:d    r38.0<8;8,1>:d    r6.1<0;1,0>:d    {Compacted}
        shl (16|M0)              r2.0<1>:d     r44.0<8;8,1>:d    7:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r44.0<8;8,1>:ud   25:w              
        send (16|M0)             r16:w    r14     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r40.0<8;8,1>:d    ~r44.0<8;8,1>:d  {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r10.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r44.0<8;8,1>:d    r42.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
(W)     or (1|M0)                r7.3<1>:d     r4.0<0;1,0>:d     2:w              
(W)     or (1|M0)                r6.1<1>:d     r4.0<0;1,0>:d     3:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
(W)     mul (1|M0)               r5.4<1>:d     r7.3<0;1,0>:d     64:w              
(W)     mul (1|M0)               r6.2<1>:d     r6.1<0;1,0>:d     64:w              
(W)     cmp (16|M0)   (lt)f1.0   null<1>:d     r6.1<0;1,0>:ud    0x3F:uw             
        add (16|M0)              r18.0<1>:d    r36.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r14.0<1>:ud   r50.0<8;8,1>:ud   2:w              
        add (16|M0)              r20.0<1>:d    r18.0<8;8,1>:d    r8.0<0;1,0>:d    {Compacted}
        shl (16|M0)              r18.0<1>:d    r50.0<8;8,1>:d    19:w              
        add (16|M0)              r24.0<1>:d    r20.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r12.0<1>:d    r50.0<8;8,1>:d    30:w              
        shr (16|M0)              r20.0<1>:ud   r50.0<8;8,1>:ud   13:w              
        add (16|M0)              r36.0<1>:d    r46.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r28.0<1>:d    r36.0<8;8,1>:d    26:w              
        shr (16|M0)              r22.0<1>:ud   r36.0<8;8,1>:ud   6:w              
        shl (16|M0)              r10.0<1>:d    r36.0<8;8,1>:d    21:w              
        shr (16|M0)              r2.0<1>:ud    r36.0<8;8,1>:ud   11:w              
        shl (16|M0)              r30.0<1>:d    r36.0<8;8,1>:d    7:w              
        shr (16|M0)              r26.0<1>:ud   r36.0<8;8,1>:ud   25:w              
        or (16|M0)               r28.0<1>:d    r28.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        and (16|M0)              r32.0<1>:d    r36.0<8;8,1>:d    r44.0<8;8,1>:d   {Compacted}
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r42.0<8;8,1>:d    ~r36.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r2.0<1>:d     r50.0<8;8,1>:d    10:w              
        xor (16|M0)              r26.0<1>:d    r48.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r10.0<1>:ud   r50.0<8;8,1>:ud   22:w              
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r30.0<1>:d    r48.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r40.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r50.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r26.0<1>:d    r26.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r28.0<1>:d    r28.0<8;8,1>:d    r8.1<0;1,0>:d    {Compacted}
        add (16|M0)              r12.0<1>:d    r12.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r50.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r14.0<1>:d    r28.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r46.0<1>:d    r24.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r24.0<1>:d    r50.0<8;8,1>:d    r48.0<8;8,1>:d   {Compacted}
        add (16|M0)              r40.0<1>:d    r34.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r18.0<1>:d    r46.0<8;8,1>:d    30:w              
        shr (16|M0)              r10.0<1>:ud   r46.0<8;8,1>:ud   2:w              
        shl (16|M0)              r32.0<1>:d    r40.0<8;8,1>:d    26:w              
        shr (16|M0)              r2.0<1>:ud    r40.0<8;8,1>:ud   6:w              
        shl (16|M0)              r20.0<1>:d    r46.0<8;8,1>:d    19:w              
        or (16|M0)               r18.0<1>:d    r18.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r22.0<1>:ud   r46.0<8;8,1>:ud   13:w              
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r2.0<1>:d     r38.0<8;8,1>:d    r5.4<0;1,0>:d    {Compacted}
        shl (16|M0)              r12.0<1>:d    r46.0<8;8,1>:d    10:w              
        shr (16|M0)              r16.0<1>:ud   r46.0<8;8,1>:ud   22:w              
        or (16|M0)               r20.0<1>:d    r20.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        send (16|M0)             r10:w    r2      0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        and (16|M0)              r24.0<1>:d    r46.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        or (16|M0)               r12.0<1>:d    r12.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r16.0<1>:d    r38.0<8;8,1>:d    r6.2<0;1,0>:d    {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r30.0<1>:d    r40.0<8;8,1>:d    21:w              
        shr (16|M0)              r34.0<1>:ud   r40.0<8;8,1>:ud   11:w              
        send (16|M0)             r20:w    r16     0xC            0x04205EFF           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r40.0<8;8,1>:d    7:w              
        or (16|M0)               r30.0<1>:d    r30.0<8;8,1>:d    r34.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r40.0<8;8,1>:ud   25:w              
        add (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r34.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r40.0<8;8,1>:d    r36.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r44.0<8;8,1>:d    ~r40.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r2.0<1>:d     r34.0<8;8,1>:d    30:w              
        shr (16|M0)              r16.0<1>:ud   r34.0<8;8,1>:ud   2:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r42.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r24.0<1>:d    r34.0<8;8,1>:d    19:w              
        shr (16|M0)              r26.0<1>:ud   r34.0<8;8,1>:ud   13:w              
        shr (16|M0)              r28.0<1>:ud   r34.0<8;8,1>:ud   22:w              
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r22.0<1>:d    r34.0<8;8,1>:d    10:w              
        xor (16|M0)              r18.0<1>:d    r46.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r8.2<0;1,0>:d    {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r46.0<8;8,1>:d    r50.0<8;8,1>:d   {Compacted}
        or (16|M0)               r22.0<1>:d    r22.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r18.0<1>:d    r34.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r18.0<1>:d    r18.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r22.0<8;8,1>:d   {Compacted}
        add (16|M0)              r2.0<1>:d     r2.0<8;8,1>:d     r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r12.0<1>:d    r32.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        add (16|M0)              r42.0<1>:d    r48.0<8;8,1>:d    r12.0<8;8,1>:d   {Compacted}
        add (16|M0)              r48.0<1>:d    r12.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        shl (16|M0)              r10.0<1>:d    r42.0<8;8,1>:d    26:w              
        shr (16|M0)              r16.0<1>:ud   r42.0<8;8,1>:ud   6:w              
        shl (16|M0)              r26.0<1>:d    r42.0<8;8,1>:d    21:w              
        shr (16|M0)              r30.0<1>:ud   r42.0<8;8,1>:ud   11:w              
        shl (16|M0)              r24.0<1>:d    r42.0<8;8,1>:d    7:w              
        shr (16|M0)              r28.0<1>:ud   r42.0<8;8,1>:ud   25:w              
        or (16|M0)               r10.0<1>:d    r10.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        or (16|M0)               r26.0<1>:d    r26.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        and (16|M0)              r14.0<1>:d    r42.0<8;8,1>:d    r40.0<8;8,1>:d   {Compacted}
        or (16|M0)               r24.0<1>:d    r24.0<8;8,1>:d    r28.0<8;8,1>:d   {Compacted}
        and (16|M0)              r22.0<1>:d    r36.0<8;8,1>:d    ~r42.0<8;8,1>:d  {Compacted}
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r32.0<1>:d    r48.0<8;8,1>:d    30:w              
        shr (16|M0)              r18.0<1>:ud   r48.0<8;8,1>:ud   2:w              
        shl (16|M0)              r16.0<1>:d    r48.0<8;8,1>:d    19:w              
        xor (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r30.0<1>:ud   r48.0<8;8,1>:ud   13:w              
        or (16|M0)               r14.0<1>:d    r14.0<8;8,1>:d    r22.0<8;8,1>:d   {Compacted}
        shl (16|M0)              r2.0<1>:d     r48.0<8;8,1>:d    10:w              
        add (16|M0)              r10.0<1>:d    r44.0<8;8,1>:d    r10.0<8;8,1>:d   {Compacted}
        shr (16|M0)              r12.0<1>:ud   r48.0<8;8,1>:ud   22:w              
        xor (16|M0)              r24.0<1>:d    r34.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r32.0<1>:d    r32.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        or (16|M0)               r16.0<1>:d    r16.0<8;8,1>:d    r30.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r14.0<8;8,1>:d   {Compacted}
        and (16|M0)              r26.0<1>:d    r34.0<8;8,1>:d    r46.0<8;8,1>:d   {Compacted}
        or (16|M0)               r2.0<1>:d     r2.0<8;8,1>:d     r12.0<8;8,1>:d   {Compacted}
        and (16|M0)              r24.0<1>:d    r48.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r16.0<8;8,1>:d   {Compacted}
        add (16|M0)              r10.0<1>:d    r10.0<8;8,1>:d    r8.3<0;1,0>:d    {Compacted}
        xor (16|M0)              r24.0<1>:d    r24.0<8;8,1>:d    r26.0<8;8,1>:d   {Compacted}
        xor (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r2.0<8;8,1>:d    {Compacted}
        add (16|M0)              r18.0<1>:d    r10.0<8;8,1>:d    r20.0<8;8,1>:d   {Compacted}
        add (16|M0)              r32.0<1>:d    r32.0<8;8,1>:d    r24.0<8;8,1>:d   {Compacted}
        add (16|M0)              r44.0<1>:d    r50.0<8;8,1>:d    r18.0<8;8,1>:d   {Compacted}
        add (16|M0)              r50.0<1>:d    r18.0<8;8,1>:d    r32.0<8;8,1>:d   {Compacted}
(~f1.0) break (16|M0)                        L11304                  L11304                
L11288:
(W)     add (1|M0)               r4.0<1>:d     r4.0<0;1,0>:d     4:w              
L11304:
        while (16|M0)                        L9480                                
L11320:
        shl (16|M0)              r2.0<1>:d     r82.0<8;8,1>:d    5:w              
        add (16|M0)              r12.0<1>:d    r50.0<8;8,1>:d    1779033703:d              
        add (16|M0)              r14.0<1>:d    r48.0<8;8,1>:d    -1150833019:d              
        add (16|M0)              r16.0<1>:d    r34.0<8;8,1>:d    1013904242:d              
        add (16|M0)              r18.0<1>:d    r46.0<8;8,1>:d    -1521486534:d              
        add (16|M0)              r20.0<1>:d    r44.0<8;8,1>:d    1359893119:d              
        add (16|M0)              r22.0<1>:d    r42.0<8;8,1>:d    -1694144372:d              
        add (16|M0)              r24.0<1>:d    r40.0<8;8,1>:d    528734635:d              
        add (16|M0)              r26.0<1>:d    r36.0<8;8,1>:d    1541459225:d              
        add (16|M0)              r8.0<1>:d     r2.0<8;8,1>:d     16:w              
        sends (16|M0)            null:w   r2      r12     0x20C            0x04025003           // wr:2+8, rd:0; hdc.dc1; untyped surface write with xyzw
(W)     mov (1|M0)               r10.0<1>:ud   0x1C:uw                             
        sends (16|M0)            null:w   r8      r20     0x20C            0x04025003           // wr:2+8, rd:0; hdc.dc1; untyped surface write with xyzw
(W)     send (1|M0)              r5       r10     0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r26.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L11608                  L12888                
L11576:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L12888                  L12888                
L11608:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r26.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L11672                  L12872                
L11640:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L12872                  L12872                
L11672:
(W)     mov (1|M0)               r2.0<1>:ud    0x18:uw                             
(W)     send (1|M0)              r5       r2      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f1.1   null<1>:d     r24.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.1)  if (16|M0)                           L11768                  L12856                
L11736:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L12856                  L12856                
L11768:
        cmp (16|M0)   (lt)f1.0   null<1>:d     r24.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.0)  if (16|M0)                           L11832                  L12840                
L11800:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L12840                  L12840                
L11832:
(W)     mov (1|M0)               r2.0<1>:ud    0x14:uw                             
(W)     send (1|M0)              r5       r2      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r22.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L11928                  L12824                
L11896:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L12824                  L12824                
L11928:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r22.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L11992                  L12808                
L11960:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L12808                  L12808                
L11992:
(W)     mov (1|M0)               r2.0<1>:ud    0x10:uw                             
(W)     send (1|M0)              r5       r2      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f1.1   null<1>:d     r20.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.1)  if (16|M0)                           L12088                  L12792                
L12056:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L12792                  L12792                
L12088:
        cmp (16|M0)   (lt)f1.0   null<1>:d     r20.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.0)  if (16|M0)                           L12152                  L12776                
L12120:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L12776                  L12776                
L12152:
(W)     mov (1|M0)               r2.0<1>:ud    0xC:uw                             
(W)     send (1|M0)              r5       r2      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r18.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L12248                  L12760                
L12216:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L12760                  L12760                
L12248:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r18.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L12312                  L12744                
L12280:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L12744                  L12744                
L12312:
(W)     mov (1|M0)               r2.0<1>:ud    0x8:uw                             
(W)     send (1|M0)              r5       r2      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f1.1   null<1>:d     r16.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.1)  if (16|M0)                           L12408                  L12728                
L12376:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L12728                  L12728                
L12408:
        cmp (16|M0)   (lt)f1.0   null<1>:d     r16.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f1.0)  if (16|M0)                           L12472                  L12712                
L12440:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L12712                  L12712                
L12472:
(W)     mov (1|M0)               r2.0<1>:ud    0x4:uw                             
(W)     send (1|M0)              r5       r2      0xA            0x02110801           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
        cmp (16|M0)   (gt)f0.1   null<1>:d     r14.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.1)  if (16|M0)                           L12568                  L12696                
L12536:
        mov (16|M0)              r6.0<1>:d     0:w                              
        else (16|M0)                         L12696                  L12696                
L12568:
        cmp (16|M0)   (lt)f0.0   null<1>:d     r14.0<8;8,1>:ud   r5.0<0;1,0>:ud  
(f0.0)  if (16|M0)                           L12632                  L12680                
L12600:
        mov (16|M0)              r6.0<1>:d     1:w                              
        else (16|M0)                         L12680                  L12680                
L12632:
(W)     mov (1|M0)               r2.0<1>:uq    r5.1<0;1,0>:uq                  
(W)     send (1|M0)              r3       r2:uq   0xC            0x041401FF           // wr:2+0, rd:1; hdc.dc1; a64 dword gathering read
        cmp (16|M0)   (le)f0.0   r8.0<1>:ud    r12.0<8;8,1>:ud   r3.0<0;1,0>:ud   {Compacted}
        mov (16|M0)              r6.0<1>:d     -r8.0<8;8,1>:d                   {Compacted}
L12680:
        endif (16|M0)                        L12696                                
L12696:
        endif (16|M0)                        L12712                                
L12712:
        endif (16|M0)                        L12728                                
L12728:
        endif (16|M0)                        L12744                                
L12744:
        endif (16|M0)                        L12760                                
L12760:
        endif (16|M0)                        L12776                                
L12776:
        endif (16|M0)                        L12792                                
L12792:
        endif (16|M0)                        L12808                                
L12808:
        endif (16|M0)                        L12824                                
L12824:
        endif (16|M0)                        L12840                                
L12840:
        endif (16|M0)                        L12856                                
L12856:
        endif (16|M0)                        L12872                                
L12872:
        endif (16|M0)                        L12888                                
L12888:
        endif (16|M0)                        L12904                                
L12904:
        shl (16|M0)              r2.0<1>:d     r82.0<8;8,1>:d    2:w              
(W)     mov (8|M0)               r127.0<1>:ud  r78.0<8;8,1>:ud                  {Compacted}
        sends (16|M0)            null:w   r2      r6      0x8C            0x04025E02           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     mov (1|M0)               null<1>:ud    f0.0<0;1,0>:ud                  
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L12976:
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
