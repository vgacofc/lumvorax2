/* nx48_alltime_record.c — Implémentation persistance monotone (C100). */
#define _POSIX_C_SOURCE 200809L
#include "nx48_alltime_record.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void nx48_alltime_init_empty(nx48_alltime_record_t *r) {
    if (!r) return;
    memset(r, 0, sizeof(*r));
    r->schema_version = NX48_ALLTIME_SCHEMA_VERSION;
    r->best_lz_alltime = 0;
    r->best_nonce_alltime = 0;
    r->ts_unix_first = (int64_t)time(NULL);
    r->ts_unix_last  = r->ts_unix_first;
    r->update_count  = 0;
}

static void s_copy_safe(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

int nx48_alltime_load(const char *path, nx48_alltime_record_t *r) {
    if (!r) return -1;
    nx48_alltime_init_empty(r);
    if (!path) return 0;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        if (errno == ENOENT) return 0;     /* premier run jamais : OK */
        return -1;
    }
    char hdr[1024]; char line[2048];
    if (!fgets(hdr, sizeof(hdr), fp)) { fclose(fp); return 0; }
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return 0; }
    fclose(fp);

    /* Parse champ par champ : on tolère les virgules dans aucun champ
     * car nos données sont hex/runid/wallet (sans virgule par construction). */
    int schema = 0, lz = 0;
    unsigned long long nonce_ull = 0;
    char header_hex[NX48_ALLTIME_HEADER_HEX_LEN] = {0};
    char wallet[NX48_ALLTIME_WALLET_LEN] = {0};
    char rid_first[NX48_ALLTIME_RUNID_LEN] = {0};
    char rid_last [NX48_ALLTIME_RUNID_LEN] = {0};
    long long ts_first = 0, ts_last = 0;
    unsigned long long upd_cnt = 0;

    int n = sscanf(line,
        "%d,%d,%llu,%160[^,],%95[^,],%95[^,],%95[^,],%lld,%lld,%llu",
        &schema, &lz, &nonce_ull,
        header_hex, wallet, rid_first, rid_last,
        &ts_first, &ts_last, &upd_cnt);

    if (n < 3) return -1;   /* CSV malformé */

    r->schema_version    = schema;
    r->best_lz_alltime   = lz;
    r->best_nonce_alltime = (uint32_t)nonce_ull;
    s_copy_safe(r->header_hex,     sizeof(r->header_hex),     header_hex);
    s_copy_safe(r->wallet_address, sizeof(r->wallet_address), wallet);
    s_copy_safe(r->run_id_first,   sizeof(r->run_id_first),   rid_first);
    s_copy_safe(r->run_id_last,    sizeof(r->run_id_last),    rid_last);
    r->ts_unix_first = (int64_t)ts_first;
    r->ts_unix_last  = (int64_t)ts_last;
    r->update_count  = (uint64_t)upd_cnt;
    return 0;
}

static int s_write_atomic(const char *path, const nx48_alltime_record_t *r) {
    if (!path || !r) return -1;
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp.%d", path, (int)getpid());

    FILE *fp = fopen(tmp, "w");
    if (!fp) return -1;
    fprintf(fp,
        "schema_version,best_lz_alltime,best_nonce_alltime,"
        "header_hex_80B,wallet_address,"
        "run_id_first,run_id_last_update,"
        "ts_unix_first,ts_unix_last_update,update_count\n");
    fprintf(fp,
        "%d,%d,%u,%s,%s,%s,%s,%lld,%lld,%llu\n",
        r->schema_version,
        r->best_lz_alltime,
        (unsigned)r->best_nonce_alltime,
        r->header_hex[0]     ? r->header_hex     : "-",
        r->wallet_address[0] ? r->wallet_address : "-",
        r->run_id_first[0]   ? r->run_id_first   : "-",
        r->run_id_last[0]    ? r->run_id_last    : "-",
        (long long)r->ts_unix_first,
        (long long)r->ts_unix_last,
        (unsigned long long)r->update_count);
    fflush(fp);
    int fd = fileno(fp);
    if (fd >= 0) fsync(fd);
    fclose(fp);

    if (rename(tmp, path) != 0) {
        unlink(tmp);
        return -1;
    }
    return 0;
}

int nx48_alltime_try_update(const char *path,
                            int  lz_new,
                            uint32_t nonce_new,
                            const char *header_hex_80B,
                            const char *wallet_address,
                            const char *run_id) {
    if (!path) return -1;

    /* Verrou exclusif sur fichier sentinelle pour éviter les races
     * inter-processus (mining engine + agent + outils ext.). */
    char lockpath[512];
    snprintf(lockpath, sizeof(lockpath), "%s.lock", path);
    int lockfd = open(lockpath, O_CREAT | O_RDWR, 0644);
    if (lockfd < 0) return -1;
    if (flock(lockfd, LOCK_EX) != 0) { close(lockfd); return -1; }

    nx48_alltime_record_t cur;
    int rc_load = nx48_alltime_load(path, &cur);
    if (rc_load < 0) { flock(lockfd, LOCK_UN); close(lockfd); return -1; }

    if (lz_new <= cur.best_lz_alltime) {
        /* Pas un nouveau record absolu — on ne touche RIEN (monotonie). */
        flock(lockfd, LOCK_UN); close(lockfd);
        return 0;
    }

    /* Nouveau record absolu — on met à jour. */
    if (cur.update_count == 0 && cur.run_id_first[0] == '\0') {
        s_copy_safe(cur.run_id_first, sizeof(cur.run_id_first),
                    run_id ? run_id : "anon");
        cur.ts_unix_first = (int64_t)time(NULL);
    }
    cur.best_lz_alltime    = lz_new;
    cur.best_nonce_alltime = nonce_new;
    s_copy_safe(cur.header_hex,     sizeof(cur.header_hex),     header_hex_80B);
    s_copy_safe(cur.wallet_address, sizeof(cur.wallet_address), wallet_address);
    s_copy_safe(cur.run_id_last,    sizeof(cur.run_id_last),
                run_id ? run_id : "anon");
    cur.ts_unix_last = (int64_t)time(NULL);
    cur.update_count++;

    int rc = s_write_atomic(path, &cur);
    flock(lockfd, LOCK_UN); close(lockfd);
    return (rc == 0) ? 1 : -1;
}

int nx48_alltime_serialize_jsonl(const nx48_alltime_record_t *r,
                                 char *out, size_t out_size) {
    if (!r || !out || out_size < 64) return -1;
    int n = snprintf(out, out_size,
        "{\"schema_version\":%d,\"best_lz_alltime\":%d,"
        "\"best_nonce_alltime\":%u,\"header_hex\":\"%s\","
        "\"wallet_address\":\"%s\",\"run_id_first\":\"%s\","
        "\"run_id_last\":\"%s\",\"ts_unix_first\":%lld,"
        "\"ts_unix_last\":%lld,\"update_count\":%llu}\n",
        r->schema_version, r->best_lz_alltime,
        (unsigned)r->best_nonce_alltime,
        r->header_hex[0]     ? r->header_hex     : "",
        r->wallet_address[0] ? r->wallet_address : "",
        r->run_id_first[0]   ? r->run_id_first   : "",
        r->run_id_last[0]    ? r->run_id_last    : "",
        (long long)r->ts_unix_first,
        (long long)r->ts_unix_last,
        (unsigned long long)r->update_count);
    if (n < 0 || (size_t)n >= out_size) return -1;
    return 0;
}
