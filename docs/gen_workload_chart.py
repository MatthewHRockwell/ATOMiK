#!/usr/bin/env python3
"""Generate workload comparison chart from captured hardware data."""
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# Captured data from workload_csr_20260408.txt (live Zynq hardware)
workloads = [
    ("8x256B\n25%", 18435, 1221),
    ("8x4KB\n25%", 6955438, 1223),
    ("32x1KB\n10%", 696709, 3225),
    ("64x1KB\n5%", 1392901, 5881),
    ("64x4KB\n5%", 55125636, 11837),
]

labels = [w[0] for w in workloads]
sw = [w[1] for w in workloads]
hw = [w[2] for w in workloads]

x = np.arange(len(labels))
width = 0.35

fig, ax = plt.subplots(figsize=(10, 5.5))
bars_sw = ax.bar(x - width/2, sw, width, label='Software (memcmp)', color='#e74c3c', alpha=0.85)
bars_hw = ax.bar(x + width/2, hw, width, label='ATOMiK (detect)', color='#2ecc71', alpha=0.85)

ax.set_yscale('log')
ax.set_ylabel('Cycles (log scale)', fontsize=12)
ax.set_title('Multi-Buffer Change Detection: Software vs ATOMiK\n'
             'VexRiscv SMP @ 100 MHz · Linux 6.9 · Live Zynq Hardware',
             fontsize=13, fontweight='bold')
ax.set_xticks(x)
ax.set_xticklabels(labels, fontsize=10)
ax.legend(fontsize=11, loc='upper left')
ax.set_ylim(500, 2e8)

# Add speedup labels
for i, (s, h) in enumerate(zip(sw, hw)):
    speedup = s / h
    ax.annotate(f'{speedup:,.0f}x', xy=(i + width/2, h),
                xytext=(0, 8), textcoords='offset points',
                ha='center', fontsize=9, fontweight='bold', color='#27ae60')

# Add the key message
ax.text(0.98, 0.02,
        'Software scales with region count × size (O(N·S))\n'
        'ATOMiK scales with region count only (O(N))',
        transform=ax.transAxes, fontsize=9, ha='right', va='bottom',
        bbox=dict(boxstyle='round,pad=0.4', facecolor='#f0f0f0', alpha=0.8))

plt.tight_layout()
plt.savefig('docs/workload_chart.png', dpi=150)
plt.savefig('docs/workload_chart.svg')
print('Saved: docs/workload_chart.png, docs/workload_chart.svg')
