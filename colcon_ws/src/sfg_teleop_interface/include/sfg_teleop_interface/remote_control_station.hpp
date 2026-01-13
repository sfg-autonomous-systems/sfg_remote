#pragma once

#include <iostream>
#include <memory>

#include <rclcpp/rclcpp.hpp>

namespace sfg_teleop_interface
{
    class RemoteControlStation : public rclcpp::Node
    {
    public:
        RemoteControlStation();

    private:
        void PrintHelloMessage();
    };
}