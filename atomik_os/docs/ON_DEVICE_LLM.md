# On-Device LLM — Research + Plan

> Closes task #46. Establishes the realistic envelope for offline AI on
> ATOMiK hardware and proposes a phased path that ships value at every
> step.

---

## Why offline matters

Three reasons we need a story for AI without the cloud:

1. **Privacy.** The BUSINESS_MODEL.md pitch is "the cheapest path is
   the most private path." Local inference makes that literally true.
2. **Latency.** UART-relayed Claude calls are ~2-3 sec round-trip from
   the AX7020. Local inference can be 50-200 ms even on slow hardware,
   which is the difference between "feels like AI" and "feels like the
   internet."
3. **Demo robustness.** Trade shows, investor demos, hardware
   roadshows often have terrible Wi-Fi. A demo that works without an
   internet connection is dramatically more reliable.

## Realistic target: an intent classifier, not a full LLM

ATOMiK OS doesn't need a chat-completion model on-device. It needs to
**translate user intent into a bounded vocabulary of field-delta
commands.** That's a *classification* problem, not a generation
problem. A 5–50 MB model with a softmax over the command grammar
beats a 4 GB Llama running at 0.5 tok/s on slow hardware every time.

The space:

| Class | Examples | Suitability |
|-------|----------|-------------|
| Intent classifier | DistilBERT, MobileBERT | **best fit** — fast, small, reliable for known commands |
| Tiny generative LLM | Phi-2 (2.7B), TinyLlama (1.1B), Llama 3.2 1B | OK for laptop, painful on AX7020 |
| Embedding-only model | MiniLM-L6 (~22 MB) | Useful for similarity routing of unseen prompts to known commands |
| Logistic regression on word embeddings | hand-trained scikit-learn | **smallest viable** — kilobytes, sub-ms inference |

## Hardware constraints

### AX7020 reference board (current)

- NaxRiscv RV64GC @ 100 MHz, in-order, no SIMD
- 512 MB DDR3, but the OS is constrained to ~256 MB usable for apps
- No GPU, no NPU, no SIMD intrinsics
- Linux 6.9 + glibc

A 1B-parameter LLM at ~1.5 GB FP16 doesn't fit even if we wanted to
run it. A quantized 4-bit version is ~700 MB — fits memory but at
0.5 tok/s the UX is unusable.

Realistic ceiling for AX7020: a **MiniLM-class embedding model**
(~22 MB ONNX) running at ~50–200 ms per 128-token query, or a hand-
trained logistic regression / FastText classifier at sub-millisecond
latency.

### Planned ATOMiK laptop build

Open question — depends on what SoC ships. Reasonable assumptions:

- 4 cores, 1.5–2 GHz, NEON or RVV-class SIMD
- 4–8 GB DDR4
- Maybe a small NPU

That envelope makes a 1–3B-parameter quantized LLM tractable at
~5–15 tok/s. Phi-3 mini (3.8B int4 ≈ 2.4 GB) is the sweet spot.

## Software options

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| **llama.cpp** (riscv64 build) | mature, ggml-quantized, runs everywhere | not yet performant on RV64 in-order | **best for laptop**, OK for AX7020 with small models |
| **candle** (Rust) | clean codebase, ONNX-ish | less RISC-V mature | watch |
| **MLC-LLM** | TVM-tuned per device | huge build dep | too heavy for AX7020 |
| **ggml** (no llama.cpp wrapper) | leaner, used by whisper.cpp | fewer model paths | viable for the embedding-only path |
| **Custom minimal transformer** in pure C | full control, smallest binary | reinventing tokenizers, attention | only if we need to fit < 1 MB |
| **scikit-learn / FastText** classifiers | tiny, instant | not generative | **best for v0.18 intent path** |

## Phased plan

### v0.18 — Local intent classifier (target: AX7020)

- Train a small classifier (logistic regression on FastText
  embeddings, or a 4-layer MLP on bag-of-character-ngrams) over a
  hand-labeled dataset of (utterance, ATOMiK command) pairs.
- Target size: < 500 KB on disk.
- Latency target: < 50 ms for a 64-character utterance on 100 MHz
  NaxRiscv.
- Inference path: Python (or transpiled C) reading the model weights
  from `/tmp/atomik_os_intent.bin`.
- Hooked into `llm.c` as a new provider: `local-intent` chosen by
  default when the user types `/ai` and the wallet has zero balance.
- Coverage: handles ~90% of common commands ("show calendar", "make
  it red", "add buy milk", "summarize my day"). Unknown utterances
  fall back to the cloud LLM (or to a "I don't know yet" message if
  the user has the cloud disabled).

### v0.19 — Embedding-router

- Add an embedding model (MiniLM-L6 ONNX, 22 MB) that maps any
  utterance to a vector and finds nearest among the known command
  templates by cosine similarity.
- Lets the system handle phrasings the classifier didn't train on,
  without paying for cloud inference.

### v1.1 — Quantized small LLM on the laptop

- Target: Phi-3 mini int4 (~2.4 GB) via llama.cpp.
- Use case: open-ended generation in Notes / Brief, which the intent
  classifier can't handle.
- Same `llm.c` provider abstraction; just a new backend that pipes
  prompts to a local llama.cpp process.

### Long-term — ASIC NPU on ATOMiK silicon

- The dynamic-vproc work parked in `project_atomik_os_design.md` lines
  up here: allocate N delta-cells as a vector accelerator for matrix-
  multiply, run the embedding model on the ATOMiK substrate. That's
  the multi-year story.

## Smallest viable prototype

Today's deliverable: a single-file Python intent classifier trained
on a hand-labeled corpus, that emits ATOMiK commands. Ships as
`tools/atomik_local_intent.py`. No model file > 100 KB. No cloud
calls. Demonstrates the v0.18 path is real.

See the prototype in `tools/atomik_local_intent.py`.
