#include "sfg_remote/remote.hpp"

#include <SDL3/SDL.h>

#include "sfg_remote/app/remote_gui.hpp"

namespace sfg_remote
{
    Remote::Remote(const rclcpp::NodeOptions &options) : Node("remote", options)
    {
        using namespace magic_enum::bitwise_operators;

        m_main_window = std::make_unique<sfg_imgui_vendor::MainWindow>(
            get_name(),
            [this]()
            {
                return std::make_unique<app::RemoteGui>(this);
            },
            sfg_imgui_vendor::MainWindow::Flags::Resizable |
                sfg_imgui_vendor::MainWindow::Flags::Gamepad |
                sfg_imgui_vendor::MainWindow::Flags::VSync |
                sfg_imgui_vendor::MainWindow::Flags::Fullscreen);
    }
}