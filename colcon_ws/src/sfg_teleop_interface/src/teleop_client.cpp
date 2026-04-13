#include "sfg_teleop_interface/teleop_client.hpp"
#include "sfg_utils/fqn/ros_fqn_builder.hpp"

namespace sfg_teleop_interface
{
    // globals
    const double max_linear_velocity = 2.8;  // in m/s
    const double max_angular_velocity = 1.5; // in rad/s
    const double max_stick_val_abs = 32767;

    TeleopClient::TeleopClient() : Node("teleop_client"), m_robot_name(""), m_is_armed(false)
    {
        using namespace sfg_utils::fqn;

        this->m_first_frame_ready = false;

        // create service instances
        m_discovery_service = create_client<sfg_agent_msgs::srv::GetDiscoveredAgents>(RosFqnBuilder().scope(Scope::Local).agent().resource(Resource::Custom, "get_discovered_agents").build());
        this->FetchDiscoveredAgentMetadata();
    }

    TeleopClient::~TeleopClient()
    {
        // Release heap allocated resources
        std::scoped_lock lk(m_texture_mutex);
        if (m_frame_texture)
        {
            SDL_DestroyTexture(m_frame_texture);
            m_frame_texture = nullptr;
        }
    }

    void TeleopClient::RefreshDiscoveryNow()
    {
        this->FetchDiscoveredAgentMetadata();
    }

    void TeleopClient::FetchDiscoveredAgentMetadata()
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

    void TeleopClient::SwitchToRobot(const std::string &name)
    {
        // when name actually changed, recreate subscription with to matching topic
        if (name == m_robot_name)
        {
            return;
        }

        // ...
        auto iterator = this->m_agent_metadata.find(name);
        if (iterator == this->m_agent_metadata.end())
        {
            return;
        }

        const auto &selected_agent = iterator->second;

        // print some metadata for selected robot
        RCLCPP_INFO(get_logger(), "Metadata for robot %s:", selected_agent.agent_name.c_str());
        RCLCPP_INFO(get_logger(), "  primary camera: %s", selected_agent.cameras.at(0).c_str());
        RCLCPP_INFO(get_logger(), "  primary lidar:  %s", selected_agent.lidars.at(0).c_str());

        // disarm before switching to another
        if (this->m_is_armed)
        {
            this->Disarm();
        }

        // now switch over
        m_robot_name = name;
        this->CreatePublisherAndSubscriber();

        // recreate the trigger-action service client and arm the newly selected robot
        using namespace sfg_utils::fqn;
        // --> ros2 service call /global/sfg_go2_01/locomotion_controller/trigger_action sfg_agent_msgs/srv/TriggerAction "{action: 'disarm'}"
        this->m_trigger_action_service = create_client<sfg_agent_msgs::srv::TriggerAction>(RosFqnBuilder().scope(Scope::Global).component(Component::LocomotionController).agent(m_robot_name).resource(Resource::TriggerAction).build());

#ifdef _AUTOMATIC_ARMING
        this->Arm();
#endif
    }

    void TeleopClient::DetachFromRobot()
    {
        // disarm before detach
        if (this->m_is_armed)
        {
            this->Disarm();
        }
        this->DestroyPublisherAndSubscriber();
        this->m_robot_name = "";
        this->m_first_frame_ready = false;
    }

    const std::map<std::string, sfg_agent_msgs::msg::Metadata> &TeleopClient::GetAllRobotAgentMetadata() const
    {
        return this->m_agent_metadata;
    }

    const sfg_agent_msgs::msg::Metadata *TeleopClient::GetMetadataForSelected() const
    {
        if (m_robot_name == "")
        {
            return nullptr;
        }
        return &this->m_agent_metadata.at(this->m_robot_name);
    }

    float TeleopClient::GetVideoFrametime() const
    {
        return this->m_video_frametime;
    }

    bool TeleopClient::IsArmed() const
    {
        return this->m_is_armed;
    }

    void TeleopClient::CreatePublisherAndSubscriber()
    {
        using namespace sfg_utils::fqn;
        std::string topic_string = "";

#ifndef _BYPASS_AGENT_DECODER
        // define topic name and qos
        const std::string primary_camera_name = this->m_agent_metadata.find(m_robot_name)->second.cameras.at(0);
        topic_string = RosFqnBuilder().scope(Scope::Local).agent(m_robot_name).component(Component::Camera, primary_camera_name).stream(Stream::Color).resource(Resource::ImageRaw).build();
        auto qos = rclcpp::SensorDataQoS();

        // create the camera subscriber
        m_image_sub = this->create_subscription<sensor_msgs::msg::Image>(
            topic_string,
            qos,
            std::bind(&TeleopClient::OnImageReceived, this, _1));
#else
        // define topic name and qos
        topic_string = "in";
        auto qos = rmw_qos_profile_sensor_data;

        // create the camera subscriber
        m_image_sub = image_transport::create_subscription(
            this,
            topic_string,
            std::bind(&TeleopClient::OnImageReceived, this, _1),
            "ffmpeg",
            qos);
#endif

        // create command publisher
        topic_string = RosFqnBuilder().scope(Scope::Global).agent(m_robot_name).component(Component::LocomotionController).resource(Resource::CmdVel).build();
        m_control_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(topic_string, 10);

        // create additional subs & pubs below
        // ...
    }

