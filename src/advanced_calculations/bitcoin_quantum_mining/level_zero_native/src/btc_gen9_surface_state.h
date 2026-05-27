/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_surface_state.h — Surface State & Binding Table Gen9
 *
 * CYCLE C390 — Solution Surface States + Binding Table
 * ROOT CAUSE C389: Surface States NULL → dataport writes collapse to 0
 * SOLUTION C390: Implémenter modèle mémoire Gen9 complet
 *
 * ARCHITECTURE Gen9 Memory Model:
 * Kernel BTI → Binding Table → Surface State → GEM Buffer
 */

#ifndef BTC_GEN9_SURFACE_STATE_H
#define BTC_GEN9_SURFACE_STATE_H

#include <stdint.h>

/* ══════════════════════════════════════════════════════════════════════
 * Gen9 Surface State Structure (16 DWORDs = 64 bytes)
 * Intel® Graphics Programmer's Reference Manual Vol 2c Part 1
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Gen9 Surface Type
 */
#define GEN9_SURFACE_TYPE_BUFFER    0  /* 1D buffer */
#define GEN9_SURFACE_TYPE_2D        1  /* 2D texture */
#define GEN9_SURFACE_TYPE_3D        2  /* 3D texture */
#define GEN9_SURFACE_TYPE_CUBE      3  /* Cube map */

/**
 * Gen9 Surface Format (pour RAW buffer)
 */
#define GEN9_SURFACE_FORMAT_RAW     0x1FF  /* RAW format (no conversion) */

/**
 * Gen9 Surface State (64 bytes aligned)
 */
typedef struct __attribute__((packed)) {
    /* DW0 */
    uint32_t surface_type : 3;           /* [2:0] Surface Type */
    uint32_t surface_array : 1;          /* [3] Surface Array */
    uint32_t surface_format : 9;         /* [12:4] Surface Format */
    uint32_t reserved0 : 1;              /* [13] Reserved */
    uint32_t surface_vertical_alignment : 2;  /* [15:14] Vertical Alignment */
    uint32_t surface_horizontal_alignment : 2; /* [17:16] Horizontal Alignment */
    uint32_t tile_mode : 2;              /* [19:18] Tile Mode (0=linear) */
    uint32_t vertical_line_stride : 1;   /* [20] Vertical Line Stride */
    uint32_t vertical_line_stride_offset : 1; /* [21] Vertical Line Stride Offset */
    uint32_t sampler_l2_bypass : 1;      /* [22] Sampler L2 Bypass */
    uint32_t render_cache_read_write : 1; /* [23] Render Cache Read Write Mode */
    uint32_t media_boundary_pixel_mode : 2; /* [25:24] Media Boundary Pixel Mode */
    uint32_t cube_face_enables : 6;      /* [31:26] Cube Face Enables */
    
    /* DW1 */
    uint32_t base_mip_level : 5;         /* [4:0] Base Mip Level */
    uint32_t memory_object_control : 7;  /* [11:5] Memory Object Control State (MOCS) */
    uint32_t width : 14;                 /* [25:12] Width (minus 1) */
    uint32_t reserved1 : 6;              /* [31:26] Reserved */
    
    /* DW2 */
    uint32_t height : 14;                /* [13:0] Height (minus 1) */
    uint32_t reserved2 : 2;              /* [15:14] Reserved */
    uint32_t depth : 11;                 /* [26:16] Depth (minus 1) */
    uint32_t reserved3 : 5;              /* [31:27] Reserved */
    
    /* DW3 */
    uint32_t surface_pitch : 18;         /* [17:0] Surface Pitch (minus 1) */
    uint32_t reserved4 : 3;              /* [20:18] Reserved */
    uint32_t depth_stencil_resource : 1; /* [21] Depth/Stencil Resource */
    uint32_t reserved5 : 10;             /* [31:22] Reserved */
    
    /* DW4 */
    uint32_t minimum_array_element : 11; /* [10:0] Minimum Array Element */
    uint32_t reserved6 : 4;              /* [14:11] Reserved */
    uint32_t render_target_view_extent : 11; /* [25:15] Render Target View Extent */
    uint32_t reserved7 : 6;              /* [31:26] Reserved */
    
    /* DW5 */
    uint32_t mip_count : 4;              /* [3:0] Mip Count */
    uint32_t surface_min_lod : 4;        /* [7:4] Surface Min LOD */
    uint32_t mip_tail_start_lod : 4;     /* [11:8] Mip Tail Start LOD */
    uint32_t reserved8 : 2;              /* [13:12] Reserved */
    uint32_t coherency_type : 1;         /* [14] Coherency Type */
    uint32_t reserved9 : 3;              /* [17:15] Reserved */
    uint32_t ewa_disable_for_cube : 1;   /* [18] EWA Disable for Cube */
    uint32_t y_offset : 3;               /* [21:19] Y Offset */
    uint32_t reserved10 : 1;             /* [22] Reserved */
    uint32_t x_offset : 9;               /* [31:23] X Offset */
    
    /* DW6 */
    uint32_t reserved11 : 16;            /* [15:0] Reserved */
    uint32_t aux_surface_mode : 3;       /* [18:16] Auxiliary Surface Mode */
    uint32_t reserved12 : 1;             /* [19] Reserved */
    uint32_t aux_surface_pitch : 9;      /* [28:20] Auxiliary Surface Pitch */
    uint32_t reserved13 : 3;             /* [31:29] Reserved */
    
    /* DW7 */
    uint32_t shader_channel_select_r : 3; /* [2:0] Shader Channel Select R */
    uint32_t shader_channel_select_g : 3; /* [5:3] Shader Channel Select G */
    uint32_t shader_channel_select_b : 3; /* [8:6] Shader Channel Select B */
    uint32_t shader_channel_select_a : 3; /* [11:9] Shader Channel Select A */
    uint32_t reserved14 : 4;             /* [15:12] Reserved */
    uint32_t resource_min_lod : 12;      /* [27:16] Resource Min LOD */
    uint32_t reserved15 : 4;             /* [31:28] Reserved */
    
    /* DW8-9: Surface Base Address (64-bit) */
    uint64_t surface_base_address;       /* [63:0] Surface Base Address */
    
    /* DW10-11: Auxiliary Surface Base Address (64-bit) */
    uint64_t aux_surface_base_address;   /* [63:0] Auxiliary Surface Base Address */
    
    /* DW12 */
    uint32_t aux_table_index : 11;       /* [10:0] Auxiliary Table Index */
    uint32_t reserved16 : 5;             /* [15:11] Reserved */
    uint32_t clear_value_address_enable : 1; /* [16] Clear Value Address Enable */
    uint32_t reserved17 : 15;            /* [31:17] Reserved */
    
    /* DW13-14: Clear Value Address (64-bit) */
    uint64_t clear_value_address;        /* [63:0] Clear Value Address */
    
    /* DW15 */
    uint32_t reserved18;                 /* [31:0] Reserved */
    
} gen9_surface_state_t;

