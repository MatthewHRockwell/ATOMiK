# Design memory

Mirror of the canonical ATOMiK OS design memos that live in
`~/.claude/projects/-home-mattrock-Projects-ATOMiK/memory/` on the
primary developer laptop. Committed here as belt-and-braces backup so
**no design pillar is ever stranded on a single machine**.

These are not API docs. They're the strategic + architectural
decisions that future work must respect:

| File | Subject |
|------|---------|
| `project_atomik_os_design.md`         | High-level OS design pillars + roadmap |
| `project_atomik_os_invariant_frame.md`| THE pitch — apps are field-delta streams over a shared compiled UI frame |
| `project_atomik_os_document.md`       | Universal Document app — chat-driven UI morphing |
| `project_atomik_os_edge_apps.md`      | App = manifest + agent dispatcher + adaptive UI |
| `project_atomik_os_token_economics.md`| Token-pay AI-app economics |
| `project_atomik_os_partnerships.md`   | Render someone else's gen-UI output (Vercel v0 / Thesys / AdaptiveCards / Anthropic Artifacts) |
| `feedback_commit_obsessively.md`      | The discipline rule that exists because we lost a desktop UI to a chat-disconnect once |

## How these stay in sync

1. The canonical copies are in `~/.claude/.../memory/`. Edit those when
   refining the design.
2. Periodically (and after any major design pivot) run:
   ```
   cp ~/.claude/projects/-home-mattrock-Projects-ATOMiK/memory/project_atomik_os_*.md \
      ~/.claude/projects/-home-mattrock-Projects-ATOMiK/memory/feedback_commit_obsessively.md \
      atomik_os/docs/design_memory/
   git add atomik_os/docs/design_memory/ && git commit -m "Mirror design memory"
   ```
3. If you're picking up the project on a fresh machine, the laptop-side
   memory directory may be empty — read these files first to understand
   the architecture and the strategic decisions.
