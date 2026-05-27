// I915_BATCH_BUILDER_AUTO.C - Implémentation Système Automatique Batch Buffers
// Objectif: Reproduire le système automatique d'OpenCL pour i915 DRM

#include "../include/i915_batch_builder_auto.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

// ============================================================================
// CONSTANTES GEN9
// ============================================================================

#define GEN9_PIPELINE_SELECT_GPGPU 0x69041312
#define GEN9_STATE_BASE_ADDRESS 0x61010011
#define GEN9_BINDING_TABLE_POOL_ALLOC 0x19000001
#define GEN9_MEDIA_VFE_STATE 0x70000007
#define GEN9_MEDIA_CURBE_LOAD 0x70010001
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x61020001
#define GEN9_GPGPU_WALKER 0x11010014
#define GEN9_MEDIA_STATE_FLUSH 0x70040000
#define GEN9_PIPE_CONTROL 0x7A000004
#define GEN9_MI_BATCH_BUFFER_END 0x05000000

#define INITIAL_BO_CAPACITY 16
#define INITIAL_RELOCATION_CAPACITY 64

// ============================================================================
// HELPERS INTERNES
// ============================================================================

static void ensure_bo_capacity(i915_batch_builder_t *builder) {
    if (builder->bo_count >= builder->bo_capacity) {
        builder->bo_capacity *= 2;
        builder->bos = realloc(builder->bos, 
            builder->bo_capacity * sizeof(i915_bo_t));
    }
}

static void ensure_relocation_capacity(i915_batch_builder_t *builder) {
    if (builder->relocation_count >= builder->relocation_capacity) {
        builder->relocation_capacity *= 2;
        builder->relocations = realloc(builder->relocations,
            builder->relocation_capacity * sizeof(i915_relocation_t));
    }
}

static void ensure_batch_capacity(i915_batch_builder_t *builder, uint32_t needed) {
    if (builder->batch_offset + needed >= builder->batch_capacity) {
        builder->batch_capacity *= 2;
        // Remapper le batch
        munmap(builder->batch_ptr, builder->batch_bo.size);
        builder->batch_bo.size = builder->batch_capacity * sizeof(uint32_t);
        
        struct drm_i915_gem_create create = { .size = builder->batch_bo.size };
        ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create);
        builder->batch_bo.handle = create.handle;
        
        struct drm_i915_gem_mmap mmap_req = {
            .handle = builder->batch_bo.handle,
            .size = builder->batch_bo.size
        };
        ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_req);
        builder->batch_ptr = (uint32_t *)mmap_req.addr_ptr;
    }
}

static void emit_dword(i915_batch_builder_t *builder, uint32_t dword) {
    ensure_batch_capacity(builder, 1);
    builder->batch_ptr[builder->batch_offset++] = dword;
}

// ============================================================================
// GESTION DU BUILDER
// ============================================================================

i915_batch_builder_t* i915_batch_builder_create(int drm_fd, uint32_t batch_size) {
    i915_batch_builder_t *builder = calloc(1, sizeof(i915_batch_builder_t));
    if (!builder) return NULL;
    
    builder->drm_fd = drm_fd;
    builder->bo_capacity = INITIAL_BO_CAPACITY;
    builder->bos = calloc(builder->bo_capacity, sizeof(i915_bo_t));
    
    builder->relocation_capacity = INITIAL_RELOCATION_CAPACITY;
    builder->relocations = calloc(builder->relocation_capacity, sizeof(i915_relocation_t));
    
    // Créer batch buffer
    builder->batch_capacity = batch_size / sizeof(uint32_t);
    struct drm_i915_gem_create create = { .size = batch_size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        free(builder->bos);
        free(builder->relocations);
        free(builder);
        return NULL;
    }
    
    builder->batch_bo.handle = create.handle;
    builder->batch_bo.size = batch_size;
    builder->batch_bo.flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    // Mapper batch
    struct drm_i915_gem_mmap mmap_req = {
        .handle = builder->batch_bo.handle,
        .size = batch_size
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_req) < 0) {
        struct drm_gem_close close_arg = { .handle = builder->batch_bo.handle };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        free(builder->bos);
        free(builder->relocations);
        free(builder);
        return NULL;
    }
    
    builder->batch_ptr = (uint32_t *)mmap_req.addr_ptr;
    builder->batch_bo.cpu_ptr = builder->batch_ptr;
    builder->batch_bo.is_mapped = true;
    
    builder->is_initialized = true;
    
    return builder;
}

