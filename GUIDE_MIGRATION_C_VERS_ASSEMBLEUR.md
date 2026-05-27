# GUIDE PRATIQUE — MIGRATION C → ASSEMBLEUR
## Plan Concret pour LumVorax

**Date** : 2026-05-07  
**Objectif** : Migration progressive C → Assembleur x86-64  
**Philosophie** : Contrôle total du hardware, zéro abstraction

---

## 🎯 STRATÉGIE DE MIGRATION

### Principe : Migration Progressive par Modules

```
Phase 1 : Modules Critiques (20% code, 80% temps CPU)
Phase 2 : Modules Intermédiaires (30% code, 15% temps CPU)
Phase 3 : Modules Restants (50% code, 5% temps CPU)
```

### Architecture Cible

```
LumVorax Assembleur
├── Noyau ASM (100% assembleur)
│   ├── lum_core.asm          → Structures LUM
│   ├── vorax_ops.asm         → Opérations VORAX
│   ├── sha256_native.asm     → SHA-256 pur
│   └── memory_mgr.asm        → Gestion mémoire
│
├── Couche Système (100% assembleur)
│   ├── syscalls.asm          → Appels système directs
│   ├── threading.asm         → Threads natifs
│   └── io_direct.asm         → I/O sans libc
│
└── Interface C (wrapper minimal)
    └── lumvorax_api.c        → API publique
```

---

## 📋 PHASE 1 : MODULES CRITIQUES (Semaines 1-8)

### Module 1.1 : Structures LUM en Assembleur

**Fichier** : `src/asm/lum_core.asm`

