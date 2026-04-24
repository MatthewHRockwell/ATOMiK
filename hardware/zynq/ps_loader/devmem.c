/* Minimal devmem — read/write physical memory via /dev/mem.
 * Usage:
 *   devmem <addr>             # read 32-bit
 *   devmem <addr> 32 <value>  # write 32-bit
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: devmem <addr> [32 <val>]\n"); return 1; }
    unsigned long addr = strtoul(argv[1], NULL, 0);
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("/dev/mem"); return 1; }
    unsigned long page = addr & ~0xFFFUL;
    unsigned long off  = addr &  0xFFFUL;
    volatile uint8_t *m = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, page);
    if (m == MAP_FAILED) { perror("mmap"); return 1; }
    volatile uint32_t *p = (volatile uint32_t *)(m + off);
    if (argc >= 4) {
        uint32_t val = strtoul(argv[3], NULL, 0);
        *p = val;
        printf("0x%08lx <- 0x%08x\n", addr, val);
    } else {
        printf("0x%08lx = 0x%08x\n", addr, (uint32_t)*p);
    }
    munmap((void *)m, 4096);
    close(fd);
    return 0;
}
