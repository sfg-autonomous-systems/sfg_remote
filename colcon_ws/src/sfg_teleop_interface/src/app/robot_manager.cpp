#include "sfg_teleop_interface/app/robot_manager.hpp"
#include "sfg_utils/fqn/ros_fqn_builder.hpp"

namespace sfg_teleop_interface::app
{
    RobotManager::RobotManager(rclcpp::Node *node) : m_node(node)
    {
        using namespace sfg_utils::fqn;

        // create service instances
        m_discovery_service = m_node->create_client<sfg_agent_msgs::srv::GetDiscoveredAgents>(RosFqnBuilder().scope(Scope::Local).agent().resource(Resource::Custom, "get_discovered_agents").build());
        this->fetch_discovery_agents_metadata();

        m_selected_robot_name = "";
    }

    RobotManager::~RobotManager()
    {
        if (m_node)
        {
            m_node = nullptr;
        }

        m_selected_robot_name = "";
    }

    void RobotManager::refresh_discovery_now()
    {
        this->fetch_discovery_agents_metadata();
    }

    void RobotManager::create_publisher_and_subscriber()
    {
        using namespace sfg_utils::fqn;
        std::string topic_string = "";

        // create the command publisher
        topic_string = RosFqnBuilder().scope(Scope::Global).agent(m_selected_robot_name).component(Component::LocomotionController).resource(Resource::CmdVel).build();
        m_control_pub = m_node->create_publisher<geometry_msgs::msg::TwistStamped>(topic_string, 10);

        // create the image subscriber
        const std::string primary_camera_name = this->m_agent_metadata.find(m_selected_robot_name)->second.cameras.at(0);
        topic_string = RosFqnBuilder().scope(Scope::Local).agent(m_selected_robot_name).component(Component::Camera, primary_camera_name).stream(Stream::Color).resource(Resource::ImageRaw).build();
        auto qos = rclcpp::SensorDataQoS();

        m_image_sub = m_node->create_subscription<sensor_msgs::msg::Image>(
            topic_string,
            qos,
            std::bind(&RobotManager::on_image_received, this, _1));
    }

    void RobotManager::destroy_publisher_and_subscriber()
    {
        if (m_control_pub)
        {
            m_control_pub = nullptr;
        }

        if (m_image_sub)
        {
            m_image_sub = nullptr;
        }
    }

    void RobotManager::publish_move_command()
    {
    }

    void RobotManager::fetch_discovery_agents_metadata()
    {
        auto future = m_discovery_service->async_send_request(
            std::make_shared<sfg_agent_msgs::srv::GetDiscoveredAgents::Request>(),
            [this](rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedFuture future)
            {
                future.wait();

                if (future.valid())
                {
                    // clear the list
                    this->m_agent_metadata.clear();

                    // store the new list/map
                    auto payload = future.get();

                    for (auto entry : payload->metadata)
                    {
                        this->m_agent_metadata[entry.agent_name] = entry;
                    }
                }
            });
    }

    void RobotManager::on_image_received(const sensor_msgs::msg::Image::ConstSharedPtr &msg)
    {
        if (!msg)
        {
            RCLCPP_WARN(m_node->get_logger(), "Invalid image message!");
        }

        // perform conversion into drawable image data structure
        // ...
    }
}