#include "sfg_teleop_interface/app/teleop_interface_gui.hpp"

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

    TeleopInterfaceGui::TeleopInterfaceGui(rclcpp::Node *node)
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
    }

    void TeleopInterfaceGui::render_internal()
    {
        auto &io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##main_window", nullptr, s_main_window_flags);

        // ...

        ImGui::End();
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
                    std::cerr << "Failed to open gamepad " << jid << ": " << SDL_GetError() << "\n";
                    continue;
                }

                const char *name = SDL_GetGamepadName(m_gamepad);
                SDL_GamepadType type = SDL_GetGamepadType(m_gamepad);
                std::cout << "Opened gamepad ID " << jid << " | Name: " << (name ? name : "Unknown") << " | Type: " << type << "\n";
            }
            else
            {
                return;
            }
        }

        // runtime error
        std::cerr << "No Gamepad was found!" << "\n";
    }
}