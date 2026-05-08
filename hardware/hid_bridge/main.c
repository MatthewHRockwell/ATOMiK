/* main.c — ATOMiK input controller firmware (Pi Pico, RP2040).
 *
 * Reads USB HID keyboards via TinyUSB's PIO-USB host stack, translates
 * each key press to one ASCII byte, sends that byte over UART to the
 * AX7020.  atomik_os reads from the matching /dev/ttyLXU* and treats
 * each byte as a normal keystroke — same path the existing FIFO-stdin
 * injection uses, so atomik_os requires zero changes.
 *
 * Architectural framing per project_v033_plan.md (ChatGPT 2026-05-08):
 * this is the "ATOMiK input controller", a permanent peripheral, not a
 * workaround for the flaky native PS USB host.  Native USB hardening
 * stays a real engineering thread (v0.35) but doesn't block the demo.
 *
 * Hardware:
 *   - Raspberry Pi Pico (RP2040)
 *   - PIO-USB host on GP2 (D+) / GP3 (D-)
 *   - UART0 TX on GP0 → AX7020 PMOD/UART input
 *   - 5V VBUS for the dongle: pico's VBUS pin (when powered via its
 *     own USB-C input from the demo enclosure's 5V rail)
 *
 * Build (with the Pico SDK):
 *   git clone https://github.com/sekigon-gonnoc/Pico-PIO-USB.git
 *   mkdir build && cd build
 *   cmake -DPICO_SDK_PATH=$PICO_SDK_PATH ..
 *   make
 *   # Flash the resulting hid_bridge.uf2 to the Pico in BOOTSEL mode
 */

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include "pio_usb.h"

#include "hid_to_ascii.h"

/* ---------- pin map ---------- */
#define HOST_USB_DP_PIN   2     /* must be even-numbered; D- = DP+1 */
#define UART_INSTANCE     uart0
#define UART_BAUD         115200
#define UART_TX_PIN       0
#define UART_RX_PIN       1     /* unused for v1 (one-way bridge) */
#define LED_PIN           PICO_DEFAULT_LED_PIN

/* ---------- core 1: TinyUSB host on PIO-USB ---------- */

static void core1_main(void) {
    /* Configure PIO-USB to use GP2/GP3 for the USB D+/D- pair, then
     * hand control to TinyUSB's host task.  TinyUSB's tuh_init+tuh_task
     * loop runs forever on this core; HID reports trigger callbacks
     * that we forward to UART (those callbacks fire on this core too).
     */
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = HOST_USB_DP_PIN;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tuh_init(BOARD_TUH_RHPORT);

    while (true) {
        tuh_task();
    }
}

/* ---------- core 0: setup + idle ---------- */

int main(void) {
    /* The PIO-USB host stack requires the system clock at exactly
     * 120 MHz (or 240 MHz) so the PIO state machines hit USB timing.
     * stdlib's default 125 MHz is wrong for PIO-USB. */
    set_sys_clock_khz(120000, true);

    stdio_init_all();

    /* UART for output to AX7020. */
    uart_init(UART_INSTANCE, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_INSTANCE, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_INSTANCE, true);

    /* LED for visual feedback — blinks on each forwarded keystroke so
     * we can confirm the bridge is alive without an oscilloscope. */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    /* Boot signal: 3 quick blinks so the operator knows the firmware
     * is running before they plug in a dongle. */
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1); sleep_ms(80);
        gpio_put(LED_PIN, 0); sleep_ms(80);
    }

    /* Launch USB host stack on core 1. */
    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    /* Core 0 idles; everything reactive happens in tuh callbacks on core 1. */
    while (true) {
        sleep_ms(1000);
    }
    return 0;
}

/* ---------- TinyUSB HID host callbacks ---------- */

/* Called on every HID report received from a mounted keyboard.  The
 * report layout for a boot-protocol keyboard is:
 *   byte 0  = modifier bitmask (Shift, Ctrl, Alt, GUI)
 *   byte 1  = reserved
 *   byte 2..7 = up to 6 simultaneously-pressed key codes
 *
 * Wireless dongles often present as a "boot-protocol keyboard" plus
 * sometimes a separate mouse interface; we handle the keyboard
 * interface and ignore the mouse for v1.
 */
static uint8_t s_prev_keys[6] = {0};

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const *report, uint16_t len) {
    (void)dev_addr; (void)instance;
    if (len < 3) return;

    uint8_t mod   = report[0];
    const uint8_t *keys = &report[2];

    /* Edge-detect: emit a byte for each key that's newly pressed
     * compared to the previous report.  Held keys do NOT auto-repeat
     * here — atomik_os handles repeat semantics if needed.  This
     * matches typewriter-style "press → one event" UX which is what
     * the input router expects. */
    for (int i = 0; i < 6; i++) {
        uint8_t k = keys[i];
        if (k == 0) continue;

        int already_held = 0;
        for (int j = 0; j < 6; j++) {
            if (s_prev_keys[j] == k) { already_held = 1; break; }
        }
        if (already_held) continue;

        char c = hid_to_ascii(k, mod);
        if (c == 0) continue;

        uart_putc_raw(UART_INSTANCE, c);
        gpio_put(LED_PIN, 1);
        /* Tiny LED pulse — too short to perceive as flicker but long
         * enough to verify on a scope.  Don't sleep too long here:
         * we're inside a USB-host callback. */
        sleep_us(2000);
        gpio_put(LED_PIN, 0);
    }

    /* Save the current keymap for next-edge detection. */
    for (int i = 0; i < 6; i++) s_prev_keys[i] = keys[i];
}

/* Mount/unmount events — purely informational, blink LED on connect. */
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len) {
    (void)dev_addr; (void)instance; (void)desc_report; (void)desc_len;
    /* Long-on flash so the operator can see "dongle paired". */
    gpio_put(LED_PIN, 1); sleep_ms(200); gpio_put(LED_PIN, 0);
    /* Request HID reports — TinyUSB needs an explicit request_report
     * call to start receiving data from boot-protocol keyboards. */
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr; (void)instance;
    /* Three short flashes: dongle removed. */
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1); sleep_ms(40);
        gpio_put(LED_PIN, 0); sleep_ms(40);
    }
}
