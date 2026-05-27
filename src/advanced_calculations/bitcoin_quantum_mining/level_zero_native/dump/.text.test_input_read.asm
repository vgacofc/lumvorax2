L0:
(W)     mov (8|M0)               r2.0<1>:ud    r0.0<1;1,0>:ud                  
(W)     or (1|M0)                cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x4C0:uw              {Switch}
(W)     mov (1|M0)               r6.0<1>:uq    r3.0<0;1,0>:uq                  
(W)     mov (1|M0)               r8.0<1>:uq    r3.1<0;1,0>:uq                  
(W)     mov (1|M0)               r5.0<1>:ud    0x4:uw                             
(W)     mov (1|M0)               r9.0<1>:ud    0x8:uw                             
(W)     mov (1|M0)               r11.0<1>:ud   0xC:uw                             
(W)     mov (8|M0)               r127.0<1>:ud  r2.0<8;8,1>:ud                   {Compacted}
(W)     send (1|M0)              r4       r6:uq   0xC            0x041401FF           // wr:2+0, rd:1; hdc.dc1; a64 dword gathering read
(W)     sends (1|M0)             null:ud  r8      r4      0x4C            0x040681FF           // wr:2+1, rd:0; hdc.dc1; a64 dword scattering write
(W)     send (1|M0)              r7       r5      0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     sends (1|M0)             null:ud  r5      r7      0x4A            0x02030801           // wr:1+1, rd:0; hdc.dc0; byte scattering write 32b
(W)     send (1|M0)              r10      r9      0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     sends (1|M0)             null:ud  r9      r10     0x4A            0x02030801           // wr:1+1, rd:0; hdc.dc0; byte scattering write 32b
(W)     send (1|M0)              r12      r11     0xA            0x02110800           // wr:1+0, rd:1; hdc.dc0; byte gathering read 32b
(W)     sends (1|M0)             null:ud  r11     r12     0x4A            0x02030801           // wr:1+1, rd:0; hdc.dc0; byte scattering write 32b
(W)     send (1|M0)              null     r127    0x27            0x02000010           {EOT} // wr:1+0, rd:0; spawner; end of thread
L264:
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
