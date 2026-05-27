L0:
(W)     mov (8|M0)               r2.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mov (1|M0)               r4.0<1>:uq    0xABCDEF0012345678:uq                             
(W)     mov (8|M0)               r3.8<1>:w     0xC840:uv                             
(W)     mov (1|M0)               r4.1<1>:uq    0xCAFEBABEDEADBEEF:uq                             
(W)     mov (8|M0)               r127.0<1>:ud  r2.0<8;8,1>:ud                   {Compacted}
(W)     mov (8|M0)               r8.0<1>:d     r4.0<0;1,0>:d                    {Compacted}
(W)     add (8|M0)               r6.0<1>:uq    r3.0<0;1,0>:uq    r3.8<8;8,1>:w   
(W)     mov (4|M0)               r8.0<1>:d     r4.0<4;4,1>:d                    {Compacted}
(W)     sends (8|M0)             null:ud  r6      r8      0x4C            0x040681FF           // wr:2+1, rd:0; hdc.dc1; a64 dword scattering write
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L152:
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
