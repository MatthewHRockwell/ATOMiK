// =============================================================================
// ATOMiK BNN Accelerator — Firmware Driver + ASCII Classifier
//
// MMIO registers for the 32->16->4 binary neural network accelerator.
// Hardware: XNOR-popcount engine, 40-cycle inference.
// Firmware: ASCII character classifier with per-neuron thresholds.
//
// Base address: 0xC000_0200 (S3 sub-slot, addr[9:8]=10)
// =============================================================================

#ifndef BNN_V3_H
#define BNN_V3_H

#include <stdint.h>

// --- MMIO Register Addresses ---
#define BNN_BASE       0xC0000200

#define BNN_CTRL       (*(volatile uint32_t*)(BNN_BASE + 0x00))
#define BNN_INPUT      (*(volatile uint32_t*)(BNN_BASE + 0x04))
#define BNN_HIDDEN     (*(volatile uint32_t*)(BNN_BASE + 0x08))
#define BNN_OUTPUT     (*(volatile uint32_t*)(BNN_BASE + 0x0C))
#define BNN_WGHT_ADDR  (*(volatile uint32_t*)(BNN_BASE + 0x10))
#define BNN_WGHT_DATA  (*(volatile uint32_t*)(BNN_BASE + 0x14))
#define BNN_THRESH     (*(volatile uint32_t*)(BNN_BASE + 0x18))
#define BNN_STATUS     (*(volatile uint32_t*)(BNN_BASE + 0x1C))

// --- BNN_CTRL bit fields ---
#define BNN_CTRL_RUN       (1 << 0)
#define BNN_CTRL_DONE      (1 << 1)
#define BNN_CTRL_BUSY      (1 << 2)
#define BNN_CTRL_WINNER_MASK  0x30
#define BNN_CTRL_WINNER_SHIFT 4
#define BNN_CTRL_LATENCY_MASK  0xFF00
#define BNN_CTRL_LATENCY_SHIFT 8

// --- Network dimensions ---
#define BNN_NUM_INPUTS   32
#define BNN_NUM_HIDDEN   16
#define BNN_NUM_OUTPUTS   4
#define BNN_NUM_WEIGHTS  18

// --- Hardware MMIO API ---

static inline void bnn_write_weight(uint32_t idx, uint32_t val) {
    BNN_WGHT_ADDR = idx;
    BNN_WGHT_DATA = val;
}

static inline uint32_t bnn_read_weight(uint32_t idx) {
    BNN_WGHT_ADDR = idx;
    return BNN_WGHT_DATA;
}

static inline void bnn_load_weights(const uint32_t *w) {
    for (int i = 0; i < BNN_NUM_WEIGHTS; i++)
        bnn_write_weight(i, w[i]);
}

static inline void bnn_set_thresholds(uint32_t hidden_thresh, uint32_t output_thresh) {
    BNN_THRESH = (output_thresh << 8) | (hidden_thresh & 0x3F);
}

static inline uint32_t bnn_infer(uint32_t input) {
    BNN_INPUT = input;
    BNN_CTRL = BNN_CTRL_RUN;
    while (!(BNN_CTRL & BNN_CTRL_DONE))
        ;
    return (BNN_CTRL & BNN_CTRL_WINNER_MASK) >> BNN_CTRL_WINNER_SHIFT;
}

static inline void bnn_infer_full(uint32_t input, uint32_t *hidden_out,
                                   uint32_t *output_out, uint32_t *ctrl_out) {
    BNN_INPUT = input;
    BNN_CTRL = BNN_CTRL_RUN;
    while (!(BNN_CTRL & BNN_CTRL_DONE))
        ;
    *hidden_out = BNN_HIDDEN;
    *output_out = BNN_OUTPUT;
    *ctrl_out   = BNN_CTRL;
}

static inline uint32_t bnn_last_latency(void) {
    return (BNN_CTRL & BNN_CTRL_LATENCY_MASK) >> BNN_CTRL_LATENCY_SHIFT;
}

static inline uint32_t bnn_infer_count(void) {
    return BNN_STATUS & 0xFFFF;
}

// =========================================================================
// Hardware validation weights (for B1-B7 tests)
// =========================================================================
static const uint32_t bnn_weights_pattern_classifier[18] = {
    0x00000000, 0x0000FFFF, 0xFFFF0000, 0x00FF00FF,
    0xFF00FF00, 0x55555555, 0xAAAAAAAA, 0x0F0F0F0F,
    0xF0F0F0F0, 0xFFF00000, 0x000FFF00, 0xFFFFFFFF,
    0xFF0000FF, 0x00FFFF00, 0x33333333, 0xCCCCCCCC,
    0xE0600007, 0xFFFF0E00,
};

static const char *bnn_class_names[4] = {
    "Stable", "Active", "Anomaly", "Noise"
};

// =========================================================================
// ASCII Character Classifier (firmware-side with per-neuron thresholds)
// 100% accuracy on all 128 ASCII characters
// Classes: LETTER, DIGIT, SYMBOL, SPACE/CTRL
// =========================================================================

