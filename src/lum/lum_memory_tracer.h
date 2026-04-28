/* ============================================================================
 * LUM Memory Tracer — Cycle C111
 * ----------------------------------------------------------------------------
 * Traçage exhaustif de la mémoire d'un process Linux au format LUM 100%.
 *
 * Granularités supportées :
 *   - LUM_TRACE_GRANULARITY_PAGE   : 1 LUM par page 4 KiB (snapshot léger)
 *   - LUM_TRACE_GRANULARITY_BYTE   : 1 LUM par octet (dump intégral)
 *   - LUM_TRACE_GRANULARITY_BIT    : 1 LUM par bit (dump bit-par-bit complet)
 *
 * Périmètre :
 *   - Mémoire process self (via /proc/self/{maps,pagemap,mem})
 *   - Mémoire process tier (via /proc/<pid>/{maps,pagemap,mem}, capability ptrace)
 *
 * Limites assumées (NON un bug, vraie contrainte technique documentée) :
 *   - Mémoire kernel : nécessite /proc/kcore + root → cf. lum_kernel_tracer.c (C112)
 *   - Mémoire machine totale : nécessite hyperviseur ou DMA → hors portée userspace
 *   - Pages swap-out : sortie disque requise (PFN==0 dans pagemap)
 *
 * Format de sortie : fichier binaire .lum (séquence de lum_t alignés 64 octets)
 * Reconstruction garantie diff=0 si le process est gelé pendant snapshot+restore.
 * ============================================================================ */
#ifndef LUM_MEMORY_TRACER_H_INCLUDED
#define LUM_MEMORY_TRACER_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lum_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LUM_TRACE_GRANULARITY_PAGE     = 0,  /* 1 LUM = 1 page de 4096 octets */
    LUM_TRACE_GRANULARITY_BYTE     = 1,  /* 1 LUM = 1 octet                */
    LUM_TRACE_GRANULARITY_BIT      = 2,  /* 1 LUM = 1 bit (présence bit)   */
    LUM_TRACE_GRANULARITY_HUGEPAGE = 3   /* 1 LUM = 1 huge page de 2 MiB (C114) */
} lum_trace_granularity_t;

typedef struct {
    uint64_t total_lums_emitted;     /* Nombre total de lum_t écrits      */
    uint64_t total_pages_scanned;    /* Pages VMA scannées                */
    uint64_t total_pages_resident;   /* Pages avec PFN > 0                */
    uint64_t total_bytes_dumped;     /* Octets effectivement écrits       */
    uint64_t snapshot_ns;            /* Durée snapshot en nanosecondes    */
    uint32_t magic;                  /* 0x4C554D54 ('LUMT')               */
} lum_trace_stats_t;

/* ----------------------------------------------------------------------------
 * Snapshot mémoire process self → fichier .lum
 *
 * Paramètres :
 *   out_path        : chemin du fichier de sortie (sera créé/écrasé)
 *   granularity     : niveau de granularité (cf. enum)
 *   include_anon    : inclure les zones [anon] (heap, stack, mmap anonymes)
 *   include_files   : inclure les zones mappées sur fichier (.so, exec, data)
 *   stats           : (out) statistiques du snapshot, NULL accepté
 *
 * Retourne : 0 succès, -errno sinon.
 * Thread safety : NON re-entrant (utilise /proc/self/mem qui est sériel)
 * ---------------------------------------------------------------------------- */
int lum_memory_snapshot_self(const char* out_path,
                              lum_trace_granularity_t granularity,
                              bool include_anon,
                              bool include_files,
                              lum_trace_stats_t* stats);

/* ----------------------------------------------------------------------------
 * Reconstruction mémoire depuis .lum dans un buffer cible
 *
 * Paramètres :
 *   in_path         : fichier .lum à relire
 *   target_buffer   : buffer mémoire à remplir (taille >= snapshot_size)
 *   target_size     : taille du buffer cible (octets)
 *   bytes_restored  : (out) nombre d'octets restaurés
 *
 * Retourne : 0 succès (diff=0 garanti), -errno sinon.
 * Note : la reconstruction préserve les adresses virtuelles d'origine dans
 *        lum_t.memory_address mais écrit séquentiellement dans target_buffer.
 * ---------------------------------------------------------------------------- */
int lum_memory_reconstruct(const char* in_path,
                            void* target_buffer,
                            size_t target_size,
                            size_t* bytes_restored);

/* ----------------------------------------------------------------------------
 * Validation diff=0 entre 2 snapshots (snapshot ↔ reconstruction)
 *
 * Paramètres :
 *   original_path   : fichier .lum d'origine
 *   reconstructed   : buffer mémoire reconstruit
 *   reconstructed_size : taille du buffer reconstruit
 *   diff_count      : (out) nombre d'octets différents (0 = parfait)
 *
 * Retourne : 0 si validation OK et diff=0, -1 sinon.
 * ---------------------------------------------------------------------------- */
int lum_memory_validate_diff_zero(const char* original_path,
                                   const void* reconstructed,
                                   size_t reconstructed_size,
                                   uint64_t* diff_count);

/* ── C116-KERNEL : smaps_rollup — accès kernel /proc/self/smaps_rollup ───── */
typedef struct {
    uint64_t rss_kb;
    uint64_t anon_huge_kb;
    uint64_t private_dirty_kb;
    uint64_t shared_dirty_kb;
    uint64_t referenced_kb;
    uint64_t vm_size_kb;
    uint64_t huge_pages_count;
    uint64_t page_count;
} lum_smaps_rollup_t;

int  lum_memory_smaps_rollup(lum_smaps_rollup_t *smaps);
void lum_memory_smaps_print(const lum_smaps_rollup_t *smaps);

#ifdef __cplusplus
}
#endif

#endif /* LUM_MEMORY_TRACER_H_INCLUDED */
