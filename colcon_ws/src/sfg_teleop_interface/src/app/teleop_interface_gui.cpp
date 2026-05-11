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
}