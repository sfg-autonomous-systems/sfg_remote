#pragma once

#include <rclcpp/rclcpp.hpp>

#include "sfg_imgui_vendor/main_window.hpp"

namespace sfg_universal_remote_control
{
    class UniversalRemoteControl : public rclcpp::Node
    {
    public:
        UniversalRemoteControl(const rclcpp::NodeOptions &options);

    private:
        std::unique_ptr<sfg_imgui_vendor::MainWindow> m_main_window;
    };
}