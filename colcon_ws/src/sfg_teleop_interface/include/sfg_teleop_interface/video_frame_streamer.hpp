#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <mutex>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <SDL3/SDL.h>

namespace sfg_teleop_interface
{
    using std::placeholders::_1;

    // helper function
    static bool encoding_to_sdl_format(const std::string &enc, SDL_PixelFormat &out_fmt, int &out_bpp)
    {
        if (enc == "rgb8")
        {
            out_fmt = SDL_PIXELFORMAT_RGB24;
            out_bpp = 3;
            return true;
        }
        if (enc == "bgr8")
        {
            out_fmt = SDL_PIXELFORMAT_BGR24;
            out_bpp = 3;
            return true;
        }
        if (enc == "rgba8")
        {
            out_fmt = SDL_PIXELFORMAT_RGBA32;
            out_bpp = 4;
            return true;
        }
        if (enc == "bgra8")
        {
            out_fmt = SDL_PIXELFORMAT_BGRA32;
            out_bpp = 4;
            return true;
        }
        // add mappings for additional supported pixel formats here ...
        return false;
    }

    class VideoFrameStreamerNode : public rclcpp::Node
    {
    public:
        explicit VideoFrameStreamerNode();
        ~VideoFrameStreamerNode() override;

        void RenderVideoFrame(const int window_width, const int window_height);

        // Getter & Setter
        void ChangeRobotName(const std::string &name);
        void PassRendererHandle(SDL_Renderer *renderer);

    private:
        void OnImageReceived(const sensor_msgs::msg::Image::ConstSharedPtr &msg);
        SDL_FRect AspectFitRect(int win_w, int win_h) const;

    private:
        // Renderer and Texture state
        SDL_Renderer *m_renderer = nullptr;
        mutable std::mutex m_textureMutex;
        SDL_Texture *m_frameTexture = nullptr;
        SDL_PixelFormat m_pixelFormat = SDL_PIXELFORMAT_UNKNOWN;
        int m_textureWidth, m_textureHeight = 0;

        // ROS related
        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_image_sub;
        std::string m_robot_name;
    };
}