"""Tests for the deterministic schema inference engine."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.inference.heuristics import (
    classify_delta_type,
    classify_vertical,
    is_delta_candidate,
    snap_width,
)
from pipeline.inference.schema_inferrer import InferenceHints, SchemaInferrer
from pipeline.verification.interfaces import (
    InterfaceField,
    InterfaceOperation,
    LanguageInterface,
)


class TestVerticalClassification:
    def test_video_keywords(self):
        assert classify_vertical("FrameDecoder", ["pixel_data", "motion_vector"]) == "Video"

    def test_finance_keywords(self):
        assert classify_vertical("PriceTick", ["price_delta", "volume"]) == "Finance"

    def test_edge_keywords(self):
        assert classify_vertical("IMUFusion", ["accel_x", "gyro_z"]) == "Edge"

    def test_audio_keywords(self):
        assert classify_vertical("AudioMixer", ["sample_rate", "pcm_data"]) == "Audio"

    def test_network_keywords(self):
        assert classify_vertical("PacketRouter", ["latency", "throughput"]) == "Network"

    def test_robotics_keywords(self):
        assert classify_vertical("JointController", ["servo_angle", "trajectory"]) == "Robotics"

    def test_medical_keywords(self):
        assert classify_vertical("ECGMonitor", ["heart_rate", "pulse"]) == "Medical"

    def test_fallback_compute(self):
        assert classify_vertical("FooBar", ["x", "y", "z"]) == "Compute"


class TestWidthSnapping:
    def test_zero_default(self):
        assert snap_width(0) == 64

    def test_negative_default(self):
        assert snap_width(-1) == 64

    def test_exact_8(self):
        assert snap_width(8) == 8

    def test_exact_64(self):
        assert snap_width(64) == 64

    def test_exact_128(self):
        assert snap_width(128) == 128

    def test_snap_1_to_8(self):
        assert snap_width(1) == 8

    def test_snap_33_to_32(self):
        assert snap_width(33) == 32

    def test_snap_48_to_32_or_64(self):
        # 48 is equidistant from 32 and 64; snaps to first found (32)
        result = snap_width(48)
        assert result in (32, 64)

    def test_snap_300_to_256(self):
        assert snap_width(300) == 256


class TestDeltaTypeClassification:
    def test_bitmask_from_flags(self):
        assert classify_delta_type("alert_flags", "int") == "bitmask_delta"

    def test_bitmask_from_mask(self):
        assert classify_delta_type("status_mask", "uint64") == "bitmask_delta"

    def test_bitmask_from_status(self):
        assert classify_delta_type("device_status", "u32") == "bitmask_delta"

    def test_stream_from_frame(self):
        assert classify_delta_type("frame_delta", "bytes") == "delta_stream"

    def test_stream_from_buffer(self):
        assert classify_delta_type("data_buffer", "Vec") == "delta_stream"

    def test_stream_from_stream(self):
        assert classify_delta_type("video_stream", "bytes") == "delta_stream"

    def test_parameter_default(self):
        assert classify_delta_type("price_delta", "float") == "parameter_delta"

    def test_parameter_generic(self):
        assert classify_delta_type("temperature", "f64") == "parameter_delta"


class TestFieldExclusion:
    def test_name_excluded(self):
        assert not is_delta_candidate("name")

    def test_logger_excluded(self):
        assert not is_delta_candidate("logger")

    def test_config_excluded(self):
        assert not is_delta_candidate("config")

    def test_id_excluded(self):
        assert not is_delta_candidate("id")

    def test_delta_field_included(self):
        assert is_delta_candidate("price_delta")

    def test_motion_included(self):
        assert is_delta_candidate("motion_vector")

    def test_underscore_stripped(self):
        assert not is_delta_candidate("_name_")


class TestSchemaInferrer:
    def _make_iface(self, **kwargs) -> LanguageInterface:
        """Helper to build a LanguageInterface with defaults."""
        defaults = dict(
            language="python",
            file_path="/src/trading/engine.py",
            struct_name="TradingEngine",
            fields=[
                InterfaceField(name="price_delta", type_name="float", bit_width=64),
                InterfaceField(name="volume_delta", type_name="int", bit_width=64),
                InterfaceField(name="trade_flags", type_name="int", bit_width=32),
            ],
            operations=[
                InterfaceOperation(name="accumulate", return_type="None"),
                InterfaceOperation(name="get_state", return_type="dict"),
                InterfaceOperation(name="rollback", return_type="None"),
            ],
            constants={"HISTORY_DEPTH": 512},
        )
        defaults.update(kwargs)
        return LanguageInterface(**defaults)

    def test_infer_produces_valid_schema(self):
        iface = self._make_iface()
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        assert "catalogue" in schema
        assert "schema" in schema
        assert "delta_fields" in schema["schema"]
        assert "operations" in schema["schema"]
        assert "constraints" in schema["schema"]

    def test_catalogue_inference(self):
        iface = self._make_iface()
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        cat = schema["catalogue"]
        assert cat["object"] == "TradingEngine"
        assert cat["vertical"] == "Finance"
        assert cat["version"] == "1.0.0"

    def test_vertical_override(self):
        iface = self._make_iface()
        inferrer = SchemaInferrer()
        hints = InferenceHints(vertical="Edge")
        schema = inferrer.infer(iface, hints)

        assert schema["catalogue"]["vertical"] == "Edge"

    def test_delta_fields_inferred(self):
        iface = self._make_iface()
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        fields = schema["schema"]["delta_fields"]
        assert "price_delta" in fields
        assert "volume_delta" in fields
        assert "trade_flags" in fields
        assert fields["trade_flags"]["type"] == "bitmask_delta"
        assert fields["price_delta"]["type"] == "parameter_delta"

    def test_excluded_fields_not_in_delta(self):
        iface = self._make_iface(fields=[
            InterfaceField(name="price_delta", type_name="float", bit_width=64),
            InterfaceField(name="name", type_name="str", bit_width=0),
            InterfaceField(name="logger", type_name="Logger", bit_width=0),
        ])
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        fields = schema["schema"]["delta_fields"]
        assert "price_delta" in fields
        assert "name" not in fields
        assert "logger" not in fields

    def test_operations_inference(self):
        iface = self._make_iface()
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        ops = schema["schema"]["operations"]
        assert ops["accumulate"]["enabled"] is True
        assert "reconstruct" in ops  # from get_state
        assert ops["reconstruct"]["enabled"] is True
        assert "rollback" in ops
        assert ops["rollback"]["enabled"] is True
        assert ops["rollback"]["history_depth"] == 512  # from constants

    def test_operations_minimal(self):
        """Only accumulate when no matching method names."""
        iface = self._make_iface(operations=[
            InterfaceOperation(name="compute", return_type="int"),
        ], constants={})
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        ops = schema["schema"]["operations"]
        assert "accumulate" in ops
        assert "reconstruct" not in ops
        assert "rollback" not in ops

    def test_constraints_defaults(self):
        iface = self._make_iface()
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        constraints = schema["schema"]["constraints"]
        assert constraints["target_frequency_mhz"] == 94.5
        assert constraints["max_memory_mb"] == 64

    def test_wide_stream_encoding(self):
        """Wide delta_stream fields should get spatiotemporal encoding."""
        iface = self._make_iface(fields=[
            InterfaceField(name="frame_buffer", type_name="bytes", bit_width=128),
        ])
        inferrer = SchemaInferrer()
        schema = inferrer.infer(iface)

        field = schema["schema"]["delta_fields"]["frame_buffer"]
        assert field["type"] == "delta_stream"
        assert field["encoding"] == "spatiotemporal_4x4x4"
        assert field["compression"] == "xor"

    def test_version_override(self):
        iface = self._make_iface()
        inferrer = SchemaInferrer()
        hints = InferenceHints(version="2.5.0")
        schema = inferrer.infer(iface, hints)

        assert schema["catalogue"]["version"] == "2.5.0"
