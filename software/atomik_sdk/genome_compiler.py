"""
Genome Compiler Module - Low-level API for hardware configuration.

This module provides the GenomeCompiler class for compiling high-level
ATOMiK configurations into genome (.gnm) files that can be loaded
onto hardware accelerators.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class CodonMapping:
    """Mapping from a 2-bit codon to its semantic meaning."""

    codon: str  # "A", "G", "T", or "C"
    binary: int  # 0b00, 0b01, 0b10, 0b11
    meaning: str


CODON_TABLE = {
    "A": CodonMapping("A", 0b00, "identity"),
    "G": CodonMapping("G", 0b01, "increment"),
    "T": CodonMapping("T", 0b10, "decrement"),
    "C": CodonMapping("C", 0b11, "invert"),
}


@dataclass
class GenomeInstruction:
    """A single instruction in the compiled genome."""

    opcode: int
    operand_a: int
    operand_b: int
    codon_sequence: str

    def to_bytes(self) -> bytes:
        """Serialize instruction to 4 bytes."""
        packed = (
            (self.opcode & 0xFF)
            | ((self.operand_a & 0xFF) << 8)
            | ((self.operand_b & 0xFF) << 16)
            | ((self._encode_codons() & 0xFF) << 24)
        )
        return packed.to_bytes(4, byteorder="little")

    def _encode_codons(self) -> int:
        """Encode codon sequence to a byte."""
        result = 0
        for i, c in enumerate(self.codon_sequence[:4]):
            if c in CODON_TABLE:
                result |= CODON_TABLE[c].binary << (i * 2)
        return result


@dataclass
class Genome:
    """A complete genome configuration for ATOMiK hardware."""

    name: str
    version: str
    instructions: list[GenomeInstruction] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_bytes(self) -> bytes:
        """Serialize the entire genome to bytes."""
        header = self._create_header()
        body = b"".join(inst.to_bytes() for inst in self.instructions)
        return header + body

    def _create_header(self) -> bytes:
        """Create the genome file header."""
        magic = b"ATOM"
        version = int(self.version.replace(".", "")).to_bytes(2, "little")
        inst_count = len(self.instructions).to_bytes(2, "little")
        name_bytes = self.name.encode("utf-8")[:32].ljust(32, b"\x00")
        return magic + version + inst_count + name_bytes

    def save(self, path: str | Path) -> None:
        """Save genome to a .gnm file."""
        with open(path, "wb") as f:
            f.write(self.to_bytes())

    @classmethod
    def load(cls, path: str | Path) -> Genome:
        """Load genome from a .gnm file."""
        with open(path, "rb") as f:
            data = f.read()

        if data[:4] != b"ATOM":
            raise ValueError("Invalid genome file format")

        version = f"{data[4]}.{data[5]}"
        inst_count = int.from_bytes(data[6:8], "little")
        name = data[8:40].rstrip(b"\x00").decode("utf-8")

        instructions = []
        offset = 40
        for _ in range(inst_count):
            packed = int.from_bytes(data[offset : offset + 4], "little")
            inst = GenomeInstruction(
                opcode=packed & 0xFF,
                operand_a=(packed >> 8) & 0xFF,
                operand_b=(packed >> 16) & 0xFF,
                codon_sequence=cls._decode_codons((packed >> 24) & 0xFF),
            )
            instructions.append(inst)
            offset += 4

        return cls(name=name, version=version, instructions=instructions)

    @staticmethod
    def _decode_codons(byte_val: int) -> str:
        """Decode a byte to codon sequence."""
        codons = ["A", "G", "T", "C"]
        result = ""
        for i in range(4):
            idx = (byte_val >> (i * 2)) & 0b11
            result += codons[idx]
        return result


class GenomeCompiler:
    """
    Compiler for transforming high-level configurations into genome files.

    The GenomeCompiler takes JSON schema definitions and produces binary
    genome files that can be loaded onto ATOMiK hardware accelerators.

    Example:
        >>> compiler = GenomeCompiler()
        >>> compiler.load_schema("eth_schema.json")
        >>> genome = compiler.compile()
        >>> genome.save("output.gnm")
    """

    # Opcode definitions
    OPCODES = {
        "NOP": 0x00,
        "XOR": 0x01,
        "AND": 0x02,
        "OR": 0x03,
        "NOT": 0x04,
        "SHIFT_L": 0x05,
        "SHIFT_R": 0x06,
        "ROTATE_L": 0x07,
        "ROTATE_R": 0x08,
        "COMPARE": 0x09,
        "BRANCH": 0x0A,
        "LOAD": 0x0B,
        "STORE": 0x0C,
        "EMIT": 0x0D,
        "CLASSIFY": 0x0E,
        "HALT": 0x0F,
    }

    def __init__(self):
        """Initialize the GenomeCompiler."""
        self.schema: dict[str, Any] | None = None
        self.instructions: list[GenomeInstruction] = []
        self.labels: dict[str, int] = {}
        self.metadata: dict[str, Any] = {}

    def load_schema(self, path: str | Path) -> None:
        """
        Load a genome schema from a JSON file.

        Args:
            path: Path to the schema JSON file.
        """
        with open(path) as f:
            self.schema = json.load(f)

        self.metadata = self.schema.get("metadata", {})

    def compile(self, name: str = "default", version: str = "1.0") -> Genome:
        """
        Compile the loaded schema into a Genome.

        Args:
            name: Name for the compiled genome.
            version: Version string.

        Returns:
            Compiled Genome object.

        Raises:
            ValueError: If no schema is loaded.
        """
        if self.schema is None:
            raise ValueError("No schema loaded")

        self.instructions = []
        self.labels = {}

        # First pass: collect labels
        for i, stmt in enumerate(self.schema.get("instructions", [])):
            if "label" in stmt:
                self.labels[stmt["label"]] = i

        # Second pass: compile instructions
        for stmt in self.schema.get("instructions", []):
            inst = self._compile_statement(stmt)
            self.instructions.append(inst)

        return Genome(
            name=name,
            version=version,
            instructions=self.instructions,
            metadata=self.metadata,
        )

    def _compile_statement(self, stmt: dict[str, Any]) -> GenomeInstruction:
        """Compile a single statement to an instruction."""
        op = stmt.get("op", "NOP").upper()
        opcode = self.OPCODES.get(op, 0x00)

        operand_a = self._resolve_operand(stmt.get("a", 0))
        operand_b = self._resolve_operand(stmt.get("b", 0))
        codon_seq = stmt.get("codons", "AAAA")

        return GenomeInstruction(
            opcode=opcode,
            operand_a=operand_a,
            operand_b=operand_b,
            codon_sequence=codon_seq,
        )

    def _resolve_operand(self, operand: int | str) -> int:
        """Resolve an operand value, handling labels."""
        if isinstance(operand, str):
            if operand in self.labels:
                return self.labels[operand]
            elif operand.startswith("0x"):
                return int(operand, 16)
            else:
                return int(operand)
        return int(operand)

    def create_default_schema(self) -> dict[str, Any]:
        """
        Create a default schema template.

        Returns:
            Dictionary representing a basic schema.
        """
        return {
            "metadata": {
                "description": "Default ATOMiK genome schema",
                "author": "GenomeCompiler",
                "target": "tang_nano_9k",
            },
            "instructions": [
                {"op": "LOAD", "a": 0, "b": 0, "codons": "AAAA"},
                {"op": "XOR", "a": 0, "b": 1, "codons": "AGCT"},
                {"op": "CLASSIFY", "a": 0, "b": 0, "codons": "AAAA"},
                {"op": "EMIT", "a": 0, "b": 0, "codons": "AAAA"},
                {"op": "HALT", "a": 0, "b": 0, "codons": "CCCC"},
            ],
        }
