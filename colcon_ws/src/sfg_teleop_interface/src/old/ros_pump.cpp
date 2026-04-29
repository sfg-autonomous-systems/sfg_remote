#include "sfg_teleop_interface/ros_pump.hpp"

namespace sfg_teleop_interface
{
    RosPump::RosPump(int argc, char **argv)
    {
        rclcpp::init(argc, argv);
        this->m_exec = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
        std::cout << "ROS2 pump initialized. \n";
    }

    RosPump::~RosPump()
    {
        this->Stop();
        if (rclcpp::ok())
        {
            rclcpp::shutdown();
        }
    }

    void RosPump::Add_Node(const rclcpp::Node::SharedPtr &node)
    {
        this->m_exec->add_node(node);
    }

    void RosPump::Start()
    {
        this->m_running = true;
        std::cout << "ROS2 pump started spinning. \n";
    }

    void RosPump::Stop()
    {
        this->m_running = false;
        std::cout << "ROS2 pump stopped spinning. \n";
    }

    void RosPump::Poll(std::chrono::microseconds max_duration)
    {
        if (!m_running || !rclcpp::ok())
            return;

        const auto start = std::chrono::steady_clock::now();
        while (true)
        {
            this->m_exec->spin_some();

            const auto now = std::chrono::steady_clock::now();
            if (now - start >= max_duration)
                break;
        }
    }

    bool RosPump::Ok() const
    {
        return rclcpp::ok();
    }
}