#include "sfg_universal_remote_control/app/agent_manager.hpp"
#include "sfg_utils/fqn/ros_fqn_builder.hpp"

namespace sfg_universal_remote_control::app
{
    constexpr auto max_linear_velocity = 2.8;  // in m/s
    constexpr auto max_angular_velocity = 1.5; // in rad/s
    constexpr auto max_stick_val_abs = 32767.0;

    AgentManager::AgentManager(rclcpp::Node *node)
        : m_node(node),
          m_selected_agent_name(""),
          m_is_armed(false),
          m_new_image_available(false)
    {
        using namespace sfg_utils::fqn;

        // create service instances
        m_discovery_service = m_node->create_client<sfg_agent_msgs::srv::GetDiscoveredAgents>(
            RosFqnBuilder().scope(Scope::Local).agent().resource(Resource::Custom, "get_discovered_agents").build());

        fetch_discovery_agents_metadata();
    }

    AgentManager::~AgentManager()
    {
        if (m_selected_agent_name != "")
        {
            detach_from_agent();
        }

        if (m_node)
        {
            m_node = nullptr;
        }
    }

    void AgentManager::refresh_discovery_now()
    {
        fetch_discovery_agents_metadata();
    }

    void AgentManager::switch_to_agent(const std::string &name)
    {
        // when name actually changed, recreate subscribers and publishers for matching topic
        if (name == m_selected_agent_name)
        {
            return;
        }

        auto iterator = m_agent_metadata.find(name);

        if (iterator == m_agent_metadata.end())
        {
            return;
        }

        const auto &selected_agent = iterator->second;

        // Print some metadata for selected agent.
        RCLCPP_INFO(m_node->get_logger(), "Metadata for agent %s:", selected_agent.agent_name.c_str());
        RCLCPP_INFO(m_node->get_logger(), "  primary camera: %s", selected_agent.cameras.at(0).c_str());
        RCLCPP_INFO(m_node->get_logger(), "  primary lidar:  %s", selected_agent.lidars.at(0).c_str());

        // disarm before switching to another
        if (m_is_armed)
        {
            disarm();
        }

        // Now switch over.
        m_selected_agent_name = name;
        create_publisher_and_subscriber();

        // Recreate the trigger-action service client and arm the newly selected agent.
        using namespace sfg_utils::fqn;

        m_trigger_action_service = m_node->create_client<sfg_agent_msgs::srv::TriggerAction>(
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
        // disarm before detach
        if (m_is_armed)
        {
            disarm();
        }

        destroy_publisher_and_subscriber();
        m_selected_agent_name = "";
    }

    const std::map<std::string, sfg_agent_msgs::msg::Metadata> &AgentManager::get_all_agent_metadata() const
    {
        return m_agent_metadata;
    }

    const sfg_agent_msgs::msg::Metadata *AgentManager::get_metadata_for_selected() const
    {
        if (m_selected_agent_name == "")
        {
            return nullptr;
        }
        return &m_agent_metadata.at(m_selected_agent_name);
    }

    bool AgentManager::is_armed() const
    {
        return m_is_armed;
    }

    bool AgentManager::is_selected() const
    {
        return (m_selected_agent_name != ""); // empty name means no agent is selected.
    }

    bool AgentManager::is_new_image_available()
    {
        std::lock_guard<std::mutex> lock(m_image_mutex);
        return m_new_image_available;
    }

    cv::Mat AgentManager::get_latest_image()
    {
        std::lock_guard<std::mutex> lock(m_image_mutex);
        m_new_image_available = false;
        return m_pending_image_rgba.clone();
    }

    void AgentManager::create_publisher_and_subscriber()
    {
        using namespace sfg_utils::fqn;
        std::string topic_string = "";

        // create the command publisher
        topic_string = RosFqnBuilder().scope(Scope::Global).agent(m_selected_agent_name).component(Component::LocomotionController).resource(Resource::CmdVel).build();
        m_control_publisher = m_node->create_publisher<geometry_msgs::msg::TwistStamped>(topic_string, 10);

        // create the image subscriber
        const std::string primary_camera_name = m_agent_metadata.find(m_selected_agent_name)->second.cameras.at(0);
        topic_string = RosFqnBuilder().scope(Scope::Local).agent(m_selected_agent_name).component(Component::Camera, primary_camera_name).stream(Stream::Color).resource(Resource::ImageRaw).build();
        auto qos = rclcpp::SensorDataQoS();

        m_control_subscriber = m_node->create_subscription<sensor_msgs::msg::Image>(
            topic_string,
            qos,
            std::bind(&AgentManager::on_image_received, this, std::placeholders::_1));
    }

    void AgentManager::destroy_publisher_and_subscriber()
    {
        if (m_control_publisher)
        {
            m_control_publisher = nullptr;
        }

        if (m_control_subscriber)
        {
            m_control_subscriber = nullptr;
        }
    }

    void AgentManager::arm()
    {
        if (m_selected_agent_name == "")
        {
            return;
        }

        // create request for arm service
        auto request = std::make_shared<sfg_agent_msgs::srv::TriggerAction::Request>();
        request->action = "arm";

        // request arm service
        auto future = m_trigger_action_service->async_send_request(
            request,
            [this](rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedFuture future)
            {
                future.wait();
                // RCLCPP_INFO(m_node->get_logger(), "Completed arm service request");

                if (future.valid())
                {
                    auto payload = future.get();
                    auto res = payload->result;
                    switch (res)
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
                            RCLCPP_WARN(m_node->get_logger(), "Arming response status unknown. Status Code = %d", res);
                            break;
                        }
                    }
                }
            });
    }

    void AgentManager::disarm()
    {
        if (m_selected_agent_name == "")
        {
            return;
        }

        // create request for disarm service
        auto request = std::make_shared<sfg_agent_msgs::srv::TriggerAction::Request>();
        request->action = "disarm";

        // request disarm service
        auto future = m_trigger_action_service->async_send_request(
            request,
            [this](rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedFuture future)
            {
                future.wait();
                // RCLCPP_INFO(m_node->get_logger(), "Completed disarm service request");

                if (future.valid())
                {
                    auto payload = future.get();
                    auto res = payload->result;
                    switch (res)
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
                            RCLCPP_WARN(m_node->get_logger(), "Disarming response status unknown. Status Code = %d", res);
                            break;
                        }
                    }
                }
            });
    }

    void AgentManager::publish_move_command(double leftX, double leftY, double rightX, double rightY)
    {
        if (!m_is_armed)
        {
            return;
        }

        // linear mapping of stick positions onto target velocities (max stick position eql. to max velocity)

        // normalise stick positions ro range -1.0 to 1.0
        leftX /= max_stick_val_abs;
        leftY /= max_stick_val_abs;
        rightX /= max_stick_val_abs;
        rightY /= max_stick_val_abs;

        // upper and lower clamping
        if (leftX > 1.0)
        {
            leftX = 1.0;
        }
        if (leftX < -1.0)
        {
            leftX = -1.0;
        }
        if (leftY > 1.0)
        {
            leftY = 1.0;
        }
        if (leftY < -1.0)
        {
            leftY = -1.0;
        }
        if (rightX > 1.0)
        {
            rightX = 1.0;
        }
        if (rightX < -1.0)
        {
            rightX = -1.0;
        }
        if (rightY > 1.0)
        {
            rightY = 1.0;
        }
        if (rightY < -1.0)
        {
            rightY = -1.0;
        }

        // center sensitivity deadzone
        if (abs(leftX) < 0.4)
        {
            leftX = 0.0;
        }
        if (abs(leftY) < 0.1)
        {
            leftY = 0.0;
        }
        if (abs(rightX) < 0.2)
        {
            rightX = 0.0;
        }
        if (abs(rightY) < 0.2)
        {
            rightY = 0.0;
        }

        // construct twist message and publish
        geometry_msgs::msg::TwistStamped t_cmd;
        t_cmd.twist.linear.x = -leftY * max_linear_velocity; // moving back and forth
        t_cmd.twist.linear.y = -leftX * max_linear_velocity; // moving sideways
        t_cmd.twist.linear.z = 0.0;                          // moving up and down

        t_cmd.twist.angular.x = 0.0;                            // turning sideways (roll)
        t_cmd.twist.angular.y = rightY * max_angular_velocity;  // turning up and down (pitch)
        t_cmd.twist.angular.z = -rightX * max_angular_velocity; // turning left and right (yaw)
        t_cmd.header.stamp = m_node->now();

        m_control_publisher->publish(t_cmd);
    }

    void AgentManager::fetch_discovery_agents_metadata()
    {
        auto future = m_discovery_service->async_send_request(
            std::make_shared<sfg_agent_msgs::srv::GetDiscoveredAgents::Request>(),
            [this](rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedFuture future)
            {
                future.wait();

                if (future.valid())
                {
                    // clear the list
                    m_agent_metadata.clear();

                    // store the new list/map
                    auto payload = future.get();

                    for (auto entry : payload->metadata)
                    {
                        m_agent_metadata[entry.agent_name] = entry;
                    }
                }
            });
    }

    void AgentManager::on_image_received(const sensor_msgs::msg::Image::ConstSharedPtr &msg)
    {
        if (!msg)
        {
            RCLCPP_WARN(m_node->get_logger(), "Invalid image message!");
        }

        // perform conversion into drawable image data structure
        try
        {
            // Convert the ROS image to an OpenCV Mat in RGBA format (ideal for OpenGL/ImGui)
            cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::RGBA8);

            // Lock and copy the CPU buffer so the render thread can safely access it
            {
                std::lock_guard<std::mutex> lock(m_image_mutex);
                m_pending_image_rgba = cv_ptr->image;
                m_new_image_available = true;
            }
        }
        catch (const cv_bridge::Exception &e)
        {
            RCLCPP_ERROR(m_node->get_logger(), "Image conversion failed! cv_bridge exception: %s", e.what());
        }
    }
}