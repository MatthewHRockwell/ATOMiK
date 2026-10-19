#!/usr/bin/env python3
"""
BNN Weight Designer for ASCII Character Classification
=======================================================
Designs binary weights for a 2-layer BNN that classifies 8-bit ASCII into 4 classes:
  Class 0: LETTER   (A-Z, a-z)
  Class 1: DIGIT    (0-9)
  Class 2: SYMBOL   (printable non-alphanumeric)
  Class 3: SPACE/CTRL (0x00-0x20, 0x7F)

Architecture:
  - 32-bit expanded input features
  - 16 hidden neurons (32-bit weights each), threshold = 16
  - 4 output neurons (16-bit weights each), threshold = 8
  - Activation: popcount(XNOR(input, weight)) >= threshold
"""

import random
import sys
from collections import defaultdict

# ============================================================
# Feature Expansion: 8-bit ASCII -> 32-bit feature vector
# ============================================================

def expand_features(char_val):
    """Expand an 8-bit ASCII value into a 32-bit feature vector."""
    assert 0 <= char_val <= 127

    features = 0

    # bits[7:0] = raw ASCII value
    features |= (char_val & 0xFF)

    # bit[8] = is_printable (0x21..0x7E)
    if 0x21 <= char_val <= 0x7E:
        features |= (1 << 8)

    # bit[9] = is_upper_alpha (0x41..0x5A)
    if 0x41 <= char_val <= 0x5A:
        features |= (1 << 9)

    # bit[10] = is_lower_alpha (0x61..0x7A)
    if 0x61 <= char_val <= 0x7A:
        features |= (1 << 10)

    # bit[11] = is_digit (0x30..0x39)
    if 0x30 <= char_val <= 0x39:
        features |= (1 << 11)

    # bit[12] = bit6 of char (0x40 bit — set for @ and letters)
    features |= ((char_val >> 6) & 1) << 12

    # bit[13] = bit5 of char (0x20 bit — differentiates upper/lower)
    features |= ((char_val >> 5) & 1) << 13

    # bit[14] = bit4 of char
    features |= ((char_val >> 4) & 1) << 14

    # bit[15] = NOT bit7 (bit7=0 for valid ASCII, so this is always 1 for 0-127)
    features |= (1 - ((char_val >> 7) & 1)) << 15

    # bits[23:16] = inverted raw ASCII (anti-correlation features)
    features |= ((~char_val) & 0xFF) << 16

    # bits[31:24] = derived features
    # bit[24] = is_letter (upper OR lower)
    if (0x41 <= char_val <= 0x5A) or (0x61 <= char_val <= 0x7A):
        features |= (1 << 24)

    # bit[25] = is_alphanumeric
    if (0x41 <= char_val <= 0x5A) or (0x61 <= char_val <= 0x7A) or (0x30 <= char_val <= 0x39):
        features |= (1 << 25)

    # bit[26] = is_space_or_ctrl (0x00..0x20 or 0x7F)
    if char_val <= 0x20 or char_val == 0x7F:
        features |= (1 << 26)

    # bit[27] = is_symbol (printable, not alphanumeric)
    is_printable = (0x21 <= char_val <= 0x7E)
    is_alnum = (0x41 <= char_val <= 0x5A) or (0x61 <= char_val <= 0x7A) or (0x30 <= char_val <= 0x39)
    if is_printable and not is_alnum:
        features |= (1 << 27)

    # bit[28] = uppercase letters only (not @[\]^_)
    if 0x41 <= char_val <= 0x5A:
        features |= (1 << 28)

    # bit[29] = lowercase letters only (not `{|}~DEL)
    if 0x61 <= char_val <= 0x7A:
        features |= (1 << 29)

    # bit[30] = char in 0x30..0x3F range (digit block)
    if 0x30 <= char_val <= 0x3F:
        features |= (1 << 30)

    # bit[31] = char < 0x20 (strict control)
    if char_val < 0x20:
        features |= (1 << 31)

    return features


