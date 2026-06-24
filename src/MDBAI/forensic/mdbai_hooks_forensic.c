/**
 * MDBAI Hooks Forensiques - Traçage Bit-Level avec LD_PRELOAD
 * Cycle C192 - Implémentation Hooks malloc/open/pthread
 * 
 * Ce module implémente les hooks forensiques pour traçage transparent
 * via LD_PRELOAD de TOUS les appels système critiques.
 * 
 * Capacités:
 * - Memory tracking (malloc/free/calloc/realloc)
 * - I/O tracking (open/read/write/close)
 * - Thread tracking (pthread_create/pthread_join)
 * - Logs JSON forensiques (forensic_trace.jsonl)
 * - Snapshots forensiques (.mdba avec magic 0x4D444241)
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

/* ============================================================================
 * CONFIGURATION FORENSIQUE
 * ============================================================================ */

#define MDBAI_MAGIC 0x4D444241  /* "MDBA" */
#define FORENSIC_LOG_DEFAULT "/tmp/mdbai_forensic_trace.jsonl"
#define SNAPSHOT_DIR_DEFAULT "/tmp/mdbai_snapshots"
#define MAX_PATH_LEN 4096

/* Variables globales forensiques */
static FILE *g_forensic_log = NULL;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_event_counter = 0;
static int g_forensic_enabled = 1;

/* Pointeurs vers fonctions réelles */
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;
static int (*real_open)(const char*, int, ...) = NULL;
static ssize_t (*real_read)(int, void*, size_t) = NULL;
static ssize_t (*real_write)(int, const void*, size_t) = NULL;
static int (*real_close)(int) = NULL;
static int (*real_pthread_create)(pthread_t*, const pthread_attr_t*, void*(*)(void*), void*) = NULL;
static int (*real_pthread_join)(pthread_t, void**) = NULL;

/* ============================================================================
 * UTILITAIRES FORENSIQUES
 * ============================================================================ */

/**
 * Obtenir timestamp nanoseconde
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * Initialiser log forensique
 */
static void init_forensic_log(void) {
    if (g_forensic_log != NULL) return;
    
    const char *log_path = getenv("MDBAI_LOG_FILE");
    if (log_path == NULL) {
        log_path = FORENSIC_LOG_DEFAULT;
    }
    
    g_forensic_log = fopen(log_path, "a");
    if (g_forensic_log == NULL) {
        fprintf(stderr, "[MDBAI] ERREUR: Impossible d'ouvrir %s\n", log_path);
        g_forensic_enabled = 0;
        return;
    }
    
    /* Désactiver buffering pour logs temps réel */
    setbuf(g_forensic_log, NULL);
    
    fprintf(stderr, "[MDBAI] Forensic tracking initialized: %s\n", log_path);
}

/**
 * Logger événement forensique JSON
 */
static void log_forensic_event(const char *event_type, const char *details) {
    if (!g_forensic_enabled || g_forensic_log == NULL) return;
    
    pthread_mutex_lock(&g_log_mutex);
    
    uint64_t timestamp = get_timestamp_ns();
    uint64_t event_id = ++g_event_counter;
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    
    fprintf(g_forensic_log,
            "{\"event_id\":%lu,\"timestamp_ns\":%lu,\"pid\":%d,\"tid\":%lu,"
            "\"event_type\":\"%s\",\"details\":{%s}}\n",
            event_id, timestamp, pid, (unsigned long)tid, event_type, details);
    
    pthread_mutex_unlock(&g_log_mutex);
}

/**
 * Créer snapshot forensique .mdba
 */
