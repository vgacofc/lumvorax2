/**
 * LUMVORAX BATCH DECODER ENGINE - Implémentation Gen9
 * 
 * "Wireshark du GPU" - Décodage complet batch buffers Intel Gen9
 * 
 * CYCLE: C576
 * PRIORITÉ: #2 (Critique pour Surface States)
 * NIVEAU: Forensic maximal
 * 
 * EXPERTISE:
 * - Architecture GPU Intel Gen9 (Skylake/Kaby Lake)
 * - Command Streamer Gen9
 * - Format batch buffers i915 DRM
 * - Validation hardware Gen9
 * - Analyse causale GPU
 */

#include "batch_decoder_gen9.h"
#include <string.h>
#include <time.h>
#include <assert.h>

// ============================================================================
// CONFIGURATION GLOBALE
// ============================================================================

static decoder_config_t g_config = {
    .verbose = true,
    .validate_reserved_bits = true,
    .decode_addresses = true,
    .analyze_causality = true,
    .detect_anomalies = true,
    .output_file = NULL
};

// ============================================================================
// TABLES DE RÉFÉRENCE GEN9
// ============================================================================

/**
 * Table des opcodes MI (Machine Instructions)
 */
typedef struct {
    uint32_t opcode;
    gpu_command_type_t type;
    const char *name;
    uint32_t min_dwords;
    uint32_t max_dwords;
} mi_command_info_t;

static const mi_command_info_t MI_COMMANDS[] = {
    {0x00, CMD_MI_NOOP, "MI_NOOP", 1, 1},
    {0x0A, CMD_MI_BATCH_BUFFER_END, "MI_BATCH_BUFFER_END", 1, 1},
    {0x22, CMD_MI_LOAD_REGISTER_IMM, "MI_LOAD_REGISTER_IMM", 3, 3},
    {0x29, CMD_MI_LOAD_REGISTER_MEM, "MI_LOAD_REGISTER_MEM", 4, 4},
    {0x20, CMD_MI_STORE_DATA_IMM, "MI_STORE_DATA_IMM", 4, 5},
    {0x24, CMD_MI_STORE_REGISTER_MEM, "MI_STORE_REGISTER_MEM", 4, 4},
    {0x2F, CMD_MI_ATOMIC, "MI_ATOMIC", 3, 3},
    {0x26, CMD_MI_FLUSH_DW, "MI_FLUSH_DW", 4, 5},
};

/**
 * Table des opcodes 3D/Media
 */
typedef struct {
    uint32_t opcode;
    uint32_t subopcode;
    gpu_command_type_t type;
    const char *name;
    uint32_t dword_count;
} gfx_command_info_t;

static const gfx_command_info_t GFX_COMMANDS[] = {
    // Pipeline
    {0x69, 0x04, CMD_PIPELINE_SELECT, "PIPELINE_SELECT", 1},
    {0x61, 0x01, CMD_STATE_BASE_ADDRESS, "STATE_BASE_ADDRESS", 16},
    {0x02, 0x01, CMD_STATE_SIP, "STATE_SIP", 3},
    
    // Media
    {0x70, 0x00, CMD_MEDIA_VFE_STATE, "MEDIA_VFE_STATE", 9},
    {0x71, 0x00, CMD_MEDIA_CURBE_LOAD, "MEDIA_CURBE_LOAD", 4},
    {0x72, 0x00, CMD_MEDIA_INTERFACE_DESCRIPTOR_LOAD, "MEDIA_INTERFACE_DESCRIPTOR_LOAD", 4},
    {0x04, 0x00, CMD_MEDIA_STATE_FLUSH, "MEDIA_STATE_FLUSH", 2},
    {0x05, 0x00, CMD_GPGPU_WALKER, "GPGPU_WALKER", 15},
    
    // 3D State
    {0x78, 0x05, CMD_3DSTATE_BINDING_TABLE_POINTERS, "3DSTATE_BINDING_TABLE_POINTERS_PS", 2},
    {0x79, 0x02, CMD_3DSTATE_SAMPLER_STATE_POINTERS, "3DSTATE_SAMPLER_STATE_POINTERS_PS", 2},
};