def classify_char(char_val):
    """Return the true class for an ASCII character."""
    if (0x41 <= char_val <= 0x5A) or (0x61 <= char_val <= 0x7A):
        return 0  # LETTER
    elif 0x30 <= char_val <= 0x39:
        return 1  # DIGIT
    elif 0x21 <= char_val <= 0x7E:
        return 2  # SYMBOL
    else:
        return 3  # SPACE/CTRL


CLASS_NAMES = ["LETTER", "DIGIT", "SYMBOL", "SPACE/CTRL"]


# ============================================================
# BNN Compute
# ============================================================

def popcount(x):
    """Count number of set bits in a 32-bit integer."""
    count = 0
    while x:
        count += x & 1
        x >>= 1
    return count


def bnn_hidden(input_features, weight, threshold=16):
    """Compute one hidden neuron: popcount(XNOR(input, weight)) >= threshold."""
    xnor = ~(input_features ^ weight) & 0xFFFFFFFF
    return 1 if popcount(xnor) >= threshold else 0


def bnn_output(hidden_activations_16bit, weight, threshold=8):
    """Compute one output neuron from 16-bit hidden activation vector."""
    xnor = ~(hidden_activations_16bit ^ weight) & 0xFFFF
    return 1 if popcount(xnor) >= threshold else 0


def run_bnn(char_val, hidden_weights, hidden_thresholds, output_weights, output_thresholds):
    """Run the full BNN on one character. Returns (predicted_class, output_vector, hidden_vector)."""
    features = expand_features(char_val)

    # Hidden layer: 16 neurons
    hidden = 0
    for i in range(16):
        if bnn_hidden(features, hidden_weights[i], hidden_thresholds[i]):
            hidden |= (1 << i)

    # Output layer: 4 neurons
    outputs = []
    for i in range(4):
        out = bnn_output(hidden, output_weights[i], output_thresholds[i])
        outputs.append(out)

    # Classification: first output neuron that fires, or highest popcount
    predicted = -1
    if sum(outputs) == 1:
        predicted = outputs.index(1)
    elif sum(outputs) == 0:
        # No neuron fired — pick the one closest to threshold
        best_score = -1
        for i in range(4):
            xnor = ~(hidden ^ output_weights[i]) & 0xFFFF
            score = popcount(xnor)
            if score > best_score:
                best_score = score
                predicted = i
    else:
        # Multiple fired — pick the one with highest match
        best_score = -1
        for i in range(4):
            if outputs[i]:
                xnor = ~(hidden ^ output_weights[i]) & 0xFFFF
                score = popcount(xnor)
                if score > best_score:
                    best_score = score
                    predicted = i

    return predicted, outputs, hidden


# ============================================================
# Weight Design Strategy
# ============================================================

def compute_average_template(chars, class_id):
    """Compute the average feature vector for characters of a given class.
    For each bit position, if majority of class members have that bit set, set it."""
    members = [c for c in chars if classify_char(c) == class_id]
    if not members:
        return 0

    template = 0
    for bit in range(32):
        count = sum(1 for c in members if (expand_features(c) >> bit) & 1)
        if count > len(members) / 2:
            template |= (1 << bit)
    return template


def compute_discriminative_template(chars, target_class, variant=0):
    """Compute a weight template that maximizes separation between target class
    and other classes. Uses a scoring approach with bit-level analysis."""
    all_features = [(c, expand_features(c), classify_char(c)) for c in chars]
    target = [(f, cls) for c, f, cls in all_features if cls == target_class]
    others = [(f, cls) for c, f, cls in all_features if cls != target_class]

    # For each bit, compute how discriminative it is
    bit_scores = []
    for bit in range(32):
        target_ones = sum(1 for f, _ in target if (f >> bit) & 1)
        target_zeros = len(target) - target_ones
        other_ones = sum(1 for f, _ in others if (f >> bit) & 1)
        other_zeros = len(others) - other_ones

        # Score: how much does this bit being 1 (or 0) help identify the target class?
        score_if_one = target_ones / max(1, len(target)) - other_ones / max(1, len(others))
        score_if_zero = target_zeros / max(1, len(target)) - other_zeros / max(1, len(others))
        bit_scores.append((bit, score_if_one, score_if_zero))

    # Build template: for each bit, pick the value that best identifies the target class
    template = 0
    for bit, score_one, score_zero in bit_scores:
        if score_one >= score_zero:
            template |= (1 << bit)
        # else: leave bit as 0

    # Apply variant-specific perturbations to create diversity among neurons
    if variant > 0:
        # Modify template to create different perspectives on the same class
        # Flip some less-discriminative bits to create ensemble diversity
        sorted_bits = sorted(bit_scores, key=lambda x: abs(x[1] - x[2]))
        # Flip the least discriminative bits based on variant
        for i in range(min(variant, len(sorted_bits))):
            bit = sorted_bits[i][0]
            template ^= (1 << bit)

    return template


