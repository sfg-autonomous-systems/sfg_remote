#include "sfg_universal_remote_control/universal_remote_control.hpp"

#include <SDL3/SDL.h>

#include "sfg_universal_remote_control/app/universal_remote_control_gui.hpp"

namespace sfg_universal_remote_control
{
    UniversalRemoteControl::UniversalRemoteControl(const rclcpp::NodeOptions &options) : Node("universal_remote_control", options)
    {
        using namespace magic_enum::bitwise_operators;

        m_main_window = std::make_unique<sfg_imgui_vendor::MainWindow>(
            get_name(),
            [this]()
            {
                return std::make_unique<app::UniversalRemoteControlGui>(this);
            },
            sfg_imgui_vendor::MainWindow::Flags::Resizable |
                sfg_imgui_vendor::MainWindow::Flags::Gamepad |
                sfg_imgui_vendor::MainWindow::Flags::VSync |
                sfg_imgui_vendor::MainWindow::Flags::Fullscreen);
    }
}