// ============================================================================
// INITIALISATION
// ============================================================================

void batch_decoder_init(decoder_config_t *config) {
    if (config) {
        memcpy(&g_config, config, sizeof(decoder_config_t));
    }
    
    if (g_config.output_file == NULL) {
        g_config.output_file = stdout;
    }
}

// ============================================================================
// IDENTIFICATION COMMANDES
// ============================================================================

gpu_command_type_t identify_command_type(uint32_t dword0) {
    // Extraction opcode client (bits 31-29)
    uint32_t client = extract_bits(dword0, 29, 31);
    
    // MI Commands (client = 0)
    if (client == 0) {
        // MI_BATCH_BUFFER_END: 0x05000000
        if (dword0 == 0x05000000) {
            return CMD_MI_BATCH_BUFFER_END;
        }
        
        uint32_t mi_opcode = extract_bits(dword0, 23, 28);
        for (size_t i = 0; i < sizeof(MI_COMMANDS) / sizeof(MI_COMMANDS[0]); i++) {
            if (MI_COMMANDS[i].opcode == mi_opcode) {
                return MI_COMMANDS[i].type;
            }
        }
        return CMD_UNKNOWN;
    }
    
    // 3D/Media Commands (client = 3)
    if (client == 3) {
        // Décodage direct par opcode complet (masque bits 16-28)
        uint32_t opcode_masked = (dword0 >> 16) & 0x1FFF;
        
        // PIPELINE_SELECT: 0x69040002 → bits 16-28 = 0x0904
        if (opcode_masked == 0x0904) {
            return CMD_PIPELINE_SELECT;
        }
        
        // STATE_BASE_ADDRESS: 0x61010010 → bits 16-28 = 0x0101
        if (opcode_masked == 0x0101) {
            return CMD_STATE_BASE_ADDRESS;
        }
        
        // MEDIA_VFE_STATE: 0x70000007 → bits 16-28 = 0x1000
        if (opcode_masked == 0x1000) {
            return CMD_MEDIA_VFE_STATE;
        }
        
        // MEDIA_CURBE_LOAD: 0x71000002 → bits 16-28 = 0x1100
        if (opcode_masked == 0x1100) {
            return CMD_MEDIA_CURBE_LOAD;
        }
        
        // MEDIA_INTERFACE_DESCRIPTOR_LOAD: 0x72000002 → bits 16-28 = 0x1200
        if (opcode_masked == 0x1200) {
            return CMD_MEDIA_INTERFACE_DESCRIPTOR_LOAD;
        }
        
        // MEDIA_STATE_FLUSH: 0x70040000 → bits 16-28 = 0x1004
        if (opcode_masked == 0x1004) {
            return CMD_MEDIA_STATE_FLUSH;
        }
        
        // GPGPU_WALKER: 0x7A05000D → bits 16-28 = 0x1A05
        if (opcode_masked == 0x1A05) {
            return CMD_GPGPU_WALKER;
        }
        
        return CMD_UNKNOWN;
    }
    
    return CMD_UNKNOWN;
}

const char* get_command_name(gpu_command_type_t type) {
    // Recherche dans MI_COMMANDS
    for (size_t i = 0; i < sizeof(MI_COMMANDS) / sizeof(MI_COMMANDS[0]); i++) {
        if (MI_COMMANDS[i].type == type) {
            return MI_COMMANDS[i].name;
        }
    }
    
    // Recherche dans GFX_COMMANDS
    for (size_t i = 0; i < sizeof(GFX_COMMANDS) / sizeof(GFX_COMMANDS[0]); i++) {
        if (GFX_COMMANDS[i].type == type) {
            return GFX_COMMANDS[i].name;
        }
    }
    
    return "UNKNOWN";
}

