# atomik_core.h

**Single-header C99 library for ATOMiK delta-state algebra.**

Same 4-operation API as the Python library, 50-100x faster. Zero dependencies beyond `<stdint.h>` and `<string.h>`.

## Usage

```c
#define ATOMIK_IMPLEMENTATION   // in exactly ONE .c file
#include "atomik_core.h"
```

## Quick Start

```c
atomik_ctx_t ctx;
atomik_init(&ctx);
atomik_load(&ctx, 0xDEADBEEFCAFEBABEULL);
atomik_accum(&ctx, 0x00000000000000FFULL);

uint64_t state = atomik_read(&ctx);
// state == 0xDEADBEEFCAFEBA41

// Undo: re-apply the same delta
atomik_accum(&ctx, 0x00000000000000FFULL);
assert(atomik_read(&ctx) == 0xDEADBEEFCAFEBABEULL);
```

## Multi-Context Table

```c
atomik_table_t table;
atomik_table_init(&table, 256);

atomik_table_load(&table, 0, 0xCAFEBABE);
atomik_table_accum(&table, 0, 0x00000001);
uint64_t s = atomik_table_read(&table, 0);  // 0xCAFEBABF

atomik_table_free(&table);
```

Stack-allocated alternative (no malloc):

```c
ATOMIK_TABLE_STATIC_SIZE(16);  // defines atomik_table_16_t
atomik_table_16_t my_table;
atomik_table_static_init_16(&my_table);
```

## Fingerprinting

```c
atomik_fingerprint_t fp;
atomik_fp_init(&fp);
atomik_fp_load(&fp, data, len);

// Later, when data changes:
atomik_fp_update(&fp, new_data, len);
if (atomik_fp_changed(&fp)) {
    printf("Data modified!\n");
}
```

## Build

No build system needed. Just include the header:

```bash
gcc -o my_app my_app.c -O2
```

Run the test suite:

```bash
gcc -o test_atomik_core test_atomik_core.c -Wall -Wextra -O2
./test_atomik_core
```

## Performance

| Operation | Throughput | Latency |
|-----------|-----------|---------|
| LOAD | ~500M ops/sec | ~2 ns |
| ACCUM | ~500M ops/sec | ~2 ns |
| READ | ~500M ops/sec | ~2 ns |
| SWAP | ~500M ops/sec | ~2 ns |

Measured on x86-64. ARM and RISC-V performance is similar — XOR is a single instruction on every ISA.

## API Reference

### Context (single 64-bit state)

| Function | Description |
|----------|-------------|
| `atomik_init(ctx)` | Initialize context to zero |
| `atomik_load(ctx, value)` | Set reference, clear accumulator |
| `atomik_accum(ctx, delta)` | XOR delta into accumulator |
| `atomik_read(ctx)` | Return reference ^ accumulator |
| `atomik_swap(ctx)` | Snapshot + new epoch, returns old state |

### Table (N independent contexts)

| Function | Description |
|----------|-------------|
| `atomik_table_init(tbl, n)` | Allocate n contexts (heap) |
| `atomik_table_free(tbl)` | Free table memory |
| `atomik_table_load(tbl, addr, val)` | Load context at address |
| `atomik_table_accum(tbl, addr, delta)` | Accumulate at address |
| `atomik_table_read(tbl, addr)` | Read state at address |
| `atomik_table_swap(tbl, addr)` | Swap context at address |

### Fingerprint (change detection)

| Function | Description |
|----------|-------------|
| `atomik_fp_init(fp)` | Initialize fingerprint |
| `atomik_fp_load(fp, data, len)` | Set reference from data |
| `atomik_fp_update(fp, data, len)` | Compute current fingerprint |
| `atomik_fp_changed(fp)` | Returns 1 if data differs from reference |
| `atomik_fp_delta(fp)` | Returns XOR of reference and current |

## License

Apache 2.0 for evaluation and non-commercial use. [Commercial license](https://github.com/MatthewHRockwell/ATOMiK) required for production deployment. Patent pending.
