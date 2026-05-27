#include "sfg_teleop_interface/app/robot_manager.hpp"
#include "sfg_utils/fqn/ros_fqn_builder.hpp"

namespace sfg_teleop_interface::app
{
    const double max_linear_velocity = 2.8;  // in m/s
    const double max_angular_velocity = 1.5; // in rad/s
    const double max_stick_val_abs = 32767;

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
        m_is_armed = false;
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

    void RobotManager::publish_move_command(double leftX, double leftY, double rightX, double rightY)
    {
        if (!m_is_armed)
        {
            return;
        }

        // linear mapping of stick positions onto target velocities (max stick position eql. to max velocity) //

        // normalise stick positions ro range -1.0 to 1.0
        leftX /= max_stick_val_abs;
        leftY /= max_stick_val_abs;
        rightX /= max_stick_val_abs;
        rightY /= max_stick_val_abs;

        // upper and lower clamping
        if (leftX > 1.0)
            leftX = 1.0;
        if (leftX < -1.0)
            leftX = -1.0;
        if (leftY > 1.0)
            leftY = 1.0;
        if (leftY < -1.0)
            leftY = -1.0;
        if (rightX > 1.0)
            rightX = 1.0;
        if (rightX < -1.0)
            rightX = -1.0;
        if (rightY > 1.0)
            rightY = 1.0;
        if (rightY < -1.0)
            rightY = -1.0;

        // center sensitivity deadzone
        if (abs(leftX) < 0.4)
            leftX = 0.0;
        if (abs(leftY) < 0.1)
            leftY = 0.0;
        if (abs(rightX) < 0.2)
            rightX = 0.0;
        if (abs(rightY) < 0.2)
            rightY = 0.0;

        // construct twist message and publish
        geometry_msgs::msg::TwistStamped t_cmd;
        t_cmd.twist.linear.x = -leftY * max_linear_velocity; // moving back and forth
        t_cmd.twist.linear.y = -leftX * max_linear_velocity; // moving sideways
        t_cmd.twist.linear.z = 0.0;                          // moving up and down

        t_cmd.twist.angular.x = 0.0;                            // turning sideways (roll)
        t_cmd.twist.angular.y = rightY * max_angular_velocity;  // turning up and down (pitch)
        t_cmd.twist.angular.z = -rightX * max_angular_velocity; // turning left and right (yaw)
        t_cmd.header.stamp = m_node->now();

        m_control_pub->publish(t_cmd);
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