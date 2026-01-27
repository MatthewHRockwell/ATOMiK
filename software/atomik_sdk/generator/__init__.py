"""
ATOMiK SDK Generator

Multi-language code generator for ATOMiK delta-state computing modules.

Example usage:
    from atomik_sdk.generator import GeneratorEngine, GeneratorConfig

    # Create engine
    engine = GeneratorEngine(GeneratorConfig(
        output_dir="generated",
        verbose=True
    ))

    # Load schema
    engine.load_schema("sdk/schemas/examples/terminal-io.json")

    # Register language generators (to be implemented in Phase 4A.3+)
    # engine.register_generator('python', PythonGenerator())

    # Generate code
    results = engine.generate(target_languages=['python'])

    # Write files
    files = engine.write_output(results)
"""

from .code_emitter import CodeEmitter, GeneratedFile, GenerationResult, MultiLanguageEmitter
from .core import GeneratorConfig, GeneratorEngine
from .namespace_mapper import NamespaceMapper, NamespaceMapping
from .schema_validator import SchemaValidator, ValidationResult

__version__ = "1.0.0"

__all__ = [
    # Core classes
    "GeneratorEngine",
    "GeneratorConfig",

    # Validation
    "SchemaValidator",
    "ValidationResult",

    # Namespace mapping
    "NamespaceMapper",
    "NamespaceMapping",

    # Code emission
    "CodeEmitter",
    "GeneratedFile",
    "GenerationResult",
    "MultiLanguageEmitter",
]
