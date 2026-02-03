# Quick-Start Prompts for Claude Code

Copy-paste any of these prompts to resume funding pipeline work.

## Resume work

Read FUNDING_CONTEXT.md and business/funding_strategy/status.json,
then show what to do next.

## Check issues

Read FUNDING_CONTEXT.md and business/funding_strategy/debug_log.json.
Show unresolved issues and suggest fixes.

## Submit next application

Read FUNDING_CONTEXT.md then run
`python -m business.funding_strategy.agents begin`
for the next ready program.

## Update data room

Read FUNDING_CONTEXT.md then run
`python business/data_room/_generate.py` and commit results.

## Full status

Run `python -m business.funding_strategy.agents plan`
and summarise what's ready, blocked, and submitted.

## Force sync

Run `python -m business.funding_strategy.agents sync`
to update all documentation from current pipeline state.

## View debug log

Run `python -m business.funding_strategy.agents debug`
to show the structured issue log with resolution status.
