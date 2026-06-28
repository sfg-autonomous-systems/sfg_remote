#include "sfg_universal_remote_control/app/agent_manager.hpp"
#include "sfg_utils/fqn/ros_fqn_builder.hpp"

namespace sfg_universal_remote_control::app
{
    AgentManager::AgentManager(rclcpp::Node *node) : m_node(node)
    {
        using namespace sfg_utils::fqn;

        // Set up interfaces.
        m_get_discovered_agents_client = m_node->create_client<sfg_agent_msgs::srv::GetDiscoveredAgents>(
            RosFqnBuilder().scope(Scope::Local).agent().resource(Resource::Custom, "get_discovered_agents").build());

        get_discovered_agents();
    }

    AgentManager::~AgentManager()
    {
        if (has_selected())
        {
            detach_from_agent();
        }
    }

    void AgentManager::refresh_discovered_agents()
    {
        get_discovered_agents();
    }

    void AgentManager::switch_to_agent(const std::string &name)
    {
        // when name actually changed, recreate subscribers and publishers for matching topic
        if (name == m_selected_agent_name)
        {
            return;
        }

        auto iterator = m_agents.find(name);

        if (iterator == m_agents.end())
        {
            return;
        }

        const auto &selected_agent = iterator->second;

        // Print relevant metadata for selected agent.
        RCLCPP_INFO(m_node->get_logger(), "Metadata for agent %s:", selected_agent.agent_name.c_str());
        RCLCPP_INFO(m_node->get_logger(), "  primary camera: %s", selected_agent.cameras.at(0).c_str());

        // Disarm before switching to another.
        if (is_armed())
        {
            disarm();
        }

        // Now switch over.
        m_selected_agent_name = name;
        create_publisher_and_subscriber();

        // Recreate the trigger-action service client and arm the newly selected agent.
        using namespace sfg_utils::fqn;

        m_trigger_action_client = m_node->create_client<sfg_agent_msgs::srv::TriggerAction>(
            RosFqnBuilder()
                .scope(Scope::Global)
                .component(Component::LocomotionController)
                .agent(m_selected_agent_name)
                .resource(Resource::TriggerAction)
                .build());

        arm();
    }

    void AgentManager::detach_from_agent()
    {
        // Disarm before detach.
        if (is_armed())
        {
            disarm();
        }

        destroy_publisher_and_subscriber();
        m_selected_agent_name = "";
    }

    const std::map<std::string, sfg_agent_msgs::msg::Metadata> &AgentManager::get_all_agents() const
    {
        return m_agents;
    }

    const sfg_agent_msgs::msg::Metadata *AgentManager::get_metadata_for_selected() const
    {
        if (!has_selected())
        {
            return nullptr;
        }
        return &m_agents.at(m_selected_agent_name);
    }

    bool AgentManager::is_armed() const
    {
        return m_is_armed;
    }

    bool AgentManager::has_selected() const
    {
        // Empty name means no agent is selected.
        return !m_selected_agent_name.empty();
    }

    bool AgentManager::is_new_image_available()
    {
        std::lock_guard<std::mutex> lock(m_image_mutex);
        return m_new_image_available;
    }

    cv::Mat AgentManager::get_image()
    {
        std::lock_guard<std::mutex> lock(m_image_mutex);
        m_new_image_available = false;
        return m_image.clone();
    }

    void AgentManager::create_publisher_and_subscriber()
    {
        using namespace sfg_utils::fqn;

        // Create the command publisher.
        auto topic = RosFqnBuilder().scope(Scope::Global).agent(m_selected_agent_name).component(Component::LocomotionController).resource(Resource::CmdVel).build();
        m_cmd_vel_publisher = m_node->create_publisher<geometry_msgs::msg::TwistStamped>(topic, rclcpp::SensorDataQoS());

        // Create the image subscriber.
        auto primary_camera_name = m_agents.find(m_selected_agent_name)->second.cameras.at(0);
        topic = RosFqnBuilder().scope(Scope::Local).agent(m_selected_agent_name).component(Component::Camera, primary_camera_name).stream(Stream::Color).resource(Resource::ImageRaw).build();

        m_image_subscriber = m_node->create_subscription<sensor_msgs::msg::Image>(
            topic,
            rclcpp::SensorDataQoS(),
            std::bind(&AgentManager::image_callback, this, std::placeholders::_1));
    }

    void AgentManager::destroy_publisher_and_subscriber()
    {
        if (m_cmd_vel_publisher)
        {
            m_cmd_vel_publisher = nullptr;
        }

        if (m_image_subscriber)
        {
            m_image_subscriber = nullptr;
        }
    }

    void AgentManager::arm()
    {
        if (!has_selected())
        {
            return;
        }

        // Create request for arming service.
        auto request = std::make_shared<sfg_agent_msgs::srv::TriggerAction::Request>();
        request->action = "arm";

        // Request arming service.
        auto future = m_trigger_action_client->async_send_request(
            request,
            [this](rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedFuture future)
            {
                if (!future.valid())
                {
                    return;
                }

                auto payload = future.get();
                auto result = payload->result;

                switch (result)
                {
                    case sfg_agent_msgs::srv::TriggerAction_Response::FAILURE:
                    {
                        RCLCPP_ERROR(m_node->get_logger(), "Arming failed!");
                        m_is_armed = false;
                        break;
                    }
                    case sfg_agent_msgs::srv::TriggerAction_Response::SUCCESS:
                    {
                        RCLCPP_INFO(m_node->get_logger(), "Arming successful.");
                        m_is_armed = true;
                        break;
                    }
                    case sfg_agent_msgs::srv::TriggerAction_Response::UNSUPPORTED:
                    {
                        RCLCPP_INFO(m_node->get_logger(), "Arming not implemented (ignored).");
                        m_is_armed = true;
                        break;
                    }
                    default:
                    {
                        RCLCPP_WARN(m_node->get_logger(), "Arming response status unknown. Status Code = %d", result);
                        break;
                    }
                }
            });
    }

