L0:
(W)     mov (8|M0)               r3.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mul (1|M0)               r4.0<1>:d     r8.2<0;1,0>:d     r3.1<0;1,0>:d   
        add (16|M0)              r5.0<1>:d     r4.0<0;1,0>:d     r1.0<16;16,1>:uw
        add (16|M16)             r9.0<1>:d     r4.0<0;1,0>:d     r2.0<16;16,1>:uw
        add (16|M0)              r11.0<1>:d    r5.0<8;8,1>:d     r7.0<0;1,0>:d    {Compacted}
        add (16|M16)             r13.0<1>:d    r9.0<8;8,1>:d     r7.0<0;1,0>:d   
        cmp (16|M0)   (lt)f0.0   null<1>:d     r11.0<8;8,1>:ud   0xA:uw             
        cmp (16|M16)  (lt)f0.0   null<1>:d     r13.0<8;8,1>:ud   0xA:uw             
(f0.0)  if (32|M0)                           L248                  L248                
L152:
        add (16|M0)              r15.0<1>:d    r11.0<8;8,1>:d    0xC372:uw             
        shl (16|M0)              r19.0<1>:d    r11.0<8;8,1>:d    2:w              
        add (16|M16)             r17.0<1>:d    r13.0<8;8,1>:d    0xC372:uw             
        shl (16|M16)             r21.0<1>:d    r13.0<8;8,1>:d    2:w              
        sends (16|M0)            null:w   r19     r15     0x8C            0x04025E00           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M16)           null:w   r21     r17     0x8C            0x04025E00           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
L248:
        endif (32|M0)                        L264                                
L264:
(W)     mov (8|M0)               r127.0<1>:ud  r3.0<8;8,1>:ud                   {Compacted}
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L288:
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