uint32_t get_command_length(uint32_t dword0) {
    uint32_t client = extract_bits(dword0, 29, 31);
    
    // MI Commands: longueur dans bits 7-0 (+ 2)
    if (client == 0) {
        uint32_t length_field = extract_bits(dword0, 0, 7);
        return length_field + 2;
    }
    
    // 3D/Media Commands: longueur dans bits 7-0 (+ 2)
    if (client == 3) {
        uint32_t length_field = extract_bits(dword0, 0, 7);
        return length_field + 2;
    }
    
    return 1; // Par défaut
}

// ============================================================================
// DÉCODEURS SPÉCIALISÉS
// ============================================================================

bool decode_mi_batch_buffer_end(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_MI_BATCH_BUFFER_END;
    cmd->name = "MI_BATCH_BUFFER_END";
    cmd->dword_count = 1;
    cmd->raw_data[0] = data[0];
    
    // Champ: Opcode
    cmd->fields[0] = (decoded_field_t){
        .name = "MI Command Opcode",
        .value = extract_bits(data[0], 23, 28),
        .bit_start = 23,
        .bit_end = 28,
        .description = "MI_BATCH_BUFFER_END (0x0A)",
        .is_valid = true
    };
    cmd->field_count = 1;
    
    cmd->is_valid = true;
    cmd->is_synchronization = true;
    
    return true;
}

bool decode_state_base_address(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_STATE_BASE_ADDRESS;
    cmd->name = "STATE_BASE_ADDRESS";
    cmd->dword_count = 16;
    
    // Copie données brutes
    for (uint32_t i = 0; i < 16; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 0: Header
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "DWord Length",
        .value = extract_bits(data[0], 0, 7),
        .bit_start = 0,
        .bit_end = 7,
        .description = "Length = 14 (16 DWords total)",
        .is_valid = (extract_bits(data[0], 0, 7) == 14)
    };
    
    // DWord 1-2: General State Base Address (64-bit)
    uint64_t general_state_base = ((uint64_t)data[2] << 32) | data[1];
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "General State Base Address",
        .value = (uint32_t)general_state_base,
        .bit_start = 12,
        .bit_end = 63,
        .description = "Base address for general state data",
        .is_address = true,
        .is_valid = true
    };
    
    // DWord 3: General State Buffer Size
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "General State Buffer Size",
        .value = data[3],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Size in pages (4KB)",
        .is_valid = true
    };
    
    // DWord 4-5: Surface State Base Address (64-bit) - CRITIQUE
    uint64_t surface_state_base = ((uint64_t)data[5] << 32) | data[4];
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Surface State Base Address",
        .value = (uint32_t)surface_state_base,
        .bit_start = 12,
        .bit_end = 63,
        .description = "Base address for surface states (CRITICAL)",
        .is_address = true,
        .is_valid = true
    };
    
    // DWord 6-7: Dynamic State Base Address (64-bit)
    uint64_t dynamic_state_base = ((uint64_t)data[7] << 32) | data[6];
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Dynamic State Base Address",
        .value = (uint32_t)dynamic_state_base,
        .bit_start = 12,
        .bit_end = 63,
        .description = "Base address for dynamic state",
        .is_address = true,
        .is_valid = true
    };
    
    // DWord 8-9: Indirect Object Base Address (64-bit)
    uint64_t indirect_object_base = ((uint64_t)data[9] << 32) | data[8];
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Indirect Object Base Address",
        .value = (uint32_t)indirect_object_base,
        .bit_start = 12,
        .bit_end = 63,
        .description = "Base address for indirect objects",
        .is_address = true,
        .is_valid = true
    };
    
    // DWord 10-11: Instruction Base Address (64-bit) - CRITIQUE
    uint64_t instruction_base = ((uint64_t)data[11] << 32) | data[10];
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Instruction Base Address",
        .value = (uint32_t)instruction_base,
        .bit_start = 12,
        .bit_end = 63,
        .description = "Base address for kernel instructions (CRITICAL)",
        .is_address = true,
        .is_valid = true
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->modifies_state = true;
    cmd->requires_relocation = true;
    
    return true;
}

