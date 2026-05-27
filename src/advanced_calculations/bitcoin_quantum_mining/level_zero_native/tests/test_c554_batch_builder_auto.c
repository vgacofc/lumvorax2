// TEST C554 - SYSTÈME AUTOMATIQUE BATCH BUILDER
// Objectif: Tester le système automatique de construction batch buffers
// Reproduit exactement le comportement OpenCL mais avec notre API

#include "../include/i915_batch_builder_auto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    printf("=== TEST C554 - BATCH BUILDER AUTOMATIQUE (0%%) ===\n");
    printf("Système automatique de construction batch buffers\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("[5%%] DRM ouvert\n");
    
    // Créer batch builder
    i915_batch_builder_t *builder = i915_batch_builder_create(fd, 4096);
    if (!builder) {
        fprintf(stderr, "Échec création batch builder\n");
        close(fd);
        return 1;
    }
    printf("[10%%] Batch builder créé\n");
    
    // Créer heap buffer (16KB)
    uint32_t heap_bo = i915_batch_builder_create_bo(builder, 16384, 
        EXEC_OBJECT_SUPPORTS_48B_ADDRESS);
    if (heap_bo == UINT32_MAX) {
        fprintf(stderr, "Échec création heap BO\n");
        i915_batch_builder_destroy(builder);
        close(fd);
        return 1;
    }
    printf("[15%%] Heap BO créé (index %u)\n", heap_bo);
    
    // Créer output buffer (4KB)
    uint32_t output_bo = i915_batch_builder_create_bo(builder, 4096,
        EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE);
    if (output_bo == UINT32_MAX) {
        fprintf(stderr, "Échec création output BO\n");
        i915_batch_builder_destroy(builder);
        close(fd);
        return 1;
    }
    printf("[20%%] Output BO créé (index %u)\n", output_bo);
    
    // Mapper heap
    uint8_t *heap = i915_batch_builder_map_bo(builder, heap_bo);
    if (!heap) {
        fprintf(stderr, "Échec mapping heap\n");
        i915_batch_builder_destroy(builder);
        close(fd);
        return 1;
    }
    memset(heap, 0, 16384);
    printf("[25%%] Heap mappé\n");
    
    // Mapper output
    uint32_t *output = i915_batch_builder_map_bo(builder, output_bo);
    if (!output) {
        fprintf(stderr, "Échec mapping output\n");
        i915_batch_builder_destroy(builder);
        close(fd);
        return 1;
    }
    memset(output, 0xFF, 4096);
    printf("[30%%] Output mappé\n");
    
    // === CONSTRUCTION HEAP ===
    printf("\n=== CONSTRUCTION HEAP (35%%) ===\n");
    
    // Surface State @ 0x0000
    uint32_t *surface_state = (uint32_t *)heap;
    surface_state[0] = 0x00000004;  // Type: BUFFER
    // surface_state[1-2] seront relocalisés automatiquement
    surface_state[3] = 0x00000FFF;  // Width: 4095
    surface_state[4] = 0x00000000;  // Height/Depth
    surface_state[5] = 0x00001000;  // Pitch: 4096
    printf("[40%%] Surface State configuré\n");
    
    // Binding Table @ 0x0040
    uint32_t *binding_table = (uint32_t *)(heap + 0x40);
    binding_table[0] = 0x00000000;  // Offset vers Surface State
    printf("[45%%] Binding Table configuré\n");
    
    // IDRT @ 0x0050
    uint32_t *idrt = (uint32_t *)(heap + 0x50);
    idrt[0] = 0x00000070;  // Kernel offset (sera patché avec GTT)
    idrt[1] = 0x00000000;  // Reserved
    idrt[2] = 0x00000000;  // Sampler
    idrt[3] = 0x00000040;  // BTI offset (sera patché avec GTT)
    idrt[4] = 0x00000020;  // CURBE: Length=32
    idrt[5] = 0x00000001;  // Threads=1
    idrt[6] = 0x0000000D;  // Cross-Thread=13 DWords
    idrt[7] = 0x00000000;  // Reserved
    printf("[50%%] IDRT configuré\n");
    
    // Kernel ISA @ 0x0070
    uint32_t kernel_isa[] = {
        // MOV r1.0<1>:ud 0x12345678
        0x00600001, 0x20020c20, 0x00000000, 0x12345678,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        // SEND (write) r1 to surface
        0x07800031, 0x20001cc4, 0x00000020, 0x02180001,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        // EOT
        0x05000000, 0x20000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000
    };
    memcpy(heap + 0x70, kernel_isa, sizeof(kernel_isa));
    printf("[55%%] Kernel ISA copié\n");
    
    // CURBE Data @ 0x00F0
    uint32_t *curbe = (uint32_t *)(heap + 0xF0);
    for (int i = 0; i < 8; i++) curbe[i] = 0x00000000;
    printf("[60%%] CURBE data initialisé\n");
    
    // === CONSTRUCTION BATCH AUTOMATIQUE ===
    printf("\n=== CONSTRUCTION BATCH AUTOMATIQUE (65%%) ===\n");
    
    // PIPELINE_SELECT
    i915_batch_emit_pipeline_select(builder, 2);  // GPGPU
    printf("[70%%] PIPELINE_SELECT émis\n");
    
    // STATE_BASE_ADDRESS (automatique avec relocations)
    state_base_address_config_t sba_config = i915_create_default_sba_config(heap_bo);
    i915_batch_emit_state_base_address(builder, &sba_config);
    
    // Ajouter relocations STATE_BASE_ADDRESS
    // Surface State Base
    i915_batch_add_relocation(builder, heap_bo, 5 * 4, 0,
        I915_GEM_DOMAIN_RENDER, 0);
    // Dynamic State Base
    i915_batch_add_relocation(builder, heap_bo, 7 * 4, 0,
        I915_GEM_DOMAIN_RENDER, 0);
    // Instruction Base
    i915_batch_add_relocation(builder, heap_bo, 11 * 4, 0,
        I915_GEM_DOMAIN_INSTRUCTION, 0);
    
    printf("[75%%] STATE_BASE_ADDRESS émis (avec relocations)\n");
    
    // BINDING_TABLE_POOL_ALLOC
    i915_batch_emit_binding_table_pool_alloc(builder, heap_bo, 4096);
    printf("[80%%] BINDING_TABLE_POOL_ALLOC émis\n");
    
    // MEDIA_VFE_STATE
    media_vfe_state_config_t vfe_config = i915_create_default_vfe_config();
    i915_batch_emit_media_vfe_state(builder, &vfe_config);
    printf("[85%%] MEDIA_VFE_STATE émis\n");
    
    // MEDIA_CURBE_LOAD
    i915_batch_emit_media_curbe_load(builder, heap_bo, 0xF0, 32);
    printf("[90%%] MEDIA_CURBE_LOAD émis\n");
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    i915_batch_emit_media_interface_descriptor_load(builder, heap_bo, 0x50, 32);
    printf("[92%%] MEDIA_INTERFACE_DESCRIPTOR_LOAD émis\n");
    
    // GPGPU_WALKER
    gpgpu_walker_config_t walker_config = i915_create_default_walker_config(1, 1, 1);
    i915_batch_emit_gpgpu_walker(builder, &walker_config);
    printf("[94%%] GPGPU_WALKER émis\n");
    
    // MEDIA_STATE_FLUSH
    i915_batch_emit_media_state_flush(builder);
    printf("[96%%] MEDIA_STATE_FLUSH émis\n");
    
    // PIPE_CONTROL
    i915_batch_emit_pipe_control(builder, 0x00100000);
    printf("[98%%] PIPE_CONTROL émis\n");
    
    // MI_BATCH_BUFFER_END
    i915_batch_emit_batch_buffer_end(builder);
    printf("[100%%] MI_BATCH_BUFFER_END émis\n");
    
    // Dump batch pour debugging
    i915_batch_builder_dump(builder, "logs/batch_c554_dump.txt");
    printf("\n✓ Batch dumpé dans logs/batch_c554_dump.txt\n");
    
    // === FINALISATION ET EXÉCUTION ===
    printf("\n=== FINALISATION ET EXÉCUTION ===\n");
    
    // Finaliser (pinning + relocations)
    if (!i915_batch_builder_finalize(builder)) {
        fprintf(stderr, "❌ Échec finalisation\n");
        i915_batch_builder_destroy(builder);
        close(fd);
        return 1;
    }
    printf("✓ Batch finalisé (pinning + relocations)\n");
    
    // Afficher adresses GTT
    printf("\nAdresses GTT:\n");
    printf("  heap:   0x%016lx\n", i915_batch_get_bo_gtt_address(builder, heap_bo));
    printf("  output: 0x%016lx\n", i915_batch_get_bo_gtt_address(builder, output_bo));
    
    // Patcher IDRT avec adresses GTT absolues
    uint64_t heap_gtt = i915_batch_get_bo_gtt_address(builder, heap_bo);
    idrt[0] = (uint32_t)((heap_gtt + 0x70) & 0xFFFFFFFF);
    idrt[3] = (uint32_t)((heap_gtt + 0x40) & 0xFFFFFFFF);
    printf("\nIDRT patché:\n");
    printf("  DW0 (Kernel): 0x%08x\n", idrt[0]);
    printf("  DW3 (BTI):    0x%08x\n", idrt[3]);
    
    // Exécuter
    printf("\nExécution...\n");
    if (!i915_batch_builder_execute(builder, 1000000000)) {
        fprintf(stderr, "❌ Échec exécution\n");
        i915_batch_builder_destroy(builder);
        close(fd);
        return 1;
    }
    printf("✓ Batch exécuté\n");
    
    // Attendre
    if (!i915_batch_builder_wait(builder, 1000000000)) {
        fprintf(stderr, "❌ Timeout\n");
        i915_batch_builder_destroy(builder);
        close(fd);
        return 1;
    }
    printf("✓ Exécution terminée\n");
    
    // === VÉRIFICATION RÉSULTAT ===
    printf("\n=== RÉSULTAT ===\n");
    printf("output[0] = 0x%08x\n", output[0]);
    printf("output[1] = 0x%08x\n", output[1]);
    
    bool success = (output[0] == 0x12345678);
    
    if (success) {
        printf("\n🎉🎉🎉 SUCCÈS TOTAL! 🎉🎉🎉\n");
        printf("GPU NATIF FONCTIONNE AVEC SYSTÈME AUTOMATIQUE!\n");
    } else {
        printf("\n❌ Échec - résultat incorrect\n");
    }
    
    // Cleanup
    i915_batch_builder_destroy(builder);
    close(fd);
    
    return success ? 0 : 1;
}

// Made with Bob
