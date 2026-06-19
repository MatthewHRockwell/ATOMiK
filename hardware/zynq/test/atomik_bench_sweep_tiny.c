/* atomik_bench_sweep_tiny.c — nostdlib RV64 build of the parallel-bank sweep.
 *
 * Same measurement as atomik_bench_sweep.c but with zero libc, so it gzips to
 * ~2 KB and uploads over UART in seconds.  Drives the atomik_parallel_bench
 * engine at 0xF0021000, sweeps active_banks 1/2/4/8 for a fixed delta count,
 * prints the hardware cycle count per bank-count, and verifies the hardware
 * XOR result against an independent software recomputation.
 *
 * Build:
 *   riscv64-linux-gnu-gcc -O2 -nostdlib -nostartfiles -static \
 *     -march=rv64gc -mabi=lp64d -o atomik_bench_sweep_tiny \
 *     atomik_bench_sweep_tiny.c
 * Run (as root):   ./atomik_bench_sweep_tiny [count] [seed_hex]
 */
#include <stdint.h>

/* ── raw RV64 Linux syscalls ─────────────────────────────────────────── */
static long sc(long n, long a, long b, long c, long d, long e, long f) {
    register long a0 asm("a0")=a, a1 asm("a1")=b, a2 asm("a2")=c;
    register long a3 asm("a3")=d, a4 asm("a4")=e, a5 asm("a5")=f, a7 asm("a7")=n;
    asm volatile("ecall":"+r"(a0):"r"(a1),"r"(a2),"r"(a3),"r"(a4),"r"(a5),"r"(a7):"memory");
    return a0;
}
#define SYS_openat 56
#define SYS_close  57
#define SYS_write  64
#define SYS_mmap   222
#define SYS_exit   93
#define AT_FDCWD   (-100)
#define O_RDWR     2
#define O_SYNC     0x101000
#define PROT_RW    3
#define MAP_SHARED 1

static int   s_open(const char *p, long fl){ return (int)sc(SYS_openat, AT_FDCWD,(long)p,fl,0,0,0); }
static void *s_mmap(long len,long fd,long off){ return (void*)sc(SYS_mmap,0,len,PROT_RW,MAP_SHARED,fd,off); }
static void  s_write(const char*b,long n){ sc(SYS_write,1,(long)b,n,0,0,0); }

/* ── tiny output helpers ─────────────────────────────────────────────── */
static void puts_(const char*s){ long n=0; while(s[n])n++; s_write(s,n); }
static void puthex64(uint64_t v){ char b[18]; b[0]='0'; b[1]='x';
    for(int i=0;i<16;i++){ b[2+i]="0123456789abcdef"[(v>>((15-i)*4))&0xF]; } s_write(b,18); }
static void putu(uint64_t v){ char b[24]; int i=24; if(!v){ puts_("0"); return; }
    while(v){ b[--i]='0'+(v%10); v/=10; } s_write(b+i,24-i); }
static void pad(uint64_t v,int w){ /* print v then spaces to width w */
    char t[24]; int i=24; uint64_t x=v; if(!x)t[--i]='0'; while(x){ t[--i]='0'+(x%10); x/=10; }
    int len=24-i; s_write(t+i,len); for(int k=len;k<w;k++) s_write(" ",1); }

/* ── deterministic delta, must match dmix() in atomik_parallel_bench.v ── */
static uint64_t dmix(uint64_t x){ x^=x<<13; x^=x>>7; x^=x<<17; return x; }
static uint64_t ref_xor(uint64_t seed,uint32_t count){ uint64_t a=0;
    for(uint32_t i=0;i<count;i++) a^=dmix(seed^(uint64_t)i); return a; }

