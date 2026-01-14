#include "sfg_teleop_interface/control_command_publisher.hpp"

namespace sfg_teleop_interface
{
    const double max_linear_velocity = 3.0;  // in m/s
    const double max_angular_velocity = 1.5; // in rad/s
    const double max_stick_val_abs = 32767;

    ControlCommandPublisher::ControlCommandPublisher() : rclcpp::Node("control_command_publisher")
    {
        std::string topic_name = "global/dummy/bla/foo";
        this->m_pub_control = this->create_publisher<geometry_msgs::msg::TwistStamped>(topic_name, 10);
    }

    void ControlCommandPublisher::PublishCommand(double left_stick_x, double left_stick_y, double right_stick_x, double right_stick_y)
    {
        // linear mapping of stick positions onto target velocities (max stick position eql. to max velocity) //

        // normalise stick positions to range -1.0 to 1.0
        left_stick_x /= max_stick_val_abs;
        left_stick_y /= max_stick_val_abs;
        right_stick_x /= max_stick_val_abs;
        right_stick_y /= max_stick_val_abs;

        // upper and lower clamping
        if (left_stick_x > 1.0)
            left_stick_x = 1.0;
        if (left_stick_x < -1.0)
            left_stick_x = -1.0;
        if (left_stick_y > 1.0)
            left_stick_y = 1.0;
        if (left_stick_y < -1.0)
            left_stick_y = -1.0;
        if (right_stick_x > 1.0)
            right_stick_x = 1.0;
        if (right_stick_x < -1.0)
            right_stick_x = -1.0;
        if (right_stick_y > 1.0)
            right_stick_y = 1.0;
        if (right_stick_y < -1.0)
            right_stick_y = -1.0;

        // center sensitivity deadzone
        if (abs(left_stick_x) < 0.4)
            left_stick_x = 0.0;
        if (abs(left_stick_y) < 0.1)
            left_stick_y = 0.0;
        if (abs(right_stick_x) < 0.2)
            right_stick_x = 0.0;
        if (abs(right_stick_y) < 0.2)
            right_stick_y = 0.0;

        // construct twist message and publish
        geometry_msgs::msg::TwistStamped t_cmd;
        t_cmd.twist.linear.x = -left_stick_y * max_linear_velocity; // moving back and forth
        t_cmd.twist.linear.y = left_stick_x * max_linear_velocity;  // moving sideways
        // t_cmd.twist.linear.y = 0.0;
        t_cmd.twist.linear.z = 0.0; // moving up and down

        t_cmd.twist.angular.x = 0.0;                                  // turning sideways (roll)
        t_cmd.twist.angular.y = right_stick_y * max_angular_velocity; // turning up and down (pitch)
        // t_cmd.twist.angular.y = 0.0;
        t_cmd.twist.angular.z = -right_stick_x * max_angular_velocity; // turning left and right (yaw)
        // t_cmd.twist.angular.z = 0.0;
        t_cmd.header.stamp = now();
        this->m_pub_control->publish(t_cmd);
        // std::cout << "Published control command. \n";
    }

    void ControlCommandPublisher::ChangeRobot(const std::string &robot_name)
    {
        // construct full topic name
        std::string topic_name = "/global/" + robot_name + "/locomotion_controller/cmd_vel";

        // recreate publisher for updated topic name
        this->m_pub_control = this->create_publisher<geometry_msgs::msg::TwistStamped>(topic_name, 10);
    }
}