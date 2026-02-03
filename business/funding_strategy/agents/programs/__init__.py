"""Program agent registry — imports all application agents."""

from __future__ import annotations

from .accelerators import (
    AlchemistAccelerator,
    HAXAccelerator,
    SiliconCatalyst,
    Techstars,
    YCombinator,
)
from .base import BaseApplication
from .grants import ChipsAct, DoeSbir, DodSbir, NasaSbir, NsfSbir
from .incorporation import Incorporation
from .outreach import DefenseOutreach, VCOutreach
from .sam_gov import SamGov
from .zero_cost import CDL, ICorp, IntelPartnerAlliance, NvidiaInception

ALL_PROGRAMS: list[type[BaseApplication]] = [
    # Phase 0 — Entity Formation
    Incorporation,
    SamGov,
    # Phase 1 — Zero-cost
    NvidiaInception,
    CDL,
    ICorp,
    IntelPartnerAlliance,
    # Phase 2 — Grants
    NsfSbir,
    DodSbir,
    DoeSbir,
    NasaSbir,
    ChipsAct,
    # Phase 3 — Accelerators
    SiliconCatalyst,
    AlchemistAccelerator,
    HAXAccelerator,
    YCombinator,
    Techstars,
    # Phase 4 — Outreach
    VCOutreach,
    DefenseOutreach,
]


def get_all_programs() -> list[BaseApplication]:
    """Instantiate every registered program agent."""
    return [cls() for cls in ALL_PROGRAMS]