    void TeleopClient::DestroyPublisherAndSubscriber()
    {
        this->m_control_pub = nullptr;

#ifndef _BYPASS_AGENT_DECODER
        this->m_image_sub = nullptr;
#else
        this->m_image_sub.shutdown();
#endif
    }

    void TeleopClient::PublishMoveCommand(double left_stick_x, double left_stick_y, double right_stick_x, double right_stick_y)
    {
        // skip if robot is disarmed
        if (!this->m_is_armed)
        {
            return;
        }

        // linear mapping of stick positions onto target velocities (max stick position eql. to max velocity) //

        // normalise stick positions to range -1.0 to 1.0
        left_stick_x /= max_stick_val_abs;
        left_stick_y /= max_stick_val_abs;
        right_stick_x /= max_stick_val_abs;
        right_stick_y /= max_stick_val_abs;

        // upper and lower clamping
        if (left_stick_x > 1.0)
            left_stick_x = 1.0;
        if (left_stick_x < -1.0)
            left_stick_x = -1.0;
        if (left_stick_y > 1.0)
            left_stick_y = 1.0;
        if (left_stick_y < -1.0)
            left_stick_y = -1.0;
        if (right_stick_x > 1.0)
            right_stick_x = 1.0;
        if (right_stick_x < -1.0)
            right_stick_x = -1.0;
        if (right_stick_y > 1.0)
            right_stick_y = 1.0;
        if (right_stick_y < -1.0)
            right_stick_y = -1.0;

        // center sensitivity deadzone
        if (abs(left_stick_x) < 0.4)
            left_stick_x = 0.0;
        if (abs(left_stick_y) < 0.1)
            left_stick_y = 0.0;
        if (abs(right_stick_x) < 0.2)
            right_stick_x = 0.0;
        if (abs(right_stick_y) < 0.2)
            right_stick_y = 0.0;

        // construct twist message and publish
        geometry_msgs::msg::TwistStamped t_cmd;
        t_cmd.twist.linear.x = -left_stick_y * max_linear_velocity; // moving back and forth
        t_cmd.twist.linear.y = -left_stick_x * max_linear_velocity; // moving sideways
        // t_cmd.twist.linear.y = 0.0;
        t_cmd.twist.linear.z = 0.0; // moving up and down

        t_cmd.twist.angular.x = 0.0;                                  // turning sideways (roll)
        t_cmd.twist.angular.y = right_stick_y * max_angular_velocity; // turning up and down (pitch)
        // t_cmd.twist.angular.y = 0.0;
        t_cmd.twist.angular.z = -right_stick_x * max_angular_velocity; // turning left and right (yaw)
        // t_cmd.twist.angular.z = 0.0;
        t_cmd.header.stamp = now();
        this->m_control_pub->publish(t_cmd);
    }