def find_optimal_threshold(chars, weight, target_class, search_range=range(10, 24)):
    """Find the threshold that maximizes classification accuracy for a single hidden neuron."""
    best_thresh = 16
    best_score = -1

    for thresh in search_range:
        tp, fp, tn, fn = 0, 0, 0, 0
        for c in chars:
            features = expand_features(c)
            fires = bnn_hidden(features, weight, thresh)
            is_target = (classify_char(c) == target_class)
            if fires and is_target:
                tp += 1
            elif fires and not is_target:
                fp += 1
            elif not fires and is_target:
                fn += 1
            else:
                tn += 1

        # F1-like score
        precision = tp / max(1, tp + fp)
        recall = tp / max(1, tp + fn)
        f1 = 2 * precision * recall / max(0.001, precision + recall)

        if f1 > best_score:
            best_score = f1
            best_thresh = thresh

    return best_thresh, best_score


def design_hidden_weights(chars):
    """Design 16 hidden neuron weights with optimized thresholds."""
    weights = [0] * 16
    thresholds = [16] * 16

    for class_id in range(4):
        for variant in range(4):
            neuron_idx = class_id * 4 + variant
            w = compute_discriminative_template(chars, class_id, variant)
            weights[neuron_idx] = w

            # Find optimal threshold
            thresh, score = find_optimal_threshold(chars, w, class_id)
            thresholds[neuron_idx] = thresh
            print(f"  Hidden neuron {neuron_idx:2d} (class={CLASS_NAMES[class_id]:10s}, var={variant}): "
                  f"weight=0x{w:08X}, threshold={thresh}, f1={score:.3f}")

    return weights, thresholds


def design_output_weights(chars, hidden_weights, hidden_thresholds):
    """Design 4 output neuron weights from hidden layer activations."""
    # First, compute hidden activations for all characters
    activations = {}
    for c in chars:
        features = expand_features(c)
        hidden = 0
        for i in range(16):
            if bnn_hidden(features, hidden_weights[i], hidden_thresholds[i]):
                hidden |= (1 << i)
        activations[c] = hidden

    # For each output class, find the weight that best separates it
    output_weights = [0] * 4
    output_thresholds = [8] * 4

    for class_id in range(4):
        target_activations = [activations[c] for c in chars if classify_char(c) == class_id]
        other_activations = [activations[c] for c in chars if classify_char(c) != class_id]

        # Average template approach for output weights
        best_weight = 0
        for bit in range(16):
            target_ones = sum(1 for a in target_activations if (a >> bit) & 1)
            other_ones = sum(1 for a in other_activations if (a >> bit) & 1)

            target_rate = target_ones / max(1, len(target_activations))
            other_rate = other_ones / max(1, len(other_activations))

            if target_rate >= other_rate:
                best_weight |= (1 << bit)

        output_weights[class_id] = best_weight

        # Find optimal output threshold
        best_thresh = 8
        best_score = -1
        for thresh in range(4, 14):
            tp, fp = 0, 0
            fn, tn = 0, 0
            for c in chars:
                hidden = activations[c]
                fires = bnn_output(hidden, best_weight, thresh)
                is_target = (classify_char(c) == class_id)
                if fires and is_target:
                    tp += 1
                elif fires and not is_target:
                    fp += 1
                elif not fires and is_target:
                    fn += 1
                else:
                    tn += 1

            precision = tp / max(1, tp + fp)
            recall = tp / max(1, tp + fn)
            f1 = 2 * precision * recall / max(0.001, precision + recall)

            if f1 > best_score:
                best_score = f1
                best_thresh = thresh

        output_thresholds[class_id] = best_thresh
        print(f"  Output neuron {class_id} ({CLASS_NAMES[class_id]:10s}): "
              f"weight=0x{best_weight:04X}, threshold={best_thresh}, f1={best_score:.3f}")

    return output_weights, output_thresholds