bool decode_media_vfe_state(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_MEDIA_VFE_STATE;
    cmd->name = "MEDIA_VFE_STATE";
    cmd->dword_count = 9;
    
    for (uint32_t i = 0; i < 9; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 1: Scratch Space Base Pointer
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Scratch Space Base Pointer",
        .value = data[1],
        .bit_start = 10,
        .bit_end = 31,
        .description = "Base address for scratch space",
        .is_address = true,
        .is_valid = true
    };
    
    // DWord 2: Maximum Number of Threads
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Maximum Number of Threads",
        .value = extract_bits(data[2], 0, 15),
        .bit_start = 0,
        .bit_end = 15,
        .description = "Max concurrent threads",
        .is_valid = true
    };
    
    // DWord 2: Number of URB Entries
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Number of URB Entries",
        .value = extract_bits(data[2], 16, 23),
        .bit_start = 16,
        .bit_end = 23,
        .description = "URB entry count",
        .is_valid = true
    };
    
    // DWord 3: URB Entry Allocation Size
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "URB Entry Allocation Size",
        .value = extract_bits(data[3], 16, 31),
        .bit_start = 16,
        .bit_end = 31,
        .description = "Size per URB entry",
        .is_valid = true
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->modifies_state = true;
    
    return true;
}

bool decode_media_idrt_load(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_MEDIA_INTERFACE_DESCRIPTOR_LOAD;
    cmd->name = "MEDIA_INTERFACE_DESCRIPTOR_LOAD";
    cmd->dword_count = 4;
    
    for (uint32_t i = 0; i < 4; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 2: Interface Descriptor Total Length
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Interface Descriptor Total Length",
        .value = extract_bits(data[2], 0, 16),
        .bit_start = 0,
        .bit_end = 16,
        .description = "Total length of IDRT in bytes",
        .is_valid = true
    };
    
    // DWord 3: Interface Descriptor Data Start Address
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Interface Descriptor Data Start Address",
        .value = data[3],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Offset to IDRT (relative to Dynamic State Base)",
        .is_address = true,
        .is_valid = true
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->modifies_state = true;
    cmd->requires_relocation = true;
    
    return true;
}

bool decode_gpgpu_walker(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_GPGPU_WALKER;
    cmd->name = "GPGPU_WALKER";
    cmd->dword_count = 15;
    
    for (uint32_t i = 0; i < 15; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 1: Interface Descriptor Offset
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Interface Descriptor Offset",
        .value = extract_bits(data[1], 0, 5),
        .bit_start = 0,
        .bit_end = 5,
        .description = "Offset into IDRT",
        .is_valid = true
    };
    
    // DWord 2: Indirect Data Length
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Indirect Data Length",
        .value = extract_bits(data[2], 0, 16),
        .bit_start = 0,
        .bit_end = 16,
        .description = "Length of indirect data",
        .is_valid = true
    };
    
    // DWord 3: Indirect Data Start Address
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Indirect Data Start Address",
        .value = data[3],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Address of indirect data (CURBE)",
        .is_address = true,
        .is_valid = true
    };
    
    // DWord 4: Thread Group ID X Dimension
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Thread Group ID X Dimension",
        .value = data[4],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Number of thread groups in X",
        .is_valid = true
    };
    
    // DWord 5: Thread Group ID Y Dimension
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Thread Group ID Y Dimension",
        .value = data[5],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Number of thread groups in Y",
        .is_valid = true
    };
    
    // DWord 6: Thread Group ID Z Dimension
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Thread Group ID Z Dimension",
        .value = data[6],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Number of thread groups in Z",
        .is_valid = true
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->accesses_memory = true;
    cmd->requires_relocation = true;
    
    return true;
}