    void TeleopClient::Arm()
    {
        if (m_robot_name == "")
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

                if (future.valid())
                {
                    auto payload = future.get();
                    auto res = payload->result;
                    switch (res)
                    {
                        case sfg_agent_msgs::srv::TriggerAction_Response::FAILURE:
                        {
                            RCLCPP_ERROR(get_logger(), "Arming failed!");
                            this->m_is_armed = false;
                            break;
                        }

                        case sfg_agent_msgs::srv::TriggerAction_Response::SUCCESS:
                        {
                            RCLCPP_INFO(get_logger(), "Arming successful.");
                            this->m_is_armed = true;
                            break;
                        }

                        case sfg_agent_msgs::srv::TriggerAction_Response::UNSUPPORTED:
                        {
                            RCLCPP_INFO(get_logger(), "Arming not implemented (ignored).");
                            this->m_is_armed = true;
                            break;
                        }

                        default:
                        {
                            RCLCPP_WARN(get_logger(), "Arming response status unknown. Status Code = %d", res);
                            break;
                        }
                    }
                }
            });
    }

    void TeleopClient::Disarm()
    {
        if (m_robot_name == "")
        {
            return;
        }

        // create request for arm service
        auto request = std::make_shared<sfg_agent_msgs::srv::TriggerAction::Request>();
        request->action = "disarm";

        // request arm service
        auto future = m_trigger_action_service->async_send_request(
            request,
            [this](rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedFuture future)
            {
                future.wait();
                // RCLCPP_INFO(this->get_logger(), "Completed disarm service request");

                if (future.valid())
                {
                    auto payload = future.get();
                    auto res = payload->result;
                    switch (res)
                    {
                        case sfg_agent_msgs::srv::TriggerAction_Response::FAILURE:
                        {
                            RCLCPP_ERROR(get_logger(), "Disarming failed!");
                            break;
                        }

                        case sfg_agent_msgs::srv::TriggerAction_Response::SUCCESS:
                        {
                            RCLCPP_INFO(get_logger(), "Disarming successful.");
                            this->m_is_armed = false;
                            break;
                        }

                        case sfg_agent_msgs::srv::TriggerAction_Response::UNSUPPORTED:
                        {
                            RCLCPP_INFO(get_logger(), "Disarming not implemented (ignored).");
                            this->m_is_armed = true;
                            break;
                        }

                        default:
                        {
                            RCLCPP_WARN(get_logger(), "Disarming response status unknown. Status Code = %d", res);
                            break;
                        }
                    }
                }
            });
    }

    void TeleopClient::RenderVideoFrame(const int window_width, const int window_height)
    {
        // don't render when detached
        if (this->m_robot_name == "" || !m_first_frame_ready)
        {
            return;
        }

        if (m_frame_texture && m_renderer)
        {
            // Get rect for correct positioning and scaling with respect to the window dimension
            SDL_FRect destination = AspectFitRect(window_width, window_height);
            SDL_RenderTexture(m_renderer, m_frame_texture, nullptr, &destination);
        }
    }

    void TeleopClient::PassRendererHandle(SDL_Renderer *renderer)
    {
        m_renderer = renderer;
    }

    void TeleopClient::OnImageReceived(const sensor_msgs::msg::Image::ConstSharedPtr &msg)
    {
        if (!msg)
        {
            RCLCPP_WARN(this->get_logger(), "Invalid image message!");
            return;
        }

        // detect image pixel format and resolution
        SDL_PixelFormat fmt{};
        int bytes_per_pixel = 0;

        if (!ENCODING_TO_SDL_FORMAT(msg->encoding, fmt, bytes_per_pixel))
        {
            RCLCPP_WARN(this->get_logger(), "Unsupported pixel format detected: %s", msg->encoding.c_str());
            return;
        }
        const int image_width = static_cast<int>(msg->width);
        const int image_height = static_cast<int>(msg->height);

        std::scoped_lock lk(m_texture_mutex);
        if (!m_frame_texture || image_width != m_texture_width || image_height != m_texture_height || fmt != m_pixel_format)
        {
            // destroy and recreate matching texture
            if (m_frame_texture)
            {
                SDL_DestroyTexture(m_frame_texture);
                m_frame_texture = nullptr;
            }

            m_frame_texture = SDL_CreateTexture(m_renderer, fmt, SDL_TEXTUREACCESS_STREAMING, image_width, image_height);
            if (!m_frame_texture)
            {
                RCLCPP_ERROR(this->get_logger(), "SDL_CreateTexture failed: %s", SDL_GetError());
                return;
            }

            // store updated texture dimensions
            m_texture_width = image_width;
            m_texture_height = image_height;
            m_pixel_format = fmt;

            RCLCPP_INFO(this->get_logger(), "Recreated video texture: %dx%d %s", image_width, image_height, msg->encoding.c_str());
        }

        // Upload image into texture for subsequent rendering
        if (!SDL_UpdateTexture(m_frame_texture, nullptr, msg->data.data(), static_cast<int>(msg->step)) != 0)
        {
            RCLCPP_ERROR(this->get_logger(), "SDL_UpdateTexture failed: %s", SDL_GetError());
            return;
        }

        m_first_frame_ready = true;

        // continously measure frametime of camera video stream and apply moving average filter
        constexpr size_t WindowSize = 60; // average over last 60 frames

        static std::array<double, WindowSize> samples{};
        static size_t index = 0;
        static size_t count = 0;
        static double sum = 0.0;
        static auto last_frame_received = std::chrono::steady_clock::now();

        auto current_frame_received = std::chrono::steady_clock::now();
        double dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(current_frame_received - last_frame_received)).count();
        last_frame_received = current_frame_received;

        // Remove oldest value from sum
        sum -= samples[index];

        // Insert new value
        samples[index] = dt;
        sum += dt;

        // Move circular index
        index = (index + 1) % WindowSize;

        if (count < WindowSize)
            ++count;

        this->m_video_frametime = static_cast<float>(sum / count);
    }

    SDL_FRect TeleopClient::AspectFitRect(int win_w, int win_h) const
    {
        SDL_FRect dst{0.f, 0.f, 0.f, 0.f};
        if (win_w <= 0 || win_h <= 0 || m_texture_width <= 0 || m_texture_height <= 0)
            return dst;

        const float win_aspect = static_cast<float>(win_w) / static_cast<float>(win_h);
        const float img_aspect = static_cast<float>(m_texture_width) / static_cast<float>(m_texture_height);

        if (win_aspect > img_aspect)
        {
            // Window is wider than image -> fit by height
            dst.h = static_cast<float>(win_h);
            dst.w = dst.h * img_aspect;
            dst.x = (static_cast<float>(win_w) - dst.w) * 0.5f;
            dst.y = 0.f;
        }
        else
        {
            // Window is taller than image -> fit by width
            dst.w = static_cast<float>(win_w);
            dst.h = dst.w / img_aspect;
            dst.x = 0.f;
            dst.y = (static_cast<float>(win_h) - dst.h) * 0.5f;
        }

        return dst;
    }
}