#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "sfg_agent_msgs/srv/get_discovered_agents.hpp"
#include "sfg_agent_msgs/srv/trigger_action.hpp"
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <image_transport/image_transport.hpp>
#include <rclcpp/rclcpp.hpp>
#include <SDL3/SDL.h>
#include <sensor_msgs/msg/image.hpp>

// #define _BYPASS_AGENT_DECODER
// #define _AUTOMATIC_ARMING

namespace sfg_teleop_interface
{
    using std::placeholders::_1;

    // helper function
    static bool ENCODING_TO_SDL_FORMAT(const std::string &enc, SDL_PixelFormat &out_fmt, int &out_bpp)
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

    class TeleopClient : public rclcpp::Node
    {
    public:
        explicit TeleopClient();
        ~TeleopClient() override;

        void RefreshDiscoveryNow();
        void SwitchToRobot(const std::string &name);

        // Getters
        const std::map<std::string, sfg_agent_msgs::msg::Metadata> &GetAllRobotAgentMetadata() const;
        const sfg_agent_msgs::msg::Metadata *GetMetadataForSelected() const;
        float GetVideoFrametime() const;
        bool IsArmed() const;

        // Control related
        void PublishMoveCommand(double left_stick_x, double left_stick_y, double right_stick_x, double right_stick_y);
        void Arm();
        void Disarm();

        // Video and telemetry related
        void RenderVideoFrame(const int window_width, const int window_height);
        void PassRendererHandle(SDL_Renderer *renderer);

    private:
        void CreatePublisherAndSubscriber();
        void FetchDiscoveredAgentMetadata();
        void OnImageReceived(const sensor_msgs::msg::Image::ConstSharedPtr &msg);
        SDL_FRect AspectFitRect(int win_w, int win_h) const;

    private:
        SDL_Renderer *m_renderer = nullptr;
        SDL_Texture *m_frame_texture = nullptr;
        SDL_PixelFormat m_pixel_format = SDL_PIXELFORMAT_UNKNOWN;
        mutable std::mutex m_texture_mutex;
        int m_texture_width, m_texture_height = 0;
        float m_video_frametime = 0.0;

        // ROS related
#ifndef _BYPASS_AGENT_DECODER
        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_image_sub;
#else
        image_transport::Subscriber m_image_sub;
#endif

        rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr m_control_pub;
        rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedPtr m_discovery_service;
        rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedPtr m_trigger_action_service;

        std::map<std::string, sfg_agent_msgs::msg::Metadata> m_agent_metadata;
        std::string m_robot_name;
        bool m_is_armed;
    };
}