bool decode_mi_load_register_mem(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_MI_LOAD_REGISTER_MEM;
    cmd->name = "MI_LOAD_REGISTER_MEM";
    cmd->dword_count = 4;
    
    for (uint32_t i = 0; i < 4; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 1: Register Address
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Register Address",
        .value = data[1],
        .bit_start = 2,
        .bit_end = 22,
        .description = "MMIO register address",
        .is_valid = true
    };
    
    // DWord 2-3: Memory Address (64-bit)
    uint64_t mem_addr = ((uint64_t)data[3] << 32) | data[2];
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Memory Address",
        .value = (uint32_t)mem_addr,
        .bit_start = 0,
        .bit_end = 63,
        .description = "Source memory address",
        .is_address = true,
        .is_valid = true
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->accesses_memory = true;
    cmd->requires_relocation = true;
    
    return true;
}

bool decode_mi_store_data_imm(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_MI_STORE_DATA_IMM;
    cmd->name = "MI_STORE_DATA_IMM";
    
    // Longueur variable: 4 ou 5 DWords
    uint32_t length = extract_bits(data[0], 0, 7) + 2;
    cmd->dword_count = length;
    
    for (uint32_t i = 0; i < length; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 0: Use Global GTT
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Use Global GTT",
        .value = extract_bits(data[0], 22, 22),
        .bit_start = 22,
        .bit_end = 22,
        .description = "1 = Use GTT, 0 = Use PPGTT",
        .is_valid = true
    };
    
    // DWord 1-2: Address (64-bit)
    uint64_t addr = ((uint64_t)data[2] << 32) | data[1];
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Memory Address",
        .value = (uint32_t)addr,
        .bit_start = 2,
        .bit_end = 63,
        .description = "Destination memory address",
        .is_address = true,
        .is_valid = true
    };
    
    // DWord 3: Immediate Data Low
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Immediate Data Low",
        .value = data[3],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Data to write (low 32 bits)",
        .is_valid = true
    };
    
    // DWord 4: Immediate Data High (si présent)
    if (length == 5) {
        cmd->fields[field_idx++] = (decoded_field_t){
            .name = "Immediate Data High",
            .value = data[4],
            .bit_start = 0,
            .bit_end = 31,
            .description = "Data to write (high 32 bits)",
            .is_valid = true
        };
    }
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->accesses_memory = true;
    cmd->requires_relocation = true;
    
    return true;
}

bool decode_pipeline_select(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_PIPELINE_SELECT;
    cmd->name = "PIPELINE_SELECT";
    cmd->dword_count = 1;
    cmd->raw_data[0] = data[0];
    
    uint32_t field_idx = 0;
    
    // Pipeline Selection
    uint32_t pipeline = extract_bits(data[0], 0, 1);
    const char *pipeline_name = (pipeline == 0) ? "3D" : 
                                (pipeline == 1) ? "Media" : 
                                (pipeline == 2) ? "GPGPU" : "Reserved";
    
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Pipeline Selection",
        .value = pipeline,
        .bit_start = 0,
        .bit_end = 1,
        .description = pipeline_name,
        .is_valid = (pipeline <= 2)
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->modifies_state = true;
    
    return true;
}

bool decode_media_curbe_load(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_MEDIA_CURBE_LOAD;
    cmd->name = "MEDIA_CURBE_LOAD";
    cmd->dword_count = 4;
    
    for (uint32_t i = 0; i < 4; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 2: CURBE Total Data Length
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "CURBE Total Data Length",
        .value = extract_bits(data[2], 0, 16),
        .bit_start = 0,
        .bit_end = 16,
        .description = "Total length of CURBE data in bytes",
        .is_valid = true
    };
    
    // DWord 3: CURBE Data Start Address
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "CURBE Data Start Address",
        .value = data[3],
        .bit_start = 0,
        .bit_end = 31,
        .description = "Offset to CURBE data (relative to Dynamic State Base)",
        .is_address = true,
        .is_valid = true
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->modifies_state = true;
    cmd->requires_relocation = true;
    
    return true;
}

