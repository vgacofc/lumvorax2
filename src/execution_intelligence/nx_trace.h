/*
 * NX-Trace — Execution Intelligence System (EIS)
 * Traçage sémantique d'exécution avec reconstruction causale
 * 
 * Différence clé vs Intel PT :
 *   Intel PT = observe instructions CPU
 *   NX-Trace = comprend INTENTIONS du code
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 * Cycle : C142
 * Date : 2026-04-30
 */

#ifndef NX_TRACE_H
#define NX_TRACE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Niveaux de traçage adaptatifs */
typedef enum {
    NX_TRACE_LEVEL_INSTRUCTION = 0,  /* Équivalent Intel PT (via eBPF) */
    NX_TRACE_LEVEL_FUNCTION    = 1,  /* Appels de fonction */
    NX_TRACE_LEVEL_SEMANTIC    = 2,  /* Intentions (ex: "calcul SHA-256") */
    NX_TRACE_LEVEL_CAUSAL      = 3   /* Graphe causal complet */
} nx_trace_level_t;

/* Type d'événement sémantique */
typedef enum {
    NX_EVENT_FUNCTION_CALL    = 0,
    NX_EVENT_FUNCTION_RETURN  = 1,
    NX_EVENT_LOOP_ITERATION   = 2,
    NX_EVENT_MEMORY_ACCESS    = 3,
    NX_EVENT_BRANCH_TAKEN     = 4,
    NX_EVENT_BRANCH_NOT_TAKEN = 5,
    NX_EVENT_SYSCALL          = 6,
    NX_EVENT_EXCEPTION        = 7,
    NX_EVENT_CUSTOM           = 8
} nx_event_type_t;

/* Événement de trace */
typedef struct {
    uint64_t ts_ns;              /* Timestamp CLOCK_MONOTONIC_RAW */
    uint64_t instruction_ptr;    /* RIP (si disponible) */
    uint64_t memory_address;     /* Adresse mémoire accédée */
    uint32_t thread_id;          /* TID */
    uint32_t cpu_id;             /* CPU core */
    nx_event_type_t event_type;  /* Type d'événement */
    char     semantic_label[64]; /* "SHA256_round_23" */
    char     causal_parent[64];  /* Nœud parent dans graphe */
    uint8_t  memory_delta[32];   /* Delta mémoire (avant/après) */
    uint32_t crc32c;             /* Intégrité CRC32C */
} nx_trace_event_t;

/* Configuration du tracer */
typedef struct {
    nx_trace_level_t level;      /* Niveau de traçage */
    bool enable_memory_delta;    /* Capturer deltas mémoire */
    bool enable_causal_graph;    /* Construire graphe causal */
    uint32_t buffer_size;        /* Taille buffer (events) */
    uint32_t sampling_rate;      /* 1 = tous, 10 = 1/10, etc. */
    char output_path[256];       /* Chemin fichier sortie */
} nx_trace_config_t;

/* Contexte du tracer */
typedef struct {
    nx_trace_config_t config;
    nx_trace_event_t* buffer;
    uint32_t buffer_index;
    uint32_t buffer_size;
    uint32_t total_events;
    uint64_t start_ts_ns;
    bool is_active;
    int output_fd;
} nx_trace_context_t;

/* API publique */

/**
 * Initialise le système de traçage NX-Trace
 * @param config Configuration du tracer
 * @return Contexte alloué ou NULL si erreur
 */
nx_trace_context_t* nx_trace_init(const nx_trace_config_t* config);

/**
 * Démarre le traçage
 * @param ctx Contexte du tracer
 * @return 0 si succès, -1 si erreur
 */
int nx_trace_start(nx_trace_context_t* ctx);

/**
 * Arrête le traçage
 * @param ctx Contexte du tracer
 * @return 0 si succès, -1 si erreur
 */
int nx_trace_stop(nx_trace_context_t* ctx);

/**
 * Enregistre un événement de trace
 * @param ctx Contexte du tracer
 * @param event Événement à enregistrer
 * @return 0 si succès, -1 si erreur
 */
int nx_trace_record(nx_trace_context_t* ctx, const nx_trace_event_t* event);

/**
 * Enregistre un événement sémantique simplifié
 * @param ctx Contexte du tracer
 * @param label Label sémantique
 * @param memory_addr Adresse mémoire (optionnel, 0 si non applicable)
 * @return 0 si succès, -1 si erreur
 */
int nx_trace_record_semantic(nx_trace_context_t* ctx, const char* label, uint64_t memory_addr);

/**
 * Flush le buffer vers le fichier
 * @param ctx Contexte du tracer
 * @return Nombre d'événements écrits, -1 si erreur
 */
int nx_trace_flush(nx_trace_context_t* ctx);

/**
 * Libère les ressources du tracer
 * @param ctx Contexte du tracer
 */
void nx_trace_destroy(nx_trace_context_t* ctx);

/**
 * Obtient les statistiques du tracer
 * @param ctx Contexte du tracer
 * @param total_events Pointeur pour recevoir le nombre total d'événements
 * @param buffer_usage Pointeur pour recevoir l'utilisation du buffer (%)
 * @return 0 si succès, -1 si erreur
 */
int nx_trace_get_stats(const nx_trace_context_t* ctx, uint64_t* total_events, float* buffer_usage);

/**
 * Reconstruit le graphe causal depuis un fichier de trace
 * @param trace_file Chemin du fichier de trace
 * @param output_dot Chemin du fichier DOT de sortie (format Graphviz)
 * @return 0 si succès, -1 si erreur
 */
int nx_trace_reconstruct_causal_graph(const char* trace_file, const char* output_dot);

/**
 * Replay déterministe d'une trace
 * @param trace_file Chemin du fichier de trace
 * @param callback Fonction appelée pour chaque événement
 * @param user_data Données utilisateur passées au callback
 * @return Nombre d'événements rejoués, -1 si erreur
 */
int nx_trace_replay(const char* trace_file, 
                    void (*callback)(const nx_trace_event_t*, void*),
                    void* user_data);

/* Macros de traçage pour instrumentation facile */
#define NX_TRACE_FUNCTION_ENTRY(ctx) \
    nx_trace_record_semantic(ctx, __func__, 0)

#define NX_TRACE_FUNCTION_EXIT(ctx) \
    nx_trace_record_semantic(ctx, __func__ "_exit", 0)

#define NX_TRACE_SEMANTIC(ctx, label) \
    nx_trace_record_semantic(ctx, label, 0)

#define NX_TRACE_MEMORY(ctx, label, addr) \
    nx_trace_record_semantic(ctx, label, (uint64_t)(addr))

#endif /* NX_TRACE_H */

// Made with Bob
