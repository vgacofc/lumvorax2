/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gpu_async_logger.h — Header logging GPU asynchrone
 *
 * CYCLE C171 — Correction AC-C170-02
 */

#ifndef BTC_GPU_ASYNC_LOGGER_H
#define BTC_GPU_ASYNC_LOGGER_H

#include <stdint.h>
#include <stddef.h>

/* ── Initialisation logger asynchrone ────────────────────────────── */
/* buffer_capacity : Taille ring buffer (recommandé : 262144 = 1 batch GPU) */
int btc_gpu_async_logger_init(size_t buffer_capacity);

/* ── Arrêt propre (flush buffer + join thread) ───────────────────── */
void btc_gpu_async_logger_shutdown(void);

/* ── Log hash GPU (non-bloquant si buffer non plein) ─────────────── */
int btc_gpu_async_log_hash(uint32_t nonce, uint32_t leading_zeros, uint64_t elapsed_ns);

/* ── Nombre d'entries en attente dans buffer ─────────────────────── */
size_t btc_gpu_async_logger_pending(void);

#endif /* BTC_GPU_ASYNC_LOGGER_H */

// Made with Bob
