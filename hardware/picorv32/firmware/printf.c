// Minimal printf for bare-metal RV32I
// Supports: %d, %u, %x, %s, %c, %%
// No division hardware — uses powers-of-10 table with repeated subtraction.

#include <stdint.h>
#include <stdarg.h>

// Provided by firmware.c
extern int putchar(int c);

static void print_str(const char *s)
{
    while (*s) putchar(*s++);
}

static void print_unsigned(uint32_t v)
{
    // Powers of 10 for uint32_t (max 4,294,967,295)
    static const uint32_t pow10[] = {
        1000000000, 100000000, 10000000, 1000000,
        100000, 10000, 1000, 100, 10, 1
    };
    int started = 0;
    for (int i = 0; i < 10; i++) {
        int digit = 0;
        while (v >= pow10[i]) {
            v -= pow10[i];
            digit++;
        }
        if (digit || started || i == 9) {
            putchar('0' + digit);
            started = 1;
        }
    }
}

static void print_signed(int32_t v)
{
    if (v < 0) {
        putchar('-');
        // Handle INT32_MIN carefully
        if (v == (int32_t)0x80000000) {
            print_str("2147483648");
            return;
        }
        v = -v;
    }
    print_unsigned((uint32_t)v);
}

static void print_hex32(uint32_t v, int width)
{
    // Print exactly 'width' hex digits (0-padded), or auto-width if width==0
    if (width == 0) {
        // Auto-width: skip leading zeros
        int started = 0;
        for (int i = 7; i >= 0; i--) {
            int d = (v >> (4 * i)) & 0xF;
            if (d || started || i == 0) {
                putchar("0123456789abcdef"[d]);
                started = 1;
            }
        }
    } else {
        for (int i = width - 1; i >= 0; i--)
            putchar("0123456789abcdef"[(v >> (4 * i)) & 0xF]);
    }
}

void mini_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            putchar(*fmt++);
            continue;
        }
        fmt++; // skip '%'

        // Check for '0' padding and width (support %08x style)
        char pad = ' ';
        int width = 0;
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        (void)pad; // pad only used for hex currently

        switch (*fmt) {
        case 'd': {
            int32_t val = va_arg(ap, int32_t);
            print_signed(val);
            break;
        }
        case 'u': {
            uint32_t val = va_arg(ap, uint32_t);
            print_unsigned(val);
            break;
        }
        case 'x': {
            uint32_t val = va_arg(ap, uint32_t);
            print_hex32(val, width);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            print_str(s ? s : "(null)");
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            putchar(c);
            break;
        }
        case '%':
            putchar('%');
            break;
        default:
            putchar('%');
            putchar(*fmt);
            break;
        }
        fmt++;
    }

    va_end(ap);
}