void i915_batch_builder_destroy(i915_batch_builder_t *builder) {
    if (!builder) return;
    
    // Unmapper et fermer tous les BOs
    for (uint32_t i = 0; i < builder->bo_count; i++) {
        if (builder->bos[i].is_mapped) {
            munmap(builder->bos[i].cpu_ptr, builder->bos[i].size);
        }
        struct drm_gem_close close_arg = { .handle = builder->bos[i].handle };
        ioctl(builder->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    }
    
    // Unmapper et fermer batch
    if (builder->batch_bo.is_mapped) {
        munmap(builder->batch_ptr, builder->batch_bo.size);
    }
    struct drm_gem_close close_arg = { .handle = builder->batch_bo.handle };
    ioctl(builder->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    free(builder->bos);
    free(builder->relocations);
    free(builder);
}

void i915_batch_builder_reset(i915_batch_builder_t *builder) {
    if (!builder) return;
    
    builder->batch_offset = 0;
    builder->relocation_count = 0;
    builder->is_finalized = false;
    memset(builder->batch_ptr, 0, builder->batch_bo.size);
}

// ============================================================================
// GESTION DES BUFFERS
// ============================================================================

uint32_t i915_batch_builder_create_bo(
    i915_batch_builder_t *builder,
    uint64_t size,
    uint32_t flags)
{
    ensure_bo_capacity(builder);
    
    struct drm_i915_gem_create create = { .size = size };
    if (ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        return UINT32_MAX;
    }
    
    uint32_t index = builder->bo_count++;
    builder->bos[index].handle = create.handle;
    builder->bos[index].size = size;
    builder->bos[index].flags = flags | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    builder->bos[index].is_mapped = false;
    builder->bos[index].is_pinned = false;
    
    return index;
}

void* i915_batch_builder_map_bo(
    i915_batch_builder_t *builder,
    uint32_t bo_index)
{
    if (bo_index >= builder->bo_count) return NULL;
    
    i915_bo_t *bo = &builder->bos[bo_index];
    if (bo->is_mapped) return bo->cpu_ptr;
    
    struct drm_i915_gem_mmap mmap_req = {
        .handle = bo->handle,
        .size = bo->size
    };
    
    if (ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_req) < 0) {
        return NULL;
    }
    
    bo->cpu_ptr = (void *)mmap_req.addr_ptr;
    bo->is_mapped = true;
    
    return bo->cpu_ptr;
}

void i915_batch_builder_unmap_bo(
    i915_batch_builder_t *builder,
    uint32_t bo_index)
{
    if (bo_index >= builder->bo_count) return;
    
    i915_bo_t *bo = &builder->bos[bo_index];
    if (!bo->is_mapped) return;
    
    munmap(bo->cpu_ptr, bo->size);
    bo->cpu_ptr = NULL;
    bo->is_mapped = false;
}

// ============================================================================
// CONSTRUCTION DU BATCH - COMMANDES
// ============================================================================

void i915_batch_emit_pipeline_select(
    i915_batch_builder_t *builder,
    uint32_t mode)
{
    emit_dword(builder, GEN9_PIPELINE_SELECT_GPGPU);
    emit_dword(builder, mode);
    builder->pipeline_select_mode = mode;
}

void i915_batch_emit_state_base_address(
    i915_batch_builder_t *builder,
    const state_base_address_config_t *config)
{
    (void)builder->batch_offset;  // Utilisé pour debug si nécessaire
    
    emit_dword(builder, GEN9_STATE_BASE_ADDRESS);  // Opcode + Length=19
    
    // General State Base
    emit_dword(builder, config->general_state_enable ? 0x00000001 : 0x00000000);
    emit_dword(builder, 0x00000000);  // LOW (sera relocalisé si enable)
    emit_dword(builder, 0x00000000);  // HIGH
    
    // Surface State Base
    emit_dword(builder, config->surface_state_enable ? 0x00000001 : 0x00000000);
    (void)builder->batch_offset;  // surface_offset pour debug
    emit_dword(builder, 0x00000000);  // LOW (relocation)
    
    // Dynamic State Base
    emit_dword(builder, config->dynamic_state_enable ? 0x00000001 : 0x00000000);
    (void)builder->batch_offset;  // dynamic_offset pour debug
    emit_dword(builder, 0x00000000);  // LOW (relocation)
    
    // Indirect Object Base
    emit_dword(builder, config->indirect_object_enable ? 0x00000001 : 0x00000000);
    emit_dword(builder, 0x00000000);  // LOW
    
    // Instruction Base
    emit_dword(builder, config->instruction_enable ? 0x00000001 : 0x00000000);
    (void)builder->batch_offset;  // instruction_offset pour debug
    emit_dword(builder, 0x00000000);  // LOW (relocation)
    
    // Buffer Sizes
    emit_dword(builder, config->general_buffer_size);
    emit_dword(builder, config->dynamic_buffer_size);
    // Split 64-bit indirect_buffer_size en 2x 32-bit
    uint64_t indirect_size = config->indirect_buffer_size;
    emit_dword(builder, (uint32_t)(indirect_size & 0xFFFFFFFFULL));
    emit_dword(builder, (uint32_t)((indirect_size >> 32) & 0xFFFFFFFFULL));
    emit_dword(builder, config->instruction_buffer_size);
    
    // Bindless Surface State Base
    emit_dword(builder, config->bindless_surface_enable ? 0x00000001 : 0x00000000);
    emit_dword(builder, 0x00000000);  // LOW
    
    // Sauvegarder config pour relocations
    builder->sba_config = *config;
}

void i915_batch_emit_binding_table_pool_alloc(
    i915_batch_builder_t *builder,
    uint32_t bo_index,
    uint32_t pool_size)
{
    emit_dword(builder, GEN9_BINDING_TABLE_POOL_ALLOC);
    uint32_t pool_offset = builder->batch_offset;
    emit_dword(builder, 0x00000000);  // Base Address LOW (relocation)
    emit_dword(builder, pool_size);
    
    // Ajouter relocation
    i915_batch_add_relocation(builder, bo_index, pool_offset * 4, 0,
        I915_GEM_DOMAIN_RENDER, 0);
}

void i915_batch_emit_media_vfe_state(
    i915_batch_builder_t *builder,
    const media_vfe_state_config_t *config)
{
    emit_dword(builder, GEN9_MEDIA_VFE_STATE);
    emit_dword(builder, config->scratch_space_base_low);
    emit_dword(builder, config->scratch_space_base_high);
    emit_dword(builder, (config->max_threads << 16) | (config->urb_entries & 0xFFFF));
    emit_dword(builder, config->urb_entry_size);
    emit_dword(builder, config->curbe_allocation_size);
    emit_dword(builder, config->scoreboard_mask);
    emit_dword(builder, 0x00000000);  // Scoreboard 0
    emit_dword(builder, 0x00000000);  // Scoreboard 1
    
    builder->vfe_config = *config;
}

void i915_batch_emit_media_curbe_load(
    i915_batch_builder_t *builder,
    uint32_t bo_index,
    uint32_t offset,
    uint32_t length)
{
    emit_dword(builder, GEN9_MEDIA_CURBE_LOAD);
    emit_dword(builder, 0x00000000);  // Reserved
    emit_dword(builder, length);
    uint32_t curbe_offset = builder->batch_offset;
    emit_dword(builder, 0x00000000);  // Start Address (relocation)
    
    // Ajouter relocation
    i915_batch_add_relocation(builder, bo_index, curbe_offset * 4, offset,
        I915_GEM_DOMAIN_RENDER, 0);
}

void i915_batch_emit_media_interface_descriptor_load(
    i915_batch_builder_t *builder,
    uint32_t bo_index __attribute__((unused)),
    uint32_t offset,
    uint32_t length)
{
    emit_dword(builder, GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD);
    emit_dword(builder, 0x00000000);  // Reserved
    emit_dword(builder, length);
    emit_dword(builder, offset);  // Offset heap-relative (pas de relocation)
}

void i915_batch_emit_gpgpu_walker(
    i915_batch_builder_t *builder,
    const gpgpu_walker_config_t *config)
{
    emit_dword(builder, GEN9_GPGPU_WALKER);
    emit_dword(builder, config->interface_descriptor_offset);
    emit_dword(builder, config->indirect_data_length);
    emit_dword(builder, (uint32_t)(config->indirect_data_start_address & 0xFFFFFFFF));
    emit_dword(builder, config->thread_group_id_start_x);
    emit_dword(builder, config->thread_group_id_start_y);
    emit_dword(builder, config->thread_group_id_start_z);
    emit_dword(builder, config->thread_group_id_x_dimension);
    emit_dword(builder, config->thread_group_id_y_dimension);
    emit_dword(builder, config->thread_group_id_z_dimension);
    emit_dword(builder, config->thread_width_counter_max);
    emit_dword(builder, config->thread_height_counter_max);
    emit_dword(builder, config->thread_depth_counter_max);
    emit_dword(builder, config->right_execution_mask);
    emit_dword(builder, config->bottom_execution_mask);
    
    builder->walker_config = *config;
}

void i915_batch_emit_media_state_flush(i915_batch_builder_t *builder) {
    emit_dword(builder, GEN9_MEDIA_STATE_FLUSH);
    emit_dword(builder, 0x00000000);
}

void i915_batch_emit_pipe_control(
    i915_batch_builder_t *builder,
    uint32_t flags)
{
    emit_dword(builder, GEN9_PIPE_CONTROL);
    emit_dword(builder, flags);
    emit_dword(builder, 0x00000000);  // Address LOW
    emit_dword(builder, 0x00000000);  // Address HIGH
    emit_dword(builder, 0x00000000);  // Immediate Data LOW
    emit_dword(builder, 0x00000000);  // Immediate Data HIGH
}

void i915_batch_emit_batch_buffer_end(i915_batch_builder_t *builder) {
    emit_dword(builder, GEN9_MI_BATCH_BUFFER_END);
}

// ============================================================================
// RELOCATIONS
// ============================================================================

void i915_batch_add_relocation(
    i915_batch_builder_t *builder,
    uint32_t target_bo_index,
    uint64_t batch_offset,
    uint64_t delta,
    uint32_t read_domains,
    uint32_t write_domain)
{
    ensure_relocation_capacity(builder);
    
    i915_relocation_t *reloc = &builder->relocations[builder->relocation_count++];
    reloc->target_bo_index = target_bo_index;
    reloc->offset = batch_offset;
    reloc->delta = delta;
    reloc->read_domains = read_domains;
    reloc->write_domain = write_domain;
}

// Suite dans le prochain fichier (partie 2/3)...

// Made with Bob


// ============================================================================
// EXÉCUTION
// ============================================================================

bool i915_batch_builder_finalize(i915_batch_builder_t *builder) {
    if (!builder || builder->is_finalized) return false;
    
    // CORRECTION ROOT CAUSE #62: Mapper et copier le batch buffer dans le BO
    if (!builder->batch_bo.cpu_ptr) {
        struct drm_i915_gem_mmap mmap_arg = {
            .handle = builder->batch_bo.handle,
            .offset = 0,
            .size = builder->batch_bo.size,
            .flags = 0
        };
        
        if (ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
            return false;
        }
        
        builder->batch_bo.cpu_ptr = (void *)mmap_arg.addr_ptr;
        builder->batch_bo.is_mapped = true;
    }
    
    // Copier le contenu du batch dans le BO mappé
    memcpy(builder->batch_bo.cpu_ptr, builder->batch_ptr,
           builder->batch_offset * sizeof(uint32_t));
    
    // Construire liste d'exec objects
    uint32_t total_objects = builder->bo_count + 1;  // +1 pour batch
    struct drm_i915_gem_exec_object2 *objects =
        calloc(total_objects, sizeof(struct drm_i915_gem_exec_object2));
    
    // Ajouter tous les BOs
    for (uint32_t i = 0; i < builder->bo_count; i++) {
        objects[i].handle = builder->bos[i].handle;
        objects[i].flags = builder->bos[i].flags;
    }
    
    // Ajouter batch en dernier
    objects[builder->bo_count].handle = builder->batch_bo.handle;
    objects[builder->bo_count].flags = builder->batch_bo.flags;
    
    // Convertir relocations au format i915
    struct drm_i915_gem_relocation_entry *i915_relocs = NULL;
    if (builder->relocation_count > 0) {
        i915_relocs = calloc(builder->relocation_count,
            sizeof(struct drm_i915_gem_relocation_entry));
        
        for (uint32_t i = 0; i < builder->relocation_count; i++) {
            i915_relocation_t *reloc = &builder->relocations[i];
            i915_relocs[i].target_handle = builder->bos[reloc->target_bo_index].handle;
            i915_relocs[i].offset = reloc->offset;
            i915_relocs[i].delta = reloc->delta;
            i915_relocs[i].read_domains = reloc->read_domains;
            i915_relocs[i].write_domain = reloc->write_domain;
        }
        
        // CORRECTION ROOT CAUSE #61: Attacher relocations au BATCH BO, pas au heap!
        // Les relocations décrivent les patchs à faire DANS le batch buffer
        objects[builder->bo_count].relocation_count = builder->relocation_count;
        objects[builder->bo_count].relocs_ptr = (uint64_t)i915_relocs;
    }
    
    // Exécuter une fois pour pinning (obtenir adresses GTT)
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)objects,
        .buffer_count = total_objects,
        .batch_start_offset = 0,
        .batch_len = builder->batch_offset * 4,
        .flags = I915_EXEC_RENDER
    };
    
    int ret = ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    if (ret == 0) {
        // Sauvegarder adresses GTT
        for (uint32_t i = 0; i < builder->bo_count; i++) {
            builder->bos[i].gtt_offset = objects[i].offset;
            builder->bos[i].is_pinned = true;
        }
        builder->batch_bo.gtt_offset = objects[builder->bo_count].offset;
        builder->batch_bo.is_pinned = true;
        
        // Patcher IDRT avec adresses GTT absolues si nécessaire
        // (À implémenter selon besoins spécifiques)
        
        builder->is_finalized = true;
    }
    
    free(i915_relocs);
    free(objects);
    
    return ret == 0;
}

