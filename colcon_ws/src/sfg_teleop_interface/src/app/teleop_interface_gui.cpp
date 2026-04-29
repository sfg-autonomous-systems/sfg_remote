#include "sfg_teleop_interface/app/teleop_interface_gui.hpp"

#include <iostream>

namespace sfg_teleop_interface::app
{
    TeleopInterfaceGui::TeleopInterfaceGui(rclcpp::Node *node)
    {
    }

    void TeleopInterfaceGui::render_internal()
    {
        auto &io = ImGui::GetIO();
        std::cout << "Test\n";
    }
}