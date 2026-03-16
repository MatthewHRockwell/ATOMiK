#!/usr/bin/env python3
"""Example: Distributed Cache Synchronization with ATOMiK.

Problem: Multiple cache nodes need to stay in sync. Traditional approaches
require either a consensus protocol (Raft/Paxos — complex, slow) or
full-state replication (simple but O(n) bandwidth per update).

ATOMiK solution: Each node accumulates XOR deltas. When deltas are
broadcast to peers (in any order), all nodes converge to the same state.
No leader election, no write-ahead log, no ordering constraints.

Key insight: ATOMiK is not "last writer wins" — it's "all deltas merge."
Every node's contributions are preserved through XOR accumulation.
"""

from atomik_core import DeltaMessage, DeltaStream


class CacheNode:
    """A single cache node using ATOMiK delta-state synchronization."""

    def __init__(self, node_id: str):
        self.node_id = node_id
        self.stream = DeltaStream(width=64)
        self._outbox: list[DeltaMessage] = []

    def init_key(self, key: int, value: int):
        """Initialize a key (must be called with same value on all nodes)."""
        self.stream.load(key, value)

    def update(self, key: int, delta: int) -> DeltaMessage:
        """Apply a delta to a key. Returns message to broadcast."""
        msg = self.stream.accum(key, delta)
        self._outbox.append(msg)
        return msg

    def get(self, key: int) -> int:
        """Read current value of a key."""
        return self.stream.read(key)

    def receive(self, msg: DeltaMessage):
        """Apply a delta received from a peer."""
        self.stream.apply(msg)

    def flush(self) -> list[DeltaMessage]:
        """Get and clear pending outbound messages."""
        msgs = list(self._outbox)
        self._outbox.clear()
        return msgs


def main():
    print("=== ATOMiK Distributed Cache Example ===\n")

    # Create 3 cache nodes across regions
    nodes = {
        "us-east": CacheNode("us-east"),
        "us-west": CacheNode("us-west"),
        "eu-west": CacheNode("eu-west"),
    }

    # Initialize all nodes with same reference state
    for node in nodes.values():
        node.init_key(0, 0x0000000000001000)  # key 0: counter starting at 0x1000
        node.init_key(1, 0x00000000AABB0000)  # key 1: status flags

    # --- Scenario: concurrent updates from different regions ---
    print("1. Concurrent updates (each node applies a delta):\n")

    # us-east increments counter by 0x0001
    nodes["us-east"].update(0, 0x0001)
    print("   us-east: key[0] += 0x0001 (counter increment)")

    # us-west increments counter by 0x0010
    nodes["us-west"].update(0, 0x0010)
    print("   us-west: key[0] += 0x0010 (counter increment)")

    # eu-west flips status bits on key 1
    nodes["eu-west"].update(1, 0x00000000000000FF)
    print("   eu-west: key[1] ^= 0xFF (status flags toggled)")

    # --- Broadcast deltas to all peers ---
    print("\n2. Broadcasting deltas (any order is fine):\n")

    # Collect all outbound messages
    all_msgs = {}
    for name, node in nodes.items():
        all_msgs[name] = node.flush()
        print(f"   {name} sends {len(all_msgs[name])} delta(s)")

    # Deliver to every other node (simulate network, out-of-order is OK)
    for sender_name, msgs in all_msgs.items():
        for msg in msgs:
            for recv_name, recv_node in nodes.items():
                if recv_name != sender_name:
                    recv_node.receive(msg)

    # --- Verify convergence ---
    print("\n3. Final state (all nodes converge):\n")
    for name, node in nodes.items():
        k0 = node.get(0)
        k1 = node.get(1)
        print(f"   {name}: key[0]=0x{k0:016x}  key[1]=0x{k1:016x}")

    # Verify all nodes have identical state
    ref_node = list(nodes.values())[0]
    for name, node in nodes.items():
        assert node.get(0) == ref_node.get(0), f"{name} key[0] diverged"
        assert node.get(1) == ref_node.get(1), f"{name} key[1] diverged"

    print("\n   All nodes converged to identical state.")

    # Verify the values are correct
    expected_k0 = 0x1000 ^ 0x0001 ^ 0x0010  # initial ^ all deltas
    expected_k1 = 0xAABB0000 ^ 0xFF
    assert ref_node.get(0) == expected_k0, f"Expected 0x{expected_k0:x}, got 0x{ref_node.get(0):x}"
    assert ref_node.get(1) == expected_k1
    print(f"   key[0] = 0x1000 ^ 0x0001 ^ 0x0010 = 0x{expected_k0:04x} (correct)")
    print(f"   key[1] = 0xAABB0000 ^ 0xFF = 0x{expected_k1:08x} (correct)")

    # --- Bandwidth analysis ---
    print("\n4. Bandwidth savings:\n")
    delta_bytes = 3 * 8  # 3 deltas × 8 bytes each
    print(f"   Total deltas transmitted:  {delta_bytes} bytes")
    print(f"   Equivalent full-state:     {2 * 8 * 3} bytes (2 keys × 8B × 3 nodes)")
    print("\n   At scale (10,000 keys, 100 updates/sec, 3 nodes):")
    full_bw = 100 * 10000 * 8 * 2  # updates × keys × 8B × broadcast to 2 peers
    delta_bw = 100 * 8 * 2         # updates × 8B delta × broadcast to 2 peers
    print(f"     Full-state sync: {full_bw / 1024 / 1024:.1f} MB/sec")
    print(f"     ATOMiK deltas:   {delta_bw / 1024:.1f} KB/sec")
    print(f"     Savings:         {(1 - delta_bw/full_bw)*100:.3f}%")
    print("\n   No consensus protocol. No leader election. No ordering.")


if __name__ == "__main__":
    main()
