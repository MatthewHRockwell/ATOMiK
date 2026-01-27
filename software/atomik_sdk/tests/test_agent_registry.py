"""Tests for specialist agent registry."""

from pipeline.agents.registry import AgentRegistry
from pipeline.agents.specialist import AgentCapability, SpecialistAgent


class MockPythonGen(SpecialistAgent):
    name = "python_generator"
    capability = AgentCapability(
        languages=["python"],
        task_types=["generate"],
        model_tier="sonnet",
    )

    def execute(self, task, context):
        return {"files_generated": 1}


class MockRustGen(SpecialistAgent):
    name = "rust_generator"
    capability = AgentCapability(
        languages=["rust"],
        task_types=["generate"],
        model_tier="sonnet",
    )

    def execute(self, task, context):
        return {"files_generated": 1}


class MockVerifier(SpecialistAgent):
    name = "verifier"
    capability = AgentCapability(
        languages=["python", "rust"],
        task_types=["verify"],
        model_tier="haiku",
    )

    def execute(self, task, context):
        return {"tests_passed": 5}


class TestAgentRegistry:
    def test_register_and_find(self):
        registry = AgentRegistry()
        registry.register(MockPythonGen())
        agent = registry.find("generate", language="python")
        assert agent is not None
        assert agent.name == "python_generator"

    def test_find_no_match(self):
        registry = AgentRegistry()
        registry.register(MockPythonGen())
        agent = registry.find("generate", language="verilog")
        assert agent is None

    def test_find_by_task_type(self):
        registry = AgentRegistry()
        registry.register(MockVerifier())
        agent = registry.find("verify", language="python")
        assert agent is not None
        assert agent.name == "verifier"

    def test_find_all(self):
        registry = AgentRegistry()
        registry.register(MockPythonGen())
        registry.register(MockRustGen())
        agents = registry.find_all("generate")
        assert len(agents) == 2

    def test_unregister(self):
        registry = AgentRegistry()
        registry.register(MockPythonGen())
        assert registry.count == 1
        registry.unregister("python_generator")
        assert registry.count == 0

    def test_health_check(self):
        registry = AgentRegistry()
        registry.register(MockPythonGen())
        registry.register(MockRustGen())
        health = registry.health_check()
        assert health["python_generator"] is True
        assert health["rust_generator"] is True

    def test_list_agents(self):
        registry = AgentRegistry()
        registry.register(MockPythonGen())
        agents = registry.list_agents()
        assert len(agents) == 1
        assert agents[0]["name"] == "python_generator"