bool decode_media_state_flush(const uint32_t *data, decoded_command_t *cmd) {
    cmd->type = CMD_MEDIA_STATE_FLUSH;
    cmd->name = "MEDIA_STATE_FLUSH";
    cmd->dword_count = 2;
    
    for (uint32_t i = 0; i < 2; i++) {
        cmd->raw_data[i] = data[i];
    }
    
    uint32_t field_idx = 0;
    
    // DWord 1: Flush to GO
    cmd->fields[field_idx++] = (decoded_field_t){
        .name = "Flush to GO",
        .value = extract_bits(data[1], 0, 0),
        .bit_start = 0,
        .bit_end = 0,
        .description = "Flush media pipeline",
        .is_valid = true
    };
    
    cmd->field_count = field_idx;
    cmd->is_valid = true;
    cmd->is_synchronization = true;
    
    return true;
}

// ============================================================================
// DÉCODAGE PRINCIPAL
// ============================================================================

bool batch_decoder_decode(
    const uint32_t *batch_data,
    uint32_t size_bytes,
    decoded_batch_t *output
) {
    memset(output, 0, sizeof(decoded_batch_t));
    
    output->size_bytes = size_bytes;
    output->decode_timestamp_ns = (uint64_t)time(NULL) * 1000000000ULL;
    
    uint32_t offset = 0;
    uint32_t dword_count = size_bytes / 4;
    
    while (offset < dword_count && output->command_count < 1024) {
        decoded_command_t *cmd = &output->commands[output->command_count];
        
        cmd->offset = offset * 4;
        uint32_t dword0 = batch_data[offset];
        
        // Identification type
        cmd->type = identify_command_type(dword0);
        cmd->opcode = dword0;
        
        // Décodage spécialisé
        bool decoded = false;
        
        switch (cmd->type) {
            case CMD_MI_BATCH_BUFFER_END:
                decoded = decode_mi_batch_buffer_end(&batch_data[offset], cmd);
                output->has_end_marker = true;
                break;
                
            case CMD_STATE_BASE_ADDRESS:
                decoded = decode_state_base_address(&batch_data[offset], cmd);
                output->pipeline_command_count++;
                break;
                
            case CMD_MEDIA_VFE_STATE:
                decoded = decode_media_vfe_state(&batch_data[offset], cmd);
                output->media_command_count++;
                break;
                
            case CMD_MEDIA_CURBE_LOAD:
                decoded = decode_media_curbe_load(&batch_data[offset], cmd);
                output->media_command_count++;
                break;
                
            case CMD_MEDIA_INTERFACE_DESCRIPTOR_LOAD:
                decoded = decode_media_idrt_load(&batch_data[offset], cmd);
                output->media_command_count++;
                break;
                
            case CMD_MEDIA_STATE_FLUSH:
                decoded = decode_media_state_flush(&batch_data[offset], cmd);
                output->media_command_count++;
                break;
                
            case CMD_GPGPU_WALKER:
                decoded = decode_gpgpu_walker(&batch_data[offset], cmd);
                output->compute_command_count++;
                break;
                
            case CMD_MI_LOAD_REGISTER_MEM:
                decoded = decode_mi_load_register_mem(&batch_data[offset], cmd);
                output->mi_command_count++;
                break;
                
            case CMD_MI_STORE_DATA_IMM:
                decoded = decode_mi_store_data_imm(&batch_data[offset], cmd);
                output->mi_command_count++;
                break;
                
            case CMD_PIPELINE_SELECT:
                decoded = decode_pipeline_select(&batch_data[offset], cmd);
                output->pipeline_command_count++;
                break;
                
            default:
                // Commande inconnue - décodage minimal
                cmd->name = "UNKNOWN";
                cmd->dword_count = get_command_length(dword0);
                cmd->is_valid = false;
                snprintf(cmd->error_msg, sizeof(cmd->error_msg),
                        "Unknown command opcode: 0x%08X", dword0);
                decoded = true;
                break;
        }
        
        if (!decoded) {
            output->error_count++;
        }
        
        // Statistiques
        if (cmd->accesses_memory) output->memory_access_count++;
        if (cmd->modifies_state) output->state_change_count++;
        if (cmd->is_synchronization) output->sync_point_count++;
        
        offset += cmd->dword_count;
        output->command_count++;
        
        // Arrêt si MI_BATCH_BUFFER_END
        if (cmd->type == CMD_MI_BATCH_BUFFER_END) {
            break;
        }
    }
    
    output->is_valid = (output->error_count == 0);
    
    return output->is_valid;
}

