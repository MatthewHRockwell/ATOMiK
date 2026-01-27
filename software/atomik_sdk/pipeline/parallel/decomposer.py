"""
Task Decomposer

Decomposes pipeline work into fine-grained tasks that can execute
in parallel. Each language becomes an independent generation task;
each language's test suite is an independent verification task.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


ALL_LANGUAGES = ["python", "rust", "c", "javascript", "verilog"]


@dataclass
class ParallelTask:
    """A decomposed task ready for parallel execution."""
    task_id: str
    task_type: str        # "generate", "verify", "synthesize"
    language: str = ""     # Language this task targets (empty for language-agnostic)
    dependencies: list[str] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return {
            "task_id": self.task_id,
            "task_type": self.task_type,
            "language": self.language,
            "dependencies": self.dependencies,
            "metadata": self.metadata,
        }


@dataclass
class DecompositionPlan:
    """A plan of parallel and sequential tasks."""
    tasks: list[ParallelTask] = field(default_factory=list)
    parallel_groups: list[list[str]] = field(default_factory=list)

    @property
    def task_count(self) -> int:
        return len(self.tasks)

    @property
    def max_parallelism(self) -> int:
        if not self.parallel_groups:
            return 1
        return max(len(g) for g in self.parallel_groups)

    def to_dict(self) -> dict[str, Any]:
        return {
            "task_count": self.task_count,
            "max_parallelism": self.max_parallelism,
            "tasks": [t.to_dict() for t in self.tasks],
            "parallel_groups": self.parallel_groups,
        }


class TaskDecomposer:
    """
    Decomposes pipeline stages into parallel tasks.

    For code generation, each language becomes an independent task.
    For verification, each language's checks run independently.
    Hardware simulation and synthesis can overlap with software
    verification.

    Example:
        >>> decomposer = TaskDecomposer()
        >>> plan = decomposer.decompose_generation(["python", "rust", "verilog"])
        >>> assert plan.max_parallelism == 3
    """

    def decompose_generation(
        self,
        languages: list[str] | None = None,
    ) -> DecompositionPlan:
        """
        Decompose code generation into per-language parallel tasks.

        Args:
            languages: Languages to generate (None = all 5).

        Returns:
            DecompositionPlan with one task per language.
        """
        langs = languages or list(ALL_LANGUAGES)
        plan = DecompositionPlan()

        task_ids = []
        for lang in langs:
            task = ParallelTask(
                task_id=f"gen_{lang}",
                task_type="generate",
                language=lang,
            )
            plan.tasks.append(task)
            task_ids.append(task.task_id)

        plan.parallel_groups.append(task_ids)
        return plan

    def decompose_verification(
        self,
        languages: list[str] | None = None,
        gen_task_ids: list[str] | None = None,
    ) -> DecompositionPlan:
        """
        Decompose verification into per-language parallel tasks.

        Each verification task depends on its language's generation task.
        """
        langs = languages or list(ALL_LANGUAGES)
        plan = DecompositionPlan()

        task_ids = []
        for lang in langs:
            deps = [f"gen_{lang}"] if gen_task_ids is None else [
                tid for tid in gen_task_ids if lang in tid
            ]
            task = ParallelTask(
                task_id=f"verify_{lang}",
                task_type="verify",
                language=lang,
                dependencies=deps,
            )
            plan.tasks.append(task)
            task_ids.append(task.task_id)

        plan.parallel_groups.append(task_ids)
        return plan

    def decompose_full_pipeline(
        self,
        languages: list[str] | None = None,
        include_hardware: bool = True,
    ) -> DecompositionPlan:
        """
        Decompose a full pipeline run into parallel task groups.

        Groups:
        1. Validate (sequential)
        2. Diff (sequential, depends on validate)
        3. Generate per-language (parallel)
        4. Verify per-language (parallel) + Hardware sim (parallel with SW verify)
        5. Metrics (sequential, after all verification)

        Returns:
            DecompositionPlan with all task groups.
        """
        langs = languages or list(ALL_LANGUAGES)
        plan = DecompositionPlan()

        # Group 1: validate
        plan.tasks.append(ParallelTask(
            task_id="validate",
            task_type="stage",
        ))
        plan.parallel_groups.append(["validate"])

        # Group 2: diff
        plan.tasks.append(ParallelTask(
            task_id="diff",
            task_type="stage",
            dependencies=["validate"],
        ))
        plan.parallel_groups.append(["diff"])

        # Group 3: generate per language
        gen_ids = []
        for lang in langs:
            tid = f"gen_{lang}"
            plan.tasks.append(ParallelTask(
                task_id=tid,
                task_type="generate",
                language=lang,
                dependencies=["diff"],
            ))
            gen_ids.append(tid)
        plan.parallel_groups.append(gen_ids)

        # Group 4: verify per language + hardware
        verify_ids = []
        for lang in langs:
            tid = f"verify_{lang}"
            plan.tasks.append(ParallelTask(
                task_id=tid,
                task_type="verify",
                language=lang,
                dependencies=[f"gen_{lang}"],
            ))
            verify_ids.append(tid)

        if include_hardware:
            hw_tid = "hardware_sim"
            plan.tasks.append(ParallelTask(
                task_id=hw_tid,
                task_type="synthesize",
                language="verilog",
                dependencies=["gen_verilog"],
            ))
            verify_ids.append(hw_tid)

        plan.parallel_groups.append(verify_ids)

        # Group 5: metrics
        plan.tasks.append(ParallelTask(
            task_id="metrics",
            task_type="stage",
            dependencies=verify_ids,
        ))
        plan.parallel_groups.append(["metrics"])

        return plan
