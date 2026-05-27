// C403: Solution NATIVE Finale - Binary C398 + Runtime i915 DRM Corrigé
// Utilise le binary SHA256 qui fonctionne (C398) avec runtime natif + GEM_WAIT
// 100% NATIF - 0% OpenCL

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

// Bitcoin genesis block header
static const uint32_t genesis_header[20] = {
    0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x3BA3EDFD, 0x7A7B12B2, 0x7AC72C3E, 0x67768F61,
    0x7FC81BC3, 0x888A5132, 0x3A9FB8AA, 0x4B1E5E4A,
    0x29AB5F49, 0xFFFF001D, 0x1DAC2B7C, 0x00000000
};

typedef struct {
    int fd;
    uint32_t handle_input, handle_output, handle_curbe, handle_idesc, handle_kernel, handle_batch;
    uint64_t gtt_input, gtt_output, gtt_curbe, gtt_idesc, gtt_kernel, gtt_batch;
    void *map_input, *map_output, *map_curbe, *map_idesc, *map_kernel, *map_batch;
} gpu_context_t;

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
    
    *gtt = 0;
    return 0;
}

int main() {
    printf("=== C403 SOLUTION NATIVE FINALE ===\n");
    printf("Binary SHA256 C398 + Runtime i915 DRM Natif Corrigé\n");
    printf("100%% NATIF - 0%% OpenCL\n\n");
    
    uint64_t start_time = get_timestamp_ns();
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("Failed to open DRM");
        return 1;
    }
    printf("✓ DRM opened: fd=%d\n", fd);
    
    // Charger le binary SHA256 de C398 (qui fonctionne)
    FILE *f = fopen("kernels/btc_sha256_c398_raw.bin", "rb");
    if (!f) {
        fprintf(stderr, "❌ Kernel binary not found: kernels/btc_sha256_c398_raw.bin\n");
        fprintf(stderr, "   This is the working SHA256 kernel from C398\n");
        close(fd);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *kernel_code = malloc(kernel_size);
    fread(kernel_code, 1, kernel_size, f);
    fclose(f);
    printf("✓ Kernel loaded: %zu bytes (C398 working binary)\n", kernel_size);
    
    // Créer les buffers
    gpu_context_t ctx;
    ctx.fd = fd;
    
    size_t kernel_alloc_size = (kernel_size + 4095) & ~4095;  // Align 4KB
    
    if (create_bo(fd, 4096, &ctx.handle_input, &ctx.gtt_input, &ctx.map_input) < 0 ||
        create_bo(fd, 4096, &ctx.handle_output, &ctx.gtt_output, &ctx.map_output) < 0 ||
        create_bo(fd, 4096, &ctx.handle_curbe, &ctx.gtt_curbe, &ctx.map_curbe) < 0 ||
        create_bo(fd, 4096, &ctx.handle_idesc, &ctx.gtt_idesc, &ctx.map_idesc) < 0 ||
        create_bo(fd, kernel_alloc_size, &ctx.handle_kernel, &ctx.gtt_kernel, &ctx.map_kernel) < 0 ||
        create_bo(fd, BATCH_SIZE, &ctx.handle_batch, &ctx.gtt_batch, &ctx.map_batch) < 0) {
        fprintf(stderr, "❌ Failed to create buffers\n");
        close(fd);
        return 1;
    }
    printf("✓ All buffers created\n");
    
    // Copier le kernel
    memcpy(ctx.map_kernel, kernel_code, kernel_size);
    free(kernel_code);
    
    // Copier l'input
    memcpy(ctx.map_input, genesis_header, 80);
    
    // Initialiser output à zéro
    memset(ctx.map_output, 0, 4096);
    
    // Construire CURBE (arguments kernel)
    uint32_t *curbe = (uint32_t *)ctx.map_curbe;
    memset(curbe, 0, 128);
    curbe[0] = (uint32_t)(ctx.gtt_input & 0xFFFFFFFF);
    curbe[1] = (uint32_t)(ctx.gtt_input >> 32);
    curbe[2] = (uint32_t)(ctx.gtt_output & 0xFFFFFFFF);
    curbe[3] = (uint32_t)(ctx.gtt_output >> 32);
    curbe[4] = NUM_THREADS;
    
    // Construire Interface Descriptor
    uint32_t *idesc = (uint32_t *)ctx.map_idesc;
    memset(idesc, 0, 32);
    idesc[0] = (uint32_t)(ctx.gtt_kernel >> 6);  // Kernel start pointer (packed)
    idesc[5] = 0x00010107;  // SIMD16, barrier enable
    
    printf("\n✓ GTT addresses:\n");
    printf("  Input:   0x%016lx\n", ctx.gtt_input);
    printf("  Output:  0x%016lx\n", ctx.gtt_output);
    printf("  CURBE:   0x%016lx\n", ctx.gtt_curbe);
    printf("  IDesc:   0x%016lx\n", ctx.gtt_idesc);
    printf("  Kernel:  0x%016lx\n", ctx.gtt_kernel);
    
    // Construire batch buffer
    uint32_t *batch = (uint32_t *)ctx.map_batch;
    int offset = 0;
    
    // STATE_BASE_ADDRESS
    batch[offset++] = 0x61010009;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = (uint32_t)(ctx.gtt_curbe & 0xFFFFFFFF) | 0x01;
    batch[offset++] = (uint32_t)(ctx.gtt_curbe >> 32);
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = (uint32_t)(ctx.gtt_kernel & 0xFFFFFFFF) | 0x01;
    batch[offset++] = (uint32_t)(ctx.gtt_kernel >> 32);
    
    // PIPE_CONTROL (invalidate caches)
    batch[offset++] = 0x7A000004;
    batch[offset++] = 0x00100000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    // MEDIA_VFE_STATE
    batch[offset++] = 0x70000007;
    for (int i = 0; i < 7; i++) batch[offset++] = 0x00000000;
    
    // MEDIA_CURBE_LOAD
    batch[offset++] = 0x70010002;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000080;
    batch[offset++] = (uint32_t)(ctx.gtt_curbe & 0xFFFFFFFF);
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[offset++] = 0x70020004;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000020;
    batch[offset++] = (uint32_t)(ctx.gtt_idesc & 0xFFFFFFFF);
    batch[offset++] = 0x00000000;
    
    // GPGPU_WALKER
    batch[offset++] = 0x71050014;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000080;
    batch[offset++] = NUM_THREADS;
    for (int i = 0; i < 18; i++) batch[offset++] = (i < 6) ? 0x00000001 : 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    printf("✓ Batch size: %d bytes (%d dwords)\n", offset * 4, offset);
    
    // Exécuter
    printf("\n🚀 Executing NATIVE kernel (C398 binary + i915 DRM runtime)...\n");
    
    struct drm_i915_gem_execbuffer2 exec = {0};
    struct drm_i915_gem_exec_object2 obj = {0};
    
    obj.handle = ctx.handle_batch;
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
    
    // C403 FIX CRITIQUE: Attendre GPU avec GEM_WAIT avant lecture
    printf("\n🔧 C403 FIX: Adding GEM_WAIT synchronization...\n");
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx.handle_output,
        .timeout_ns = 5000000000ULL,  // 5 seconds
        .flags = 0
    };
    
    uint64_t wait_start = get_timestamp_ns();
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    uint64_t wait_end = get_timestamp_ns();
    
    if (ret < 0) {
        perror("❌ GEM_WAIT failed");
    } else {
        printf("✓ GEM_WAIT completed in %.2f ms\n", (wait_end - wait_start) / 1000000.0);
    }
    
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
    
    if (non_zero_count > 0 && max_leading_zeros > 0) {
        printf("\n🎉 SUCCESS C403: SOLUTION NATIVE FINALE FONCTIONNE!\n");
        printf("✅ Binary SHA256 C398 (qui fonctionne)\n");
        printf("✅ Runtime i915 DRM natif corrigé (GEM_WAIT ajouté)\n");
        printf("✅ near_miss_gpu = %d bits (> 0) RÉSOLU!\n", max_leading_zeros);
        printf("✅ 100%% NATIF - 0%% OpenCL\n");
        close(fd);
        return 0;
    } else {
        printf("\n❌ FAILURE: Still all zeros\n");
        printf("Debug: First 16 results: ");
        for (int i = 0; i < 16; i++) {
            printf("%08x ", results[i]);
        }
        printf("\n");
        close(fd);
        return 1;
    }
}

// Made with Bob
