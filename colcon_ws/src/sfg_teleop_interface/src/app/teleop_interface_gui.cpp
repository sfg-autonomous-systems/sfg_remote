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

    TeleopInterfaceGui::TeleopInterfaceGui(rclcpp::Node *node) : m_node(node), m_manager(node), m_show_selector_dialog(false)
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
        this->process_input();

        auto &io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##main_window", nullptr, s_main_window_flags);

        ImGui::End();

        render_video_frame();
        render_overlay();
        render_selector_dialog();
    }

    void TeleopInterfaceGui::process_input()
    {
        // check for close request
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_EAST))
        {
            SDL_Event close_event;
            SDL_zero(close_event);
            close_event.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&close_event);
        }

        // button state events
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_WEST))
        {
            m_show_selector_dialog = true;
        }

        // manual dis-/arming
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
        {
            m_manager.arm();
        }
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
        {
            m_manager.disarm();
        }

        // sample stick for navigation
        m_manager.publish_move_command(
            SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTX),
            SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTY),
            SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTX),
            SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
    }

    void TeleopInterfaceGui::render_overlay()
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowPos(ImVec2(50.0, 100.0));
        ImGui::SetNextWindowBgAlpha(0.3f);
        if (ImGui::Begin("Robot agent data", nullptr, flags))
        {
            const auto *data = m_manager.get_metadata_for_selected();
            if (data)
            {
                ImGui::Text("Selected robot agent");
                ImGui::Text("Name: %s", data->agent_name.c_str());
                ImGui::Text("Cameras: ");
                for (const auto &cam : data->cameras)
                {
                    ImGui::Text(" %s ", cam.c_str());
                }
                ImGui::Text("Lidars: ");
                for (const auto &lidar : data->lidars)
                {
                    ImGui::Text(" %s ", lidar.c_str());
                }

                if (m_manager.is_armed())
                {
                    ImGui::TextColored(ImVec4(0, 255, 0, 1), "ARMED");
                }
                else
                {
                    ImGui::TextColored(ImVec4(255, 0, 0, 1), "DISARMED");
                }

                // float frametime = ((TeleopClient *)m_teleop_client_node.get())->GetVideoFrametime();
                // ImGui::Text("Video frametime %.2f ms/frame (%.1f fps)", frametime, (1000.0f / frametime));
            }
            else
            {
                ImGui::Text("Press X to see available robots");
            }
            ImGui::End();
        }
    }

    void TeleopInterfaceGui::render_video_frame()
    {
    }

    void TeleopInterfaceGui::render_selector_dialog()
    {
        if (m_show_selector_dialog)
        {
            ImGui::OpenPopup("AgentSelectDialog");
            m_show_selector_dialog = false;
        }

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("AgentSelectDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Select a robot");
            ImGui::Separator();

            for (const auto &[agent_name, metadata] : m_manager.get_all_robot_agent_metadata())
            {
                // skip agents with invalid name
                if (agent_name == "")
                {
                    continue;
                }

                if (ImGui::Button(agent_name.c_str(), ImVec2(250.0f, 0.0f)))
                {
                    m_manager.switch_to_robot(agent_name);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(85, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh", ImVec2(85, 0)))
            {
                // refresh list of discovered robot agents
                m_manager.refresh_discovery_now();
            }

            ImGui::EndPopup();
        }
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

                // Enable gamepad support for ImGUI
                auto &io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

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