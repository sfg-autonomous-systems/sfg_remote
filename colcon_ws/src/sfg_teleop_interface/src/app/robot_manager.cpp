#include "sfg_teleop_interface/app/robot_manager.hpp"

namespace sfg_teleop_interface::app
{
    RobotManager::RobotManager(rclcpp::Node *node) : m_node(node)
    {
    }

    RobotManager::~RobotManager()
    {
        if (m_node)
        {
            m_node = nullptr;
        }
    }

    void RobotManager::create_publisher_and_subscriber()
    {
    }

    void RobotManager::destroy_publisher_and_subscriber()
    {
    }

    void RobotManager::publish_move_command()
    {
    }
}