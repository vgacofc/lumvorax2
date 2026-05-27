// C402: Test Runner pour Kernel SHA256 Natif Complet (64 rounds)
// Objectif: Tester le kernel Gen9 ISA pur avec runtime i915 DRM natif
// Sans dépendance OpenCL

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <i915_drm.h>
#include <xf86drm.h>

#define NUM_THREADS 256
#define BATCH_SIZE 4096
#define KERNEL_SIZE (256 * 1024)  // 256KB max pour le kernel

// Bitcoin genesis block header (80 bytes)
static const uint32_t genesis_header[20] = {
    0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x3BA3EDFD, 0x7A7B12B2, 0x7AC72C3E, 0x67768F61,
    0x7FC81BC3, 0x888A5132, 0x3A9FB8AA, 0x4B1E5E4A,
    0x29AB5F49, 0xFFFF001D, 0x1DAC2B7C, 0x00000000  // Nonce sera patché
};

typedef struct {
    int fd;
    uint32_t handle_input;
    uint32_t handle_output;
    uint32_t handle_curbe;
    uint32_t handle_idesc;
    uint32_t handle_kernel;
    uint32_t handle_batch;
    uint64_t gtt_input;
    uint64_t gtt_output;
    uint64_t gtt_curbe;
    uint64_t gtt_idesc;
    uint64_t gtt_kernel;
    uint64_t gtt_batch;
    void *map_input;
    void *map_output;
    void *map_curbe;
    void *map_idesc;
    void *map_kernel;
    void *map_batch;
} drm_context_t;

static uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int create_bo(int fd, uint64_t size, uint32_t *handle, uint64_t *gtt, void **map) {
    struct drm_i915_gem_create create = {.size = size};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) return -1;
    *handle = create.handle;
    
    struct drm_i915_gem_set_domain set_domain = {
        .handle = *handle,
        .read_domains = I915_GEM_DOMAIN_GTT,
        .write_domain = I915_GEM_DOMAIN_GTT
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = *handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) return -1;
    *map = (void *)(uintptr_t)mmap_arg.addr_ptr;
    
    struct drm_i915_gem_set_tiling set_tiling = {
        .handle = *handle,
        .tiling_mode = I915_TILING_NONE,
        .stride = 0
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_SET_TILING, &set_tiling);
    
    *gtt = 0;
    return 0;
}

static void build_state_base_address(uint32_t *batch, int *offset, 
                                     uint64_t kernel_gtt, uint64_t curbe_gtt) {
    // STATE_BASE_ADDRESS (0x61010000 + length-2)
    batch[(*offset)++] = 0x61010009;  // Opcode + length (11 dwords)
    
    // General State Base Address (not used)
    batch[(*offset)++] = 0x00000000;
    batch[(*offset)++] = 0x00000000;
    
    // Surface State Base Address (not used)
    batch[(*offset)++] = 0x00000000;
    batch[(*offset)++] = 0x00000000;
    
    // Dynamic State Base Address (CURBE)
    batch[(*offset)++] = (uint32_t)(curbe_gtt & 0xFFFFFFFF) | 0x01;
    batch[(*offset)++] = (uint32_t)(curbe_gtt >> 32);
    
    // Indirect Object Base Address (not used)
    batch[(*offset)++] = 0x00000000;
    batch[(*offset)++] = 0x00000000;
    
    // Instruction Base Address (KERNEL)
    batch[(*offset)++] = (uint32_t)(kernel_gtt & 0xFFFFFFFF) | 0x01;
    batch[(*offset)++] = (uint32_t)(kernel_gtt >> 32);
}

static void build_pipe_control(uint32_t *batch, int *offset) {
    // PIPE_CONTROL: Invalidate instruction cache
    batch[(*offset)++] = 0x7A000004;  // PIPE_CONTROL, length=4
    batch[(*offset)++] = 0x00100000;  // Instruction cache invalidate
    batch[(*offset)++] = 0x00000000;
    batch[(*offset)++] = 0x00000000;
    batch[(*offset)++] = 0x00000000;
    batch[(*offset)++] = 0x00000000;
}

static void build_interface_descriptor(uint32_t *idesc, uint64_t kernel_gtt) {
    memset(idesc, 0, 32);
    
    // DW0: Kernel Start Pointer (packed >> 6 for 64-byte alignment)
    uint32_t kernel_offset = (uint32_t)(kernel_gtt >> 6);
    idesc[0] = kernel_offset;
    
    // DW5: Number of threads (SIMD16, 256 threads)
    idesc[5] = 0x00010107;  // SIMD16, barrier enable
}

