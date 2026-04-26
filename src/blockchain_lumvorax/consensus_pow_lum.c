/* consensus_pow_lum.c — Consensus PoW LUMVORAX (C99 Q6)
 *
 * Algorithme : Proof-of-Work double-SHA256 style Bitcoin, MAIS la difficulté est
 * exprimée en LZ (leading zeros sur le hash) plutôt qu'en target compact.
 *
 *   bloc valide ⇔ leading_zeros(double_sha256(header)) ≥ difficulty_lz
 *
 * Ajustement de difficulté toutes les 16 blocs (DIFFICULTY_WINDOW) :
 *   - Si bloc moyen miné en < TARGET_BLOCK_TIME / 2 → +1 LZ
 *   - Si bloc moyen miné en > TARGET_BLOCK_TIME × 2 → -1 LZ
 *   - Sinon difficulté inchangée
 *
 * Difficulté MIN = 8 LZ, MAX = 64 LZ (anti-runaway).
 */
#include "blockchain_lumvorax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

extern int lumvorax_genesis_compute_hash(const block_header_t* hdr, uint8_t out[32]);
extern int lumvorax_genesis_count_lz(const uint8_t hash[32]);

#define POW_DIFFICULTY_MIN     8
#define POW_DIFFICULTY_MAX     64
#define POW_TARGET_BLOCK_TIME  10  /* secondes */
#define POW_DIFFICULTY_WINDOW  16  /* recalcul tous les 16 blocs */

static int g_current_difficulty_lz = POW_DIFFICULTY_MIN;
static uint32_t g_window_timestamps[POW_DIFFICULTY_WINDOW] = {0};
static int g_window_idx = 0;
static int g_window_filled = 0;

int lumvorax_pow_get_difficulty(void) { return g_current_difficulty_lz; }

void lumvorax_pow_reset(int initial_lz) {
    g_current_difficulty_lz = (initial_lz >= POW_DIFFICULTY_MIN && initial_lz <= POW_DIFFICULTY_MAX)
                              ? initial_lz : POW_DIFFICULTY_MIN;
    g_window_idx = 0;
    g_window_filled = 0;
    memset(g_window_timestamps, 0, sizeof(g_window_timestamps));
}

/* Vérifie qu'un header satisfait la difficulté courante */
int lumvorax_pow_validate(const block_header_t* hdr) {
    if (!hdr) return 0;
    uint8_t hash[32];
    if (lumvorax_genesis_compute_hash(hdr, hash) != 0) return 0;
    int lz = lumvorax_genesis_count_lz(hash);
    return lz >= g_current_difficulty_lz ? 1 : 0;
}

/* Met à jour difficulté après ajout d'un nouveau bloc valide */
int lumvorax_pow_record_block(uint32_t timestamp) {
    g_window_timestamps[g_window_idx] = timestamp;
    g_window_idx = (g_window_idx + 1) % POW_DIFFICULTY_WINDOW;
    if (g_window_filled < POW_DIFFICULTY_WINDOW) g_window_filled++;
    if (g_window_filled < POW_DIFFICULTY_WINDOW) return g_current_difficulty_lz;
    /* Calcule durée moyenne dans la fenêtre */
    uint32_t t_min = 0xFFFFFFFFU, t_max = 0;
    for (int i = 0; i < POW_DIFFICULTY_WINDOW; i++) {
        if (g_window_timestamps[i] < t_min) t_min = g_window_timestamps[i];
        if (g_window_timestamps[i] > t_max) t_max = g_window_timestamps[i];
    }
    uint32_t span = t_max - t_min;
    double avg_block_time = (double)span / (POW_DIFFICULTY_WINDOW - 1);
    if (avg_block_time < (double)POW_TARGET_BLOCK_TIME / 2.0) {
        if (g_current_difficulty_lz < POW_DIFFICULTY_MAX) g_current_difficulty_lz++;
    } else if (avg_block_time > (double)POW_TARGET_BLOCK_TIME * 2.0) {
        if (g_current_difficulty_lz > POW_DIFFICULTY_MIN) g_current_difficulty_lz--;
    }
    return g_current_difficulty_lz;
}

/* Mine un bloc : cherche un nonce qui satisfait difficulty.
 * Retourne 1 si trouvé (hdr->nonce mis à jour), 0 si épuisement ou max_iter atteint.
 */
int lumvorax_pow_mine(block_header_t* hdr, uint64_t max_iter) {
    if (!hdr) return 0;
    int target = g_current_difficulty_lz;
    uint64_t iter = 0;
    for (uint32_t n = hdr->nonce; iter < max_iter; n++, iter++) {
        hdr->nonce = n;
        uint8_t hash[32];
        if (lumvorax_genesis_compute_hash(hdr, hash) != 0) return 0;
        if (lumvorax_genesis_count_lz(hash) >= target) return 1;
        if (n == 0xFFFFFFFFU) break;  /* nonce épuisé */
    }
    return 0;
}