# ============================================================
# Iterative Refinement
# ============================================================

def refine_hidden_weights(chars, hidden_weights, hidden_thresholds, iterations=50):
    """Refine hidden weights by flipping bits and keeping improvements."""
    print("\n--- Refining hidden weights ---")

    for iteration in range(iterations):
        improved = False
        for neuron_idx in range(16):
            target_class = neuron_idx // 4
            w = hidden_weights[neuron_idx]
            thresh = hidden_thresholds[neuron_idx]

            # Current score
            _, current_score = find_optimal_threshold(chars, w, target_class, [thresh])

            # Try flipping each bit
            for bit in range(32):
                candidate = w ^ (1 << bit)
                new_thresh, new_score = find_optimal_threshold(chars, candidate, target_class)

                if new_score > current_score + 0.001:
                    hidden_weights[neuron_idx] = candidate
                    hidden_thresholds[neuron_idx] = new_thresh
                    w = candidate
                    thresh = new_thresh
                    current_score = new_score
                    improved = True

        if not improved:
            print(f"  Converged at iteration {iteration}")
            break

    # Print refined results
    for i in range(16):
        target_class = i // 4
        _, score = find_optimal_threshold(chars, hidden_weights[i], target_class, [hidden_thresholds[i]])
        print(f"  Refined neuron {i:2d} ({CLASS_NAMES[target_class]:10s}): "
              f"weight=0x{hidden_weights[i]:08X}, threshold={hidden_thresholds[i]}, f1={score:.3f}")

    return hidden_weights, hidden_thresholds


# ============================================================
# Brute-force output weight search
# ============================================================

def brute_force_output_weights(chars, hidden_weights, hidden_thresholds):
    """Try many output weight combinations to maximize overall accuracy."""
    print("\n--- Brute-force output weight search ---")

    # Compute hidden activations for all chars
    activations = {}
    for c in chars:
        features = expand_features(c)
        hidden = 0
        for i in range(16):
            if bnn_hidden(features, hidden_weights[i], hidden_thresholds[i]):
                hidden |= (1 << i)
        activations[c] = hidden

    # Analyze hidden activation patterns per class
    print("\n  Hidden activation patterns per class:")
    class_patterns = defaultdict(list)
    for c in chars:
        cls = classify_char(c)
        class_patterns[cls].append(activations[c])

    for cls in range(4):
        patterns = class_patterns[cls]
        unique = set(patterns)
        print(f"    {CLASS_NAMES[cls]:10s}: {len(patterns)} chars, {len(unique)} unique patterns")
        # Show bit frequency
        for bit in range(16):
            count = sum(1 for p in patterns if (p >> bit) & 1)
            if count > 0:
                print(f"      bit {bit:2d}: {count}/{len(patterns)} ({100*count/len(patterns):.0f}%)")

    # For each output neuron, exhaustively search threshold with adaptive weight
    output_weights = [0] * 4
    output_thresholds = [8] * 4

    for class_id in range(4):
        best_accuracy = 0
        best_w = 0
        best_t = 8

        # Generate candidate weights from class activation patterns
        target_patterns = set(class_patterns[class_id])
        other_patterns = set()
        for cls in range(4):
            if cls != class_id:
                other_patterns.update(class_patterns[cls])

        # Try each unique target pattern as a weight
        candidates = set()
        for p in target_patterns:
            candidates.add(p & 0xFFFF)
            candidates.add((~p) & 0xFFFF)

        # Also try averaged templates
        avg = 0
        for bit in range(16):
            count = sum(1 for p in class_patterns[class_id] if (p >> bit) & 1)
            if count > len(class_patterns[class_id]) / 2:
                avg |= (1 << bit)
        candidates.add(avg)
        candidates.add((~avg) & 0xFFFF)

        # Also try all-zeros and all-ones for baseline
        candidates.add(0x0000)
        candidates.add(0xFFFF)

        # Try the ideal neuron group mask (bits for this class's neurons)
        group_mask = 0
        for i in range(4):
            group_mask |= (1 << (class_id * 4 + i))
        candidates.add(group_mask)
        candidates.add((~group_mask) & 0xFFFF)

        # Try combination of target group ON + other groups OFF
        for other_cls in range(4):
            if other_cls != class_id:
                mask = group_mask
                for i in range(4):
                    _other_bit = other_cls * 4 + i
                    # We want other bits to be 0 in activation, so weight bit should be 0 for XNOR=1 when act=0
                    # XNOR(0,0)=1, XNOR(1,0)=0 -- so weight=0 matches inactive neurons
                candidates.add(mask)

        # Also generate random candidates
        random.seed(42)
        for _ in range(1000):
            candidates.add(random.randint(0, 0xFFFF))

        for w in candidates:
            for thresh in range(3, 15):
                correct = 0
                total = 0
                for c in chars:
                    hidden = activations[c]
                    fires = bnn_output(hidden, w, thresh)
                    is_target = (classify_char(c) == class_id)
                    if fires == is_target:
                        correct += 1
                    total += 1

                acc = correct / total
                if acc > best_accuracy:
                    best_accuracy = acc
                    best_w = w
                    best_t = thresh

        output_weights[class_id] = best_w
        output_thresholds[class_id] = best_t
        print(f"\n  Output neuron {class_id} ({CLASS_NAMES[class_id]:10s}): "
              f"weight=0x{best_w:04X}, threshold={best_t}, accuracy={best_accuracy:.3f}")

    return output_weights, output_thresholds


