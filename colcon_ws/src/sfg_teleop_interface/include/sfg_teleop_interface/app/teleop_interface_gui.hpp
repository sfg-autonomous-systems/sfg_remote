#pragma once

#include <imgui/imgui.h>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>

#include "sfg_imgui_vendor/gui_element.hpp"

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

        void render_overlay();
        void render_video_frame();

    private:
        SDL_Gamepad *m_gamepad;
        rclcpp::Node *m_node;
    };
}