```nasm
; ============================================================================
; LUM CORE — Structures et Opérations de Base
; ============================================================================

section .data
    ; Magic numbers
    LUM_MAGIC_NUMBER    equ 0x4C554D00
    LUM_DESTROYED       equ 0xDEADBEEF
    
    ; Constantes
    LUM_SIZE            equ 64          ; Aligné cache line
    LUM_POOL_SIZE       equ 1048576     ; 1M LUMs

section .bss
    ; Pool global de LUMs
    alignb 64
    g_lum_pool:         resb LUM_SIZE * LUM_POOL_SIZE
    g_lum_bitmap:       resb LUM_POOL_SIZE / 8
    g_next_id:          resd 1

section .text
    global lum_create_asm
    global lum_destroy_asm
    global lum_validate_asm

; ----------------------------------------------------------------------------
; lum_t* lum_create_asm(uint8_t presence, int32_t x, int32_t y, uint8_t type)
; ----------------------------------------------------------------------------
; Arguments (System V AMD64 ABI):
;   RDI = presence (uint8_t)
;   RSI = x (int32_t)
;   RDX = y (int32_t)
;   RCX = type (uint8_t)
; Retour:
;   RAX = pointeur vers lum_t ou NULL
; ----------------------------------------------------------------------------
lum_create_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Sauvegarder arguments
    mov r12b, dil           ; presence
    mov r13d, esi           ; x
    mov r14d, edx           ; y
    mov r15b, cl            ; type
    
    ; Trouver slot libre dans pool
    xor rbx, rbx            ; index = 0
.find_slot:
    cmp rbx, LUM_POOL_SIZE
    jge .pool_full
    
    ; Vérifier bitmap
    mov rax, rbx
    shr rax, 3              ; byte_index = index / 8
    lea rcx, [rel g_lum_bitmap]
    movzx edx, byte [rcx + rax]
    
    mov cl, bl
    and cl, 7               ; bit_offset = index % 8
    bt edx, ecx             ; Test bit
    jc .slot_used
    
    ; Slot libre trouvé, marquer comme utilisé
    bts edx, ecx
    mov byte [rcx + rax], dl
    jmp .allocate
    
.slot_used:
    inc rbx
    jmp .find_slot
    
.pool_full:
    xor eax, eax            ; Retourner NULL
    jmp .done
    
.allocate:
    ; Calculer adresse LUM
    lea rax, [rel g_lum_pool]
    mov rcx, LUM_SIZE
    imul rcx, rbx
    add rax, rcx            ; RAX = adresse LUM
    
    ; Générer ID unique (atomic increment)
    lea rcx, [rel g_next_id]
    mov edx, 1
    lock xadd [rcx], edx    ; Atomic fetch-and-add
    
    ; Initialiser structure LUM (64 bytes)
    ; Offset 0: id (uint32_t)
    mov [rax + 0], edx
    
    ; Offset 4: presence (uint8_t)
    mov [rax + 4], r12b
    
    ; Offset 5: structure_type (uint8_t)
    mov [rax + 5], r15b
    
    ; Offset 6: is_destroyed (uint8_t)
    mov byte [rax + 6], 0
    
    ; Offset 7: reserved_flags (uint8_t)
    mov byte [rax + 7], 0
    
    ; Offset 8: position_x (int32_t)
    mov [rax + 8], r13d
    
    ; Offset 12: position_y (int32_t)
    mov [rax + 12], r14d
    
    ; Offset 16: timestamp (uint64_t) - RDTSC
    rdtsc
    shl rdx, 32
    or rax, rdx
    mov [rax + 16], rax
    
    ; Offset 24: memory_address (void*)
    mov [rax + 24], rax
    
    ; Offset 32: checksum (uint32_t) - calculer CRC32C
    ; TODO: Implémenter CRC32C
    mov dword [rax + 32], 0
    
    ; Offset 36: magic_number (uint32_t)
    mov dword [rax + 36], LUM_MAGIC_NUMBER
    
    ; Offset 40-63: padding (24 bytes)
    xor ecx, ecx
    mov [rax + 40], rcx
    mov [rax + 48], rcx
    mov [rax + 56], rcx
    
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; ----------------------------------------------------------------------------
; void lum_destroy_asm(lum_t* lum)
; ----------------------------------------------------------------------------
lum_destroy_asm:
    push rbp
    mov rbp, rsp
    
    ; Vérifier pointeur NULL
    test rdi, rdi
    jz .done
    
    ; Vérifier magic number
    cmp dword [rdi + 36], LUM_MAGIC_NUMBER
    jne .done
    
    ; Marquer comme détruit
    mov dword [rdi + 36], LUM_DESTROYED
    mov byte [rdi + 6], 1
    
    ; Calculer index dans pool
    lea rax, [rel g_lum_pool]
    sub rdi, rax
    mov rax, LUM_SIZE
    xor edx, edx
    div rax                 ; RAX = index
    
    ; Libérer dans bitmap
    mov rcx, rax
    shr rcx, 3              ; byte_index
    lea rbx, [rel g_lum_bitmap]
    movzx edx, byte [rbx + rcx]
    
    mov cl, al
    and cl, 7               ; bit_offset
    btr edx, ecx            ; Clear bit
    mov byte [rbx + rcx], dl
    
.done:
    pop rbp
    ret

; ----------------------------------------------------------------------------
; bool lum_validate_asm(lum_t* lum)
; ----------------------------------------------------------------------------
lum_validate_asm:
    xor eax, eax            ; Retour false par défaut
    
    test rdi, rdi
    jz .done
    
    cmp dword [rdi + 36], LUM_MAGIC_NUMBER
    jne .done
    
    cmp byte [rdi + 6], 0
    jne .done
    
    mov eax, 1              ; Retour true
    
.done:
    ret
```

**Compilation** :
```bash
nasm -f elf64 -o lum_core.o src/asm/lum_core.asm
```

### Module 1.2 : SHA-256 Pur Assembleur

**Fichier** : `src/asm/sha256_native.asm`

