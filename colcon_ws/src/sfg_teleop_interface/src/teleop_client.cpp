#include "sfg_teleop_interface/teleop_client.hpp"
#include "sfg_utils/fqn/ros_fqn_builder.hpp"

namespace sfg_teleop_interface
{
    // globals
    const double max_linear_velocity = 2.8;  // in m/s
    const double max_angular_velocity = 1.5; // in rad/s
    const double max_stick_val_abs = 32767;

    TeleopClient::TeleopClient() : Node("teleop_client"), m_robot_name("robot_01"), m_isArmed(false)
    {
        using namespace sfg_utils::fqn;

        // create service instances
        m_discovery_service = create_client<sfg_agent_msgs::srv::GetDiscoveredAgents>(RosFqnBuilder().scope(Scope::Local).agent().resource(Resource::Custom, "get_discovered_agents").build());
        this->DiscoverAgents();

        // create subscriptions and publishers for given robot name
        this->CreatePublisherAndSubscriber();
        RCLCPP_INFO(this->get_logger(), "Initialized publisher & subscriber for robot named %s", m_robot_name.c_str());
    }

    TeleopClient::~TeleopClient()
    {
        std::scoped_lock lk(m_textureMutex);
        if (m_frameTexture)
        {
            SDL_DestroyTexture(m_frameTexture);
            m_frameTexture = nullptr;
        }
    }

    void TeleopClient::DiscoverAgents()
    {
        auto future = m_discovery_service->async_send_request(
            std::make_shared<sfg_agent_msgs::srv::GetDiscoveredAgents::Request>(),
            [this](rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedFuture future)
            {
                future.wait();

                if (future.valid())
                {
                    auto payload = future.get();

                    this->m_discovered_robot_names.clear();
                    for (const auto &metadata : payload->metadata)
                    {
                        this->m_discovered_robot_names.push_back(metadata.agent_name);
                        RCLCPP_INFO(get_logger(), "Name: %s", metadata.agent_name.c_str());
                    }
                }
            });
    }

    void TeleopClient::SwitchToRobot(const std::string &name)
    {
        // when name actually changed, recreate subscription with to matching topic
        if (name != m_robot_name)
        {
            m_robot_name = name;
            this->CreatePublisherAndSubscriber();
            RCLCPP_INFO(this->get_logger(), "Changed Robot name for live video to %s", name.c_str());
        }
    }

    std::vector<std::string> TeleopClient::GetRobotAgentList() const
    {
        return this->m_discovered_robot_names;
    }

    void TeleopClient::CreatePublisherAndSubscriber()
    {
        using namespace sfg_utils::fqn;
        std::string topic_string = "";

        // create the camera subscriber
        // topic_string = "/local/" + m_robot_name + "/camera_head/color/image_raw";
        topic_string = RosFqnBuilder().scope(Scope::Local).agent(m_robot_name).component(Component::Camera, "head").stream(Stream::Color).resource(Resource::ImageRaw).build();
        auto qos = rclcpp::SensorDataQoS();
        m_image_sub = this->create_subscription<sensor_msgs::msg::Image>(
            topic_string,
            qos,
            std::bind(&TeleopClient::OnImageReceived, this, _1));

        // create command publisher
        // topic_string = "/global/" + m_robot_name + "/locomotion_controller/cmd_vel";
        topic_string = RosFqnBuilder().scope(Scope::Global).agent(m_robot_name).component(Component::LocomotionController).resource(Resource::CmdVel).build();
        m_control_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(topic_string, 10);

        // create additional subs & pubs below
        // ...
    }

    void TeleopClient::PublishMoveCommand(double left_stick_x, double left_stick_y, double right_stick_x, double right_stick_y)
    {
        // skip if robot is disarmed
        if (!this->m_isArmed)
            return;

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
        // request arm service
        // ...

        this->m_isArmed = true;
    }

    void TeleopClient::Disarm()
    {
        // request disarm service
        // ...

        this->m_isArmed = false;
    }

    void TeleopClient::RenderVideoFrame(const int window_width, const int window_height)
    {
        if (m_frameTexture && m_renderer)
        {
            // Get rect for correct positioning and scaling with respect to the window dimension
            SDL_FRect destination = AspectFitRect(window_width, window_height);
            SDL_RenderTexture(m_renderer, m_frameTexture, nullptr, &destination);
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

        std::scoped_lock lk(m_textureMutex);
        if (!m_frameTexture || image_width != m_textureWidth || image_height != m_textureHeight || fmt != m_pixelFormat)
        {
            // destroy and recreate matching texture
            if (m_frameTexture)
            {
                SDL_DestroyTexture(m_frameTexture);
                m_frameTexture = nullptr;
            }

            m_frameTexture = SDL_CreateTexture(m_renderer, fmt, SDL_TEXTUREACCESS_STREAMING, image_width, image_height);
            if (!m_frameTexture)
            {
                RCLCPP_ERROR(this->get_logger(), "SDL_CreateTexture failed: %s", SDL_GetError());
                return;
            }

            // store updated texture dimensions
            m_textureWidth = image_width;
            m_textureHeight = image_height;
            m_pixelFormat = fmt;

            RCLCPP_INFO(this->get_logger(), "Recreated video texture: %dx%d %s", image_width, image_height, msg->encoding.c_str());
        }

        // Upload image into texture for subsequent rendering
        if (!SDL_UpdateTexture(m_frameTexture, nullptr, msg->data.data(), static_cast<int>(msg->step)) != 0)
        {
            RCLCPP_ERROR(this->get_logger(), "SDL_UpdateTexture failed: %s", SDL_GetError());
            return;
        }
    }

    SDL_FRect TeleopClient::AspectFitRect(int win_w, int win_h) const
    {
        SDL_FRect dst{0.f, 0.f, 0.f, 0.f};
        if (win_w <= 0 || win_h <= 0 || m_textureWidth <= 0 || m_textureHeight <= 0)
            return dst;

        const float win_aspect = static_cast<float>(win_w) / static_cast<float>(win_h);
        const float img_aspect = static_cast<float>(m_textureWidth) / static_cast<float>(m_textureHeight);

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