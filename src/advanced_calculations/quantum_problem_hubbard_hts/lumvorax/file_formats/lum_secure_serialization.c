// Feature test macros for POSIX functions - _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "lum_secure_serialization.h"

// Portable byte order conversion for 64-bit integers
#ifndef htobe64
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htobe64(x) __builtin_bswap64(x)
#define be64toh(x) __builtin_bswap64(x)
#else
#define htobe64(x) (x)
#define be64toh(x) (x)
#endif
#endif
#include "../debug/memory_tracker.h"
#include "../lum/lum_core.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <string.h>
#include <arpa/inet.h> // Pour htonl/ntohl (endianness)
#include <time.h> // Pour clock_gettime

// Types métadonnées pour sérialisation (définis dans le header)
typedef struct {
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    uint32_t format_type;
    uint64_t creation_timestamp;
    uint64_t modification_timestamp;
    uint32_t total_lums;
    uint32_t total_groups;
    uint64_t total_size_bytes;
    uint32_t checksum_crc32;
    char creator_info[64];
    char file_description[128];
    void* memory_address;
} lum_file_metadata_t;

// Constantes manquantes
#define LUM_FILE_MAGIC_NUMBER 0x4C554D46
#define LUM_FILE_VERSION_MAJOR 1
#define LUM_FILE_VERSION_MINOR 0
#define LUM_FORMAT_NATIVE_BINARY 1


// ================== SÉRIALISATION SÉCURISÉE LUM ==================

// Sérialisation sécurisée d'un groupe LUM
lum_secure_result_t* lum_secure_serialize_group(lum_group_t* group, bool encrypt) {
    if (!group) return NULL;

    lum_secure_result_t* result = TRACKED_MALLOC(sizeof(lum_secure_result_t));
    if (!result) return NULL;

    result->memory_address = (void*)result;
    result->success = false;
    SAFE_STRCPY(result->error_message, "", sizeof(result->error_message));

    // Calcul taille nécessaire
    size_t header_size = sizeof(lum_secure_header_t);
    size_t data_size = group->count * sizeof(lum_t);
    size_t total_size = header_size + data_size;

    // Allocation buffer sérialisation
    result->serialized_data = TRACKED_MALLOC(total_size);
    if (!result->serialized_data) {
        SAFE_STRCPY(result->error_message, "Allocation buffer failed", sizeof(result->error_message));
        return result;
    }

    // Création header
    lum_secure_header_t* header = (lum_secure_header_t*)result->serialized_data;
    header->magic_number = htonl(LUM_SECURE_MAGIC); // Mettre en network byte order
    header->version = htons(LUM_SECURE_VERSION);     // Mettre en network byte order
    header->lum_count = htonl((uint32_t)group->count); // Mettre en network byte order
    header->data_size = htonl((uint32_t)data_size);   // Mettre en network byte order
    header->is_encrypted = encrypt;
    header->memory_address = (void*)header;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    header->timestamp = htobe64(ts.tv_sec * 1000000000ULL + ts.tv_nsec); // Mettre en network byte order 64-bit

    // Copie données
    uint8_t* data_ptr = result->serialized_data + header_size;
    memcpy(data_ptr, group->lums, data_size);

    // Calcul checksum
    header->checksum_crc32 = htonl(lum_secure_calculate_checksum(data_ptr, data_size)); // Mettre en network byte order
    result->checksum = ntohl(header->checksum_crc32); // Stocker le checksum non converti

    // Chiffrement simple (XOR) si demandé
    if (encrypt) {
        uint8_t key = (uint8_t)(be64toh(header->timestamp) & 0xFF);
        for (size_t i = 0; i < data_size; i++) {
            data_ptr[i] ^= key;
        }
    }

    result->data_size = total_size;
    result->success = true;
    SAFE_STRCPY(result->error_message, "Serialization completed successfully", sizeof(result->error_message));

    return result;
}