bool i915_batch_builder_execute(
    i915_batch_builder_t *builder,
    uint64_t timeout_ns)
{
    if (!builder || !builder->is_finalized) return false;
    
    // Construire liste d'exec objects
    uint32_t total_objects = builder->bo_count + 1;
    struct drm_i915_gem_exec_object2 *objects = 
        calloc(total_objects, sizeof(struct drm_i915_gem_exec_object2));
    
    for (uint32_t i = 0; i < builder->bo_count; i++) {
        objects[i].handle = builder->bos[i].handle;
        objects[i].flags = builder->bos[i].flags;
        objects[i].offset = builder->bos[i].gtt_offset;
    }
    
    objects[builder->bo_count].handle = builder->batch_bo.handle;
    objects[builder->bo_count].flags = builder->batch_bo.flags;
    objects[builder->bo_count].offset = builder->batch_bo.gtt_offset;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)objects,
        .buffer_count = total_objects,
        .batch_start_offset = 0,
        .batch_len = builder->batch_offset * 4,
        .flags = I915_EXEC_RENDER
    };
    
    int ret = ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    free(objects);
    
    if (ret < 0) return false;
    
    // Attendre si timeout spécifié
    if (timeout_ns > 0) {
        return i915_batch_builder_wait(builder, timeout_ns);
    }
    
    return true;
}