```nasm
; ============================================================================
; SHA-256 — Implémentation Pure Assembleur x86-64
; ============================================================================

section .rodata
    align 64
    K256:
        dd 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5
        dd 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5
        dd 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3
        dd 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174
        dd 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc
        dd 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da
        dd 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7
        dd 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967
        dd 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13
        dd 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85
        dd 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3
        dd 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070
        dd 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5
        dd 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3
        dd 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208
        dd 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2

    H0:
        dd 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a
        dd 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19

section .text
    global sha256_block_asm
    global sha256_init_asm
    global sha256_final_asm

; Macros pour rotations
%macro ROTR 2
    ror %1, %2
%endmacro

%macro SHR 2
    shr %1, %2
%endmacro

; ----------------------------------------------------------------------------
; void sha256_block_asm(uint32_t state[8], const uint8_t block[64])
; ----------------------------------------------------------------------------
sha256_block_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 256            ; W[64] sur stack
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Charger état initial
    mov r8d, [rdi + 0]      ; a
    mov r9d, [rdi + 4]      ; b
    mov r10d, [rdi + 8]     ; c
    mov r11d, [rdi + 12]    ; d
    mov r12d, [rdi + 16]    ; e
    mov r13d, [rdi + 20]    ; f
    mov r14d, [rdi + 24]    ; g
    mov r15d, [rdi + 28]    ; h
    
    ; Préparer W[0..15] (big-endian)
    xor ecx, ecx
.prepare_w:
    cmp ecx, 16
    jge .expand_w
    
    mov eax, [rsi + rcx*4]
    bswap eax               ; Convertir big-endian
    mov [rbp - 256 + rcx*4], eax
    
    inc ecx
    jmp .prepare_w
    
.expand_w:
    ; Calculer W[16..63]
    mov ecx, 16
.expand_loop:
    cmp ecx, 64
    jge .rounds
    
    ; W[i] = σ1(W[i-2]) + W[i-7] + σ0(W[i-15]) + W[i-16]
    
    ; σ1(W[i-2])
    mov eax, [rbp - 256 + (rcx-2)*4]
    mov ebx, eax
    ROTR eax, 17
    mov edx, ebx
    ROTR edx, 19
    xor eax, edx
    SHR ebx, 10
    xor eax, ebx            ; σ1
    
    ; + W[i-7]
    add eax, [rbp - 256 + (rcx-7)*4]
    
    ; σ0(W[i-15])
    mov ebx, [rbp - 256 + (rcx-15)*4]
    mov edx, ebx
    ROTR ebx, 7
    mov edi, edx
    ROTR edi, 18
    xor ebx, edi
    SHR edx, 3
    xor ebx, edx            ; σ0
    
    add eax, ebx
    
    ; + W[i-16]
    add eax, [rbp - 256 + (rcx-16)*4]
    
    mov [rbp - 256 + rcx*4], eax
    
    inc ecx
    jmp .expand_loop
    
.rounds:
    ; 64 rounds
    xor ecx, ecx
.round_loop:
    cmp ecx, 64
    jge .finalize
    
    ; T1 = h + Σ1(e) + Ch(e,f,g) + K[i] + W[i]
    
    ; Σ1(e)
    mov eax, r12d
    mov ebx, eax
    ROTR eax, 6
    mov edx, ebx
    ROTR edx, 11
    xor eax, edx
    ROTR ebx, 25
    xor eax, ebx            ; Σ1(e)
    
    ; Ch(e,f,g) = (e & f) ^ (~e & g)
    mov ebx, r12d
    and ebx, r13d
    mov edx, r12d
    not edx
    and edx, r14d
    xor ebx, edx            ; Ch
    
    ; T1 = h + Σ1 + Ch + K + W
    add eax, r15d
    add eax, ebx
    lea rbx, [rel K256]
    add eax, [rbx + rcx*4]
    add eax, [rbp - 256 + rcx*4]
    mov edi, eax            ; T1
    
    ; T2 = Σ0(a) + Maj(a,b,c)
    
    ; Σ0(a)
    mov eax, r8d
    mov ebx, eax
    ROTR eax, 2
    mov edx, ebx
    ROTR edx, 13
    xor eax, edx
    ROTR ebx, 22
    xor eax, ebx            ; Σ0(a)
    
    ; Maj(a,b,c) = (a & b) ^ (a & c) ^ (b & c)
    mov ebx, r8d
    and ebx, r9d
    mov edx, r8d
    and edx, r10d
    xor ebx, edx
    mov edx, r9d
    and edx, r10d
    xor ebx, edx            ; Maj
    
    add eax, ebx            ; T2
    
    ; Mise à jour état
    mov r15d, r14d          ; h = g
    mov r14d, r13d          ; g = f
    mov r13d, r12d          ; f = e
    lea r12d, [r11d + edi]  ; e = d + T1
    mov r11d, r10d          ; d = c
    mov r10d, r9d           ; c = b
    mov r9d, r8d            ; b = a
    lea r8d, [edi + eax]    ; a = T1 + T2
    
    inc ecx
    jmp .round_loop
    
.finalize:
    ; Additionner à l'état initial
    add [rdi + 0], r8d
    add [rdi + 4], r9d
    add [rdi + 8], r10d
    add [rdi + 12], r11d
    add [rdi + 16], r12d
    add [rdi + 20], r13d
    add [rdi + 24], r14d
    add [rdi + 28], r15d
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    add rsp, 256
    pop rbp
    ret
```

