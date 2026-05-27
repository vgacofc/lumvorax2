/* chain.c — Chaîne LUMVORAX bout-en-bout (C99 Q6)
 *
 * Persistance append-only : chaque bloc ajouté est sérialisé en 80 + 32 octets
 * (header + prev_hash) dans config/lumvorax_chain.dat.
 *
 * API :
 *   lumvorax_chain_open(path)        → ouvre/initialise la chaîne
 *   lumvorax_chain_append(blk)       → vérifie + persiste un bloc
 *   lumvorax_chain_get_tip(out)      → retourne le dernier hash
 *   lumvorax_chain_height()          → nombre de blocs
 *   lumvorax_chain_verify_all()      → re-vérifie toute la chaîne
 *   lumvorax_chain_close()           → ferme proprement
 */
#include "blockchain_lumvorax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>

extern void sha256_lumvorax(const uint8_t* data, size_t len, uint8_t out[32]);
extern int  lumvorax_genesis_create(block_header_t* hdr);
extern int  lumvorax_genesis_compute_hash(const block_header_t* hdr, uint8_t out[32]);
extern int  lumvorax_genesis_count_lz(const uint8_t hash[32]);

static char    g_chain_path[512] = {0};
static FILE*   g_chain_file = NULL;
static uint64_t g_chain_height = 0;
static uint8_t  g_chain_tip[32] = {0};

#define BLOCK_RECORD_BYTES 112  /* 80 header + 32 hash */

static int chain_serialize_header(const block_header_t* hdr, uint8_t buf[80]) {
    if (!hdr) return -1;
    memcpy(buf + 0,  &hdr->version, 4);
    memcpy(buf + 4,  hdr->prev_hash, 32);
    memcpy(buf + 36, hdr->merkle_root, 32);
    memcpy(buf + 68, &hdr->timestamp, 4);
    memcpy(buf + 72, &hdr->bits, 4);
    memcpy(buf + 76, &hdr->nonce, 4);
    return 0;
}

static int chain_deserialize_header(const uint8_t buf[80], block_header_t* hdr) {
    if (!hdr) return -1;
    memcpy(&hdr->version, buf + 0, 4);
    memcpy(hdr->prev_hash, buf + 4, 32);
    memcpy(hdr->merkle_root, buf + 36, 32);
    memcpy(&hdr->timestamp, buf + 68, 4);
    memcpy(&hdr->bits, buf + 72, 4);
    memcpy(&hdr->nonce, buf + 76, 4);
    return 0;
}

int lumvorax_chain_open(const char* path) {
    if (!path) return -1;
    snprintf(g_chain_path, sizeof(g_chain_path), "%s", path);
    /* Crée si absent */
    struct stat st;
    int exists = (stat(path, &st) == 0 && st.st_size > 0);
    if (!exists) {
        /* Init avec genesis */
        block_header_t gen;
        if (lumvorax_genesis_create(&gen) != 0) return -2;
        /* Mine genesis (rapide) */
        for (uint32_t n = 0; n < 1000000U; n++) {
            gen.nonce = n;
            uint8_t h[32];
            lumvorax_genesis_compute_hash(&gen, h);
            if (lumvorax_genesis_count_lz(h) >= 8) break;
        }
        /* Persiste */
        FILE* f = fopen(path, "wb");
        if (!f) return -3;
        uint8_t hdr_buf[80], hash_buf[32];
        chain_serialize_header(&gen, hdr_buf);
        lumvorax_genesis_compute_hash(&gen, hash_buf);
        fwrite(hdr_buf, 1, 80, f);
        fwrite(hash_buf, 1, 32, f);
        fclose(f);
        memcpy(g_chain_tip, hash_buf, 32);
        g_chain_height = 1;
    } else {
        /* Charge : lit dernier record */
        FILE* f = fopen(path, "rb");
        if (!f) return -4;
        uint64_t n_blocks = (uint64_t)st.st_size / BLOCK_RECORD_BYTES;
        if (st.st_size % BLOCK_RECORD_BYTES != 0) {
            fclose(f);
            return -5;  /* fichier corrompu */
        }
        if (n_blocks > 0) {
            fseek(f, (long)((n_blocks - 1) * BLOCK_RECORD_BYTES + 80), SEEK_SET);
            if (fread(g_chain_tip, 1, 32, f) != 32) {
                fclose(f);
                return -6;
            }
        }
        g_chain_height = n_blocks;
        fclose(f);
    }
    g_chain_file = fopen(path, "ab");
    return g_chain_file ? 0 : -7;
}

int lumvorax_chain_append(const block_header_t* hdr) {
    if (!hdr || !g_chain_file) return -1;
    /* Vérifie chaînage prev_hash == tip */
    if (g_chain_height > 0 && memcmp(hdr->prev_hash, g_chain_tip, 32) != 0) {
        return -2;  /* chaînage cassé */
    }
    uint8_t buf80[80], hash32[32];
    chain_serialize_header(hdr, buf80);
    lumvorax_genesis_compute_hash(hdr, hash32);
    if (fwrite(buf80, 1, 80, g_chain_file) != 80) return -3;
    if (fwrite(hash32, 1, 32, g_chain_file) != 32) return -4;
    fflush(g_chain_file);
    memcpy(g_chain_tip, hash32, 32);
    g_chain_height++;
    return 0;
}

int lumvorax_chain_get_tip(uint8_t out[32]) {
    if (!out) return -1;
    memcpy(out, g_chain_tip, 32);
    return (int)g_chain_height;
}

uint64_t lumvorax_chain_height(void) {
    return g_chain_height;
}

int lumvorax_chain_verify_all(void) {
    if (g_chain_path[0] == 0) return -1;
    FILE* f = fopen(g_chain_path, "rb");
    if (!f) return -2;
    uint8_t prev_hash[32] = {0};
    uint64_t verified = 0;
    while (1) {
        uint8_t buf80[80], hash32[32];
        if (fread(buf80, 1, 80, f) != 80) break;
        if (fread(hash32, 1, 32, f) != 32) { fclose(f); return -3; }
        block_header_t hdr;
        chain_deserialize_header(buf80, &hdr);
        if (verified > 0 && memcmp(hdr.prev_hash, prev_hash, 32) != 0) {
            fclose(f);
            return -4;  /* cassure chaîne */
        }
        uint8_t recomputed[32];
        lumvorax_genesis_compute_hash(&hdr, recomputed);
        if (memcmp(recomputed, hash32, 32) != 0) {
            fclose(f);
            return -5;  /* hash falsifié */
        }
        memcpy(prev_hash, hash32, 32);
        verified++;
    }
    fclose(f);
    return (int)verified;
}

void lumvorax_chain_close(void) {
    if (g_chain_file) {
        fclose(g_chain_file);
        g_chain_file = NULL;
    }
}