static void create_forensic_snapshot(const char *description, const void *data, size_t data_size) {
    const char *snapshot_dir = getenv("MDBAI_SNAPSHOT_DIR");
    if (snapshot_dir == NULL) {
        snapshot_dir = SNAPSHOT_DIR_DEFAULT;
    }
    
    /* Créer répertoire si nécessaire */
    mkdir(snapshot_dir, 0755);
    
    /* Nom fichier avec timestamp */
    char filename[MAX_PATH_LEN];
    uint64_t timestamp = get_timestamp_ns();
    snprintf(filename, sizeof(filename), "%s/snapshot_%lu.mdba", snapshot_dir, timestamp);
    
    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        fprintf(stderr, "[MDBAI] ERREUR: Impossible de créer snapshot %s\n", filename);
        return;
    }
    
    /* Header .mdba */
    struct {
        uint32_t magic;
        uint32_t version;
        uint64_t timestamp_ns;
        uint32_t data_size;
        char description[64];
    } header = {
        .magic = MDBAI_MAGIC,
        .version = 1,
        .timestamp_ns = timestamp,
        .data_size = (uint32_t)data_size,
    };
    strncpy(header.description, description, sizeof(header.description) - 1);
    
    fwrite(&header, sizeof(header), 1, f);
    if (data != NULL && data_size > 0) {
        fwrite(data, 1, data_size, f);
    }
    
    fclose(f);
    
    fprintf(stderr, "[MDBAI] Snapshot créé: %s (%zu bytes)\n", filename, data_size);
}

/* ============================================================================
 * HOOKS MEMORY TRACKING
 * ============================================================================ */

void* malloc(size_t size) {
    if (real_malloc == NULL) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
    }
    
    void *ptr = real_malloc(size);
    
    if (g_forensic_enabled && ptr != NULL) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"size\":%zu,\"ptr\":\"%p\"", size, ptr);
        log_forensic_event("malloc", details);
    }
    
    return ptr;
}

void free(void *ptr) {
    if (real_free == NULL) {
        real_free = dlsym(RTLD_NEXT, "free");
    }
    
    if (g_forensic_enabled && ptr != NULL) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"ptr\":\"%p\"", ptr);
        log_forensic_event("free", details);
    }
    
    real_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    if (real_calloc == NULL) {
        real_calloc = dlsym(RTLD_NEXT, "calloc");
    }
    
    void *ptr = real_calloc(nmemb, size);
    
    if (g_forensic_enabled && ptr != NULL) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"nmemb\":%zu,\"size\":%zu,\"total\":%zu,\"ptr\":\"%p\"",
                 nmemb, size, nmemb * size, ptr);
        log_forensic_event("calloc", details);
    }
    
    return ptr;
}

void* realloc(void *ptr, size_t size) {
    if (real_realloc == NULL) {
        real_realloc = dlsym(RTLD_NEXT, "realloc");
    }
    
    void *old_ptr = ptr;
    void *new_ptr = real_realloc(ptr, size);
    
    if (g_forensic_enabled) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"old_ptr\":\"%p\",\"new_ptr\":\"%p\",\"size\":%zu",
                 old_ptr, new_ptr, size);
        log_forensic_event("realloc", details);
    }
    
    return new_ptr;
}

/* ============================================================================
 * HOOKS I/O TRACKING
 * ============================================================================ */

int open(const char *pathname, int flags, ...) {
    if (real_open == NULL) {
        real_open = dlsym(RTLD_NEXT, "open");
    }
    
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    
    int fd = real_open(pathname, flags, mode);
    
    if (g_forensic_enabled && fd >= 0) {
        char details[512];
        snprintf(details, sizeof(details),
                 "\"pathname\":\"%s\",\"flags\":%d,\"mode\":%o,\"fd\":%d",
                 pathname, flags, mode, fd);
        log_forensic_event("open", details);
    }
    
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    if (real_read == NULL) {
        real_read = dlsym(RTLD_NEXT, "read");
    }
    
    ssize_t result = real_read(fd, buf, count);
    
    if (g_forensic_enabled && result > 0) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"fd\":%d,\"count\":%zu,\"bytes_read\":%zd",
                 fd, count, result);
        log_forensic_event("read", details);
        
        /* Créer snapshot pour lectures importantes (>1KB) */
        if (result > 1024) {
            char desc[64];
            snprintf(desc, sizeof(desc), "read_fd%d_%zd_bytes", fd, result);
            create_forensic_snapshot(desc, buf, (size_t)result);
        }
    }
    
    return result;
}

