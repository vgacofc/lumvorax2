(W)     mov (8|M0)               r3.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mul (1|M0)               r4.2<1>:d     r8.2<0;1,0>:d     r3.1<0;1,0>:d   
(W)     mov (1|M0)               r4.0<1>:uq    r8.0<0;1,0>:uq                  
(W)     mov (1|M0)               r6.0<1>:d     -1014846515:d                              
        add (16|M0)              r9.0<1>:d     r4.2<0;1,0>:d     r1.0<16;16,1>:uw
        add (16|M16)             r11.0<1>:d    r4.2<0;1,0>:d     r2.0<16;16,1>:uw
        add (16|M0)              r13.0<1>:d    r9.0<8;8,1>:d     r6.0<0;1,0>:d
        shl (16|M0)              r9.0<1>:d     r9.0<8;8,1>:d     2:w
        sends (16|M0)            null:w   r9      r13     0x8C            0x04025E00
        add (16|M0)              r15.0<1>:d    r11.0<8;8,1>:d    r7.0<0;1,0>:d    {Compacted}
        add (16|M16)             r17.0<1>:d    r11.0<8;8,1>:d    r7.0<0;1,0>:d   
        cmp (16|M0)   (lt)f0.0   null<1>:d     r15.0<8;8,1>:ud   0x40:uw             
        cmp (16|M16)  (lt)f0.0   null<1>:d     r17.0<8;8,1>:ud   0x40:uw             
(f0.0)  if (32|M0)                           L296                  L296                
L200:
        or (16|M0)               r19.0<1>:d    r15.0<8;8,1>:d    -1014846720:d              
        shl (16|M0)              r21.0<1>:d    r15.0<8;8,1>:d    2:w              
        or (16|M16)              r23.0<1>:d    r17.0<8;8,1>:d    -1014846720:d              
        shl (16|M16)             r25.0<1>:d    r17.0<8;8,1>:d    2:w              
        sends (16|M0)            null:w   r21     r19     0x8C            0x04025E00
        sends (16|M16)           null:w   r25     r23     0x8C            0x04025E00
L296:
        endif (32|M0)                        L312                                
L312:
(W)     mov (8|M0)               r127.0<1>:ud  r3.0<8;8,1>:ud                   {Compacted}
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT}
