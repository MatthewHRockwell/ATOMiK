# ATOMiK Benchmark Plots

Publication-quality visualizations for **Paper 2: ATOMiK Empirical Validation**.

## Generating Plots

```bash
cd experiments/analysis
pip install matplotlib numpy seaborn  # Optional: seaborn for styling
python generate_plots.py
```

## Figure Inventory

| Figure | Filename | Purpose | Key Message |
|--------|----------|---------|-------------|
| **Fig 1** | `fig1_memory_traffic.png/pdf` | Memory traffic comparison (log scale bar chart) | ATOMiK reduces memory traffic by 95-100% (2-4 orders of magnitude) |
| **Fig 2** | `fig2_execution_time.png/pdf` | Execution time by benchmark category | ATOMiK is 22-55% faster on write-heavy workloads |
| **Fig 3** | `fig3_read_write_tradeoff.png/pdf` | Read/write ratio trade-off analysis | Crossover at ~50% reads; ATOMiK wins write-heavy, baseline wins read-heavy |
| **Fig 4** | `fig4_scalability.png/pdf` | Problem size scaling (dual panel) | Both scale O(N) for time, but ATOMiK is O(N) vs O(N²) for memory |
| **Fig 5** | `fig5_parallel_efficiency.png/pdf` | Parallel composition efficiency | ATOMiK achieves 85% efficiency; baseline cannot parallelize (0%) |
| **Fig 6** | `fig6_summary_dashboard.png/pdf` | Multi-panel summary dashboard | Comprehensive overview of all key findings |
| **Fig 7** | `fig7_cache_performance.png/pdf` | Cache performance across working sets | Smaller delta footprint improves cache utilization 17-40% |

## Recommended Usage in Paper 2

### Section Mapping

| Paper Section | Recommended Figure(s) |
|---------------|----------------------|
| Abstract/Introduction | Fig 6 (summary dashboard) |
| Memory Efficiency Results | Fig 1, Fig 4b |
| Computational Overhead Results | Fig 2, Fig 3 |
| Scalability Results | Fig 4, Fig 5, Fig 7 |
| Discussion/Trade-offs | Fig 3 |
| Conclusion | Fig 6 |

### Figure Captions (Draft)

**Figure 1**: *Memory traffic comparison between baseline SCORE architecture and ATOMiK across three workload categories. ATOMiK achieves 95-100% reduction in memory traffic by storing deltas (8 bytes) instead of full state. Note logarithmic scale.*

**Figure 2**: *Execution time comparison across benchmark categories. Error bars indicate 95% confidence intervals. ATOMiK shows consistent improvement on memory and scalability benchmarks, with workload-dependent results for computational overhead.*

**Figure 3**: *Performance trade-off as a function of read/write ratio. ATOMiK outperforms baseline for write-heavy workloads (<50% reads) due to O(1) delta accumulation. Baseline outperforms for read-heavy workloads (>50% reads) due to O(N) reconstruction cost in ATOMiK. Crossover occurs at approximately 50% read ratio.*

**Figure 4**: *Scalability analysis. (a) Execution time scales linearly with problem size for both architectures, with ATOMiK maintaining 15-20% advantage. (b) Memory traffic shows divergent scaling: baseline exhibits O(N²) growth while ATOMiK maintains O(N) due to delta-based storage.*

**Figure 5**: *Parallel composition efficiency. Baseline SCORE architecture cannot parallelize due to serial data dependencies (0% efficiency). ATOMiK achieves 85% parallel efficiency by leveraging the commutativity property (δ₁⊕δ₂ = δ₂⊕δ₁) proven in Lean4, enabling tree-based parallel reduction.*

**Figure 6**: *Summary dashboard of ATOMiK vs SCORE benchmark comparison. Key findings: (a) 99%+ memory traffic reduction, (b) 22-55% speed improvement on write-heavy workloads, (c) predictable trade-offs based on read/write ratio, (d) 85% parallel efficiency vs 0% for baseline, (e) 75% of comparisons statistically significant (p<0.05).*

**Figure 7**: *Cache performance across working set sizes (L1: 1KB, L2: 64KB, L3+: 1MB). ATOMiK's smaller delta footprint improves cache utilization by 17-40%, with larger gains at bigger working sets where cache pressure is highest.*

## LaTeX Integration

```latex
\begin{figure}[t]
    \centering
    \includegraphics[width=\columnwidth]{plots/fig1_memory_traffic.pdf}
    \caption{Memory traffic comparison...}
    \label{fig:memory-traffic}
\end{figure}
```

## Color Scheme

| Element | Hex | Usage |
|---------|-----|-------|
| Baseline (SCORE) | `#E74C3C` | Red |
| ATOMiK | `#3498DB` | Blue |
| Improvement | `#27AE60` | Green (annotations) |
| Neutral | `#95A5A6` | Gray (reference lines) |

## Data Sources

All plots are generated from:
- `experiments/data/memory/memory_benchmarks.csv` (120 measurements)
- `experiments/data/overhead/overhead_benchmarks.csv` (80 measurements)
- `experiments/data/scalability/scalability_benchmarks.csv` (160 measurements)

Total: 360 measurements across 9 workloads.

---

*Generated: January 2026*
*For: Paper 2 - ATOMiK Empirical Validation*