bool i915_batch_builder_wait(
    i915_batch_builder_t *builder,
    uint64_t timeout_ns)
{
    if (!builder) return false;
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = builder->batch_bo.handle,
        .timeout_ns = timeout_ns
    };
    
    return ioctl(builder->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) == 0;
}

// ============================================================================
// HELPERS
// ============================================================================

uint64_t i915_batch_get_bo_gtt_address(
    i915_batch_builder_t *builder,
    uint32_t bo_index)
{
    if (!builder || bo_index >= builder->bo_count) return 0;
    return builder->bos[bo_index].gtt_offset;
}

void i915_batch_builder_dump(
    i915_batch_builder_t *builder,
    const char *filename)
{
    if (!builder || !filename) return;
    
    FILE *f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "=== I915 BATCH BUILDER DUMP ===\n\n");
    fprintf(f, "Batch Size: %u DWords (%u bytes)\n", 
        builder->batch_offset, builder->batch_offset * 4);
    fprintf(f, "BO Count: %u\n", builder->bo_count);
    fprintf(f, "Relocation Count: %u\n", builder->relocation_count);
    fprintf(f, "Pipeline Mode: %s\n", 
        builder->pipeline_select_mode == 2 ? "GPGPU" : "3D");
    fprintf(f, "\n");
    
    fprintf(f, "=== BUFFER OBJECTS ===\n");
    for (uint32_t i = 0; i < builder->bo_count; i++) {
        fprintf(f, "BO[%u]: handle=%u size=%lu gtt=0x%016lx mapped=%d pinned=%d\n",
            i, builder->bos[i].handle, builder->bos[i].size,
            builder->bos[i].gtt_offset, builder->bos[i].is_mapped,
            builder->bos[i].is_pinned);
    }
    fprintf(f, "\n");
    
    fprintf(f, "=== RELOCATIONS ===\n");
    for (uint32_t i = 0; i < builder->relocation_count; i++) {
        fprintf(f, "Reloc[%u]: target_bo=%u offset=0x%lx delta=0x%lx read=0x%x write=0x%x\n",
            i, builder->relocations[i].target_bo_index,
            builder->relocations[i].offset, builder->relocations[i].delta,
            builder->relocations[i].read_domains, builder->relocations[i].write_domain);
    }
    fprintf(f, "\n");
    
    fprintf(f, "=== BATCH CONTENT (HEX) ===\n");
    for (uint32_t i = 0; i < builder->batch_offset; i++) {
        if (i % 8 == 0) fprintf(f, "%04x: ", i * 4);
        fprintf(f, "%08x ", builder->batch_ptr[i]);
        if ((i + 1) % 8 == 0) fprintf(f, "\n");
    }
    if (builder->batch_offset % 8 != 0) fprintf(f, "\n");
    
    fclose(f);
}

