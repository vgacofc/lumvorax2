/* lvx_log_rotate.c — Implémentation rotation log 50 Mo (C99 universelle). */
#define _XOPEN_SOURCE 700
#include "lvx_log_rotate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

/* ───────────────────────────────────────────────────────────────────────
 * Helpers internes
 * ─────────────────────────────────────────────────────────────────────── */
static long long file_size_bytes(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (long long)st.st_size;
}

/* Construit timestamp YYYYMMDDHHMMSS (UTC) */
static void utc_timestamp_compact(char *buf, size_t bufsz) {
    time_t t = time(NULL);
    struct tm tm_buf;
    gmtime_r(&t, &tm_buf);
    strftime(buf, bufsz, "%Y%m%dT%H%M%SZ", &tm_buf);
}

/* Compare 2 chaînes pour qsort */
static int cstr_cmp(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* Purge les vieilles archives (garde au plus LVX_LOG_ROTATE_KEEP_N). */
static void purge_old_archives(const char *base_path) {
    /* base_path = ".../my.log" → on cherche ".../my.log.*" dans le même dir */
    char dir_path[1024], file_name[256];
    const char *slash = strrchr(base_path, '/');
    if (slash) {
        size_t dlen = (size_t)(slash - base_path);
        if (dlen >= sizeof(dir_path)) dlen = sizeof(dir_path) - 1;
        memcpy(dir_path, base_path, dlen);
        dir_path[dlen] = '\0';
        snprintf(file_name, sizeof(file_name), "%s", slash + 1);
    } else {
        snprintf(dir_path, sizeof(dir_path), ".");
        snprintf(file_name, sizeof(file_name), "%s", base_path);
    }

    DIR *d = opendir(dir_path);
    if (!d) return;

    char *matches[256];
    int n_matches = 0;
    struct dirent *de;
    size_t name_len = strlen(file_name);
    while ((de = readdir(d)) != NULL && n_matches < 256) {
        if (strncmp(de->d_name, file_name, name_len) != 0) continue;
        if (strlen(de->d_name) <= name_len + 1) continue;
        if (de->d_name[name_len] != '.') continue; /* préfixe + "." attendu */
        /* candidat archive */
        size_t need = strlen(dir_path) + 1 + strlen(de->d_name) + 1;
        char *full = (char *)malloc(need);
        if (!full) continue;
        snprintf(full, need, "%s/%s", dir_path, de->d_name);
        matches[n_matches++] = full;
    }
    closedir(d);

    if (n_matches > LVX_LOG_ROTATE_KEEP_N) {
        qsort(matches, (size_t)n_matches, sizeof(matches[0]), cstr_cmp);
        int n_to_delete = n_matches - LVX_LOG_ROTATE_KEEP_N;
        for (int i = 0; i < n_to_delete; ++i) {
            unlink(matches[i]);
        }
    }
    for (int i = 0; i < n_matches; ++i) free(matches[i]);
}

/* Effectue la rotation : renomme path → path.YYYYMMDDHHMMSS */
static int do_rotate(const char *path) {
    char ts[32];
    utc_timestamp_compact(ts, sizeof(ts));
    char new_path[1280];
    snprintf(new_path, sizeof(new_path), "%s.%s", path, ts);
    if (rename(path, new_path) != 0) {
        return -1;
    }
    purge_old_archives(path);
    return 0;
}

/* ───────────────────────────────────────────────────────────────────────
 * API publique
 * ─────────────────────────────────────────────────────────────────────── */
int lvx_log_check_rotate(const char *path) {
    if (!path) return -1;
    long long sz = file_size_bytes(path);
    if (sz < 0) return 0;  /* fichier n'existe pas → rien à faire */
    if ((unsigned long long)sz < LVX_LOG_ROTATE_MAX_BYTES) return 0;
    int rc = do_rotate(path);
    return (rc == 0) ? 1 : -1;
}

int lvx_log_force_rotate(const char *path) {
    if (!path) return -1;
    long long sz = file_size_bytes(path);
    if (sz < 0) return 0;
    return do_rotate(path);
}

int lvx_log_append(const char *path, const char *data, size_t len) {
    if (!path || !data) return -1;

    /* Vérification rotation AVANT écriture (économie : 1 stat() seulement) */
    long long sz = file_size_bytes(path);
    if (sz >= 0 && (unsigned long long)sz >= LVX_LOG_ROTATE_MAX_BYTES) {
        do_rotate(path);  /* échec ignoré : on essaie d'écrire quand même */
    }

    /* Ouverture O_APPEND avec verrou flock partagé writer-exclusif */
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return -1;
    if (flock(fd, LOCK_EX) != 0) {
        close(fd);
        return -1;
    }
    ssize_t total = 0;
    while ((size_t)total < len) {
        ssize_t w = write(fd, data + total, len - (size_t)total);
        if (w < 0) {
            if (errno == EINTR) continue;
            flock(fd, LOCK_UN);
            close(fd);
            return -1;
        }
        total += w;
    }
    flock(fd, LOCK_UN);
    close(fd);
    return 0;
}

int lvx_log_append_jsonl(const char *path, const char *jsonl_line) {
    if (!path || !jsonl_line) return -1;
    size_t n = strlen(jsonl_line);
    return lvx_log_append(path, jsonl_line, n);
}
