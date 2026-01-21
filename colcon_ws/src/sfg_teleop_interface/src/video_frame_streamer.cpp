#include "sfg_teleop_interface/video_frame_streamer.hpp"

namespace sfg_teleop_interface
{
    VideoFrameStreamerNode::VideoFrameStreamerNode()
        : Node("video_frame_streamer_node"),
          m_robot_name("fwgfwa")
    {
        // create subscription for given robot name
        auto qos = rclcpp::SensorDataQoS();
        m_image_sub = this->create_subscription<sensor_msgs::msg::Image>(
            "/local/" + m_robot_name + "/camera_head/color/image_raw",
            qos,
            std::bind(&VideoFrameStreamerNode::OnImageReceived, this, _1));
    }

    VideoFrameStreamerNode::~VideoFrameStreamerNode()
    {
        std::scoped_lock lk(m_textureMutex);
        if (m_frameTexture)
        {
            SDL_DestroyTexture(m_frameTexture);
            m_frameTexture = nullptr;
        }
    }

    void VideoFrameStreamerNode::RenderVideoFrame(const SDL_FRect &destination)
    {
        if (m_frameTexture && m_renderer)
        {
            SDL_RenderTexture(m_renderer, m_frameTexture, nullptr, &destination);
        }
    }

    void VideoFrameStreamerNode::PassRendererHandle(SDL_Renderer *renderer)
    {
        m_renderer = renderer;
    }

    void VideoFrameStreamerNode::ChangeRobotName(const std::string &name)
    {
        // when name actually changed, recreate subscription with to matching topic
        if (name != m_robot_name)
        {
            auto qos = rclcpp::SensorDataQoS();
            m_image_sub = this->create_subscription<sensor_msgs::msg::Image>(
                "/local/" + name + "/camera_head/color/image_raw",
                qos,
                std::bind(&VideoFrameStreamerNode::OnImageReceived, this, _1));

            m_robot_name = name;
            RCLCPP_INFO(this->get_logger(), "Changed Robot name for live video to %s", name.c_str());
        }
    }

    // subscription callback
    void VideoFrameStreamerNode::OnImageReceived(const sensor_msgs::msg::Image::ConstSharedPtr &msg)
    {
        if (!msg)
        {
            RCLCPP_WARN(this->get_logger(), "Invalid image message!");
            return;
        }

        // detect image pixel format and resolution
        SDL_PixelFormat fmt{};
        int bytes_per_pixel = 0;

        if (!encoding_to_sdl_format(msg->encoding, fmt, bytes_per_pixel))
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
}