/* Vérification taille structure */
_Static_assert(sizeof(gen9_surface_state_t) == 64, "gen9_surface_state_t must be 64 bytes");

/* ══════════════════════════════════════════════════════════════════════
 * Gen9 Binding Table
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Binding Table Entry (1 DWORD = 4 bytes)
 * Contient l'offset du Surface State dans SSH
 */
typedef struct {
    uint32_t surface_state_pointer : 16; /* [15:0] Offset in SSH (64-byte aligned) */
    uint32_t reserved : 16;              /* [31:16] Reserved */
} gen9_binding_table_entry_t;

/* ══════════════════════════════════════════════════════════════════════
 * Helper Functions
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * C390: Initialiser Surface State pour buffer RAW
 * 
 * @param ss Pointeur vers Surface State à initialiser
 * @param base_address Adresse GPU du buffer (GTT offset)
 * @param size_bytes Taille du buffer en bytes
 */
static inline void gen9_init_buffer_surface_state(
    gen9_surface_state_t *ss,
    uint64_t base_address,
    uint32_t size_bytes)
{
    memset(ss, 0, sizeof(gen9_surface_state_t));
    
    /* DW0: Surface Type = BUFFER, Format = RAW */
    ss->surface_type = GEN9_SURFACE_TYPE_BUFFER;
    ss->surface_format = GEN9_SURFACE_FORMAT_RAW;
    ss->tile_mode = 0;  /* Linear (non-tiled) */
    ss->render_cache_read_write = 1;  /* Enable render cache */
    
    /* DW1: Width = size in bytes - 1 */
    ss->width = (size_bytes - 1) & 0x3FFF;  /* Lower 14 bits */
    ss->memory_object_control = 0x3;  /* MOCS: Cached */
    
    /* DW2: Height = (size >> 14) - 1 for large buffers */
    ss->height = ((size_bytes - 1) >> 14) & 0x3FFF;
    
    /* DW3: Pitch = 0 for buffer */
    ss->surface_pitch = 0;
    
    /* DW8-9: Base Address */
    ss->surface_base_address = base_address;
    
    /* DW7: Shader Channel Select (RGBA = 0,1,2,3) */
    ss->shader_channel_select_r = 4;  /* RED */
    ss->shader_channel_select_g = 5;  /* GREEN */
    ss->shader_channel_select_b = 6;  /* BLUE */
    ss->shader_channel_select_a = 7;  /* ALPHA */
}

/**
 * C390: Créer Binding Table Entry
 * 
 * @param bte Pointeur vers Binding Table Entry
 * @param surface_state_offset Offset du Surface State dans SSH (64-byte aligned)
 */
static inline void gen9_init_binding_table_entry(
    gen9_binding_table_entry_t *bte,
    uint32_t surface_state_offset)
{
    bte->surface_state_pointer = (surface_state_offset >> 6);  /* Divide by 64 */
    bte->reserved = 0;
}

/**
 * C390: Dump Surface State pour debug
 */
static inline void gen9_dump_surface_state(
    const gen9_surface_state_t *ss,
    const char *label)
{
    printf("[C390_SURFACE_STATE] %s:\n", label);
    printf("  DW0: type=%u format=0x%x tile=%u cache_rw=%u\n",
           ss->surface_type, ss->surface_format, ss->tile_mode, ss->render_cache_read_write);
    printf("  DW1: width=%u mocs=0x%x\n", ss->width, ss->memory_object_control);
    printf("  DW2: height=%u depth=%u\n", ss->height, ss->depth);
    printf("  DW3: pitch=%u\n", ss->surface_pitch);
    printf("  DW8-9: base_address=0x%016lx\n", ss->surface_base_address);
    printf("  Size: %zu bytes\n", sizeof(gen9_surface_state_t));
}

#endif /* BTC_GEN9_SURFACE_STATE_H */

// Made with Bob