static const char *ascii_class_names[4] = {
    "LETTER", "DIGIT", "SYMBOL", "SPACE"
};

static const char *ascii_class_bars[4] = {
    "##########", "==========", "%%%%%%%%%%", ".........."
};

// Hidden layer weights (16 neurons x 32-bit)
static const uint32_t ascii_hidden_weights[16] = {
    0x33BBB7F3U, 0x33BBB7F3U, 0x33BBB7F3U, 0x33BFB7C1U,  // LETTER detectors
    0x42CEE9D0U, 0x42CEE9D0U, 0x42CFE9D1U, 0x42CE6950U,  // DIGIT detectors
    0x48D5D1EDU, 0x48D5D1EDU, 0x48D6916EU, 0x48D5516DU,  // SYMBOL detectors
    0x84FFC0BFU, 0x84FFC09EU, 0x84FFC09CU, 0x84FFC098U,  // SPACE/CTRL detectors
};

static const uint8_t ascii_hidden_thresholds[16] = {
    20, 20, 20, 19,  23, 23, 23, 23,  21, 21, 22, 21,  20, 21, 22, 22
};

// Output layer weights (4 neurons x 16-bit)
static const uint16_t ascii_output_weights[4] = {
    0x800CU, 0x8030U, 0x8A48U, 0xF000U,
};

static const uint8_t ascii_output_thresholds[4] = {
    13, 13, 11, 9
};

// Simple popcount for RV64I (no multiply instruction)
static inline int sw_popcount32(uint32_t x) {
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    // Sum the 4 bytes without multiply (RV64I has no MUL)
    return (x & 0xFF) + ((x >> 8) & 0xFF) + ((x >> 16) & 0xFF) + ((x >> 24) & 0xFF);
}

// Feature expansion: 8-bit ASCII -> 32-bit feature vector
static uint32_t bnn_expand_features(uint8_t ch) {
    uint32_t f = ch;
    if (ch >= 0x21 && ch <= 0x7E) f |= (1 << 8);
    if (ch >= 0x41 && ch <= 0x5A) f |= (1 << 9);
    if (ch >= 0x61 && ch <= 0x7A) f |= (1 << 10);
    if (ch >= 0x30 && ch <= 0x39) f |= (1 << 11);
    f |= ((ch >> 6) & 1) << 12;
    f |= ((ch >> 5) & 1) << 13;
    f |= ((ch >> 4) & 1) << 14;
    f |= ((1 - ((ch >> 7) & 1))) << 15;
    f |= ((uint32_t)(~ch & 0xFF)) << 16;
    if ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A)) f |= (1 << 24);
    if ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A) ||
        (ch >= 0x30 && ch <= 0x39)) f |= (1 << 25);
    if (ch <= 0x20 || ch == 0x7F) f |= (1 << 26);
    if ((ch >= 0x21 && ch <= 0x7E) &&
        !((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A) ||
          (ch >= 0x30 && ch <= 0x39)))
        f |= (1 << 27);
    if (ch >= 0x41 && ch <= 0x5A) f |= (1 << 28);  // Uppercase letters only (not @[\]^_)
    if (ch >= 0x61 && ch <= 0x7A) f |= (1 << 29);  // Lowercase letters only (not `{|}~DEL)
    if (ch >= 0x30 && ch <= 0x3F) f |= (1 << 30);
    if (ch < 0x20) f |= (1u << 31);
    return f;
}

// Full ASCII classifier with per-neuron thresholds
// Returns: class (0-3), sets hidden_out and scores[4]
static int ascii_classify(uint8_t ch, uint16_t *hidden_out, int scores[4]) {
    uint32_t features = bnn_expand_features(ch);

    // Hidden layer
    uint16_t hidden = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t xnor = ~(features ^ ascii_hidden_weights[i]);
        int matches = sw_popcount32(xnor);
        if (matches >= ascii_hidden_thresholds[i])
            hidden |= (1 << i);
    }

    // Output layer — pick best class
    int best_class = 0, best_score = -1;
    for (int i = 0; i < 4; i++) {
        uint16_t xnor = ~(hidden ^ ascii_output_weights[i]);
        int score = sw_popcount32(xnor & 0xFFFF);
        scores[i] = score;
        if (score > best_score) {
            best_score = score;
            best_class = i;
        }
    }

    if (hidden_out) *hidden_out = hidden;
    return best_class;
}

// Compute ATOMiK fingerprint over the ASCII classifier weights
static uint64_t ascii_weight_fingerprint(void) {
    atomik_load(0, 0ULL);
    for (int i = 0; i < 16; i++)
        atomik_accum((uint64_t)ascii_hidden_weights[i]);
    for (int i = 0; i < 4; i++)
        atomik_accum((uint64_t)ascii_output_weights[i]);
    for (int i = 0; i < 16; i++)
        atomik_accum((uint64_t)ascii_hidden_thresholds[i]);
    return atomik_read(0);
}

#endif // BNN_V3_H
