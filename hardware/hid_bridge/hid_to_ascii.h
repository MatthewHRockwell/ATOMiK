/* hid_to_ascii.h — USB HID keycode → ASCII byte translation.
 *
 * MIRRORS atomik_os/src/input.c::keycode_to_ascii() so the bridge and
 * the host see-eye-to-eye on what each USB HID Usage ID means.  When
 * we update one, we must update the other.
 *
 * USB HID Usage IDs are NOT the same as Linux keycodes.  Linux remaps
 * them in the kernel.  Since this firmware reads raw HID reports
 * directly, we translate the Usage IDs.  The mapping is:
 *   0x04..0x1D = a..z
 *   0x1E..0x27 = 1..9, 0
 *   0x28       = Enter
 *   0x29       = Esc
 *   0x2A       = Backspace
 *   0x2B       = Tab
 *   0x2C       = Space
 *   0x2D..0x38 = -=[]\#;'`,./ (US layout)
 *
 * Modifier byte (HID report byte 0):
 *   bit 0 = Left Ctrl     bit 4 = Right Ctrl
 *   bit 1 = Left Shift    bit 5 = Right Shift
 *   bit 2 = Left Alt      bit 6 = Right Alt
 *   bit 3 = Left GUI      bit 7 = Right GUI
 */
#ifndef HID_TO_ASCII_H
#define HID_TO_ASCII_H

#include <stdint.h>

#define HID_MOD_CTRL  (0x01 | 0x10)
#define HID_MOD_SHIFT (0x02 | 0x20)

static const char HID_ROW_LET[26] = "abcdefghijklmnopqrstuvwxyz";
static const char HID_ROW_NUM[10] = "1234567890";
static const char HID_ROW_NUM_S[10] = "!@#$%^&*()";

/* Translate a single HID Usage ID + modifier byte into one ASCII byte.
 * Returns 0 if the key is unmapped (modifier-only report, function
 * key, etc.) — caller should skip 0 returns rather than emitting them. */
static inline char hid_to_ascii(uint8_t keycode, uint8_t mod) {
    int shift = !!(mod & HID_MOD_SHIFT);
    int ctrl  = !!(mod & HID_MOD_CTRL);

    char c = 0;

    if (keycode >= 0x04 && keycode <= 0x1D) {
        c = HID_ROW_LET[keycode - 0x04];
        if (shift) c = (char)(c - 'a' + 'A');
    } else if (keycode >= 0x1E && keycode <= 0x27) {
        c = shift ? HID_ROW_NUM_S[keycode - 0x1E] : HID_ROW_NUM[keycode - 0x1E];
    } else {
        switch (keycode) {
        case 0x28: c = '\n'; break;     /* Enter */
        case 0x29: c = 0x1B; break;     /* Esc */
        case 0x2A: c = 0x7F; break;     /* Backspace (atomik_os accepts 0x7F or 0x08) */
        case 0x2B: c = '\t'; break;     /* Tab */
        case 0x2C: c = ' ';  break;     /* Space */
        case 0x2D: c = shift ? '_' : '-'; break;
        case 0x2E: c = shift ? '+' : '='; break;
        case 0x2F: c = shift ? '{' : '['; break;
        case 0x30: c = shift ? '}' : ']'; break;
        case 0x31: c = shift ? '|' : '\\'; break;
        case 0x33: c = shift ? ':' : ';'; break;
        case 0x34: c = shift ? '"' : '\''; break;
        case 0x35: c = shift ? '~' : '`'; break;
        case 0x36: c = shift ? '<' : ','; break;
        case 0x37: c = shift ? '>' : '.'; break;
        case 0x38: c = shift ? '?' : '/'; break;
        default:   return 0;
        }
    }

    /* Ctrl + letter → ASCII control code (Ctrl-W → 0x17, Ctrl-C → 0x03,
     * etc.)  Matches atomik_os/src/input.c::input_poll() v0.31 patch 8.
     * Only applies to a..z; doesn't transform digits or punctuation. */
    if (ctrl && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 1);
    if (ctrl && c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 1);

    return c;
}

#endif /* HID_TO_ASCII_H */
