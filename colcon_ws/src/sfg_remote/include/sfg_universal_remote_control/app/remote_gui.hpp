#pragma once

#include <imgui/imgui.h>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>

#include "sfg_imgui_vendor/gui_element.hpp"
#include "sfg_remote/app/agent_manager.hpp"
#include "sfg_remote/app/image_feed.hpp"

namespace sfg_remote::app
{

    class RemoteGui : public sfg_imgui_vendor::GuiElement
    {
    public:
        RemoteGui(rclcpp::Node *node);
        RemoteGui(const RemoteGui &) = delete;
        RemoteGui &operator=(const RemoteGui &) = delete;
        RemoteGui(RemoteGui &&) = delete;
        RemoteGui &operator=(RemoteGui &&) = delete;
        ~RemoteGui();

    protected:
        void render_internal() override;

    private:
        struct JoystickIdDeleter
        {
            void operator()(SDL_JoystickID *ptr) const noexcept
            {
                if (ptr)
                {
                    SDL_free(ptr);
                }
            }
        };

        using JoystickIdPtr = std::unique_ptr<SDL_JoystickID, JoystickIdDeleter>;

        void initialize_gamepad();
        void process_input();

        void render_overlay();
        void render_video_frame();
        void render_selector_dialog();

        SDL_Gamepad *m_gamepad;
        rclcpp::Node *m_node;
        AgentManager m_manager;
        ImageFeed m_image_feed;

        bool m_show_selector_dialog = false;
        rclcpp::Time m_last_cmd_time;
    };
}