#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <mutex>
#include <memory>
#include <vector>
#include <map>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include "sfg_agent_msgs/srv/get_discovered_agents.hpp"
#include "sfg_agent_msgs/srv/trigger_action.hpp"
#include <SDL3/SDL.h>

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
        const std::map<std::string, sfg_agent_msgs::msg::Metadata> &GetAllRobotAgentMetadata() const;

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
        SDL_Texture *m_frameTexture = nullptr;
        SDL_PixelFormat m_pixelFormat = SDL_PIXELFORMAT_UNKNOWN;
        mutable std::mutex m_textureMutex;
        int m_textureWidth, m_textureHeight = 0;

        // ROS related
        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_image_sub;
        rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr m_control_pub;
        rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedPtr m_discovery_service;
        rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedPtr m_trigger_action_service;

        std::map<std::string, sfg_agent_msgs::msg::Metadata> m_agent_metadata;
        std::string m_robot_name;
        bool m_isArmed;
    };
}