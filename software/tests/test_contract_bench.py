"""
Tests for atomik_sdk.contract_bench — the aggregation benchmark-proof harness.

These pin the honesty rules: throughput comes from the silicon-measured engine,
order-independence is only claimed for abelian aggregations, and the verdict
reflects BOTH gates.
"""

from __future__ import annotations

from pathlib import Path

import pytest

from atomik_sdk.contract_bench import ENGINE, WorkloadSpec, evaluate, is_abelian


class TestSilicon:
    def test_peak_is_eight_banks_at_clock(self):
        # 65537 deltas / 8193 cycles * 100MHz ~= 800 M/s
        assert ENGINE.peak_throughput() == pytest.approx(800e6, rel=0.01)

    def test_per_bank_is_clock(self):
        # 1 accumulation / bank / cycle at 100 MHz
        assert ENGINE.per_bank_throughput() == pytest.approx(100e6, rel=0.01)

    def test_scaling_is_linear_and_measured(self):
        s = ENGINE.scaling()
        assert s[1] == pytest.approx(1.0, abs=0.01)
        assert s[8] == pytest.approx(7.99, abs=0.05)

    def test_byte_identical_recorded(self):
        assert ENGINE.identical_across_banks
        assert ENGINE.result_hex == "0x17e9fe29b5dc0aad"


class TestVerdict:
    def test_surpass_when_well_under_peak_and_abelian(self):
        spec = WorkloadSpec("rollup", aggregation="sum", required_throughput=50e6)
        r = evaluate(spec)
        assert r.verdict == "SURPASS"
        assert r.throughput_gate.passed and r.correctness_gate.passed
        assert r.margin == pytest.approx(16.0, rel=0.05)

    def test_pass_when_just_under_peak(self):
        spec = WorkloadSpec("tight", aggregation="xor", required_throughput=600e6)
        r = evaluate(spec)
        assert r.verdict == "PASS"  # passes but <2x headroom

    def test_fail_when_over_peak(self):
        spec = WorkloadSpec("toobig", aggregation="sum", required_throughput=2e9)
        r = evaluate(spec)
        assert r.verdict == "FAIL"
        assert not r.throughput_gate.passed
        assert r.correctness_gate.passed  # correctness still holds

    def test_fail_correctness_for_non_abelian(self):
        # a non-order-independent op: we must NOT claim reorder-stability
        spec = WorkloadSpec("avg", aggregation="median", required_throughput=1e6)
        r = evaluate(spec)
        assert r.verdict == "FAIL"
        assert not r.correctness_gate.passed
        assert not is_abelian("median")

    def test_min_banks_required_scales_with_bar(self):
        # 250 M/s needs ceil(250/100) = 3 banks
        r = evaluate(WorkloadSpec("x", aggregation="sum", required_throughput=250e6))
        assert r.min_banks_required == 3
        assert r.banks_available == 8

    def test_no_reorder_requirement_still_passes(self):
        spec = WorkloadSpec("simple", aggregation="sum", required_throughput=10e6,
                            requires_reorder_stability=False)
        r = evaluate(spec)
        assert r.correctness_gate.passed


class TestBoardParse:
    SAMPLE = """ATOMiK parallel-bench  v2  N_BANKS=8
count=50000000  seed=0xdeadbeef0badf00d

banks  hw_cycles   speedup  result              ok
1      50000001    1.00    0x0badf00ddeadbeef  OK
2      25000001    1.99    0x0badf00ddeadbeef  OK
4      12500001    3.99    0x0badf00ddeadbeef  OK
8      6250001     7.99    0x0badf00ddeadbeef  OK

software reference = 0x0badf00ddeadbeef
VERIFY: ALL MATCH
"""

    def test_parse_live_output(self):
        from atomik_sdk.contract_bench.board import parse_abench
        m = parse_abench(self.SAMPLE, count=50_000_000)
        assert m.cycles == {1: 50000001, 2: 25000001, 4: 12500001, 8: 6250001}
        assert m.identical_across_banks
        assert m.result_hex == "0x0badf00ddeadbeef"
        # ~8 deltas/cycle at 8 banks -> ~800 M/s, count-independent
        assert m.peak_throughput() == pytest.approx(800e6, rel=0.01)

    def test_parse_evaluates_to_surpass(self):
        from atomik_sdk.contract_bench.board import parse_abench
        m = parse_abench(self.SAMPLE, count=50_000_000)
        spec = WorkloadSpec("live rollup", aggregation="sum", required_throughput=50e6)
        r = evaluate(spec, m)
        assert r.verdict == "SURPASS"
        assert "live" in r.provenance["measured_on"]

    def test_parse_raises_on_empty(self):
        from atomik_sdk.contract_bench.board import parse_abench
        with pytest.raises(RuntimeError):
            parse_abench("no data here", count=1)

    def test_parse_survives_one_uart_mangled_hash(self):
        # row for 1 bank lost its hex over the UART (-> 0x0000000000000000);
        # the other 3 agree, so the report must use the real value.
        from atomik_sdk.contract_bench.board import parse_abench
        mangled = self.SAMPLE.replace("1      50000001    1.00    0x0badf00ddeadbeef",
                                      "1      50000001    1.00    0x0000000000000000")
        m = parse_abench(mangled, count=50_000_000)
        assert m.result_hex == "0x0badf00ddeadbeef"
        assert m.cycles[1] == 50000001


class TestRendering:
    def test_markdown_has_both_gates_and_verdict(self):
        r = evaluate(WorkloadSpec("rollup", aggregation="sum",
                                  required_throughput=50e6, customer="Acme"))
        md = r.to_markdown()
        assert "VERDICT: SURPASS" in md
        assert "Throughput gate" in md
        assert "Correctness gate" in md
        assert "Acme" in md
        assert "0x17e9fe29b5dc0aad" in md

    def test_json_roundtrips(self):
        import json
        r = evaluate(WorkloadSpec("rollup", aggregation="sum", required_throughput=50e6))
        d = json.loads(r.to_json())
        assert d["verdict"] == "SURPASS"
        assert d["fields"]["measured_peak_throughput"] == pytest.approx(800e6, rel=0.01)

    def test_example_spec_loads_and_surpasses(self):
        ex = (Path(__file__).resolve().parents[1]
              / "atomik_sdk/contract_bench/examples/telemetry_counter_rollup.json")
        r = evaluate(WorkloadSpec.from_json(ex))
        assert r.verdict == "SURPASS"
