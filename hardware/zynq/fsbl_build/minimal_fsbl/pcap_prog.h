#ifndef PCAP_PROG_H
#define PCAP_PROG_H
#include <stdint.h>
/* Program PL bitstream. data = DDR address of .bit.bin, words = size/4.
 * Returns 0 on success. */
int pcap_program(const void *data, uint32_t words);
#endif