### Module 1.3 : Syscalls Directs

**Fichier** : `src/asm/syscalls.asm`

```nasm
; ============================================================================
; SYSCALLS — Appels Système Directs (Linux x86-64)
; ============================================================================

section .text
    global sys_write_asm
    global sys_read_asm
    global sys_open_asm
    global sys_close_asm
    global sys_mmap_asm
    global sys_munmap_asm

; ----------------------------------------------------------------------------
; ssize_t sys_write_asm(int fd, const void* buf, size_t count)
; ----------------------------------------------------------------------------
sys_write_asm:
    mov rax, 1              ; syscall write
    ; RDI = fd (déjà en place)
    ; RSI = buf (déjà en place)
    ; RDX = count (déjà en place)
    syscall
    ret

; ----------------------------------------------------------------------------
; ssize_t sys_read_asm(int fd, void* buf, size_t count)
; ----------------------------------------------------------------------------
sys_read_asm:
    mov rax, 0              ; syscall read
    syscall
    ret

; ----------------------------------------------------------------------------
; int sys_open_asm(const char* pathname, int flags, mode_t mode)
; ----------------------------------------------------------------------------
sys_open_asm:
    mov rax, 2              ; syscall open
    syscall
    ret

; ----------------------------------------------------------------------------
; int sys_close_asm(int fd)
; ----------------------------------------------------------------------------
sys_close_asm:
    mov rax, 3              ; syscall close
    syscall
    ret

; ----------------------------------------------------------------------------
; void* sys_mmap_asm(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
; ----------------------------------------------------------------------------
sys_mmap_asm:
    mov rax, 9              ; syscall mmap
    mov r10, rcx            ; 4ème arg dans R10
    mov r8, r8              ; 5ème arg dans R8
    mov r9, r9              ; 6ème arg dans R9
    syscall
    ret

; ----------------------------------------------------------------------------
; int sys_munmap_asm(void* addr, size_t length)
; ----------------------------------------------------------------------------
sys_munmap_asm:
    mov rax, 11             ; syscall munmap
    syscall
    ret
```

---

## 📋 PHASE 2 : INTÉGRATION (Semaines 9-12)

### Wrapper C pour Compatibilité

**Fichier** : `src/lumvorax_asm_wrapper.c`

```c
#include "lum_core.h"
#include <stdint.h>
#include <stdbool.h>

// Déclarations externes (assembleur)
extern lum_t* lum_create_asm(uint8_t presence, int32_t x, int32_t y, uint8_t type);
extern void lum_destroy_asm(lum_t* lum);
extern bool lum_validate_asm(lum_t* lum);
extern void sha256_block_asm(uint32_t state[8], const uint8_t block[64]);
extern ssize_t sys_write_asm(int fd, const void* buf, size_t count);

// Wrappers C (API publique)
lum_t* lum_create(uint8_t presence, int32_t x, int32_t y, lum_structure_type_e type) {
    return lum_create_asm(presence, x, y, (uint8_t)type);
}

void lum_destroy(lum_t* lum) {
    lum_destroy_asm(lum);
}

bool lum_validate(lum_t* lum) {
    return lum_validate_asm(lum);
}

// SHA-256 wrapper
void sha256_process_block(uint32_t state[8], const uint8_t block[64]) {
    sha256_block_asm(state, block);
}

// Syscall wrapper
ssize_t write_direct(int fd, const void* buf, size_t count) {
    return sys_write_asm(fd, buf, count);
}
```

