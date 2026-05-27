L0:
(W)     mov (8|M0)               r3.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mul (1|M0)               r4.0<1>:d     r9.0<0;1,0>:d     r3.1<0;1,0>:d    {Compacted}
        add (16|M0)              r5.0<1>:d     r4.0<0;1,0>:d     r1.0<16;16,1>:uw
        add (16|M16)             r10.0<1>:d    r4.0<0;1,0>:d     r2.0<16;16,1>:uw
        add (16|M0)              r12.0<1>:d    r5.0<8;8,1>:d     r7.0<0;1,0>:d    {Compacted}
        add (16|M16)             r14.0<1>:d    r10.0<8;8,1>:d    r7.0<0;1,0>:d   
        cmp (16|M0)   (lt)f0.0   null<1>:d     r12.0<8;8,1>:ud   r8.6<0;1,0>:ud  
        cmp (16|M16)  (lt)f0.0   null<1>:d     r14.0<8;8,1>:ud   r8.6<0;1,0>:ud  
(f0.0)  if (32|M0)                           L296                  L296                
L144:
        shl (16|M0)              r16.0<1>:d    r12.0<8;8,1>:d    2:w              
        shl (16|M16)             r18.0<1>:d    r14.0<8;8,1>:d    2:w              
        send (16|M0)             r20:w    r16     0xC            0x04205E00           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M0)             r24:w    r16     0xC            0x04205E01           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M16)            r22:w    r18     0xC            0x04205E00           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        send (16|M16)            r26:w    r18     0xC            0x04205E01           // wr:2+0, rd:2; hdc.dc1; untyped surface read with x
        add (16|M0)              r20.0<1>:f    r20.0<8;8,1>:f    r24.0<8;8,1>:f   {Compacted}
        add (16|M16)             r22.0<1>:f    r22.0<8;8,1>:f    r26.0<8;8,1>:f  
        sends (16|M0)            null:w   r16     r20     0x8C            0x04025E02           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M16)           null:w   r18     r22     0x8C            0x04025E02           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
L296:
        endif (32|M0)                        L312                                
L312:
(W)     mov (8|M0)               r127.0<1>:ud  r3.0<8;8,1>:ud                   {Compacted}
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L336:
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
        illegal                
