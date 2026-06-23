/* fbbench v2 — find the fastest copy strategy for DRAM->fb on this NaxRiscv.
 * The baseline glibc memcpy gives ~80MB/s; if the D-cache is non-blocking, a
 * software-pipelined copy (many loads in flight) should beat it by hiding DDR
 * latency.  If so, we apply it to fb_blit_to_screen and the whole UI speeds up. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#define FB_W 1920
#define FB_H 1080
#define FB_BYTES ((size_t)FB_W*FB_H*4)
#define WIN ((size_t)780*560*4)            /* a window's worth of bytes */
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec/1e9; }

/* manual 64-bit copy */
static void cpy_u64(uint64_t *d, const uint64_t *s, size_t n){ for(size_t i=0;i<n;i++) d[i]=s[i]; }
/* software-pipelined: 8 loads issued before any store (hide miss latency) */
static void cpy_pipe8(uint64_t *d, const uint64_t *s, size_t n){
    size_t i=0; for(; i+8<=n; i+=8){
        uint64_t a=s[i],b=s[i+1],c=s[i+2],e=s[i+3],f=s[i+4],g=s[i+5],h=s[i+6],j=s[i+7];
        d[i]=a;d[i+1]=b;d[i+2]=c;d[i+3]=e;d[i+4]=f;d[i+5]=g;d[i+6]=h;d[i+7]=j;
    }
    for(;i<n;i++) d[i]=s[i];
}
/* deeper pipeline: 16 in flight */
static void cpy_pipe16(uint64_t *d, const uint64_t *s, size_t n){
    size_t i=0; for(; i+16<=n; i+=16){
        uint64_t x[16];
        for(int k=0;k<16;k++) x[k]=s[i+k];
        for(int k=0;k<16;k++) d[i+k]=x[k];
    }
    for(;i<n;i++) d[i]=s[i];
}

int main(void){
    int fd=open("/dev/fb0",O_RDWR); if(fd<0){perror("fb0");return 1;}
    void *fb=mmap(NULL,FB_BYTES,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(fb==MAP_FAILED){perror("mmap");return 1;}
    uint64_t *a=malloc(WIN), *b=malloc(WIN);
    memset(a,0x11,WIN); memset(b,0x22,WIN);
    size_t n64=WIN/8; int M=20; double t;
    /* warm */ memcpy(b,a,WIN);

    t=now(); for(int i=0;i<M;i++) memcpy(fb,a,WIN);
    printf("FB write  memcpy   %5.1f MB/s  (%5.1f ms/win)\n", WIN*M/(now()-t)/1e6, (now()-t)/M*1e3);
    t=now(); for(int i=0;i<M;i++) cpy_u64((uint64_t*)fb,a,n64);
    printf("FB write  u64      %5.1f MB/s  (%5.1f ms/win)\n", WIN*M/(now()-t)/1e6, (now()-t)/M*1e3);
    t=now(); for(int i=0;i<M;i++) cpy_pipe8((uint64_t*)fb,a,n64);
    printf("FB write  pipe8    %5.1f MB/s  (%5.1f ms/win)\n", WIN*M/(now()-t)/1e6, (now()-t)/M*1e3);
    t=now(); for(int i=0;i<M;i++) cpy_pipe16((uint64_t*)fb,a,n64);
    printf("FB write  pipe16   %5.1f MB/s  (%5.1f ms/win)\n", WIN*M/(now()-t)/1e6, (now()-t)/M*1e3);

    t=now(); for(int i=0;i<M;i++) memcpy(b,a,WIN);
    printf("DRAM copy memcpy   %5.1f MB/s\n", WIN*M/(now()-t)/1e6);
    t=now(); for(int i=0;i<M;i++) cpy_pipe8(b,a,n64);
    printf("DRAM copy pipe8    %5.1f MB/s\n", WIN*M/(now()-t)/1e6);
    t=now(); for(int i=0;i<M;i++) cpy_pipe16(b,a,n64);
    printf("DRAM copy pipe16   %5.1f MB/s\n", WIN*M/(now()-t)/1e6);
    return 0;
}
