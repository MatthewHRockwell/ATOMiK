"""Demo acts — the 5-act demo sequence for VC presentations."""

from demo.acts.act1_basic_algebra import Act1BasicAlgebra
from demo.acts.act2_self_inverse import Act2SelfInverse
from demo.acts.act3_parallel_scaling import Act3ParallelScaling
from demo.acts.act4_domain_apps import Act4DomainApps
from demo.acts.act5_distributed_merge import Act5DistributedMerge

ALL_ACTS = [
    Act1BasicAlgebra,
    Act2SelfInverse,
    Act3ParallelScaling,
    Act4DomainApps,
    Act5DistributedMerge,
]

__all__ = [
    "ALL_ACTS",
    "Act1BasicAlgebra",
    "Act2SelfInverse",
    "Act3ParallelScaling",
    "Act4DomainApps",
    "Act5DistributedMerge",
]
