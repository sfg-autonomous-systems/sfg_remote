#include "sfg_teleop_interface/teleop_interface.hpp"

#include <SDL3/SDL.h>

#include "sfg_teleop_interface/app/teleop_interface_gui.hpp"

namespace sfg_teleop_interface
{
    TeleopInterface::TeleopInterface(const rclcpp::NodeOptions &options) : Node("teleop_interface", options)
    {
        m_main_window = std::make_unique<sfg_imgui_vendor::MainWindow>(
            get_name(),
            [this]()
            {
                return std::make_unique<sfg_teleop_interface::app::TeleopInterfaceGui>(this);
            },
            sfg_imgui_vendor::MainWindow::Flags::Resizable);
    }
}