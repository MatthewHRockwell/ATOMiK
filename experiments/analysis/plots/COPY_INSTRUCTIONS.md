# Plot Files to Copy

The following plot files have been generated and are available for download from Claude's outputs.

## PNG Files (for preview/web)
- fig1_memory_traffic.png
- fig2_execution_time.png
- fig3_read_write_tradeoff.png
- fig4_scalability.png
- fig5_parallel_efficiency.png
- fig6_summary_dashboard.png
- fig7_cache_performance.png

## PDF Files (for LaTeX/publication)
- fig1_memory_traffic.pdf
- fig2_execution_time.pdf
- fig3_read_write_tradeoff.pdf
- fig4_scalability.pdf
- fig5_parallel_efficiency.pdf
- fig6_summary_dashboard.pdf
- fig7_cache_performance.pdf

## To Generate Locally

Run the generation script:
```bash
cd experiments/analysis
pip install matplotlib numpy
python generate_plots.py
```

This will regenerate all plots from the benchmark CSV data.
