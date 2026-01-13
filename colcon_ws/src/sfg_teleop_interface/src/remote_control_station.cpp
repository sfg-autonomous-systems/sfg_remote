#include "sfg_teleop_interface/remote_control_station.hpp"

namespace sfg_teleop_interface
{
    RemoteControlStation::RemoteControlStation() : Node("remote_control_station")
    {
        // print message on construction
        PrintHelloMessage();
    }

    void RemoteControlStation::PrintHelloMessage()
    {
        std::cout << "Hello from remote control client ROS node\n";
    }
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<sfg_teleop_interface::RemoteControlStation>());
    rclcpp::shutdown();
    return 0;
}
