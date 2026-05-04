# ATOMiK OS — Business Model

> **The economic reframe: stop selling subscriptions, sell tokens.**
> AI-native apps in ATOMiK OS pay for what they use, when they use it.
> No idle compute, no monthly fee for software you didn't open this
> month.

---

## 1. The pitch in one paragraph

ATOMiK OS apps are AI-native. The Document, the dock, the agent — every
piece of intelligence is mediated by a swappable AI provider. Each
provider has a token-cost map. The user has a token wallet. Doing work
costs tokens — and only tokens. **You don't subscribe to "AI desktop
assistant ($20/mo)"; you pay 0.0003¢ per word the agent writes when
*you* ask it to.** The minimum viable interaction (a tiny field-delta on
the Document) is far cheaper than any conventional cloud app's monthly
fee, because conventional cloud apps charge you for the whole compute
budget regardless of what you used.

---

## 2. Why token-pricing is uniquely possible on ATOMiK

Three things make token-pricing the right model here:

1. **Field-delta UIs are tiny.** The whole "render a calendar" operation
   is a ~340-byte manifest + a couple dozen field deltas. You're not
   paying to render frames or stream pixels — you're paying for the
   *meaning* (the agent's natural-language interpretation), and the
   meaning is small.

2. **The accumulator survives.** Once a Document is morphed into a
   calendar, it stays a calendar. You don't re-pay the agent every
   frame. You only pay when *intent changes*. ATOMiK's delta-state
   self-inverse XOR algebra means undo is free — you can roll back a
   morph at zero cost.

3. **AI provider is a pluggable parameter.** The same Document can
   route the next command to Anthropic, OpenAI, a local model, or a
   smaller / cheaper specialist. Cost-aware routing: simple rewrite
   commands go to a cheap fast model; "summarize my whole inbox" goes
   to the bigger one. The OS picks per-task.

---

## 3. Who pays what

### Per user (pay-per-use)

| Action | Approx tokens | Approx cost (Claude Sonnet 4.6) |
|--------|---------------|---------------------------------|
| `load calendar` (mock command) | 0 — local parse | $0.00 |
| `set primitive grid` (LLM mode) | ~80 in / 30 out | < $0.001 |
| Summarize a 1k-word doc | ~1.2k in / 200 out | ~$0.008 |
| Generate a daily Brief from 3 sources | ~3k in / 400 out | ~$0.02 |
| Free-form "rewrite this paragraph" | ~250 in / 250 out | ~$0.005 |
| Calendar.create_event via NL | ~120 in / 60 out | < $0.002 |

For a typical knowledge worker the all-in monthly AI cost lands
somewhere between $1 and $20 — *fractions* of what they currently pay
for a stack of fixed-price subscriptions (ChatGPT Plus + Notion AI +
Slack AI + Gmail AI + Calendar AI + …).

### Margin

ATOMiK OS marks up the raw provider cost (e.g. 25%) and pockets the
spread. The user sees the marked-up price *before* committing. No
hidden fees, no metered subscription, no "you used 90% of your quota,
upgrade now" emails.

### Free tier

Local-only commands (the v0.10 hand-rolled parser, the agent's local
Markov predictor, anything that doesn't hit a remote LLM) are **free
forever**. Power users can also bring their own API keys and bypass
the markup entirely — we still charge a small flat platform fee for
the OS itself, similar to how a phone carrier charges device + service
separately.

---

## 4. Why this is structurally cheaper than subscription SaaS

A conventional Calendar SaaS charges you $5/month whether you opened
the app once or 1000 times. The vendor has to price for the average
user, which means light users overpay and heavy users underpay.

A token-priced ATOMiK Document:

- **Light user** (asks the agent to schedule something twice a week):
  ~$0.30/month. They keep $4.70 they would have spent on the SaaS.
- **Heavy user** (live AI summarization of every meeting transcript):
  $30/month. They get *more* than the SaaS would have given them, but
  they pay only for what they used.

Both groups win because we eliminate the "subsidize the heavy users"
contract that subscription SaaS forces.

---

## 5. Why this is structurally cheaper than "AI-included" SaaS

Notion AI, Slack AI, Microsoft Copilot etc. are bundling AI into their
existing $X/seat fee. Their pricing assumes worst-case usage and they
quietly throttle heavy users. Customers pay $30/seat extra for AI they
might use a handful of times a month.

ATOMiK OS unbundles. The "app" is just a manifest. The "AI" is just
tokens. Pay for the bytes you actually move.

---

## 6. Privacy as a feature of the cost model

Local-only operations cost zero tokens. The Markov agent, the dock
sort, the Document's hand-rolled parser, persistence, framebuffer —
all run on-device, on the ATOMiK delta-state hardware, with no cloud
roundtrip. **The cheapest path is also the most private path.** That's
not an accident — it's the architecture.

When the user does opt into a cloud LLM, the OS shows them the request
*before* it goes out. They can edit, redact, or cancel. The OS
maintains a full audit log of which token-spends went to which
provider with which payload.

---

## 7. Distribution + adoption

### Phase 1 — board demo
Hand-built ATOMiK OS image on the AX7020 reference board. Token wallet
is a debug field. Goal: prove the architecture, get user confidence.

### Phase 2 — laptop build
ATOMiK OS as the OS of the planned standalone laptop. Real internet,
real LLM backends, real wallet UI. Sells the hardware AND the OS.

### Phase 3 — distribution
- ATOMiK OS as a downloadable image for x86/ARM laptops
- App developers ship manifests, not binaries
- Token marketplace: providers compete on price + latency for the
  same task
- Manifest registry — install an "app" by entering a URL

### Phase 4 — enterprise
Per-team token budgets, SSO, usage analytics. The IT-friendly version
of the same model.

---

## 8. What we're NOT doing

- Not a subscription. The OS itself is a one-time license fee or
  bundled with hardware.
- Not ad-supported. The user pays the actual cost; ads would corrupt
  the trust model that makes the audit log valuable.
- Not vendor-locked. Token routing is configurable. BYO API keys is a
  first-class option.
- Not "AI everywhere." Local primitives stay local. Tokens are spent
  only when the user explicitly hits an LLM-backed command.
