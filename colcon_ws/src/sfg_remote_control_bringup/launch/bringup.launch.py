import launch
import sfg_utils.launch_utils
from launch_ros.actions import ComposableNodeContainer
from rospkg import get_package_name
from sfg_utils.fqn import RosFqnBuilder, RosFqnSegment, Scope

package_name = get_package_name(__file__)
local_namespace = RosFqnBuilder().scope(Scope.Local).agent()
global_namespace = RosFqnBuilder().scope(Scope.Global).agent()


def generate_launch_description() -> launch.LaunchDescription:
    agent_launch_description_entities = (
        sfg_utils.launch_utils.get_launch_description_entities(
            "sfg_agent",
            "agent.launch.py",
        )
    )

    return launch.LaunchDescription(
        [
            *agent_launch_description_entities.launch_arguments,
            *agent_launch_description_entities.nodes,
            ComposableNodeContainer(
                package="rclcpp_components",
                executable="component_container_mt",
                namespace=local_namespace.build(
                    RosFqnSegment.Scope, RosFqnSegment.Agent
                ),
                name="bringup_container",
                output="screen",
                composable_node_descriptions=agent_launch_description_entities.composable_nodes,
            ),
        ]
    )
