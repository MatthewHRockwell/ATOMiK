# ATOMiK Benchmark Statistical Analysis

**Date**: January 24, 2026
**Phase**: 2 - SCORE Comparison
**Task**: T2.8

---

# Memory Efficiency Benchmarks Analysis

**Total Measurements**: 120
**Outliers Removed**: 20
**Workloads**: 3

## Matrix

**Sample Size**: Baseline=10, ATOMiK=10

### Execution Time Ms

- **Baseline**: 27.00 ± 1.43
- **ATOMiK**: 21.06 ± 0.55
- **Improvement**: +22.0%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Peak Memory Bytes

- **Baseline**: 37163.60 ± 24.05
- **ATOMiK**: 37400.80 ± 3.47
- **Improvement**: -0.6%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Memory Traffic Bytes

- **Baseline**: 251658240.00 ± 0.00
- **ATOMiK**: 32768.00 ± 0.00
- **Improvement**: +100.0%
- **p-value**: 1.0000
- **Statistically Significant**: ❌ No

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 27.00 ± 1.43 | 21.06 ± 0.55 | +22.0% | 0.0000 | ✅ |
| peak_memory_bytes | 37163.60 ± 24.05 | 37400.80 ± 3.47 | -0.6% | 0.0000 | ✅ |
| memory_traffic_bytes | 251658240.00 ± 0.00 | 32768.00 ± 0.00 | +100.0% | 1.0000 | ❌ |

## State Machine

**Sample Size**: Baseline=20, ATOMiK=20

### Execution Time Ms

- **Baseline**: 0.19 ± 0.01
- **ATOMiK**: 0.21 ± 0.01
- **Improvement**: -14.1%
- **p-value**: 0.0035
- **Statistically Significant**: ✅ Yes

### Peak Memory Bytes

- **Baseline**: 4688.80 ± 23.89
- **ATOMiK**: 4687.20 ± 28.25
- **Improvement**: +0.0%
- **p-value**: 0.9325
- **Statistically Significant**: ❌ No

### Memory Traffic Bytes

- **Baseline**: 4024000.00 ± 0.00
- **ATOMiK**: 4032.00 ± 0.00
- **Improvement**: +99.9%
- **p-value**: 1.0000
- **Statistically Significant**: ❌ No

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 0.19 ± 0.01 | 0.21 ± 0.01 | -14.1% | 0.0035 | ✅ |
| peak_memory_bytes | 4688.80 ± 23.89 | 4687.20 ± 28.25 | +0.0% | 0.9325 | ❌ |
| memory_traffic_bytes | 4024000.00 ± 0.00 | 4032.00 ± 0.00 | +99.9% | 1.0000 | ❌ |

## Streaming

**Sample Size**: Baseline=20, ATOMiK=20

### Execution Time Ms

- **Baseline**: 11.58 ± 2.59
- **ATOMiK**: 5.17 ± 0.91
- **Improvement**: +55.4%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Peak Memory Bytes

- **Baseline**: 1083.80 ± 132.63
- **ATOMiK**: 1030.00 ± 124.62
- **Improvement**: +5.0%
- **p-value**: 0.5623
- **Statistically Significant**: ❌ No

### Memory Traffic Bytes

- **Baseline**: 5100000.00 ± 2023446.77
- **ATOMiK**: 400.00 ± 107.92
- **Improvement**: +100.0%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 11.58 ± 2.59 | 5.17 ± 0.91 | +55.4% | 0.0000 | ✅ |
| peak_memory_bytes | 1083.80 ± 132.63 | 1030.00 ± 124.62 | +5.0% | 0.5623 | ❌ |
| memory_traffic_bytes | 5100000.00 ± 2023446.77 | 400.00 ± 107.92 | +100.0% | 0.0000 | ✅ |

---

# Computational Overhead Benchmarks Analysis

**Total Measurements**: 80
**Outliers Removed**: 20
**Workloads**: 2

## Composition

**Sample Size**: Baseline=10, ATOMiK=10

### Execution Time Ms

- **Baseline**: 5.03 ± 0.03
- **ATOMiK**: 3.99 ± 0.02
- **Improvement**: +20.8%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Peak Memory Bytes

- **Baseline**: 455.20 ± 6.27
- **ATOMiK**: 9949.60 ± 3.91
- **Improvement**: -2085.8%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Memory Traffic Bytes

- **Baseline**: 48000.00 ± 0.00
- **ATOMiK**: 32.00 ± 0.00
- **Improvement**: +99.9%
- **p-value**: 1.0000
- **Statistically Significant**: ❌ No

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 5.03 ± 0.03 | 3.99 ± 0.02 | +20.8% | 0.0000 | ✅ |
| peak_memory_bytes | 455.20 ± 6.27 | 9949.60 ± 3.91 | -2085.8% | 0.0000 | ✅ |
| memory_traffic_bytes | 48000.00 ± 0.00 | 32.00 ± 0.00 | +99.9% | 1.0000 | ❌ |

