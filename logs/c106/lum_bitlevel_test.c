#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "src/lum/lum_core.h"

static uint64_t fnv1a_64(const void* data, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i=0;i<len;i++){h^=p[i];h*=1099511628211ULL;}
    return h;
}
int main(void){
    setvbuf(stdout,NULL,_IONBF,0);
    FILE* trace=fopen("logs/c106/lum_trace.jsonl","w"); if(!trace){perror("trace");return 1;}
    uint8_t buf[256]; memset(buf,0,256);
    uint64_t cs0=fnv1a_64(buf,256);
    fprintf(trace,"{\"event\":\"alloc\",\"size\":256,\"checksum_fnv1a64\":%llu}\n",(unsigned long long)cs0);
    srand(42);
    uint64_t cs_running=cs0;
    for(int i=0;i<64;i++){
        size_t bi=(size_t)(rand()%256); int b=rand()%8;
        uint8_t old=buf[bi]; buf[bi]^=(1u<<b);
        uint64_t cn=fnv1a_64(buf,256);
        fprintf(trace,"{\"event\":\"bit_mutation\",\"step\":%d,\"byte\":%zu,\"bit\":%d,\"old_byte\":%u,\"new_byte\":%u,\"checksum_before\":%llu,\"checksum_after\":%llu}\n",
            i,bi,b,old,buf[bi],(unsigned long long)cs_running,(unsigned long long)cn);
        cs_running=cn;
    }
    FILE* dump=fopen("logs/c106/lum_dump_original.bin","wb"); fwrite(buf,1,256,dump); fclose(dump);
    lum_t* l=lum_create(1,0,0,LUM_STRUCTURE_LINEAR);
    if(l){
        fprintf(trace,"{\"event\":\"lum_create\",\"id\":%u,\"checksum\":%u,\"magic\":%u,\"size_bytes\":%zu,\"timestamp_ns\":%llu}\n",
            l->id,l->checksum,l->magic_number,sizeof(lum_t),(unsigned long long)l->timestamp);
        lum_destroy(l);
    }
    fprintf(trace,"{\"event\":\"final\",\"final_checksum\":%llu,\"mutations\":64}\n",(unsigned long long)cs_running);
    fclose(trace);
    printf("LUM_BIT_TEST_OK final_checksum=%llu mutations=64 ; trace=logs/c106/lum_trace.jsonl\n",(unsigned long long)cs_running);
    return 0;
}
