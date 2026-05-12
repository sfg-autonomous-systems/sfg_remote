#pragma once

#include <imgui/imgui.h>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_joystick.h>

#include "sfg_agent_msgs/srv/get_discovered_agents.hpp"
#include "sfg_agent_msgs/srv/trigger_action.hpp"
#include "sfg_imgui_vendor/gui_element.hpp"
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <image_transport/image_transport.hpp>

namespace sfg_teleop_interface::app
{
    struct JoystickIdDel
    {
        void operator()(SDL_JoystickID *p) const noexcept
        {
            if (p)
                SDL_free(p);
        }
    };

    using JoystickIdPtr = std::unique_ptr<SDL_JoystickID, JoystickIdDel>;

    class TeleopInterfaceGui : public sfg_imgui_vendor::GuiElement
    {
    public:
        TeleopInterfaceGui(rclcpp::Node *node);
        ~TeleopInterfaceGui();

    protected:
        void render_internal() override;

    private:
        void init_gamepad();
        void create_publisher_and_subscriber();
        void destroy_publisher_and_subscriber();
        void render_overlay();
        void render_video_frame();
        void publish_move_command();

    private:
        SDL_Gamepad *m_gamepad;
        rclcpp::Node *m_node;

        image_transport::Subscriber m_image_sub;
        rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr m_control_pub;
        rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedPtr m_discovery_service;
        rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedPtr m_trigger_action_service;

        std::map<std::string, sfg_agent_msgs::msg::Metadata> m_agent_metadata;
    };
}