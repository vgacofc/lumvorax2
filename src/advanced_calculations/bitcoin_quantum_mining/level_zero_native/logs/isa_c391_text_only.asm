byte offset 0xf8: warning: unexpected padding at end of kernel
L0:
(W)     mov (8|M0)               r2.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mov (1|M0)               r4.0<1>:uq    r3.0<0;1,0>:uq                  
(W)     mov (1|M0)               r6.0<1>:d     -559038737:d                              
(W)     mov (8|M0)               r127.0<1>:ud  r2.0<8;8,1>:ud                   {Compacted}
(W)     sends (1|M0)             null:ud  r4      r6      0x4C            0x040681FF           // wr:2+1, rd:0; hdc.dc1; a64 dword scattering write
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L104:
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
        illegal                