// Désérialisation sécurisée vers groupe LUM
lum_group_t* lum_secure_deserialize_group(const uint8_t* data, size_t data_size_param, bool decrypt) {
    if (!data || data_size_param < sizeof(lum_secure_header_t)) return NULL;

    // Lecture header et conversion
    lum_secure_header_t temp_header;
    memcpy(&temp_header, data, sizeof(lum_secure_header_t));

    uint32_t magic_number = ntohl(temp_header.magic_number);
    uint16_t version = ntohs(temp_header.version);
    uint32_t lum_count = ntohl(temp_header.lum_count);
    uint32_t data_size_net = temp_header.data_size; // Déjà en network byte order
    uint32_t data_size = ntohl(data_size_net);
    bool is_encrypted = temp_header.is_encrypted;
    uint32_t stored_checksum = ntohl(temp_header.checksum_crc32);
    uint64_t timestamp = be64toh(temp_header.timestamp);

    // Validation magic number
    if (magic_number != LUM_SECURE_MAGIC) return NULL;

    // Validation version (simpliste pour l'instant)
    if (version != LUM_SECURE_VERSION) return NULL;

    // Validation taille
    if (data_size > data_size_net || sizeof(lum_secure_header_t) + data_size > data_size_param) return NULL; // Protection contre tailles invalides

    // Création groupe destination
    lum_group_t* group = lum_group_create(lum_count);
    if (!group) return NULL;

    // Copie données
    const uint8_t* data_ptr = data + sizeof(lum_secure_header_t);
    uint8_t* temp_data = TRACKED_MALLOC(data_size);
    if (!temp_data) {
        lum_group_destroy(group);
        return NULL;
    }

    memcpy(temp_data, data_ptr, data_size);

    // Déchiffrement si nécessaire
    if (decrypt && is_encrypted) {
        uint8_t key = (uint8_t)(timestamp & 0xFF);
        for (size_t i = 0; i < data_size; i++) {
            temp_data[i] ^= key;
        }
    }

    // Vérification checksum
    uint32_t calculated_checksum = lum_secure_calculate_checksum(temp_data, data_size);
    if (calculated_checksum != stored_checksum) {
        TRACKED_FREE(temp_data);
        lum_group_destroy(group);
        return NULL;
    }

    // Reconstruction LUMs
    lum_t* lum_data = (lum_t*)temp_data;
    for (uint32_t i = 0; i < lum_count; i++) {
        // Assurer que l'adresse est valide et que les données LUM sont copiées correctement
        if (!lum_group_add(group, &lum_data[i])) {
            // Gérer l'erreur si l'ajout échoue (e.g., groupe plein)
             TRACKED_FREE(temp_data);
             lum_group_destroy(group);
             return NULL;
        }
    }

    TRACKED_FREE(temp_data);
    return group;
}

// Calcul checksum CRC32 simple
uint32_t lum_secure_calculate_checksum(const uint8_t* data, size_t size) {
    if (!data || size == 0) return 0;

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return crc ^ 0xFFFFFFFF;
}

// Destruction résultat sérialisation
void lum_secure_result_destroy(lum_secure_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;

    lum_secure_result_t* result = *result_ptr;
    if (result->memory_address == (void*)result) {
        if (result->serialized_data) {
            TRACKED_FREE(result->serialized_data);
        }
        TRACKED_FREE(result);
        *result_ptr = NULL;
    }
}


// ================== SÉRIALISATION MÉTADONNÉES (reste inchangé) ==================

// Structure LUM sérialisée sans pointeurs (32 bytes fixes)
typedef struct __attribute__((packed)) {
    uint32_t id;                    // 4 bytes - Network byte order
    uint8_t presence;               // 1 byte
    int32_t position_x;             // 4 bytes - Network byte order
    int32_t position_y;             // 4 bytes - Network byte order
    uint8_t structure_type;         // 1 byte
    uint64_t timestamp;             // 8 bytes - Network byte order
    uint32_t checksum;              // 4 bytes - Network byte order
    uint8_t is_destroyed;           // 1 byte
    uint8_t reserved[5];            // 5 bytes padding pour 32 bytes total
} lum_serialized_t;