// ============================================================================
// CONFIGURATION HELPERS
// ============================================================================

state_base_address_config_t i915_create_default_sba_config(uint32_t heap_bo_index __attribute__((unused))) {
    state_base_address_config_t config = {
        .general_state_enable = true,
        .general_state_base = 0,
        
        .surface_state_enable = true,
        .surface_state_base = 0,  // Sera relocalisé vers heap
        
        .dynamic_state_enable = true,
        .dynamic_state_base = 0,  // Sera relocalisé vers heap
        
        .indirect_object_enable = true,
        .indirect_object_base = 0,
        
        .instruction_enable = true,
        .instruction_base = 0,  // Sera relocalisé vers heap
        
        .bindless_surface_enable = true,
        .bindless_surface_base = 0,
        
        .general_buffer_size = 0xFFFFF001,
        .dynamic_buffer_size = 0xFFFFF001,
        .indirect_buffer_size = 0xFFFFFFFFULL,  // 4GB-1 (32-bit max)
        .instruction_buffer_size = 0x003BF000
    };
    return config;
}

media_vfe_state_config_t i915_create_default_vfe_config(void) {
    media_vfe_state_config_t config = {
        .scratch_space_base_low = 0,
        .scratch_space_base_high = 0,
        .max_threads = 167,  // Gen9 max
        .urb_entries = 1,
        .urb_entry_size = 64,
        .curbe_allocation_size = 1922,
        .scoreboard_mask = 0
    };
    return config;
}

