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
        std::cout << "Hello from remote control client ROS node. \n";
    }
}
