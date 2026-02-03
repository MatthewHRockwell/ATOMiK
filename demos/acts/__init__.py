"""Demo acts — the 5-act demo sequence for VC presentations."""

from demos.acts.act1_basic_algebra import Act1BasicAlgebra
from demos.acts.act2_self_inverse import Act2SelfInverse
from demos.acts.act3_parallel_scaling import Act3ParallelScaling
from demos.acts.act4_domain_apps import Act4DomainApps
from demos.acts.act5_distributed_merge import Act5DistributedMerge

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
