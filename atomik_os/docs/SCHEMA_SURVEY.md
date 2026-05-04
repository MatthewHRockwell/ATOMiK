# Generative-UI Schema Survey

> Decision input for the schema-interop track. Compares the leading
> generative-UI manifest formats against ATOMiK OS's 5-primitive
> invariant frame.

---

## Our 5 primitives

| Primitive | Use case | Field convention |
|-----------|----------|------------------|
| `LIST`  | rows of textual items (Tasks, Inbox, Search results) | 0=header, 1=items, 2=footer |
| `CARD`  | one big record (Brief, Detail, Notification body)   | 0=title, 1=subtitle, 2=body |
| `GRID`  | tabular cells (Calendar, Album view, Kanban cells)  | 0=title, 1=cells |
| `FEED`  | timeline / activity stream (PRs, mentions, history) | 0=title, 1=feed-items |
| `CONVO` | alternating user/agent bubbles                      | 0=title, 1=turns |

These cover ~85% of common app UIs. New primitives are cheap to add but
we should resist proliferation — five is the right zoom level.

---

## Candidate schemas

### 1. Microsoft AdaptiveCards (v1.5+)

- **Maturity:** highest of any candidate. Shipped in Outlook, Teams,
  Cortana, Skype since 2017. Multi-platform native renderers exist.
- **Schema:** rich JSON with TextBlock / Container / ColumnSet /
  FactSet / Image / Media / ActionSet / RichTextBlock.
- **Coverage of our primitives:**
  - `CARD`: native fit (TextBlock as title + body)
  - `LIST`: derives via Container with TextBlocks
  - `GRID`: derives via ColumnSet (we already do this in our adapter)
  - `FEED`: harder — no built-in "feed item" but ActionSet+Container approximates
  - `CONVO`: no first-class concept, but Container with alternating styles works
- **Adapter status:** **shipped** (`adapters/adaptivecards_to_atomik.py`).
- **Verdict:** **most defensible interop bet** — broadest installed
  base, longest stability commitment, mature renderers we can crib
  layout heuristics from. Score: 9/10.

### 2. Thesys / GenUI (or whatever the latest "agent-rendered UI" SaaS is)

- **Maturity:** newer (2024+), focus is LLM-native generation.
- **Schema:** typically TypeScript types or a thin JSON, often closer
  to React's shape than to a generic UI tree. Less standardized.
- **Coverage of our primitives:**
  - `LIST` / `CARD` / `FEED`: native ergonomic match
  - `GRID`: present, varies per impl
  - `CONVO`: most have a "Chat" component
- **Adapter status:** not started.
- **Verdict:** more LLM-native than AdaptiveCards but the schemas are
  vendor-flux. Wait until one wins or our adapter targets the API
  output (a stable surface) rather than internal types. Score: 6/10.

### 3. Vercel v0

- **Maturity:** the dominant text-prompt → React UI service.
- **Schema:** there isn't really one — output is JSX/React component
  trees. We'd consume the rendered HTML or a stripped-down JSON
  introspection of the tree.
- **Coverage:** translation step is heavier (React → 5 primitives)
  but feasible for the common shadcn/ui patterns.
- **Adapter status:** not started — prerequisite is settling on which
  v0 export format to consume.
- **Verdict:** highest mind-share among AI-builders, but the lack of a
  shared schema means the adapter is brittle. Could run the v0 output
  through a small "extract-intent" LLM step that emits AdaptiveCards
  JSON, then reuse the AdaptiveCards adapter. Score: 7/10 (but with a
  longer build).

### 4. Anthropic Artifacts (and the underlying structured-output shape)

- **Maturity:** in active rollout; the `artifact` MCP type has stable
  fields.
- **Schema:** lightweight — kind/title/content/dependencies. Not a UI
  schema per se, but Claude already uses it to package generated UIs.
- **Coverage:** typically a single `text/html` or `application/vnd.ant.react`
  artifact. We'd decode the HTML → 5 primitives.
- **Adapter status:** not started.
- **Verdict:** strongest brand alignment. If Anthropic adopts a schema
  that slots into our renderer, the partnership story writes itself.
  Score: 8/10 conditional on Anthropic publishing a stable schema.

### 5. OpenAI structured outputs / function-calling

- **Maturity:** stable, widely deployed.
- **Schema:** developer-defined per-call (any JSON Schema). Not a UI
  schema; just a typed-output channel. Useful as a *transport* for
  any of the schemas above.
- **Adapter status:** N/A — we'd ride on top of one of the others.
- **Verdict:** orthogonal — pick a UI schema, then this is just one
  way to ship it.

---

## Decision

**Adopt AdaptiveCards as the public interop format.** Reasons:

1. Most mature ecosystem.
2. We already shipped a working adapter — no migration cost.
3. Coverage of our 5 primitives is good and the heuristics for
   inferring primitive choice from card structure are clean.
4. If Anthropic / Vercel / Thesys publish a different schema we like
   better, an adapter from THEIR schema → AdaptiveCards → ATOMiK is
   one extra hop and inherits all the work.

**Internal wire format** (`delta_log.c`, opcode-based) stays as the
on-the-wire format between OS and devices. The public manifest is
JSON (AdaptiveCards-compatible); the internal stream is binary deltas.

**Strategic next:** ship a *reverse* adapter — take an `edge_app_t`
state and emit AdaptiveCards JSON. That makes ATOMiK OS a two-way
bridge: render any AdaptiveCards source on cheap edge hardware, and
also produce AdaptiveCards-compatible output that any browser /
Outlook / Teams renderer can display. Bidirectional schema interop.

---

## Action items (stand-alone)

- [x] Ship one adapter (AdaptiveCards) — done
- [ ] Reverse adapter: `edge_app_t` → AdaptiveCards JSON
- [ ] Open one design-partner conversation (Vercel v0, Thesys, or
      Anthropic) before v1.0 locks
- [ ] Document the AdaptiveCards subset we support in
      `adapters/README.md`
