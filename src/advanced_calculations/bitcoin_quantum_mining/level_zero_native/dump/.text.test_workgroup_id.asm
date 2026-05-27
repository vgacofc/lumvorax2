L0:
(W)     mov (8|M0)               r3.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mul (1|M0)               r4.0<1>:d     r8.2<0;1,0>:d     r3.1<0;1,0>:d   
        mov (16|M0)              r5.0<1>:d     r1.0<16;16,1>:uw                
        mov (16|M16)             r9.0<1>:d     r2.0<16;16,1>:uw                
        add (16|M0)              r11.0<1>:d    r4.0<0;1,0>:d     r5.0<8;8,1>:d    {Compacted}
        add (16|M16)             r13.0<1>:d    r4.0<0;1,0>:d     r9.0<8;8,1>:d   
        add (16|M0)              r15.0<1>:d    r11.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        add (16|M16)             r17.0<1>:d    r13.0<8;8,1>:d    r7.0<0;1,0>:d   
        cmp (16|M0)   (lt)f0.0   null<1>:d     r15.0<8;8,1>:ud   0x100:uw             
        cmp (16|M16)  (lt)f0.0   null<1>:d     r17.0<8;8,1>:ud   0x100:uw             
(f0.0)  if (32|M0)                           L312                  L312                
L176:
        mul (16|M0)              r19.0<1>:d    r15.0<8;8,1>:d    12:w              
        mov (16|M0)              r23.0<1>:d    r15.0<8;8,1>:d                   {Compacted}
        mov (16|M0)              r25.0<1>:d    r5.0<8;8,1>:d                    {Compacted}
        mov (16|M0)              r27.0<1>:d    r3.1<0;1,0>:d                    {Compacted}
        mul (16|M16)             r21.0<1>:d    r17.0<8;8,1>:d    12:w              
        mov (16|M16)             r29.0<1>:d    r17.0<8;8,1>:d                  
        mov (16|M16)             r31.0<1>:d    r9.0<8;8,1>:d                   
        mov (16|M16)             r33.0<1>:d    r3.1<0;1,0>:d                   
        sends (16|M0)            null:w   r19     r23     0x18C            0x04025800           // wr:2+6, rd:0; hdc.dc1; untyped surface write with xyz
        sends (16|M16)           null:w   r21     r29     0x18C            0x04025800           // wr:2+6, rd:0; hdc.dc1; untyped surface write with xyz
L312:
        endif (32|M0)                        L328                                
L328:
(W)     mov (8|M0)               r127.0<1>:ud  r3.0<8;8,1>:ud                   {Compacted}
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L352:
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
