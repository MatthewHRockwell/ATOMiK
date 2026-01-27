"""
ATOMiK Autonomous Pipeline System

Orchestrates schema validation, code generation, verification,
hardware-in-the-loop testing, and performance metrics collection
through a stage-based autonomous pipeline.
"""

from .controller import Pipeline, PipelineConfig, PipelineResult

__all__ = ["Pipeline", "PipelineConfig", "PipelineResult"]
