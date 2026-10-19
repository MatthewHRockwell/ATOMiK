# 2026-05-21T17:29:15.447446677
import vitis

client = vitis.create_client()
client.set_workspace(path="vitis_ws")

platform = client.create_platform_component(name = "nax64_platform",hw_design = "$COMPONENT_LOCATION/../../nax64_nobit.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0")

vitis.dispose()