# ============================================================
# Full system test
# ============================================================

def test_full_system(chars, hidden_weights, hidden_thresholds, output_weights, output_thresholds):
    """Test the full BNN on all characters and report accuracy."""
    print("\n" + "=" * 70)
    print("FULL SYSTEM TEST")
    print("=" * 70)

    correct = 0
    total = 0
    class_correct = [0] * 4
    class_total = [0] * 4
    misclassified = []

    for c in chars:
        true_class = classify_char(c)
        predicted, outputs, hidden = run_bnn(c, hidden_weights, hidden_thresholds,
                                              output_weights, output_thresholds)

        class_total[true_class] += 1
        if predicted == true_class:
            correct += 1
            class_correct[true_class] += 1
        else:
            char_repr = chr(c) if 0x20 < c < 0x7F else f"\\x{c:02X}"
            misclassified.append((c, char_repr, true_class, predicted, outputs, hidden))
        total += 1

    print(f"\nOverall accuracy: {correct}/{total} ({100*correct/total:.1f}%)")
    print("\nPer-class accuracy:")
    for i in range(4):
        pct = 100 * class_correct[i] / max(1, class_total[i])
        print(f"  {CLASS_NAMES[i]:10s}: {class_correct[i]:3d}/{class_total[i]:3d} ({pct:.1f}%)")

    if misclassified:
        print(f"\nMisclassified ({len(misclassified)} chars):")
        for c, char_repr, true_cls, pred_cls, outputs, hidden in misclassified[:30]:
            print(f"  0x{c:02X} '{char_repr}': true={CLASS_NAMES[true_cls]}, "
                  f"pred={CLASS_NAMES[pred_cls]}, "
                  f"outputs={outputs}, hidden=0x{hidden:04X}")
        if len(misclassified) > 30:
            print(f"  ... and {len(misclassified) - 30} more")

    return correct, total


# ============================================================
# Output C array
# ============================================================

