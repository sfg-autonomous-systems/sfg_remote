#pragma once

#include <imgui/imgui.h>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>

#include "sfg_imgui_vendor/gui_element.hpp"
#include "sfg_teleop_interface/app/image_feed.hpp"
#include "sfg_teleop_interface/app/robot_manager.hpp"
#include "sfg_teleop_interface/app/utils.hpp"

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

    const int64_t min_cmd_period = 25;

    class TeleopInterfaceGui : public sfg_imgui_vendor::GuiElement
    {
    public:
        TeleopInterfaceGui(rclcpp::Node *node);
        ~TeleopInterfaceGui();

    protected:
        void render_internal() override;

    private:
        void init_gamepad();
        void process_input();

        void render_overlay();
        void render_video_frame();
        void render_selector_dialog();

    private:
        SDL_Gamepad *m_gamepad;
        rclcpp::Node *m_node;
        RobotManager m_manager;
        ImageFeed m_camera_viewer;

        bool m_show_selector_dialog;
    };
}