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
        using namespace sfg_utils::fqn;

        init_gamepad();

        // create discovery service instance
        m_discovery_service = node->create_client<sfg_agent_msgs::srv::GetDiscoveredAgents>(RosFqnBuilder().scope(Scope::Local).agent().resource(Resource::Custom, "get_discovered_agents").build());

        // create pubs and subs
        this->create_publisher_and_subscriber();
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
                    // std::cerr << "Failed to open gamepad " << jid << ": " << SDL_GetError() << "\n";
                    RCLCPP_WARN(this->m_node->get_logger(), "Could not open gamepad %u: %s", jid, SDL_GetError());
                    continue;
                }

                const char *name = SDL_GetGamepadName(m_gamepad);
                SDL_GamepadType type = SDL_GetGamepadType(m_gamepad);

                // std::cout << "Opened gamepad ID " << jid << " | Name: " << (name ? name : "Unknown") << " | Type: " << type << "\n";
                RCLCPP_INFO(this->m_node->get_logger(), "Opened gamepad ID %u | Name: %s", jid, (name ? name : "Unknown"));
            }
            else
            {
                return;
            }
        }

        RCLCPP_ERROR(this->m_node->get_logger(), "No Gamepad was found!");
        // throw runtime error ...
    }

    void TeleopInterfaceGui::create_publisher_and_subscriber()
    {
    }

    void TeleopInterfaceGui::destroy_publisher_and_subscriber()
    {
    }

    void TeleopInterfaceGui::publish_move_command()
    {
    }
}