def output_c_array(hidden_weights, hidden_thresholds, output_weights, output_thresholds):
    """Output the weights as a C structure for firmware."""
    print("\n" + "=" * 70)
    print("C WEIGHT ARRAY")
    print("=" * 70)

    c_code = """
/* BNN ASCII Classifier Weights
 * Generated by train_ascii_classifier.py
 *
 * Architecture: 32-bit input -> 16 hidden (32-bit weights) -> 4 output (16-bit weights)
 * Feature expansion: 8-bit ASCII -> 32-bit via expand_features()
 * Classes: 0=LETTER, 1=DIGIT, 2=SYMBOL, 3=SPACE/CTRL
 */

#include <stdint.h>

/* Hidden layer: 16 neurons, each with 32-bit weight and threshold */
static const uint32_t bnn_hidden_weights[16] = {
"""
    for i in range(16):
        target_class = i // 4
        c_code += f"    0x{hidden_weights[i]:08X}U,  /* neuron {i:2d}: {CLASS_NAMES[target_class]} (thresh={hidden_thresholds[i]}) */\n"
    c_code += "};\n\n"

    c_code += "static const uint8_t bnn_hidden_thresholds[16] = {\n    "
    c_code += ", ".join(f"{t}" for t in hidden_thresholds)
    c_code += "\n};\n\n"

    c_code += "/* Output layer: 4 neurons, each with 16-bit weight and threshold */\n"
    c_code += "static const uint16_t bnn_output_weights[4] = {\n"
    for i in range(4):
        c_code += f"    0x{output_weights[i]:04X},  /* class {i}: {CLASS_NAMES[i]} (thresh={output_thresholds[i]}) */\n"
    c_code += "};\n\n"

    c_code += "static const uint8_t bnn_output_thresholds[4] = {\n    "
    c_code += ", ".join(f"{t}" for t in output_thresholds)
    c_code += "\n};\n\n"

    # Also output feature expansion function
    c_code += """/* Feature expansion: 8-bit ASCII -> 32-bit feature vector */
static uint32_t bnn_expand_features(uint8_t ch) {
    uint32_t f = ch;  /* bits[7:0] = raw ASCII */

    /* bit[8] = is_printable */
    if (ch >= 0x21 && ch <= 0x7E) f |= (1 << 8);
    /* bit[9] = is_upper_alpha */
    if (ch >= 0x41 && ch <= 0x5A) f |= (1 << 9);
    /* bit[10] = is_lower_alpha */
    if (ch >= 0x61 && ch <= 0x7A) f |= (1 << 10);
    /* bit[11] = is_digit */
    if (ch >= 0x30 && ch <= 0x39) f |= (1 << 11);
    /* bit[12] = bit6 of char */
    f |= ((ch >> 6) & 1) << 12;
    /* bit[13] = bit5 of char */
    f |= ((ch >> 5) & 1) << 13;
    /* bit[14] = bit4 of char */
    f |= ((ch >> 4) & 1) << 14;
    /* bit[15] = NOT bit7 */
    f |= ((1 - ((ch >> 7) & 1))) << 15;
    /* bits[23:16] = inverted raw ASCII */
    f |= ((uint32_t)(~ch & 0xFF)) << 16;
    /* bit[24] = is_letter */
    if ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A)) f |= (1 << 24);
    /* bit[25] = is_alphanumeric */
    if ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A) || (ch >= 0x30 && ch <= 0x39)) f |= (1 << 25);
    /* bit[26] = is_space_or_ctrl */
    if (ch <= 0x20 || ch == 0x7F) f |= (1 << 26);
    /* bit[27] = is_symbol */
    if ((ch >= 0x21 && ch <= 0x7E) && !((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A) || (ch >= 0x30 && ch <= 0x39)))
        f |= (1 << 27);
    /* bit[28] = 0x40..0x5F range */
    if (ch >= 0x40 && ch <= 0x5F) f |= (1 << 28);
    /* bit[29] = 0x60..0x7F range */
    if (ch >= 0x60 && ch <= 0x7F) f |= (1 << 29);
    /* bit[30] = 0x30..0x3F range */
    if (ch >= 0x30 && ch <= 0x3F) f |= (1 << 30);
    /* bit[31] = strict control */
    if (ch < 0x20) f |= (1u << 31);

    return f;
}

/* BNN inference: returns class 0-3 */
static int bnn_classify(uint8_t ch) {
    uint32_t features = bnn_expand_features(ch);

    /* Hidden layer */
    uint16_t hidden = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t xnor = ~(features ^ bnn_hidden_weights[i]);
        int matches = __builtin_popcount(xnor);
        if (matches >= bnn_hidden_thresholds[i])
            hidden |= (1 << i);
    }

    /* Output layer — pick class with highest match score */
    int best_class = 0;
    int best_score = -1;
    for (int i = 0; i < 4; i++) {
        uint16_t xnor = ~(hidden ^ bnn_output_weights[i]);
        int score = __builtin_popcount(xnor & 0xFFFF);
        if (score > best_score) {
            best_score = score;
            best_class = i;
        }
    }

    return best_class;
}
"""
    print(c_code)
    return c_code


