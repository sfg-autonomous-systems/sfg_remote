import sfg_utils.launch_utils
from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare
from rospkg import get_package_name
from sfg_utils.fqn import RosFqnBuilder, RosFqnSegment, Scope

package_name = get_package_name(__file__)
local_namespace = RosFqnBuilder().scope(Scope.Local).agent()
global_namespace = RosFqnBuilder().scope(Scope.Global).agent()


def generate_launch_description() -> LaunchDescription:
    agent_launch_description_entities = (
        sfg_utils.launch_utils.get_launch_description_entities(
            "sfg_agent", "agent.launch.py"
        )
    )

    agent_discoverer_node = ComposableNode(
        package="sfg_agent",
        plugin="sfg_agent::AgentDiscoverer",
        namespace=local_namespace.build(RosFqnSegment.Scope, RosFqnSegment.Agent),
        parameters=[
            PathJoinSubstitution(
                [
                    FindPackageShare(package_name),
                    "config",
                    "agent_discoverer.yaml",
                ]
            ),
        ],
        extra_arguments=[{"use_intra_process_comms": True}],
    )

    agent_decoder_node = ComposableNode(
        package="sfg_agent",
        plugin="sfg_agent::AgentDecoder",
        namespace=local_namespace.build(RosFqnSegment.Scope, RosFqnSegment.Agent),
        parameters=[
            PathJoinSubstitution(
                [
                    FindPackageShare(package_name),
                    "config",
                    "agent_decoder.yaml",
                ]
            ),
            {
                "container_name": f"{local_namespace.build(RosFqnSegment.Scope, RosFqnSegment.Agent)}/bringup_container",
            },
        ],
    )

    return LaunchDescription(
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
                composable_node_descriptions=(
                    *agent_launch_description_entities.composable_nodes,
                    agent_discoverer_node,
                    agent_decoder_node,
                ),
                output="screen",
            ),
        ]
    )
