/*
 * C197.35 - Replay Batch OpenCL Exact
 * 
 * STRATÉGIE: Capturer batch OpenCL complet + rejouer exactement
 * MÉTHODE: LD_PRELOAD pour intercepter EXECBUFFER2 + rejouer
 * 
 * Architecture:
 *   OpenCL (1×) → Capture batch complet → Rejouer avec nos données
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>

// DRM/i915 headers
#include <libdrm/i915_drm.h>
#include <libdrm/drm.h>

// Structure pour sauvegarder batch capturé
typedef struct {
    uint32_t buffer_count;
    uint64_t buffers_ptr;
    uint32_t batch_start_offset;
    uint32_t batch_len;
    uint64_t flags;
    uint64_t rsvd1; // context_id
    uint64_t rsvd2;
    
    // Buffers
    struct drm_i915_gem_exec_object2* exec_objects;
    
    // Batch data
    void* batch_data;
} captured_batch_t;

static captured_batch_t g_captured_batch = {0};
static int g_capture_mode = 1;

// Intercepter ioctl pour capturer EXECBUFFER2
typedef int (*ioctl_fn)(int fd, unsigned long request, ...);
static ioctl_fn original_ioctl = NULL;

int ioctl(int fd, unsigned long request, ...) {
    if (!original_ioctl) {
        original_ioctl = (ioctl_fn)dlsym(RTLD_NEXT, "ioctl");
    }
    
    va_list args;
    va_start(args, request);
    void* arg = va_arg(args, void*);
    va_end(args);
    
    // Intercepter EXECBUFFER2
    if (g_capture_mode && request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2* execbuf = (struct drm_i915_gem_execbuffer2*)arg;
        
        printf("[CAPTURE] EXECBUFFER2 intercepté !\n");
        printf("  buffer_count: %u\n", execbuf->buffer_count);
        printf("  batch_len: %u\n", execbuf->batch_len);
        printf("  flags: 0x%llx\n", execbuf->flags);
        printf("  ctx_id: %llu\n", execbuf->rsvd1);
        
        // Sauvegarder execbuffer
        g_captured_batch.buffer_count = execbuf->buffer_count;
        g_captured_batch.buffers_ptr = execbuf->buffers_ptr;
        g_captured_batch.batch_start_offset = execbuf->batch_start_offset;
        g_captured_batch.batch_len = execbuf->batch_len;
        g_captured_batch.flags = execbuf->flags;
        g_captured_batch.rsvd1 = execbuf->rsvd1;
        g_captured_batch.rsvd2 = execbuf->rsvd2;
        
        // Copier exec_objects
        size_t exec_objects_size = execbuf->buffer_count * sizeof(struct drm_i915_gem_exec_object2);
        g_captured_batch.exec_objects = malloc(exec_objects_size);
        memcpy(g_captured_batch.exec_objects, (void*)execbuf->buffers_ptr, exec_objects_size);
        
        printf("  Exec objects capturés: %u\n", execbuf->buffer_count);
        for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
            printf("    [%u] handle=%u, offset=0x%llx, flags=0x%llx\n",
                   i,
                   g_captured_batch.exec_objects[i].handle,
                   g_captured_batch.exec_objects[i].offset,
                   g_captured_batch.exec_objects[i].flags);
        }
        
        // Sauvegarder dans fichier
        FILE* f = fopen("/tmp/c197_35_captured_batch.bin", "wb");
        if (f) {
            fwrite(&g_captured_batch, sizeof(captured_batch_t), 1, f);
            fwrite(g_captured_batch.exec_objects, exec_objects_size, 1, f);
            fclose(f);
            printf("✅ Batch sauvegardé: /tmp/c197_35_captured_batch.bin\n");
        }
        
        g_capture_mode = 0; // Capturer une seule fois
    }
    
    return original_ioctl(fd, request, arg);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("=== C197.35 - Replay Batch OpenCL Exact ===\n\n");
    
    // Étape 1: Exécuter OpenCL pour capturer batch
    printf("[1] Exécution OpenCL pour capture...\n");
    
    // Créer kernel OpenCL simple
    FILE* f = fopen("/tmp/test_kernel.cl", "w");
    if (f) {
        fprintf(f, "__kernel void test(__global uint* output) {\n");
        fprintf(f, "    uint gid = get_global_id(0);\n");
        fprintf(f, "    output[gid] = 0x12345678;\n");
        fprintf(f, "}\n");
        fclose(f);
    }
    
    // Exécuter avec Python + PyOpenCL
    system("python3 << 'EOF'\n"
           "import pyopencl as cl\n"
           "import numpy as np\n"
           "\n"
           "# Créer contexte\n"
           "platforms = cl.get_platforms()\n"
           "devices = platforms[0].get_devices(device_type=cl.device_type.GPU)\n"
           "ctx = cl.Context([devices[0]])\n"
           "queue = cl.CommandQueue(ctx)\n"
           "\n"
           "# Lire kernel\n"
           "with open('/tmp/test_kernel.cl', 'r') as f:\n"
           "    kernel_source = f.read()\n"
           "\n"
           "# Compiler\n"
           "prg = cl.Program(ctx, kernel_source).build()\n"
           "\n"
           "# Créer buffer\n"
           "output = np.zeros(256, dtype=np.uint32)\n"
           "output_buf = cl.Buffer(ctx, cl.mem_flags.WRITE_ONLY, output.nbytes)\n"
           "\n"
           "# Exécuter (sera intercepté par notre ioctl)\n"
           "prg.test(queue, (256,), None, output_buf)\n"
           "queue.finish()\n"
           "\n"
           "# Lire résultats\n"
           "cl.enqueue_copy(queue, output, output_buf)\n"
           "print(f'Output[0]: 0x{output[0]:08x}')\n"
           "EOF\n");
    
    // Étape 2: Charger batch capturé
    printf("\n[2] Chargement batch capturé...\n");
    
    f = fopen("/tmp/c197_35_captured_batch.bin", "rb");
    if (!f) {
        printf("❌ Batch non capturé\n");
        printf("   Exécutez avec: LD_PRELOAD=./c197_35_replay_opencl_batch_exact python3 ...\n");
        return 1;
    }
    
    captured_batch_t loaded_batch;
    fread(&loaded_batch, sizeof(captured_batch_t), 1, f);
    
    size_t exec_objects_size = loaded_batch.buffer_count * sizeof(struct drm_i915_gem_exec_object2);
    loaded_batch.exec_objects = malloc(exec_objects_size);
    fread(loaded_batch.exec_objects, exec_objects_size, 1, f);
    fclose(f);
    
    printf("✅ Batch chargé:\n");
    printf("   buffer_count: %u\n", loaded_batch.buffer_count);
    printf("   batch_len: %u\n", loaded_batch.batch_len);
    printf("   flags: 0x%llx\n", loaded_batch.flags);
    printf("   ctx_id: %llu\n", loaded_batch.rsvd1);
    
    // Étape 3: Rejouer batch
    printf("\n[3] Replay batch...\n");
    
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        printf("❌ Erreur open DRM: %s\n", strerror(errno));
        free(loaded_batch.exec_objects);
        return 1;
    }
    
    // Créer nouveau context
    struct drm_i915_gem_context_create_ext ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create) < 0) {
        printf("❌ Erreur CONTEXT_CREATE: %s\n", strerror(errno));
        close(drm_fd);
        free(loaded_batch.exec_objects);
        return 1;
    }
    
    printf("✅ Context créé: id=%u\n", ctx_create.ctx_id);
    
    // Préparer execbuffer avec nouveau context
    struct drm_i915_gem_execbuffer2 replay_execbuf = {
        .buffers_ptr = (uintptr_t)loaded_batch.exec_objects,
        .buffer_count = loaded_batch.buffer_count,
        .batch_start_offset = loaded_batch.batch_start_offset,
        .batch_len = loaded_batch.batch_len,
        .flags = loaded_batch.flags,
        .rsvd1 = ctx_create.ctx_id, // Nouveau context
    };
    
    printf("   Soumission EXECBUFFER2...\n");
    
    // Désactiver capture pour replay
    g_capture_mode = 0;
    
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &replay_execbuf);
    if (ret < 0) {
        printf("❌ Erreur EXECBUFFER2: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        free(loaded_batch.exec_objects);
        return 1;
    }
    
    printf("✅ Batch rejoué avec succès !\n");
    
    // Cleanup
    close(drm_fd);
    free(loaded_batch.exec_objects);
    
    printf("\n=== SUCCÈS C197.35 ===\n");
    printf("✅ Batch OpenCL capturé\n");
    printf("✅ Batch rejoué avec nouveau context\n");
    printf("✅ EXECBUFFER2 accepté par i915\n");
    printf("\n🎯 Prochaine étape: C197.36 - Remplacer ISA/buffers dynamiquement\n");
    
    return 0;
}

// Made with Bob