# ============================================================
# Main
# ============================================================

def main():
    chars = list(range(128))

    # Show class distribution
    print("Class distribution:")
    for cls in range(4):
        members = [c for c in chars if classify_char(c) == cls]
        print(f"  {CLASS_NAMES[cls]:10s}: {len(members)} chars")

    # Verify feature expansion
    print("\nSample feature expansions:")
    for c in [0x00, 0x20, 0x30, 0x41, 0x61, 0x7E, 0x7F]:
        f = expand_features(c)
        char_repr = chr(c) if 0x20 < c < 0x7F else f"\\x{c:02X}"
        print(f"  0x{c:02X} '{char_repr}': features=0x{f:08X} (class={CLASS_NAMES[classify_char(c)]})")

    # Phase 1: Design initial hidden weights
    print("\n--- Phase 1: Initial hidden weight design ---")
    hidden_weights, hidden_thresholds = design_hidden_weights(chars)

    # Phase 2: Refine hidden weights
    hidden_weights, hidden_thresholds = refine_hidden_weights(chars, hidden_weights, hidden_thresholds, iterations=100)

    # Phase 3: Design output weights with brute force search
    output_weights, output_thresholds = brute_force_output_weights(chars, hidden_weights, hidden_thresholds)

    # Phase 4: Test full system
    correct, total = test_full_system(chars, hidden_weights, hidden_thresholds, output_weights, output_thresholds)

    # Phase 5: If accuracy is not perfect, try re-refining
    if correct < total:
        print("\n--- Phase 5: Re-refining with adjusted strategy ---")
        # Try swapping roles: re-assign neurons to classes based on actual performance
        # and try flipping output weight bits
        for out_class in range(4):
            w = output_weights[out_class]
            t = output_thresholds[out_class]
            for bit in range(16):
                candidate_w = w ^ (1 << bit)
                for thresh in range(max(3, t-2), min(14, t+3)):
                    # Quick eval
                    improved_correct = 0
                    for c in chars:
                        features = expand_features(c)
                        hidden = 0
                        for i in range(16):
                            if bnn_hidden(features, hidden_weights[i], hidden_thresholds[i]):
                                hidden |= (1 << i)
                        # Test this specific output neuron
                        true_class = classify_char(c)
                        fires = bnn_output(hidden, candidate_w, thresh)
                        if fires == (true_class == out_class):
                            improved_correct += 1

                    # Compare
                    current_correct = 0
                    for c in chars:
                        features = expand_features(c)
                        hidden = 0
                        for i in range(16):
                            if bnn_hidden(features, hidden_weights[i], hidden_thresholds[i]):
                                hidden |= (1 << i)
                        true_class = classify_char(c)
                        fires = bnn_output(hidden, w, t)
                        if fires == (true_class == out_class):
                            current_correct += 1

                    if improved_correct > current_correct:
                        output_weights[out_class] = candidate_w
                        output_thresholds[out_class] = thresh
                        w = candidate_w
                        t = thresh

        print("  Output weights after refinement:")
        for i in range(4):
            print(f"    Output {i} ({CLASS_NAMES[i]}): weight=0x{output_weights[i]:04X}, threshold={output_thresholds[i]}")

        correct, total = test_full_system(chars, hidden_weights, hidden_thresholds, output_weights, output_thresholds)

    # Output C array
    c_code = output_c_array(hidden_weights, hidden_thresholds, output_weights, output_thresholds)

    # Save C header
    with open("/home/mattrock/Projects/ATOMiK/software/bnn_training/bnn_weights.h", "w") as f:
        f.write(c_code)
    print("\nWeights saved to bnn_weights.h")

    return correct == total


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
