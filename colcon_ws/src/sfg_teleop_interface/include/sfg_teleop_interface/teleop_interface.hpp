#pragma once

#include <rclcpp/rclcpp.hpp>

#include "sfg_imgui_vendor/main_window.hpp"

namespace sfg_teleop_interface
{
    class TeleopInterface : public rclcpp::Node
    {
    public:
        TeleopInterface(const rclcpp::NodeOptions &options);

    private:
        std::unique_ptr<sfg_imgui_vendor::MainWindow> m_main_window;
    };
}