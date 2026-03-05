#include "sfg_teleop_interface/video_frame_streamer.hpp"

namespace sfg_teleop_interface
{
    VideoFrameStreamerNode::VideoFrameStreamerNode()
        : Node("video_frame_streamer_node"),
          m_robot_name("sfg_go2_01")
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

    void VideoFrameStreamerNode::RenderVideoFrame(const int window_width, const int window_height)
    {
        if (m_frameTexture && m_renderer)
        {
            // Get rect for correct positioning and scaling with respect to the window dimension
            SDL_FRect destination = AspectFitRect(window_width, window_height);
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

    SDL_FRect VideoFrameStreamerNode::AspectFitRect(int win_w, int win_h) const
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