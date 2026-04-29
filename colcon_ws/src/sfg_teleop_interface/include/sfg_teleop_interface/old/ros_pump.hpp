#pragma once

#include <iostream>
#include <memory>
#include <chrono>
#include <rclcpp/rclcpp.hpp>

namespace sfg_teleop_interface
{
    class RosPump
    {
    public:
        RosPump(int argc, char **argv);
        ~RosPump();

        void Add_Node(const rclcpp::Node::SharedPtr &node);

        void Start();
        void Stop();
        void Poll(std::chrono::microseconds max_duration);
        bool Ok() const;

    private:
        std::shared_ptr<rclcpp::Executor> m_exec;
        bool m_running = false;
    };
}