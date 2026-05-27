/* ═══════════════════════════════════════════════════════════════════════════
   BITCOIN MINING KERNEL NATIF C187 — HEADER API
   ═══════════════════════════════════════════════════════════════════════════
   
   CYCLE: C187
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   DESCRIPTION:
   Header pour kernel SHA-256 natif C pur (bypass OpenCL complet)
   
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef BITCOIN_MINING_KERNEL_NATIVE_C187_H
#define BITCOIN_MINING_KERNEL_NATIVE_C187_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
   API PRINCIPALE
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Mine un batch de nonces Bitcoin en C pur natif
 * 
 * @param midstate[8]     État SHA-256 intermédiaire (après premiers 64 octets)
 * @param tail[4]         Derniers 16 octets du block header
 * @param nonce_start     Nonce de départ
 * @param batch_size      Nombre de nonces à tester
 * @param out_nonce       [OUT] Meilleur nonce trouvé
 * @param out_hash[8]     [OUT] Hash correspondant au meilleur nonce
 * @param out_best_bits   [OUT] Nombre de bits leading zeros du meilleur hash
 * 
 * @note Performance CPU: ~3.87 MH/s (Core i5-8250U)
 * @note Utilise SIMD (SSE/AVX) si disponible
 */
void bitcoin_mine_batch_native_c187(
    const uint32_t midstate[8],
    const uint32_t tail[4],
    uint32_t nonce_start,
    uint32_t batch_size,
    uint32_t* out_nonce,
    uint32_t out_hash[8],
    uint32_t* out_best_bits
);

#ifdef __cplusplus
}
#endif

#endif /* BITCOIN_MINING_KERNEL_NATIVE_C187_H */

// Made with Bob
