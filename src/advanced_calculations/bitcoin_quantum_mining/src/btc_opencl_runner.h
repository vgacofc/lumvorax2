/*
 * LumVorax C63 — btc_opencl_runner.h
 * =====================================
 * Interface OpenCL SHA-256 double-hash pour Intel UHD 620.
 * STANDARD_NAMES.md v4.4 §M-BTC-GPU-C63
 */
#ifndef BTC_OPENCL_RUNNER_H
#define BTC_OPENCL_RUNNER_H

#include <stdint.h>
#include <stddef.h>

#ifdef BTC_OPENCL_ENABLED
/* C119-P2.3 — Cibler OpenCL 2.0 explicitement.
 * Sans CL_TARGET_OPENCL_VERSION, le header CL/cl.h émet un warning et
 * tombe par défaut sur 1.2, ce qui désactive certaines features (atomic
 * fetch_add 64 bits, SVM). 200 = 2.0, supporté par Intel UHD 620 driver. */
#  ifndef CL_TARGET_OPENCL_VERSION
#    define CL_TARGET_OPENCL_VERSION 200
#  endif
#  ifdef __APPLE__
#    include <OpenCL/opencl.h>
#  else
#    include <CL/cl.h>
#  endif
#endif

/* Codes de retour */
#define BTC_OCL_OK            0
#define BTC_OCL_ERR_NO_DEVICE 1
#define BTC_OCL_ERR_BUILD     2
#define BTC_OCL_ERR_RUNTIME   3

/* Batch par défaut : 262144 nonces (64 workgroups × 4096 items) */
#define BTC_OCL_BATCH_DEFAULT (262144u)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * btc_opencl_init — Initialiser OpenCL (device, programme, buffers)
 * batch_size : nombre de nonces par dispatch (0 = défaut 262144)
 * Retourne BTC_OCL_OK ou code erreur.
 */
int btc_opencl_init(size_t batch_size);

/*
 * btc_opencl_mine_batch — Lancer un batch de mining sur GPU
 * midstate     : [8 uint32] état SHA-256 après bloc 0 du header
 * tail         : [4 uint32] bytes 64..75 du header BTC (sans nonce)
 * nonce_start  : nonce de départ de ce batch
 * batch_size   : nonces à tester (0 = g_batch_size)
 * target_bits  : bits zéro requis
 * out_nonce    : nonce trouvé (0xFFFFFFFF si aucun)
 * out_hash     : [8 uint32] hash final (si nonce trouvé)
 * out_best_bits: meilleur near-miss du batch (NULL si pas voulu)
 * Retourne BTC_OCL_OK ou code erreur.
 */
int btc_opencl_mine_batch(
    const uint32_t* midstate,
    const uint32_t* tail,
    uint32_t        nonce_start,
    size_t          batch_size,
    uint32_t        target_bits,
    uint32_t*       out_nonce,
    uint32_t*       out_hash,
    uint32_t*       out_best_bits
);

/*
 * btc_opencl_benchmark_mhs — Mesure le hashrate GPU sur 1 batch
 * Retourne MH/s ou 0.0 si OpenCL indisponible.
 */
double btc_opencl_benchmark_mhs(size_t batch_size);

/*
 * btc_opencl_cleanup — Libérer toutes les ressources OpenCL
 */
void btc_opencl_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* BTC_OPENCL_RUNNER_H */
