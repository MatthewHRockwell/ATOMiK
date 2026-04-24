# ATOMiK Agent Memory State Tracking

## Problem

Agents maintain multiple state buffers: observations, actions, rewards, model weights, hidden states. Checkpointing the full agent state every tick wastes bandwidth on buffers that didn't change. Knowing *which* buffers changed enables selective backup and logging.

## Solution

`atomik-agent-mem` uses ATOMiK fingerprint comparison to identify which agent buffers changed each tick, enabling selective checkpoint instead of full state dump.

## Demo Result (Mock Mode)

5 agent buffers (observation 8KB, action 1KB, reward 256B, model_weights 16KB, hidden_state 4KB = 29 KB total):

| Pattern | Buffers Changed | Checkpoint |
|---------|:--------------:|:----------:|
| Every tick | observation, action, hidden_state | 13 KB |
| Every 2 ticks | + reward | 13.5 KB |
| Every 4 ticks | + model_weights | 29 KB (all) |

Over 8 ticks: 234 KB full checkpoint vs 137 KB selective = **41.5% bandwidth saved**.

Model weights (16KB, 55% of state) only change on learning steps. ATOMiK correctly skips them on non-learning ticks, cutting checkpoint cost nearly in half.

## How It Works

Each tick:
1. Agent acts (mutates some buffers)
2. For each buffer: recompute fingerprint, compare via ATOMiK
3. Report which buffers changed
4. Checkpoint only changed buffers

## Use Cases

- **RL agent checkpointing**: skip model weights between learning steps
- **Multi-agent systems**: track which agents' states diverged
- **Stateful service monitoring**: detect which components updated
