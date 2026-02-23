// Minimal printf for bare-metal RV64I (no div instruction)
// Supports: %d, %u, %x, %s, %c, %%, %lx (64-bit hex)
#ifndef PRINTF_V3_H
#define PRINTF_V3_H

#include <stdint.h>

void mini_printf(const char *fmt, ...);

#endif
