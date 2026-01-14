#pragma once

#include <iostream>
#include <memory>
#include <functional>
#include <chrono>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

namespace sfg_teleop_interface
{
    class TelemetrySubscriber : public rclcpp::Node
    {
    public:
        TelemetrySubscriber();

    private:
        void Callback();

        rclcpp::Subscription<std_msgs::msg::String>::SharedPtr m_sub_telemetry;
    };
}