// ============================================================================
// AFFICHAGE WIRESHARK-LIKE
// ============================================================================

void batch_decoder_print_wireshark(const decoded_batch_t *batch) {
    FILE *out = g_config.output_file;
    
    fprintf(out, "\n");
    fprintf(out, "═══════════════════════════════════════════════════════════════════════════════\n");
    fprintf(out, "  LUMVORAX BATCH DECODER - Wireshark GPU Mode\n");
    fprintf(out, "═══════════════════════════════════════════════════════════════════════════════\n");
    fprintf(out, "\n");
    fprintf(out, "Batch Size: %u bytes (%u DWords)\n", batch->size_bytes, batch->size_bytes / 4);
    fprintf(out, "Commands: %u total\n", batch->command_count);
    fprintf(out, "  - MI Commands: %u\n", batch->mi_command_count);
    fprintf(out, "  - Pipeline Commands: %u\n", batch->pipeline_command_count);
    fprintf(out, "  - Media Commands: %u\n", batch->media_command_count);
    fprintf(out, "  - Compute Commands: %u\n", batch->compute_command_count);
    fprintf(out, "\n");
    fprintf(out, "Statistics:\n");
    fprintf(out, "  - Memory Accesses: %u\n", batch->memory_access_count);
    fprintf(out, "  - State Changes: %u\n", batch->state_change_count);
    fprintf(out, "  - Sync Points: %u\n", batch->sync_point_count);
    fprintf(out, "  - Errors: %u\n", batch->error_count);
    fprintf(out, "  - Warnings: %u\n", batch->warning_count);
    fprintf(out, "\n");
    fprintf(out, "Validation: %s\n", batch->is_valid ? "✅ VALID" : "❌ INVALID");
    fprintf(out, "End Marker: %s\n", batch->has_end_marker ? "✅ Present" : "⚠️ Missing");
    fprintf(out, "\n");
    fprintf(out, "───────────────────────────────────────────────────────────────────────────────\n");
    fprintf(out, "\n");
    
    // Affichage détaillé des commandes
    for (uint32_t i = 0; i < batch->command_count; i++) {
        const decoded_command_t *cmd = &batch->commands[i];
        
        fprintf(out, "Frame %u: %s\n", i, cmd->name);
        fprintf(out, "  Offset: 0x%04X (%u bytes)\n", cmd->offset, cmd->offset);
        fprintf(out, "  Length: %u DWords (%u bytes)\n", cmd->dword_count, cmd->dword_count * 4);
        fprintf(out, "  Opcode: 0x%08X\n", cmd->opcode);
        
        if (!cmd->is_valid) {
            fprintf(out, "  ❌ ERROR: %s\n", cmd->error_msg);
        }
        
        // Affichage des champs
        for (uint32_t j = 0; j < cmd->field_count; j++) {
            const decoded_field_t *field = &cmd->fields[j];
            
            fprintf(out, "    ├─ %s: ", field->name);
            
            if (field->is_address) {
                fprintf(out, "0x%08X (GTT address)", field->value);
            } else {
                fprintf(out, "0x%X (%u)", field->value, field->value);
            }
            
            fprintf(out, "\n");
            fprintf(out, "    │  Bits [%u:%u] - %s\n", 
                    field->bit_start, field->bit_end, field->description);
            
            if (!field->is_valid) {
                fprintf(out, "    │  ⚠️ INVALID\n");
            }
        }
        
        // Flags
        if (cmd->requires_relocation) {
            fprintf(out, "    └─ 🔗 Requires relocation\n");
        }
        if (cmd->accesses_memory) {
            fprintf(out, "    └─ 💾 Accesses memory\n");
        }
        if (cmd->modifies_state) {
            fprintf(out, "    └─ ⚙️ Modifies GPU state\n");
        }
        if (cmd->is_synchronization) {
            fprintf(out, "    └─ 🔄 Synchronization point\n");
        }
        
        fprintf(out, "\n");
    }
    
    fprintf(out, "═══════════════════════════════════════════════════════════════════════════════\n");
    fprintf(out, "\n");
}

