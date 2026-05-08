/* tusb_config.h — TinyUSB configuration for the ATOMiK input controller.
 *
 * Configures TinyUSB as a HID HOST running on PIO-USB (RP2040 has no
 * hardware USB-host controller; the PIO-USB software stack is the
 * supported workaround on the Pico).  Plus a small USB-CDC DEVICE
 * interface on the Pico's native USB port for debug prints — this
 * lets us tail USB stack messages without sharing the AX7020 UART.
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU                 OPT_MCU_RP2040
#define CFG_TUSB_OS                  OPT_OS_PICO
#define CFG_TUSB_DEBUG               0

/* The Pico's native USB peripheral runs as a CDC device (so we can
 * see debug prints over USB-serial).  PIO-USB is the host on GP2/GP3. */
#define CFG_TUH_RPI_PIO_USB          1
#define BOARD_TUH_RHPORT             1

#define CFG_TUSB_RHPORT0_MODE        OPT_MODE_DEVICE
#define CFG_TUSB_RHPORT1_MODE        (OPT_MODE_HOST | OPT_MODE_HIGH_SPEED)

/* HID host */
#define CFG_TUH_ENABLED              1
#define CFG_TUH_HUB                  1     /* allow a hub between Pico and dongle */
#define CFG_TUH_DEVICE_MAX           (CFG_TUH_HUB ? 4 : 1)
#define CFG_TUH_HID                  4
#define CFG_TUH_HID_EPIN_BUFSIZE     64
#define CFG_TUH_HID_EPOUT_BUFSIZE    64

/* CDC device for debug */
#define CFG_TUD_ENABLED              1
#define CFG_TUD_CDC                  1
#define CFG_TUD_CDC_RX_BUFSIZE       128
#define CFG_TUD_CDC_TX_BUFSIZE       128

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
