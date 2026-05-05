# MiniLM-L6 embedding router on AX7020 — evaluation

> Closes task #56. Question: should v0.19c add a 22 MB ONNX embedding
> model on the board to handle prompts the Jaccard trigram classifier
> misses, or is the cheaper path to expand the trigram pattern table?
>
> **Verdict: defer the ONNX route, expand the pattern table instead.**
> Reasoning below. This document is the proof-of-decision artifact —
> if a partner conversation later lands a use case that requires the
> ONNX path, this is the doc to reread.

---

## Hardware envelope

| Resource | AX7020 budget | MiniLM-L6 ONNX cost | Headroom |
|----------|---------------|---------------------|----------|
| RAM (DDR3) | 512 MB total, ~256 MB usable | ~70 MB peak (model + activations) | tight |
| Flash | 16 MB QSPI | 22 MB ONNX (compressed: ~12 MB int8) | overflow |
| CPU | 100 MHz NaxRiscv RV64GC, in-order, no SIMD | needs vector dot products | painful |
| Inference latency target | <100 ms for snappy UX | est. 300–800 ms per 64-token query | misses target |

The 22 MB model doesn't fit in flash. We'd have to load over USB or
network, which contradicts the "offline / private / robust at trade
shows" pitch. Even if loaded into DDR3, dot-product-heavy inference
on a 100 MHz in-order CPU without SIMD is roughly 3–8× our latency
target.

## Software stack

| Option | Status on RV64GC | Verdict |
|--------|------------------|---------|
| ONNX Runtime | builds for RV64 but heavy (~50 MB lib) | too big for 16 MB QSPI |
| ggml | builds for RV64, lean (~2 MB) | viable but still doesn't beat the latency math |
| candle (Rust) | RV64 unproven | risky to bet on |
| custom int8 dot-product loop | doable, ~500 LOC | feasible long-term, not a v0.19 task |

The cleanest stack is `ggml`, but the underlying CPU math is the
gating factor, not the runtime.

## Cheaper alternative: expand the trigram pattern table

The Jaccard classifier currently has 15 labels with 4–6 training
utterances each. Threshold is 0.05. The labels cover the core
ATOMiK command grammar but miss creative paraphrases. Examples that
should classify but don't today:

- "give me my agenda" → load calendar (currently misses)
- "what's coming up" → load brief (currently misses)
- "make this look like Notion" → set primitive list (currently misses)
- "vibe is too loud" → set accent green (impossible to classify cheaply)

The first three are fixable by adding 5–8 training utterances per
label, ~1 KB of additional data. The fourth is a genuine semantic
case that needs embeddings — but it's also vanishingly rare in
practice for a UI-modification grammar.

## Decision

- **v0.19c (now):** expand the trigram pattern table with 5–8 more
  utterances per label. Cost: ~1 KB binary growth. Latency: still
  sub-millisecond. Coverage: rough estimate 95%+ of paraphrases for
  the same 15 labels.
- **v0.20+ (deferred):** keep ONNX/MiniLM as a candidate for the
  ATOMiK laptop build (see `BOOT_IMAGE.md`) where the CPU envelope
  is friendlier (1–2 GHz multi-core with NEON/RVV). Not a near-term
  AX7020 task.

## What success looks like

A user types any of the example phrases above into Document; the
local-intent classifier correctly routes them within the existing
sub-millisecond budget. No model file. No network. No new dependency.

That's the v0.19c deliverable.