void batch_decoder_print(const decoded_batch_t *batch) {
    batch_decoder_print_wireshark(batch);
}

// ============================================================================
// VALIDATION
// ============================================================================

uint32_t batch_decoder_validate(decoded_batch_t *batch) {
    uint32_t error_count = 0;
    
    // Validation: doit avoir MI_BATCH_BUFFER_END
    if (!batch->has_end_marker) {
        error_count++;
        batch->warning_count++;
    }
    
    // Validation de chaque commande
    for (uint32_t i = 0; i < batch->command_count; i++) {
        decoded_command_t *cmd = &batch->commands[i];
        
        if (!cmd->is_valid) {
            error_count++;
        }
        
        // Validation des champs
        for (uint32_t j = 0; j < cmd->field_count; j++) {
            if (!cmd->fields[j].is_valid) {
                error_count++;
            }
        }
    }
    
    batch->error_count = error_count;
    batch->is_valid = (error_count == 0);
    
    return error_count;
}

// ============================================================================
// ANALYSE CAUSALE
// ============================================================================

void batch_decoder_analyze_causality(decoded_batch_t *batch __attribute__((unused))) {
    // TODO: Implémentation complète analyse causale
    // - Graphe de dépendances mémoire
    // - Détection hazards RAW/WAR/WAW
    // - Analyse pipeline stalls
    // - Détection deadlocks potentiels
}

uint32_t batch_decoder_detect_anomalies(decoded_batch_t *batch) {
    uint32_t anomaly_count = 0;
    
    // Anomalie: pas de MI_BATCH_BUFFER_END
    if (!batch->has_end_marker) {
        anomaly_count++;
    }
    
    // Anomalie: STATE_BASE_ADDRESS manquant
    bool has_state_base_address = false;
    for (uint32_t i = 0; i < batch->command_count; i++) {
        if (batch->commands[i].type == CMD_STATE_BASE_ADDRESS) {
            has_state_base_address = true;
            break;
        }
    }
    
    if (!has_state_base_address && batch->compute_command_count > 0) {
        anomaly_count++;
    }
    
    return anomaly_count;
}

void batch_decoder_compare(
    const decoded_batch_t *batch_opencl,
    const decoded_batch_t *batch_native,
    FILE *output
) {
    // TODO: Implémentation complète comparaison différentielle
    fprintf(output, "Differential analysis: OpenCL vs Native\n");
    fprintf(output, "OpenCL commands: %u\n", batch_opencl->command_count);
    fprintf(output, "Native commands: %u\n", batch_native->command_count);
}

void batch_decoder_generate_dependency_graph(
    const decoded_batch_t *batch __attribute__((unused)),
    const char *output_path __attribute__((unused))
) {
    // TODO: Génération graphe DOT
}

void batch_decoder_print_json(const decoded_batch_t *batch __attribute__((unused))) {
    // TODO: Format JSON
}

// Made with Bob
