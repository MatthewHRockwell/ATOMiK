"""
Configuration Auto-Tuner

Adjusts pipeline configuration parameters based on historical
performance data: worker pool size, retry depth, model routing
weights, and budget allocation.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass
class TuningResult:
    """Result of auto-tuning a configuration parameter."""
    parameter: str
    old_value: Any
    new_value: Any
    reason: str

    def to_dict(self) -> dict[str, Any]:
        return {
            "parameter": self.parameter,
            "old_value": self.old_value,
            "new_value": self.new_value,
            "reason": self.reason,
        }


class ConfigTuner:
    """
    Auto-tunes pipeline configuration based on historical data.

    Analyzes past run metrics to recommend optimal values for:
    - Worker pool size (based on parallelism benefit)
    - Retry depth (based on success rate at each depth)
    - Model routing weights (based on per-tier success and cost)
    - Budget allocation (based on per-stage token consumption)

    Example:
        >>> tuner = ConfigTuner()
        >>> results = tuner.tune_all(run_history)
        >>> for r in results:
        ...     print(f"{r.parameter}: {r.old_value} -> {r.new_value}")
    """

    def __init__(
        self,
        min_workers: int = 1,
        max_workers: int = 8,
        min_retry_depth: int = 1,
        max_retry_depth: int = 5,
    ) -> None:
        self.min_workers = min_workers
        self.max_workers = max_workers
        self.min_retry_depth = min_retry_depth
        self.max_retry_depth = max_retry_depth

    def tune_all(
        self,
        run_history: list[dict[str, Any]],
        current_config: dict[str, Any] | None = None,
    ) -> list[TuningResult]:
        """
        Run all tuning analyses and return recommendations.

        Args:
            run_history: List of past run metrics dicts.
            current_config: Current configuration values.

        Returns:
            List of TuningResult recommendations.
        """
        config = current_config or {}
        results: list[TuningResult] = []

        if len(run_history) < 2:
            return results

        workers_result = self.tune_workers(run_history, config.get("max_workers", 4))
        if workers_result:
            results.append(workers_result)

        retry_result = self.tune_retry_depth(run_history, config.get("retry_depth", 3))
        if retry_result:
            results.append(retry_result)

        routing_results = self.tune_model_routing(run_history, config.get("routing_weights", {}))
        results.extend(routing_results)

        return results

    def tune_workers(
        self,
        run_history: list[dict[str, Any]],
        current_workers: int = 4,
    ) -> TuningResult | None:
        """
        Tune worker pool size based on observed parallelism benefit.

        If parallel speedup is plateauing, reduce workers.
        If speedup is near-linear, suggest increasing.
        """
        speedups = [
            r.get("parallel_speedup", 1.0)
            for r in run_history
            if "parallel_speedup" in r
        ]

        if len(speedups) < 2:
            return None

        avg_speedup = sum(speedups) / len(speedups)

        # Diminishing returns detection: speedup < 0.7 * workers
        efficiency = avg_speedup / current_workers if current_workers > 0 else 0

        if efficiency < 0.4 and current_workers > self.min_workers:
            new_workers = max(self.min_workers, current_workers - 1)
            return TuningResult(
                parameter="max_workers",
                old_value=current_workers,
                new_value=new_workers,
                reason=f"Diminishing returns: avg speedup {avg_speedup:.1f}x with {current_workers} workers (efficiency {efficiency:.0%})",
            )
        elif efficiency > 0.8 and current_workers < self.max_workers:
            new_workers = min(self.max_workers, current_workers + 1)
            return TuningResult(
                parameter="max_workers",
                old_value=current_workers,
                new_value=new_workers,
                reason=f"Near-linear speedup: {avg_speedup:.1f}x with {current_workers} workers (efficiency {efficiency:.0%})",
            )

        return None

    def tune_retry_depth(
        self,
        run_history: list[dict[str, Any]],
        current_depth: int = 3,
    ) -> TuningResult | None:
        """
        Tune feedback loop retry depth based on success at each level.

        If most fixes happen at depth 1, reduce depth.
        If depth 3 frequently succeeds, consider increasing.
        """
        depth_successes: dict[int, int] = {}
        depth_attempts: dict[int, int] = {}

        for run in run_history:
            feedback = run.get("feedback_iterations", [])
            for iteration in feedback:
                depth = iteration.get("iteration", 0)
                depth_attempts[depth] = depth_attempts.get(depth, 0) + 1
                if iteration.get("re_verify_passed", False):
                    depth_successes[depth] = depth_successes.get(depth, 0) + 1

        if not depth_attempts:
            return None

        # Find the deepest level with > 20% success rate
        max_useful_depth = 1
        for depth in sorted(depth_attempts.keys()):
            attempts = depth_attempts[depth]
            successes = depth_successes.get(depth, 0)
            if attempts > 0 and successes / attempts > 0.2:
                max_useful_depth = depth

        new_depth = max(self.min_retry_depth, min(self.max_retry_depth, max_useful_depth + 1))

        if new_depth != current_depth:
            return TuningResult(
                parameter="retry_depth",
                old_value=current_depth,
                new_value=new_depth,
                reason=f"Max useful retry depth is {max_useful_depth} based on {sum(depth_attempts.values())} attempts",
            )

        return None

    def tune_model_routing(
        self,
        run_history: list[dict[str, Any]],
        current_weights: dict[str, float] | None = None,
    ) -> list[TuningResult]:
        """
        Tune model routing weights based on per-tier performance.

        Adjusts the preference for each model tier based on
        success rate and cost efficiency.
        """
        results: list[TuningResult] = []
        tier_stats: dict[str, dict[str, float]] = {}

        for run in run_history:
            tier_usage = run.get("tier_usage", {})
            for tier, stats in tier_usage.items():
                if tier not in tier_stats:
                    tier_stats[tier] = {"successes": 0, "attempts": 0, "tokens": 0}
                tier_stats[tier]["attempts"] += stats.get("attempts", 0)
                tier_stats[tier]["successes"] += stats.get("successes", 0)
                tier_stats[tier]["tokens"] += stats.get("tokens", 0)

        if not tier_stats:
            return results

        weights = current_weights or {}
        for tier, stats in tier_stats.items():
            attempts = stats["attempts"]
            if attempts < 3:
                continue
            success_rate = stats["successes"] / attempts
            cost_per_success = stats["tokens"] / max(stats["successes"], 1)

            old_weight = weights.get(tier, 1.0)
            # Boost high-success, low-cost tiers
            new_weight = success_rate / max(cost_per_success / 1000, 0.1)
            new_weight = round(min(2.0, max(0.5, new_weight)), 2)

            if abs(new_weight - old_weight) > 0.1:
                results.append(TuningResult(
                    parameter=f"routing_weight_{tier}",
                    old_value=old_weight,
                    new_value=new_weight,
                    reason=f"{tier}: {success_rate:.0%} success, {cost_per_success:.0f} tokens/success",
                ))

        return results