/* ── bench MMIO ──────────────────────────────────────────────────────── */
#define R_CTRL 0
#define R_COUNT 1
#define R_ACTIVE 2
#define R_SEEDLO 3
#define R_SEEDHI 4
#define R_CYCLES 5
#define R_RESLO 6
#define R_RESHI 7
#define R_STATUS 8
static volatile uint32_t *B;
static void     wr(int i,uint32_t v){ B[i]=v; asm volatile("fence iorw,iorw"); }
static uint32_t rd(int i){ asm volatile("fence iorw,iorw"); return B[i]; }

static uint64_t run(uint32_t count,uint32_t active,uint64_t seed,uint32_t*cyc){
    wr(R_COUNT,count); wr(R_ACTIVE,active);
    wr(R_SEEDLO,(uint32_t)seed); wr(R_SEEDHI,(uint32_t)(seed>>32));
    wr(R_CTRL,1);
    long g=0; while(!(rd(R_CTRL)&2)){ if(++g>200000000) break; }
    *cyc=rd(R_CYCLES);
    return ((uint64_t)rd(R_RESHI)<<32)|rd(R_RESLO);
}

/* simple decimal/hex argv parse */
static uint64_t paru(const char*s,int hex){ uint64_t v=0; if(!s)return 0;
    if(hex&&s[0]=='0'&&(s[1]=='x'||s[1]=='X'))s+=2;
    for(;*s;s++){ char c=*s; uint64_t d;
        if(c>='0'&&c<='9')d=c-'0'; else if(hex&&c>='a'&&c<='f')d=c-'a'+10;
        else if(hex&&c>='A'&&c<='F')d=c-'A'+10; else break;
        v=hex? (v<<4)|d : v*10+d; } return v; }

int main(int argc,char**argv){
    uint32_t count = argc>1 ? (uint32_t)paru(argv[1],0) : 65537;
    uint64_t seed  = argc>2 ? paru(argv[2],1) : 0xDEADBEEF0BADF00DULL;

    int fd=s_open("/dev/mem",O_RDWR|O_SYNC);
    if(fd<0){ puts_("ERR: open /dev/mem (need root)\n"); return 1; }
    void*m=s_mmap(4096,fd,0xF0021000UL);
    if((long)m<0){ puts_("ERR: mmap\n"); return 1; }
    B=(volatile uint32_t*)m;

    uint32_t st=rd(R_STATUS);
    puts_("ATOMiK parallel-bench  v"); putu((st>>24)&0xFF);
    puts_("  N_BANKS="); putu((st>>16)&0xFF); puts_("\n");
    puts_("count="); putu(count); puts_("  seed="); puthex64(seed); puts_("\n\n");

    uint64_t ref=ref_xor(seed,count);
    puts_("banks  hw_cycles   speedup  result              ok\n");
    uint32_t banks[4]={1,2,4,8}, base=0; int allok=1;
    for(int k=0;k<4;k++){
        uint32_t b=banks[k], cyc; uint64_t res=run(count,b,seed,&cyc);
        if(k==0)base=cyc;
        int ok=(res==ref); allok&=ok;
        /* banks */ pad(b,7);
        /* cycles */ pad(cyc,12);
        /* speedup x100 -> "N.NN" */
        uint32_t sx=cyc? (base*100u)/cyc : 0;
        putu(sx/100); puts_("."); { uint32_t fr=sx%100; if(fr<10)puts_("0"); putu(fr);} puts_("    ");
        puthex64(res); puts_(ok?"  OK\n":"  MISMATCH!\n");
    }
    puts_("\nsoftware reference = "); puthex64(ref); puts_("\n");
    puts_(allok?"VERIFY: ALL MATCH\n":"VERIFY: FAILED\n");
    return allok?0:2;
}

/* nostdlib entry — naked so sp still points at argc (no prologue) */
__attribute__((naked,used)) void _start(void){
    asm volatile(
        "ld   a0, 0(sp)\n"      /* argc            */
        "addi a1, sp, 8\n"      /* argv            */
        "call main\n"           /* rc -> a0        */
        "li   a7, 93\n"         /* SYS_exit        */
        "ecall\n"
    );
}