ssize_t write(int fd, const void *buf, size_t count) {
    if (real_write == NULL) {
        real_write = dlsym(RTLD_NEXT, "write");
    }
    
    ssize_t result = real_write(fd, buf, count);
    
    if (g_forensic_enabled && result > 0) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"fd\":%d,\"count\":%zu,\"bytes_written\":%zd",
                 fd, count, result);
        log_forensic_event("write", details);
        
        /* Créer snapshot pour écritures importantes (>1KB) */
        if (result > 1024) {
            char desc[64];
            snprintf(desc, sizeof(desc), "write_fd%d_%zd_bytes", fd, result);
            create_forensic_snapshot(desc, buf, (size_t)result);
        }
    }
    
    return result;
}

int close(int fd) {
    if (real_close == NULL) {
        real_close = dlsym(RTLD_NEXT, "close");
    }
    
    if (g_forensic_enabled) {
        char details[256];
        snprintf(details, sizeof(details), "\"fd\":%d", fd);
        log_forensic_event("close", details);
    }
    
    return real_close(fd);
}

/* ============================================================================
 * HOOKS THREAD TRACKING
 * ============================================================================ */

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void*), void *arg) {
    if (real_pthread_create == NULL) {
        real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
    }
    
    int result = real_pthread_create(thread, attr, start_routine, arg);
    
    if (g_forensic_enabled && result == 0) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"thread_id\":%lu,\"start_routine\":\"%p\",\"arg\":\"%p\"",
                 (unsigned long)*thread, (void*)start_routine, arg);
        log_forensic_event("pthread_create", details);
    }
    
    return result;
}

int pthread_join(pthread_t thread, void **retval) {
    if (real_pthread_join == NULL) {
        real_pthread_join = dlsym(RTLD_NEXT, "pthread_join");
    }
    
    if (g_forensic_enabled) {
        char details[256];
        snprintf(details, sizeof(details),
                 "\"thread_id\":%lu", (unsigned long)thread);
        log_forensic_event("pthread_join", details);
    }
    
    return real_pthread_join(thread, retval);
}

/* ============================================================================
 * INITIALISATION / FINALISATION
 * ============================================================================ */

/**
 * Constructeur - Appelé automatiquement au chargement de la bibliothèque
 */
void __attribute__((constructor)) mdbai_forensic_init(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, "╔════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║  MDBAI Forensic Tracking - Cycle C192                         ║\n");
    fprintf(stderr, "║  Hooks: malloc/free/calloc/realloc/open/read/write/pthread    ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════════╝\n");
    fprintf(stderr, "\n");
    
    init_forensic_log();
    
    /* Logger événement initialisation */
    char details[256];
    snprintf(details, sizeof(details),
             "\"version\":\"1.0.0\",\"magic\":\"0x%08X\"", MDBAI_MAGIC);
    log_forensic_event("mdbai_init", details);
    
    fprintf(stderr, "[MDBAI] Forensic tracking ACTIVÉ\n");
    fprintf(stderr, "[MDBAI] Logs: %s\n", 
            getenv("MDBAI_LOG_FILE") ? getenv("MDBAI_LOG_FILE") : FORENSIC_LOG_DEFAULT);
    fprintf(stderr, "[MDBAI] Snapshots: %s\n",
            getenv("MDBAI_SNAPSHOT_DIR") ? getenv("MDBAI_SNAPSHOT_DIR") : SNAPSHOT_DIR_DEFAULT);
    fprintf(stderr, "\n");
}

/**
 * Destructeur - Appelé automatiquement au déchargement de la bibliothèque
 */
void __attribute__((destructor)) mdbai_forensic_fini(void) {
    if (g_forensic_log != NULL) {
        /* Logger événement finalisation */
        char details[256];
        snprintf(details, sizeof(details),
                 "\"total_events\":%lu", g_event_counter);
        log_forensic_event("mdbai_fini", details);
        
        fprintf(stderr, "\n[MDBAI] Forensic tracking DÉSACTIVÉ\n");
        fprintf(stderr, "[MDBAI] Total événements tracés: %lu\n", g_event_counter);
        
        fclose(g_forensic_log);
        g_forensic_log = NULL;
    }
}

// Made with Bob