## Mixed

**Sample Size**: Baseline=20, ATOMiK=20

### Execution Time Ms

- **Baseline**: 2.79 ± 0.46
- **ATOMiK**: 2.41 ± 0.30
- **Improvement**: +13.6%
- **p-value**: 0.1767
- **Statistically Significant**: ❌ No

### Peak Memory Bytes

- **Baseline**: 544.80 ± 11.05
- **ATOMiK**: 403.20 ± 10.07
- **Improvement**: +26.0%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Memory Traffic Bytes

- **Baseline**: 35949.60 ± 2181.04
- **ATOMiK**: 32.00 ± 0.00
- **Improvement**: +99.9%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 2.79 ± 0.46 | 2.41 ± 0.30 | +13.6% | 0.1767 | ❌ |
| peak_memory_bytes | 544.80 ± 11.05 | 403.20 ± 10.07 | +26.0% | 0.0000 | ✅ |
| memory_traffic_bytes | 35949.60 ± 2181.04 | 32.00 ± 0.00 | +99.9% | 0.0000 | ✅ |

---

# Scalability Benchmarks Analysis

**Total Measurements**: 160
**Outliers Removed**: 60
**Workloads**: 3

## Scaling

**Sample Size**: Baseline=30, ATOMiK=30

### Execution Time Ms

- **Baseline**: 2.82 ± 0.94
- **ATOMiK**: 2.36 ± 0.81
- **Improvement**: +16.5%
- **p-value**: 0.4617
- **Statistically Significant**: ❌ No

### Peak Memory Bytes

- **Baseline**: 4578.40 ± 1359.97
- **ATOMiK**: 4591.47 ± 1362.87
- **Improvement**: -0.3%
- **p-value**: 0.9894
- **Statistically Significant**: ❌ No

### Memory Traffic Bytes

- **Baseline**: 5591040.00 ± 2612610.12
- **ATOMiK**: 3584.00 ± 1207.68
- **Improvement**: +99.9%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 2.82 ± 0.94 | 2.36 ± 0.81 | +16.5% | 0.4617 | ❌ |
| peak_memory_bytes | 4578.40 ± 1359.97 | 4591.47 ± 1362.87 | -0.3% | 0.9894 | ❌ |
| memory_traffic_bytes | 5591040.00 ± 2612610.12 | 3584.00 ± 1207.68 | +99.9% | 0.0000 | ✅ |

## Parallel

**Sample Size**: Baseline=10, ATOMiK=10

### Execution Time Ms

- **Baseline**: 5.10 ± 0.03
- **ATOMiK**: 3.81 ± 0.02
- **Improvement**: +25.2%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Peak Memory Bytes

- **Baseline**: 616.40 ± 6.23
- **ATOMiK**: 9983.20 ± 7.19
- **Improvement**: -1519.6%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Memory Traffic Bytes

- **Baseline**: 48000.00 ± 0.00
- **ATOMiK**: 32.00 ± 0.00
- **Improvement**: +99.9%
- **p-value**: 1.0000
- **Statistically Significant**: ❌ No

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 5.10 ± 0.03 | 3.81 ± 0.02 | +25.2% | 0.0000 | ✅ |
| peak_memory_bytes | 616.40 ± 6.23 | 9983.20 ± 7.19 | -1519.6% | 0.0000 | ✅ |
| memory_traffic_bytes | 48000.00 ± 0.00 | 32.00 ± 0.00 | +99.9% | 1.0000 | ❌ |

## Cache

**Sample Size**: Baseline=10, ATOMiK=10

### Execution Time Ms

- **Baseline**: 2.22 ± 0.02
- **ATOMiK**: 1.34 ± 0.02
- **Improvement**: +39.6%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Peak Memory Bytes

- **Baseline**: 2867.60 ± 142.30
- **ATOMiK**: 2047.60 ± 17.55
- **Improvement**: +28.6%
- **p-value**: 0.0000
- **Statistically Significant**: ✅ Yes

### Memory Traffic Bytes

- **Baseline**: 846720.00 ± 0.00
- **ATOMiK**: 1024.00 ± 0.00
- **Improvement**: +99.9%
- **p-value**: 1.0000
- **Statistically Significant**: ❌ No

### Comparison Table

| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |
|--------|---------------------|-------------------|-------------|---------|-------------|
| execution_time_ms | 2.22 ± 0.02 | 1.34 ± 0.02 | +39.6% | 0.0000 | ✅ |
| peak_memory_bytes | 2867.60 ± 142.30 | 2047.60 ± 17.55 | +28.6% | 0.0000 | ✅ |
| memory_traffic_bytes | 846720.00 ± 0.00 | 1024.00 ± 0.00 | +99.9% | 1.0000 | ❌ |

---

