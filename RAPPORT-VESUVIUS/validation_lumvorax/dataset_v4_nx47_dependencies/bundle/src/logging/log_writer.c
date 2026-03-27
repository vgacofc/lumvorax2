#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

static inline uint64_t get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static char current_session_dir[256] = {0};

void log_init_session() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(current_session_dir, sizeof(current_session_dir), "trou_noir_sim/logs/session_%04d%02d%02d_%02d%02d%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    mkdir("trou_noir_sim/logs", 0777);
    mkdir(current_session_dir, 0777);
}

void log_writer_entry(const char* module, const char* event, uint64_t value) {
    if (current_session_dir[0] == 0) log_init_session();
    uint64_t ts = get_nanos();
    
    char path_bin[512], path_csv[512], path_json[512];
    snprintf(path_bin, sizeof(path_bin), "%s/raw_data.bin", current_session_dir);
    snprintf(path_csv, sizeof(path_csv), "%s/timeline.csv", current_session_dir);
    snprintf(path_json, sizeof(path_json), "%s/index.json", current_session_dir);

    FILE* fb = fopen(path_bin, "ab");
    if (fb) {
        fwrite(&ts, sizeof(uint64_t), 1, fb);
        char mod[8] = {0}; strncpy(mod, module, 7);
        char ev[16] = {0}; strncpy(ev, event, 15);
        fwrite(mod, 1, 8, fb);
        fwrite(ev, 1, 16, fb);
        fwrite(&value, sizeof(uint64_t), 1, fb);
        fclose(fb);
    }

    FILE* ft = fopen(path_csv, "a");
    if (ft) {
        fprintf(ft, "%lu,%s,%s,%lx\n", ts, module, event, value);
        fclose(ft);
    }

    FILE* fj = fopen(path_json, "a");
    if (fj) {
        fprintf(fj, "{\"ts\":%lu, \"mod\":\"%s\", \"ev\":\"%s\", \"val\":\"%lx\"}\n", ts, module, event, value);
        fclose(fj);
    }
}

uint32_t checksum_generate(const char* data, size_t len) {
    uint32_t hash = 0x811c9dc5;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint32_t)data[i];
        hash *= 0x01000193;
    }
    return hash;
}
