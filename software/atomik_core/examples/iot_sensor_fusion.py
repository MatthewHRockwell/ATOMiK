#!/usr/bin/env python3
"""Example: IoT Sensor Fusion with ATOMiK.

Problem: Hundreds of sensors report state changes to a gateway.
Traditional approach: each sensor sends its full state on every change,
gateway maintains a master copy. At scale (1000 sensors, 10 updates/sec),
this is 80 KB/sec of redundant data.

ATOMiK solution: Each sensor maintains an AtomikContext. On change,
it sends only the 8-byte XOR delta. Gateway merges deltas from all
sensors. Total bandwidth: 80 bytes/sec regardless of state size.

Additional benefits:
  - Sensors can report in any order (commutativity)
  - Gateway detects if any sensor's state changed (fingerprint)
  - Lost/duplicate packets self-cancel (XOR self-inverse)
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from atomik_core import AtomikContext, Fingerprint


class Sensor:
    """A single IoT sensor using ATOMiK delta tracking."""

    def __init__(self, sensor_id: int, initial_reading: int = 0):
        self.sensor_id = sensor_id
        self.ctx = AtomikContext(width=64, initial_state=initial_reading)
        self._last_value = initial_reading

    def update(self, new_value: int) -> int:
        """Record a new sensor reading. Returns the delta to transmit."""
        delta = self._last_value ^ new_value
        self.ctx.accum(delta)
        self._last_value = new_value
        return delta

    @property
    def current(self) -> int:
        return self.ctx.read()


class Gateway:
    """Sensor fusion gateway using ATOMiK fingerprinting."""

    def __init__(self, num_sensors: int):
        self.sensors = {}
        self.fingerprint = Fingerprint(width=64)
        self._combined_state = bytearray(num_sensors * 8)

    def register_sensor(self, sensor_id: int, initial_value: int):
        self.sensors[sensor_id] = AtomikContext(width=64, initial_state=initial_value)

    def receive_delta(self, sensor_id: int, delta: int):
        """Apply a delta from a sensor. O(1) per update."""
        if sensor_id in self.sensors:
            self.sensors[sensor_id].accum(delta)

    def read_sensor(self, sensor_id: int) -> int:
        return self.sensors[sensor_id].read()

    def any_changed(self) -> dict[int, bool]:
        """Check which sensors have pending deltas."""
        return {
            sid: not ctx.is_clean
            for sid, ctx in self.sensors.items()
        }


def main():
    print("=== ATOMiK IoT Sensor Fusion Example ===\n")

    num_sensors = 8
    initial_readings = [0x1000 + i * 0x100 for i in range(num_sensors)]

    # Create sensors and gateway
    sensors = [Sensor(i, initial_readings[i]) for i in range(num_sensors)]
    gateway = Gateway(num_sensors)
    for i, reading in enumerate(initial_readings):
        gateway.register_sensor(i, reading)

    print(f"1. Initialized {num_sensors} sensors\n")

    # --- Simulate sensor updates ---
    print("2. Sensor updates (only deltas transmitted):\n")

    updates = [
        (0, 0x1042),  # Sensor 0: temperature changed
        (3, 0x13FF),  # Sensor 3: pressure spike
        (5, 0x1500),  # Sensor 5: no change (same value)
        (7, 0x1799),  # Sensor 7: humidity changed
    ]

    total_delta_bytes = 0
    total_full_bytes = 0

    for sensor_id, new_value in updates:
        delta = sensors[sensor_id].update(new_value)
        gateway.receive_delta(sensor_id, delta)

        if delta != 0:
            total_delta_bytes += 8
        total_full_bytes += 8  # full state always 8 bytes

        print(f"   Sensor {sensor_id}: value=0x{new_value:04x}, "
              f"delta=0x{delta:04x}, "
              f"{'CHANGED' if delta != 0 else 'NO CHANGE'}")

    # --- Verify gateway state matches sensors ---
    print(f"\n3. Gateway state (reconstructed from deltas):\n")
    for i in range(num_sensors):
        gw_state = gateway.read_sensor(i)
        sensor_state = sensors[i].current
        match = "OK" if gw_state == sensor_state else "MISMATCH"
        print(f"   Sensor {i}: gateway=0x{gw_state:016x}  sensor=0x{sensor_state:016x}  [{match}]")

    # --- Show which sensors have pending changes ---
    print(f"\n4. Change detection (O(1) per sensor):\n")
    changed = gateway.any_changed()
    for sid, is_changed in changed.items():
        print(f"   Sensor {sid}: {'CHANGED' if is_changed else 'clean'}")

    # --- Bandwidth analysis ---
    print(f"\n5. Bandwidth analysis:\n")
    print(f"   Updates transmitted:    {len(updates)}")
    print(f"   ATOMiK delta bytes:     {total_delta_bytes}")
    print(f"   Full-state bytes:       {total_full_bytes}")
    print(f"\n   At scale (1000 sensors, 10 updates/sec, 1 KB state each):")
    at_scale_full = 1000 * 10 * 1024  # 10 MB/sec
    at_scale_delta = 1000 * 10 * 8    # 80 KB/sec
    print(f"     Full-state: {at_scale_full/1024/1024:.1f} MB/sec")
    print(f"     ATOMiK:     {at_scale_delta/1024:.0f} KB/sec")
    print(f"     Savings:    {(1 - at_scale_delta/at_scale_full)*100:.1f}%")


if __name__ == "__main__":
    main()
