#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static uint64_t fnv1a_64(const void* data, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i=0;i<len;i++){h^=p[i];h*=1099511628211ULL;}
    return h;
}
static long long ns_now(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec*1000000000LL + ts.tv_nsec;
}

int main(int argc, char** argv){
    setvbuf(stdout,NULL,_IONBF,0);
    int trace_on = (argc>1 && !strcmp(argv[1],"--lum-on"));
    FILE* trace = trace_on ? fopen("logs/c106/lum_trace.jsonl","w") : NULL;
    long long t0 = ns_now();
    uint8_t buf[256]; memset(buf,0,256);
    uint64_t cs0=fnv1a_64(buf,256);
    if(trace) fprintf(trace,"{\"event\":\"alloc\",\"size\":256,\"checksum_fnv1a64\":%llu,\"ts_ns\":%lld}\n",
        (unsigned long long)cs0, ns_now());
    srand(42);
    uint64_t cs_running=cs0;
    int N = trace_on ? 64 : 100000;
    for(int i=0;i<N;i++){
        size_t bi=(size_t)(rand()%256); int b=rand()%8;
        uint8_t old=buf[bi]; buf[bi]^=(1u<<b);
        uint64_t cn=fnv1a_64(buf,256);
        if(trace) fprintf(trace,"{\"event\":\"bit_mutation\",\"step\":%d,\"byte\":%zu,\"bit\":%d,\"old_byte\":%u,\"new_byte\":%u,\"cs_before\":%llu,\"cs_after\":%llu,\"ts_ns\":%lld}\n",
            i,bi,b,old,buf[bi],(unsigned long long)cs_running,(unsigned long long)cn, ns_now());
        cs_running=cn;
    }
    long long t1 = ns_now();
    if(trace_on){
        FILE* dump=fopen("logs/c106/lum_dump_original.bin","wb"); fwrite(buf,1,256,dump); fclose(dump);
        fprintf(trace,"{\"event\":\"final\",\"final_checksum\":%llu,\"mutations\":%d,\"elapsed_ns\":%lld}\n",
            (unsigned long long)cs_running, N, t1-t0);
        fclose(trace);
    }
    printf("MODE=%s mutations=%d final_cs=%llu elapsed_ns=%lld\n",
        trace_on?"LUM_ON":"LUM_OFF", N, (unsigned long long)cs_running, t1-t0);
    return 0;
}
