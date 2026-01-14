#pragma once

#include <iostream>
#include <string>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>

namespace sfg_teleop_interface
{
    class ControlCommandPublisher : public rclcpp::Node
    {
    public:
        explicit ControlCommandPublisher();
        void PublishCommand(double left_stick_x, double left_stick_y, double right_stick_x, double right_stick_y);
        void ChangeRobot(const std::string &robot_name);

    private:
        rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr m_pub_control;
    };
}