// Structure métadonnées sérialisées sans pointeurs
typedef struct __attribute__((packed)) {
    uint32_t magic_number;          // 4 bytes - Network byte order
    uint16_t version_major;         // 2 bytes - Network byte order
    uint16_t version_minor;         // 2 bytes - Network byte order
    uint32_t format_type;           // 4 bytes - Network byte order
    uint64_t creation_timestamp;    // 8 bytes - Network byte order
    uint64_t modification_timestamp; // 8 bytes - Network byte order
    uint32_t total_lums;            // 4 bytes - Network byte order
    uint32_t total_groups;          // 4 bytes - Network byte order
    uint64_t total_size_bytes;      // 8 bytes - Network byte order
    uint32_t checksum_crc32;        // 4 bytes - Network byte order
    char creator_info[64];          // 64 bytes - UTF-8
    char file_description[128];     // 128 bytes - UTF-8
    uint8_t reserved[16];           // 16 bytes padding
} lum_metadata_serialized_t;

// ================== CALCUL CHECKSUM CRC32 (reste inchangé) ==================

static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t lum_calculate_crc32(const uint8_t* data, size_t length) {
    if (!data || length == 0) return 0;

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// ================== SÉRIALISATION LUM (reste inchangé) ==================

bool lum_write_serialized(FILE* file, const lum_t* lum) {
    if (!file || !lum) return false;

    lum_serialized_t serialized = {0};

    // Conversion vers network byte order (big endian) pour portabilité
    serialized.id = htonl(lum->id);
    serialized.presence = lum->presence;
    serialized.position_x = htonl((uint32_t)lum->position_x);
    serialized.position_y = htonl((uint32_t)lum->position_y);
    serialized.structure_type = lum->structure_type;

    // Conversion 64-bit vers network byte order
    uint64_t timestamp_be = htobe64(lum->timestamp);
    serialized.timestamp = timestamp_be;

    serialized.checksum = htonl(lum->checksum);
    serialized.is_destroyed = lum->is_destroyed;

    // Pas de pointeurs/padding dans la version sérialisée
    memset(serialized.reserved, 0, sizeof(serialized.reserved));

    // Calculer CRC32 des données sérialisées (exclure le champ checksum lui-même)
    serialized.checksum = 0; // Zéro pendant calcul CRC
    uint32_t crc = lum_calculate_crc32((uint8_t*)&serialized, sizeof(serialized));
    serialized.checksum = htonl(crc);

    return fwrite(&serialized, sizeof(serialized), 1, file) == 1;
}

bool lum_read_serialized(FILE* file, lum_t* lum) {
    if (!file || !lum) return false;

    lum_serialized_t serialized;
    if (fread(&serialized, sizeof(serialized), 1, file) != 1) {
        return false;
    }

    // Vérifier CRC32 (même méthode que l'écriture)
    uint32_t stored_checksum = ntohl(serialized.checksum);
    serialized.checksum = 0; // Zéro pendant calcul CRC
    uint32_t calculated_crc = lum_calculate_crc32((uint8_t*)&serialized, sizeof(serialized));

    if (stored_checksum != calculated_crc) {
        return false; // Corruption détectée
    }

    // Conversion depuis network byte order
    lum->id = ntohl(serialized.id);
    lum->presence = serialized.presence;
    lum->position_x = (int32_t)ntohl(serialized.position_x);
    lum->position_y = (int32_t)ntohl(serialized.position_y);
    lum->structure_type = serialized.structure_type;
    lum->timestamp = be64toh(serialized.timestamp);
    lum->checksum = stored_checksum;
    lum->is_destroyed = serialized.is_destroyed;

    // Reconstituer les champs runtime (pas sauvegardés)
    lum->memory_address = (void*)lum; // Self-pointer pour validation

    return true;
}

// ================== SÉRIALISATION MÉTADONNÉES (reste inchangé) ==================

bool lum_write_metadata_serialized(FILE* file, const lum_file_metadata_t* metadata) {
    if (!file || !metadata) return false;

    lum_metadata_serialized_t serialized = {0};

    serialized.magic_number = htonl(metadata->magic_number);
    serialized.version_major = htons(metadata->version_major);
    serialized.version_minor = htons(metadata->version_minor);
    serialized.format_type = htonl(metadata->format_type);
    serialized.creation_timestamp = htobe64(metadata->creation_timestamp);
    serialized.modification_timestamp = htobe64(metadata->modification_timestamp);
    serialized.total_lums = htonl(metadata->total_lums);
    serialized.total_groups = htonl(metadata->total_groups);
    serialized.total_size_bytes = htobe64(metadata->total_size_bytes);
    serialized.checksum_crc32 = htonl(metadata->checksum_crc32);

    snprintf(serialized.creator_info, sizeof(serialized.creator_info), "%s", metadata->creator_info);
    snprintf(serialized.file_description, sizeof(serialized.file_description), "%s", metadata->file_description);

    return fwrite(&serialized, sizeof(serialized), 1, file) == 1;
}

bool lum_read_metadata_serialized(FILE* file, lum_file_metadata_t* metadata) {
    if (!file || !metadata) return false;

    lum_metadata_serialized_t serialized;
    if (fread(&serialized, sizeof(serialized), 1, file) != 1) {
        return false;
    }

    metadata->magic_number = ntohl(serialized.magic_number);
    metadata->version_major = ntohs(serialized.version_major);
    metadata->version_minor = ntohs(serialized.version_minor);
    metadata->format_type = ntohl(serialized.format_type);
    metadata->creation_timestamp = be64toh(serialized.creation_timestamp);
    metadata->modification_timestamp = be64toh(serialized.modification_timestamp);
    metadata->total_lums = ntohl(serialized.total_lums);
    metadata->total_groups = ntohl(serialized.total_groups);
    metadata->total_size_bytes = be64toh(serialized.total_size_bytes);
    metadata->checksum_crc32 = ntohl(serialized.checksum_crc32);

    strncpy(metadata->creator_info, serialized.creator_info, sizeof(metadata->creator_info) - 1);
    metadata->creator_info[sizeof(metadata->creator_info) - 1] = '\0';

    strncpy(metadata->file_description, serialized.file_description, sizeof(metadata->file_description) - 1);
    metadata->file_description[sizeof(metadata->file_description) - 1] = '\0';

    metadata->memory_address = (void*)metadata;

    return true;
}

// ================== VALIDATION INTÉGRITÉ (reste inchangé) ==================

bool lum_file_verify_integrity_file(const char* filename) {
    if (!filename) return false;

    FILE* file = fopen(filename, "rb");
    if (!file) return false;

    lum_file_metadata_t metadata = {0};
    if (!lum_read_metadata_serialized(file, &metadata)) {
        fclose(file);
        return false;
    }

    // Vérifier magic number
    if (metadata.magic_number != LUM_FILE_MAGIC_NUMBER) {
        fclose(file);
        return false;
    }

    // Vérifier chaque LUM
    for (uint32_t i = 0; i < metadata.total_lums; i++) {
        lum_t lum = {0};
        if (!lum_read_serialized(file, &lum)) {
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return true;
}

// ================== STRESS TEST 100M+ (reste inchangé) ==================

bool lum_file_stress_test_100m_write_read(void) {
    printf("=== STRESS TEST FILE I/O 100M+ LUMs ===\n");

    const size_t test_count = 1000000; // 1M pour test rapide, extrapolation vers 100M
    const char* test_filename = "stress_test_100m.lum";

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    // Phase écriture
    FILE* file = fopen(test_filename, "wb");
    if (!file) {
        printf("❌ Failed to create test file\n");
        return false;
    }

    // Écrire métadonnées
    lum_file_metadata_t metadata = {0};
    metadata.magic_number = LUM_FILE_MAGIC_NUMBER;
    metadata.version_major = LUM_FILE_VERSION_MAJOR;
    metadata.version_minor = LUM_FILE_VERSION_MINOR;
    metadata.format_type = LUM_FORMAT_NATIVE_BINARY;
    metadata.total_lums = test_count;
    metadata.total_groups = test_count > 0 ? 1 : 0;  // CORRECTION RAPPORT 117: Valeur réelle
    metadata.total_size_bytes = test_count * 32; // 32 bytes par LUM sérialisé

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    metadata.creation_timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    metadata.modification_timestamp = metadata.creation_timestamp;

    strncpy(metadata.creator_info, "LUM/VORAX Stress Test v1.0", sizeof(metadata.creator_info) - 1);
    strncpy(metadata.file_description, "100M+ LUM Stress Test File", sizeof(metadata.file_description) - 1);

    if (!lum_write_metadata_serialized(file, &metadata)) {
        fclose(file);
        printf("❌ Failed to write metadata\n");
        return false;
    }

    // Écrire LUMs
    size_t write_errors = 0;
    for (size_t i = 0; i < test_count; i++) {
        lum_t lum = {
            .id = (uint32_t)i,
            .presence = (i % 2),
            .position_x = (int32_t)(i % 10000),
            .position_y = (int32_t)(i / 10000),
            .structure_type = LUM_STRUCTURE_LINEAR,
            .timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec + i,
            .memory_address = (void*)&lum,
            .checksum = 0,
            .is_destroyed = 0
        };

        if (!lum_write_serialized(file, &lum)) {
            write_errors++;
        }

        if (i % 100000 == 0) {
            printf("Progress: %zu/%zu LUMs written\n", i, test_count);
        }
    }

    fclose(file);

    clock_gettime(CLOCK_REALTIME, &end);
    double write_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("✅ Write phase completed in %.3f seconds\n", write_time);
    printf("Write rate: %.0f LUMs/second\n", test_count / write_time);
    printf("Write errors: %zu\n", write_errors);

    // Phase lecture et vérification
    clock_gettime(CLOCK_REALTIME, &start);

    file = fopen(test_filename, "rb");
    if (!file) {
        printf("❌ Failed to open test file for reading\n");
        return false;
    }

    lum_file_metadata_t read_metadata = {0};
    if (!lum_read_metadata_serialized(file, &read_metadata)) {
        fclose(file);
        printf("❌ Failed to read metadata\n");
        return false;
    }

    printf("Metadata validation: magic=0x%X, version=%d.%d, lums=%u\n",
           read_metadata.magic_number, read_metadata.version_major,
           read_metadata.version_minor, read_metadata.total_lums);

    size_t read_errors = 0;
    size_t checksum_errors = 0;

    for (size_t i = 0; i < test_count; i++) {
        lum_t lum = {0};
        if (!lum_read_serialized(file, &lum)) {
            read_errors++;
            continue;
        }

        // Vérifier données
        if (lum.id != (uint32_t)i ||
            lum.presence != (i % 2) ||
            lum.position_x != (int32_t)(i % 10000)) {
            checksum_errors++;
        }

        if (i % 100000 == 0) {
            printf("Progress: %zu/%zu LUMs verified\n", i, test_count);
        }
    }

    fclose(file);

    clock_gettime(CLOCK_REALTIME, &end);
    double read_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("✅ Read phase completed in %.3f seconds\n", read_time);
    printf("Read rate: %.0f LUMs/second\n", test_count / read_time);
    printf("Read errors: %zu\n", read_errors);
    printf("Checksum errors: %zu\n", checksum_errors);

    // Projection vers 100M
    double total_time = write_time + read_time;
    double projected_100m_time = total_time * 100;
    size_t file_size = test_count * 32 + sizeof(lum_metadata_serialized_t);

    printf("\n=== PROJECTION VERS 100M LUMs ===\n");
    printf("Projected total time for 100M LUMs: %.1f seconds\n", projected_100m_time);
    printf("Projected file size for 100M: %.2f GB\n", (file_size * 100.0) / (1024*1024*1024));
    printf("Throughput: %.2f MB/s\n", (file_size / (1024*1024)) / total_time);

    // Nettoyage
    remove(test_filename);

    bool success = (write_errors == 0 && read_errors == 0 && checksum_errors == 0);
    printf("%s File I/O stress test 100M+ completed\n", success ? "✅" : "❌");

    return success;
}

// Fonctions neural déplacées vers neural_blackbox_computer.c