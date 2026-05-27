L0:
(W)     mov (8|M0)               r3.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mul (1|M0)               r4.0<1>:d     r8.4<0;1,0>:d     r3.1<0;1,0>:d   
(W)     mov (8|M0)               r127.0<1>:ud  r3.0<8;8,1>:ud                   {Compacted}
        add (16|M0)              r5.0<1>:d     r4.0<0;1,0>:d     r1.0<16;16,1>:uw
        add (16|M16)             r9.0<1>:d     r4.0<0;1,0>:d     r2.0<16;16,1>:uw
        add (16|M0)              r5.0<1>:d     r5.0<8;8,1>:d     r7.0<0;1,0>:d    {Compacted}
        add (16|M16)             r9.0<1>:d     r9.0<8;8,1>:d     r7.0<0;1,0>:d   
        shl (16|M0)              r11.0<1>:d    r5.0<8;8,1>:d     2:w              
        shl (16|M16)             r13.0<1>:d    r9.0<8;8,1>:d     2:w              
        sends (16|M0)            null:w   r11     r5      0x8C            0x04025E00           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
        sends (16|M16)           null:w   r13     r9      0x8C            0x04025E00           // wr:2+2, rd:0; hdc.dc1; untyped surface write with x
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L192:
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