    void AgentManager::disarm()
    {
        if (!has_selected())
        {
            return;
        }

        // Create request for disarm service.
        auto request = std::make_shared<sfg_agent_msgs::srv::TriggerAction::Request>();
        request->action = "disarm";

        // Request disarm service.
        auto future = m_trigger_action_client->async_send_request(
            request,
            [this](rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedFuture future)
            {
                if (!future.valid())
                {
                    return;
                }

                auto payload = future.get();
                auto result = payload->result;

                switch (result)
                {
                    case sfg_agent_msgs::srv::TriggerAction_Response::FAILURE:
                    {
                        RCLCPP_ERROR(m_node->get_logger(), "Disarming failed!");
                        break;
                    }
                    case sfg_agent_msgs::srv::TriggerAction_Response::SUCCESS:
                    {
                        RCLCPP_INFO(m_node->get_logger(), "Disarming successful.");
                        m_is_armed = false;
                        break;
                    }
                    case sfg_agent_msgs::srv::TriggerAction_Response::UNSUPPORTED:
                    {
                        RCLCPP_INFO(m_node->get_logger(), "Disarming not implemented (ignored).");
                        m_is_armed = true;
                        break;
                    }
                    default:
                    {
                        RCLCPP_WARN(m_node->get_logger(), "Disarming response status unknown. Status Code = %d", result);
                        break;
                    }
                }
            });
    }

    void AgentManager::publish_cmd_vel(double left_x, double left_y, double right_x, double right_y)
    {
        if (!is_armed())
        {
            return;
        }

        // Linear mapping of stick positions onto target velocities (max stick position corresponds to max velocity).
        // Normalise stick positions to range -1.0 to 1.0.
        left_x /= std::numeric_limits<std::int16_t>::max();
        left_y /= std::numeric_limits<std::int16_t>::max();
        right_x /= std::numeric_limits<std::int16_t>::max();
        right_y /= std::numeric_limits<std::int16_t>::max();

        // Upper and lower clamping.
        left_x = std::max(-1.0, std::min(left_x, 1.0));
        left_y = std::max(-1.0, std::min(left_y, 1.0));
        right_x = std::max(-1.0, std::min(right_x, 1.0));
        right_y = std::max(-1.0, std::min(right_y, 1.0));

        // Center sensitivity deadzone.
        left_x = std::abs(left_x) < 0.4 ? 0.0 : left_x;
        left_y = std::abs(left_y) < 0.1 ? 0.0 : left_y;
        right_x = std::abs(right_x) < 0.2 ? 0.0 : right_x;
        right_y = std::abs(right_y) < 0.2 ? 0.0 : right_y;

        constexpr auto max_linear_velocity = 2.8;  // In m/s.
        constexpr auto max_angular_velocity = 1.5; // In rad/s.

        // Construct twist message and publish.
        auto msg = std::make_unique<geometry_msgs::msg::TwistStamped>();

        msg->twist.linear.x = -left_y * max_linear_velocity; // Moving back and forth.
        msg->twist.linear.y = -left_x * max_linear_velocity; // Moving sideways.
        msg->twist.linear.z = 0.0;                           // Moving up and down.

        msg->twist.angular.x = 0.0;                             // Turning sideways (roll).
        msg->twist.angular.y = right_y * max_angular_velocity;  // Turning up and down (pitch).
        msg->twist.angular.z = -right_x * max_angular_velocity; // Turning left and right (yaw).
        msg->header.stamp = m_node->now();

        m_cmd_vel_publisher->publish(std::move(msg));
    }

    void AgentManager::get_discovered_agents()
    {
        m_get_discovered_agents_client->async_send_request(
            std::make_shared<sfg_agent_msgs::srv::GetDiscoveredAgents::Request>(),
            [this](rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedFuture future)
            {
                if (!future.valid())
                {
                    return;
                }

                // Clear the list.
                m_agents.clear();

                // Store the new list/map.
                for (auto metadata : future.get()->metadata)
                {
                    // ToDo: Only store agents that have a locomotion controller and a camera.
                    m_agents[metadata.agent_name] = metadata;
                }
            });
    }

    void AgentManager::image_callback(const sensor_msgs::msg::Image::ConstSharedPtr &msg)
    {
        if (!msg)
        {
            RCLCPP_WARN(m_node->get_logger(), "Invalid image message!");
        }

        // Perform conversion into a drawable image data structure.
        try
        {
            // Convert the ROS image to an OpenCV Mat in RGBA format (ideal for OpenGL/ImGui).
            auto cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::RGBA8);

            // Lock and copy the CPU buffer so the render thread can safely access it.
            {
                std::lock_guard<std::mutex> lock(m_image_mutex);
                m_image = cv_ptr->image;
                m_new_image_available = true;
            }
        }
        catch (const cv_bridge::Exception &e)
        {
            RCLCPP_ERROR(m_node->get_logger(), "Image conversion failed! cv_bridge exception: %s", e.what());
        }
    }
}