### Makefile Assembleur

**Fichier** : `Makefile.asm`

```makefile
# Makefile pour LumVorax Assembleur

AS = nasm
ASFLAGS = -f elf64 -g -F dwarf
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O3 -g
LDFLAGS = -no-pie

ASM_SRC = src/asm/lum_core.asm \
          src/asm/sha256_native.asm \
          src/asm/syscalls.asm \
          src/asm/vorax_ops.asm

ASM_OBJ = $(ASM_SRC:.asm=.o)

C_SRC = src/lumvorax_asm_wrapper.c \
        src/main.c

C_OBJ = $(C_SRC:.c=.o)

TARGET = bin/lumvorax_asm

all: $(TARGET)

$(TARGET): $(ASM_OBJ) $(C_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(ASM_OBJ) $(C_OBJ) $(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
```

---

## 🔧 OUTILS ET DEBUGGING

### GDB pour Assembleur

```bash
# Compiler avec symboles debug
nasm -f elf64 -g -F dwarf lum_core.asm

# Debugger
gdb ./bin/lumvorax_asm

# Commandes GDB utiles
(gdb) break lum_create_asm
(gdb) run
(gdb) info registers
(gdb) x/16xb $rax          # Examiner mémoire
(gdb) disassemble
(gdb) stepi                # Step instruction
```

### Profiling Assembleur

```bash
# Profiling avec perf
perf record -g ./bin/lumvorax_asm
perf report

# Analyse cache
perf stat -e cache-misses,cache-references ./bin/lumvorax_asm

# Compteurs hardware
perf stat -e cycles,instructions,branches,branch-misses ./bin/lumvorax_asm
```

---

## 📊 PLAN DE MIGRATION COMPLET

### Semaines 1-2 : Infrastructure
- [ ] Setup environnement (NASM, GDB, perf)
- [ ] Créer structure projet assembleur
- [ ] Tests unitaires assembleur

### Semaines 3-4 : Structures LUM
- [ ] `lum_core.asm` complet
- [ ] Pool allocator assembleur
- [ ] Tests validation

### Semaines 5-6 : Cryptographie
- [ ] SHA-256 pur assembleur
- [ ] CRC32C assembleur
- [ ] Benchmarks vs C

### Semaines 7-8 : Syscalls
- [ ] Tous syscalls Linux
- [ ] I/O direct
- [ ] Tests performance

### Semaines 9-10 : VORAX
- [ ] Opérations VORAX assembleur
- [ ] Vectorisation AVX-512
- [ ] Tests intégration

### Semaines 11-12 : Intégration
- [ ] Wrappers C
- [ ] Tests complets
- [ ] Documentation

---

## ⚠️ POINTS CRITIQUES

### 1. Portabilité = 0

Votre code sera **UNIQUEMENT x86-64 Linux**. Pas de :
- Windows
- macOS
- ARM
- RISC-V

### 2. Maintenance Extrême

1 ligne ASM = 10 lignes commentaires minimum

### 3. Debugging Difficile

Préparez-vous à passer 10× plus de temps en debug

### 4. Tests Massifs

Chaque fonction ASM doit avoir 100+ tests unitaires

---

## 🎯 RÉSULTAT FINAL

Après 12 semaines, vous aurez :

✅ Noyau LumVorax 100% assembleur  
✅ Zéro dépendance libc (sauf wrapper)  
✅ Contrôle total hardware  
✅ Performance maximale  

**Mais** :
❌ Portabilité perdue  
❌ Maintenance 10× plus difficile  
❌ Debugging cauchemar  

**Gain réel estimé** : +5-15% vs C optimisé

---

**Prêt à commencer ?**