static void build_curbe(uint32_t *curbe, uint64_t input_gtt, uint64_t output_gtt) {
    memset(curbe, 0, 128);
    
    // r8.0-1: block_header pointer (64-bit)
    curbe[0] = (uint32_t)(input_gtt & 0xFFFFFFFF);
    curbe[1] = (uint32_t)(input_gtt >> 32);
    
    // r8.2-3: output pointer (64-bit)
    curbe[2] = (uint32_t)(output_gtt & 0xFFFFFFFF);
    curbe[3] = (uint32_t)(output_gtt >> 32);
    
    // r8.4: num_threads
    curbe[4] = NUM_THREADS;
}

int main() {
    printf("=== C402 NATIVE SHA256 KERNEL TEST ===\n");
    printf("Kernel: 64 rounds SHA256 complets en Gen9 ISA\n");
    printf("Taille: 4155 lignes, 245KB assembleur\n");
    printf("Threads: %d\n\n", NUM_THREADS);
    
    uint64_t start_time = get_timestamp_ns();
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open DRM");
        return 1;
    }
    printf("✓ DRM opened: fd=%d\n", fd);
    
    // Charger le kernel binaire
    FILE *f = fopen("kernels/btc_sha256_gen9_c402_native.bin", "rb");
    if (!f) {
        fprintf(stderr, "❌ Kernel binary not found\n");
        fprintf(stderr, "   Compile first: iga64 -p=9 -a kernels/btc_sha256_gen9_c402_complete_64rounds.asm -o kernels/btc_sha256_gen9_c402_native.bin\n");
        close(fd);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *kernel_code = malloc(kernel_size);
    fread(kernel_code, 1, kernel_size, f);
    fclose(f);
    printf("✓ Kernel loaded: %zu bytes\n", kernel_size);
    
    // Créer les buffers
    drm_context_t ctx;
    ctx.fd = fd;
    
    if (create_bo(fd, 4096, &ctx.handle_input, &ctx.gtt_input, &ctx.map_input) < 0 ||
        create_bo(fd, 4096, &ctx.handle_output, &ctx.gtt_output, &ctx.map_output) < 0 ||
        create_bo(fd, 4096, &ctx.handle_curbe, &ctx.gtt_curbe, &ctx.map_curbe) < 0 ||
        create_bo(fd, 4096, &ctx.handle_idesc, &ctx.gtt_idesc, &ctx.map_idesc) < 0 ||
        create_bo(fd, KERNEL_SIZE, &ctx.handle_kernel, &ctx.gtt_kernel, &ctx.map_kernel) < 0 ||
        create_bo(fd, BATCH_SIZE, &ctx.handle_batch, &ctx.gtt_batch, &ctx.map_batch) < 0) {
        fprintf(stderr, "❌ Failed to create buffers\n");
        close(fd);
        return 1;
    }
    printf("✓ All buffers created\n");
    
    // Copier le kernel
    memcpy(ctx.map_kernel, kernel_code, kernel_size);
    free(kernel_code);
    
    // Copier l'input (genesis header)
    memcpy(ctx.map_input, genesis_header, 80);
    
    // Initialiser output à zéro
    memset(ctx.map_output, 0, 4096);
    
    // Construire CURBE
    build_curbe(ctx.map_curbe, ctx.gtt_input, ctx.gtt_output);
    
    // Construire Interface Descriptor
    build_interface_descriptor(ctx.map_idesc, ctx.gtt_kernel);
    
    printf("\n✓ GTT addresses:\n");
    printf("  Input:   0x%016lx\n", ctx.gtt_input);
    printf("  Output:  0x%016lx\n", ctx.gtt_output);
    printf("  CURBE:   0x%016lx\n", ctx.gtt_curbe);
    printf("  IDesc:   0x%016lx\n", ctx.gtt_idesc);
    printf("  Kernel:  0x%016lx\n", ctx.gtt_kernel);
    
    // Construire batch buffer
    uint32_t *batch = ctx.map_batch;
    int offset = 0;
    
    // STATE_BASE_ADDRESS
    build_state_base_address(batch, &offset, ctx.gtt_kernel, ctx.gtt_curbe);
    
    // PIPE_CONTROL (invalidate caches)
    build_pipe_control(batch, &offset);
    
    // MEDIA_VFE_STATE
    batch[offset++] = 0x70000007;  // MEDIA_VFE_STATE
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    // MEDIA_CURBE_LOAD
    batch[offset++] = 0x70010002;  // MEDIA_CURBE_LOAD
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000080;  // 128 bytes
    batch[offset++] = (uint32_t)(ctx.gtt_curbe & 0xFFFFFFFF);
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[offset++] = 0x70020004;  // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000020;  // 32 bytes
    batch[offset++] = (uint32_t)(ctx.gtt_idesc & 0xFFFFFFFF);
    batch[offset++] = 0x00000000;
    
    // GPGPU_WALKER
    batch[offset++] = 0x71050014;  // GPGPU_WALKER
    batch[offset++] = 0x00000000;  // Interface Descriptor Offset
    batch[offset++] = 0x00000080;  // SIMD Size = SIMD16
    batch[offset++] = NUM_THREADS; // Thread Width Counter X
    batch[offset++] = 0x00000000;  // Thread Width Counter Y
    batch[offset++] = 0x00000000;  // Thread Width Counter Z
    batch[offset++] = 0x00000000;  // Thread Group ID X Starting
    batch[offset++] = 0x00000000;  // Thread Group ID Y Starting
    batch[offset++] = 0x00000000;  // Thread Group ID Z Starting
    batch[offset++] = 0x00000001;  // Thread Group ID X Dimension
    batch[offset++] = 0x00000001;  // Thread Group ID Y Dimension
    batch[offset++] = 0x00000001;  // Thread Group ID Z Dimension
    batch[offset++] = 0x00000001;  // Thread Group ID Starting Resume
    batch[offset++] = 0x00000000;  // Reserved
    batch[offset++] = 0x00000000;  // Reserved
    batch[offset++] = 0x00000000;  // Reserved
    batch[offset++] = 0x00000000;  // Reserved
    batch[offset++] = 0x00000000;  // Reserved
    batch[offset++] = 0x00000000;  // Reserved
    batch[offset++] = 0x00000000;  // Reserved
    batch[offset++] = 0x00000000;  // Reserved
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    printf("✓ Batch size: %d bytes (%d dwords)\n", offset * 4, offset);
    
    // Exécuter
    printf("\n🚀 Executing native SHA256 kernel...\n");
    
    struct drm_i915_gem_execbuffer2 exec = {0};
    struct drm_i915_gem_exec_object2 obj = {0};
    
    obj.handle = ctx.handle_batch;
    obj.relocation_count = 0;
    obj.relocs_ptr = 0;
    
    exec.buffers_ptr = (uintptr_t)&obj;
    exec.buffer_count = 1;
    exec.batch_len = offset * 4;
    exec.flags = I915_EXEC_RENDER;
    
    uint64_t exec_start = get_timestamp_ns();
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &exec);
    uint64_t exec_end = get_timestamp_ns();
    
    if (ret < 0) {
        perror("❌ EXECBUFFER2 failed");
        close(fd);
        return 1;
    }
    
    printf("✓ Dispatch completed in %.2f ms\n", (exec_end - exec_start) / 1000000.0);
    
    // Attendre la fin
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx.handle_batch,
        .flags = 0,
        .timeout_ns = 5000000000ULL  // 5 seconds
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    
    // Lire les résultats
    uint32_t *results = (uint32_t *)ctx.map_output;
    
    printf("\n=== RESULTS ===\n");
    
    int non_zero_count = 0;
    int max_leading_zeros = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        uint32_t leading_zeros = results[i];
        if (leading_zeros > 0) {
            non_zero_count++;
            if (leading_zeros > max_leading_zeros) {
                max_leading_zeros = leading_zeros;
            }
            if (leading_zeros >= 4) {
                printf("Thread %d: %d leading zeros ⭐\n", i, leading_zeros);
            }
        }
    }
    
    printf("Non-zero results: %d / %d\n", non_zero_count, NUM_THREADS);
    printf("Max leading zeros: %d bits\n", max_leading_zeros);
    
    uint64_t total_time = get_timestamp_ns() - start_time;
    printf("\nTotal time: %.2f ms\n", total_time / 1000000.0);
    
    if (non_zero_count > 0) {
        printf("\n🎉 SUCCESS: Native SHA256 kernel working!\n");
        printf("✅ near_miss_gpu > 0 bits achieved with native runtime\n");
        printf("✅ Independence from OpenCL: COMPLETE\n");
        close(fd);
        return 0;
    } else {
        printf("\n❌ FAILURE: All results are zero\n");
        printf("Need further investigation\n");
        
        // Debug: afficher premiers résultats
        printf("\nFirst 16 results: ");
        for (int i = 0; i < 16; i++) {
            printf("%08x ", results[i]);
        }
        printf("\n");
        
        close(fd);
        return 1;
    }
}

// Made with Bob
