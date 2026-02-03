# Trade Secrets & Proprietary Knowledge

*Generated: 2026-02-03*

## Open Source vs Proprietary Delineation

### Open Source (Apache 2.0)

Available in the public repository for evaluation:
- Formal proofs and mathematical specifications
- Reference Verilog implementation
- SDK code generators (5 languages)
- Benchmark suite and test infrastructure
- Documentation and research papers

### Proprietary / Trade Secret

NOT included in the public repository:
- RTL synthesis parameters and optimisation settings
- PLL configuration details for specific FPGA families
- Production-grade timing closure strategies
- ASIC tapeout specifications
- Customer-specific IP core configurations
- Advanced merge tree topologies beyond binary XOR

## Protection Strategy

1. **Patents**: Architecture and execution model (pending)
2. **Trade secrets**: Synthesis optimisation, PLL configs
3. **Copyright**: All source code (Apache 2.0 with patent notice)
4. **Formal verification**: Mathematical proofs establish priority
