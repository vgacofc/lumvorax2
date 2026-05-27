
#ifndef LUM_OPTIMIZED_VARIANTS_H
#define LUM_OPTIMIZED_VARIANTS_H

#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "../debug/memory_tracker.h"

// CORRECTION PROMPT.TXT: Toutes variantes LUM DOIVENT avoir memory_address pour double-free protection

// Variante ultra-compacte (20 bytes) - encodage total dans uint32_t + memory_address
#pragma pack(push, 1)
typedef struct {
    time_t    timestamp;       // 8 bytes
    uint32_t  encoded_data;    // 4 bytes (presence+type+posX+posY encodés)
    void*     memory_address;  // 8 bytes - OBLIGATOIRE protection double-free
} lum_encoded32_t;
#pragma pack(pop)

// Variante hybride (21 bytes) - compromis performance/compacité + memory_address
#pragma pack(push, 1)
typedef struct {
    time_t   timestamp;        // 8 bytes
    int16_t  position_x;       // 2 bytes
    int16_t  position_y;       // 2 bytes  
    uint8_t  type_presence;    // 1 byte (type + presence fusionnés)
    void*    memory_address;   // 8 bytes - OBLIGATOIRE protection double-free
} lum_hybrid_t;
#pragma pack(pop)

// Variante compacte sans ID (24 bytes aligné) - meilleure performance + memory_address
typedef struct {
    time_t   timestamp;        // 8 bytes
    int32_t  position_x;       // 4 bytes
    int32_t  position_y;       // 4 bytes
    uint8_t  presence;         // 1 byte
    uint8_t  structure_type;   // 1 byte
    uint8_t  is_destroyed;     // 1 byte - protection double-free
    uint8_t  reserved;         // 1 byte - padding
    void*    memory_address;   // 8 bytes - OBLIGATOIRE protection double-free
} lum_compact_noid_t;

// Macros pour encodage/décodage lum_encoded32_t
#define ENCODE_LUM32(presence, type, x, y) \
    (((presence) & 1) | (((type) & 0x7F) << 1) | (((x) & 0xFFF) << 8) | (((y) & 0xFFF) << 20))

#define DECODE_LUM32_PRESENCE(encoded) ((encoded) & 1)
#define DECODE_LUM32_TYPE(encoded) (((encoded) >> 1) & 0x7F)
#define DECODE_LUM32_X(encoded) ({ \
    int32_t x = ((encoded) >> 8) & 0xFFF; \
    (x & 0x800) ? x - 0x1000 : x; \
})
#define DECODE_LUM32_Y(encoded) ({ \
    int32_t y = ((encoded) >> 20) & 0xFFF; \
    (y & 0x800) ? y - 0x1000 : y; \
})

// Macros pour lum_hybrid_t
#define ENCODE_TYPE_PRESENCE(type, presence) (((type) << 1) | ((presence) & 1))
#define DECODE_TYPE(type_presence) ((type_presence) >> 1)
#define DECODE_PRESENCE(type_presence) ((type_presence) & 1)

// DÉCLARATION EXTERNE OBLIGATOIRE
extern uint64_t lum_get_timestamp(void);

// Fonctions utilitaires AVEC memory_address pour protection double-free
static inline lum_encoded32_t* lum_create_encoded32(int32_t x, int32_t y, uint8_t type, uint8_t presence) {
    lum_encoded32_t* lum = TRACKED_MALLOC(sizeof(lum_encoded32_t));
    if (lum) {
        lum->timestamp = lum_get_timestamp();
        lum->encoded_data = ENCODE_LUM32(presence, type, x, y);
        lum->memory_address = (void*)lum;  // PROTECTION DOUBLE-FREE OBLIGATOIRE
    }
    return lum;
}

static inline lum_hybrid_t* lum_create_hybrid(int16_t x, int16_t y, uint8_t type, uint8_t presence) {
    lum_hybrid_t* lum = TRACKED_MALLOC(sizeof(lum_hybrid_t));
    if (lum) {
        lum->timestamp = lum_get_timestamp();
        lum->position_x = x;
        lum->position_y = y;
        lum->type_presence = ENCODE_TYPE_PRESENCE(type, presence);
        lum->memory_address = (void*)lum;  // PROTECTION DOUBLE-FREE OBLIGATOIRE
    }
    return lum;
}

static inline lum_compact_noid_t* lum_create_compact_noid(int32_t x, int32_t y, uint8_t type, uint8_t presence) {
    lum_compact_noid_t* lum = TRACKED_MALLOC(sizeof(lum_compact_noid_t));
    if (lum) {
        lum->timestamp = lum_get_timestamp();
        lum->position_x = x;
        lum->position_y = y;
        lum->presence = presence;
        lum->structure_type = type;
        lum->is_destroyed = 0;  // Protection double-free initialisée
        lum->memory_address = (void*)lum;  // PROTECTION DOUBLE-FREE OBLIGATOIRE
    }
    return lum;
}

// Fonctions de destruction sécurisées pour TOUTES les variantes
static inline void lum_destroy_encoded32(lum_encoded32_t** lum_ptr) {
    if (lum_ptr && *lum_ptr) {
        // Vérification double-free via memory_address
        if ((*lum_ptr)->memory_address != (void*)(*lum_ptr)) {
            return; // Déjà détruit
        }
        (*lum_ptr)->memory_address = NULL; // Marquer comme détruit
        TRACKED_FREE(*lum_ptr);
        *lum_ptr = NULL;
    }
}

static inline void lum_destroy_hybrid(lum_hybrid_t** lum_ptr) {
    if (lum_ptr && *lum_ptr) {
        // Vérification double-free via memory_address
        if ((*lum_ptr)->memory_address != (void*)(*lum_ptr)) {
            return; // Déjà détruit
        }
        (*lum_ptr)->memory_address = NULL; // Marquer comme détruit
        TRACKED_FREE(*lum_ptr);
        *lum_ptr = NULL;
    }
}

static inline void lum_destroy_compact_noid(lum_compact_noid_t** lum_ptr) {
    if (lum_ptr && *lum_ptr) {
        // Vérification double-free via is_destroyed ET memory_address
        if ((*lum_ptr)->is_destroyed || (*lum_ptr)->memory_address != (void*)(*lum_ptr)) {
            return; // Déjà détruit
        }
        (*lum_ptr)->is_destroyed = 1; // Marquer comme détruit
        (*lum_ptr)->memory_address = NULL; // Invalidation adresse
        TRACKED_FREE(*lum_ptr);
        *lum_ptr = NULL;
    }
}

#endif // LUM_OPTIMIZED_VARIANTS_H
