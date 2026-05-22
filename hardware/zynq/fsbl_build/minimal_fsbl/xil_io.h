/* Minimal xil_io.h for ps7_init.c compilation — just MMIO read/write macros */
#ifndef XIL_IO_H
#define XIL_IO_H
#include <stdint.h>
#define Xil_Out32(addr, val)  (*(volatile uint32_t *)(uintptr_t)(addr) = (uint32_t)(val))
#define Xil_In32(addr)        (*(volatile uint32_t *)(uintptr_t)(addr))
#define Xil_Out16(addr, val)  (*(volatile uint16_t *)(uintptr_t)(addr) = (uint16_t)(val))
#define Xil_In16(addr)        (*(volatile uint16_t *)(uintptr_t)(addr))
#define Xil_Out8(addr, val)   (*(volatile uint8_t  *)(uintptr_t)(addr) = (uint8_t )(val))
#define Xil_In8(addr)         (*(volatile uint8_t  *)(uintptr_t)(addr))
#endif
