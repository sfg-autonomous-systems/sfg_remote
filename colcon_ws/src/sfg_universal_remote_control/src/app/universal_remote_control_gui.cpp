#include "sfg_universal_remote_control/app/universal_remote_control_gui.hpp"

#include "sfg_utils/fqn/ros_fqn_builder.hpp"

namespace sfg_universal_remote_control::app
{
    UniversalRemoteControlGui::UniversalRemoteControlGui(rclcpp::Node *node)
        : m_node(node),
          m_manager(node),
          m_image_feed(node),
          m_last_cmd_time(0, 0, m_node->get_clock()->get_clock_type())
    {
        initialize_gamepad();
    }

    UniversalRemoteControlGui::~UniversalRemoteControlGui()
    {
        if (m_gamepad)
        {
            SDL_CloseGamepad(m_gamepad);
            m_gamepad = nullptr;
        }
    }

    void UniversalRemoteControlGui::render_internal()
    {
        constexpr auto main_window_flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoDecoration;

        process_input();
        auto &io = ImGui::GetIO();
        render_video_frame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##main_window", nullptr, main_window_flags);
        ImGui::End();

        render_overlay();
        render_selector_dialog();
    }

    void UniversalRemoteControlGui::process_input()
    {
        // Check for close request.
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_EAST))
        {
            SDL_Event close_event;
            SDL_zero(close_event);
            close_event.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&close_event);
            rclcpp::shutdown();
        }

        // Button state events.
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_WEST))
        {
            m_show_selector_dialog = true;
        }

        // Manual arming.
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
        {
            m_manager.arm();
        }

        // Manual disarming.
        if (SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
        {
            m_manager.disarm();
        }

        // Sample stick for navigation.
        constexpr auto min_cmd_period = std::chrono::milliseconds(25);
        auto current_cmd_time = m_node->now();

        if ((current_cmd_time - m_last_cmd_time) > min_cmd_period)
        {
            m_manager.publish_cmd_vel(
                SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTX),
                SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTY),
                SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTX),
                SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
            m_last_cmd_time = current_cmd_time;
        }
    }

    void UniversalRemoteControlGui::render_overlay()
    {
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowPos(ImVec2(50.0, 100.0));
        ImGui::SetNextWindowBgAlpha(0.3f);

        if (ImGui::Begin("Agent data", nullptr, flags))
        {
            const auto data = m_manager.get_metadata_for_selected();

            if (data)
            {
                ImGui::Text("Selected agent");
                ImGui::Text("Name: %s", data->agent_name.c_str());
                ImGui::Text("Cameras: ");

                for (const auto &camera : data->cameras)
                {
                    ImGui::Text(" %s ", camera.c_str());
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
            }
            else
            {
                ImGui::Text("Press X to see available agents");
            }
            ImGui::End();
        }
    }

    void UniversalRemoteControlGui::render_video_frame()
    {
        // Update camera image feed if necessary.
        if (m_manager.is_new_image_available())
        {
            cv::Mat latest_image = m_manager.get_image();
            m_image_feed.update_image_data(latest_image);
        }

        m_image_feed.render();
    }

    void UniversalRemoteControlGui::render_selector_dialog()
    {
        if (m_show_selector_dialog)
        {
            ImGui::OpenPopup("AgentSelectDialog");
            m_show_selector_dialog = false;
        }

        auto viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("AgentSelectDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Select an agent");
            ImGui::Separator();

            for (auto [agent_name, metadata] : m_manager.get_all_agents())
            {
                // Skip agents with invalid name.
                if (agent_name.empty())
                {
                    continue;
                }

                if (ImGui::Button(agent_name.c_str(), ImVec2(250.0f, 0.0f)))
                {
                    m_manager.switch_to_agent(agent_name);
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
                // Refresh list of discovered agents.
                m_manager.refresh_discovered_agents();
            }
            ImGui::EndPopup();
        }
    }

    void UniversalRemoteControlGui::initialize_gamepad()
    {
        auto joystick_count = 0;
        JoystickIdPtr joystick_id_list(SDL_GetJoysticks(&joystick_count));

        for (auto index = 0; index < joystick_count; index++)
        {
            SDL_JoystickID joystick_id = joystick_id_list.get()[index];

            if (SDL_IsGamepad(joystick_id))
            {
                m_gamepad = SDL_OpenGamepad(joystick_id);

                if (!m_gamepad)
                {
                    RCLCPP_WARN(m_node->get_logger(), "Could not open gamepad %u: %s", joystick_id, SDL_GetError());
                    continue;
                }

                auto name = SDL_GetGamepadName(m_gamepad);
                RCLCPP_INFO(m_node->get_logger(), "Opened gamepad ID %u | Name: %s", joystick_id, (name ? name : "Unknown"));

                // Enable gamepad support for ImGUI.
                auto &io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

                return;
            }
            else
            {
                RCLCPP_WARN(m_node->get_logger(), "joystick id %u is not recognized as a gamepad", joystick_id);
            }
        }

        RCLCPP_ERROR(m_node->get_logger(), "No Gamepad was found!");
        throw std::runtime_error("Failed to initialize. No Gamepad!");
    }
}