#include "sfg_teleop_interface/app/teleop_interface_gui.hpp"
#include "sfg_utils/fqn/ros_fqn_builder.hpp"

#include <iostream>

namespace sfg_teleop_interface::app
{
    static constexpr auto s_main_window_flags = ImGuiWindowFlags_NoResize |
                                                ImGuiWindowFlags_NoBackground |
                                                ImGuiWindowFlags_NoSavedSettings |
                                                ImGuiWindowFlags_NoScrollbar |
                                                ImGuiWindowFlags_NoScrollWithMouse |
                                                ImGuiWindowFlags_NoCollapse |
                                                ImGuiWindowFlags_NoDecoration;

    TeleopInterfaceGui::TeleopInterfaceGui(rclcpp::Node *node) : m_node(node)
    {
        init_gamepad();
    }

    TeleopInterfaceGui::~TeleopInterfaceGui()
    {
        if (m_gamepad)
        {
            SDL_CloseGamepad(m_gamepad);
            m_gamepad = nullptr;
        }

        if (m_node)
        {
            m_node = nullptr;
        }
    }

    void TeleopInterfaceGui::render_internal()
    {
        // check for close request
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_EAST))
        {
            SDL_Event close_event;
            SDL_zero(close_event);
            close_event.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&close_event);
        }

        auto &io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##main_window", nullptr, s_main_window_flags);

        render_video_frame();
        render_overlay();
        // ...

        ImGui::End();
    }

    void TeleopInterfaceGui::render_overlay()
    {
    }

    void TeleopInterfaceGui::render_video_frame()
    {
    }

    void TeleopInterfaceGui::init_gamepad()
    {
        int numJoysticks = 0;
        JoystickIdPtr joystickIdList(SDL_GetJoysticks(&numJoysticks));

        for (int i = 0; i < numJoysticks; i++)
        {
            SDL_JoystickID jid = joystickIdList.get()[i];
            if (SDL_IsGamepad(jid))
            {
                m_gamepad = SDL_OpenGamepad(jid);
                if (!m_gamepad)
                {
                    RCLCPP_WARN(this->m_node->get_logger(), "Could not open gamepad %u: %s", jid, SDL_GetError());
                    continue;
                }

                const char *name = SDL_GetGamepadName(m_gamepad);
                SDL_GamepadType type = SDL_GetGamepadType(m_gamepad);

                // std::cout << "Opened gamepad ID " << jid << " | Name: " << (name ? name : "Unknown") << " | Type: " << type << "\n";
                RCLCPP_INFO(this->m_node->get_logger(), "Opened gamepad ID %u | Name: %s", jid, (name ? name : "Unknown"));

                return;
            }
            else
            {
                RCLCPP_WARN(this->m_node->get_logger(), "joystick id %u is not recognized as a gamepad", jid);
            }
        }

        RCLCPP_ERROR(this->m_node->get_logger(), "No Gamepad was found!");
        throw std::runtime_error("Failed to initialize. No Gamepad!");
    }
}