interface_descriptor_config_t i915_create_default_idrt_config(
    uint64_t kernel_offset,
    uint64_t binding_table_offset,
    uint32_t curbe_length)
{
    interface_descriptor_config_t config = {
        .kernel_start_pointer = kernel_offset,
        .sampler_count = 0,
        .sampler_state_pointer = 0,
        .binding_table_entry_count = 0,
        .binding_table_pointer = binding_table_offset,
        .curbe_read_offset = 0,
        .curbe_read_length = curbe_length,
        .barrier_enable = 0,
        .slm_size = 0,
        .number_of_threads = 1,
        .cross_thread_constant_data_read_length = 13
    };
    return config;
}

gpgpu_walker_config_t i915_create_default_walker_config(
    uint32_t threads_x,
    uint32_t threads_y,
    uint32_t threads_z)
{
    gpgpu_walker_config_t config = {
        .interface_descriptor_offset = 0,
        .indirect_data_length = 0,
        .indirect_data_start_address = 0,
        .thread_group_id_start_x = 0,
        .thread_group_id_start_y = 0,
        .thread_group_id_start_z = 0,
        .thread_group_id_x_dimension = threads_x,
        .thread_group_id_y_dimension = threads_y,
        .thread_group_id_z_dimension = threads_z,
        .thread_width_counter_max = 1,
        .thread_height_counter_max = 1,
        .thread_depth_counter_max = 1,
        .right_execution_mask = 0xFFFFFFFF,
        .bottom_execution_mask = 0xFFFFFFFF
    };
    return config;
}
