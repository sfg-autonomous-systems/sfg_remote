#pragma once

#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <image_transport/image_transport.hpp>
#include <memory>
#include <mutex>
#include <rclcpp/rclcpp.hpp>

#include "sfg_agent_msgs/srv/get_discovered_agents.hpp"
#include "sfg_agent_msgs/srv/trigger_action.hpp"

namespace sfg_teleop_interface::app
{
    using std::placeholders::_1;

    class RobotManager
    {
    public:
        RobotManager(rclcpp::Node *node);
        ~RobotManager();

        void refresh_discovery_now();
        void switch_to_robot(const std::string &name);
        void detach_from_robot();

        // getters
        const std::map<std::string, sfg_agent_msgs::msg::Metadata> &get_all_robot_agent_metadata() const;
        const sfg_agent_msgs::msg::Metadata *get_metadata_for_selected() const;
        bool is_armed() const;
        bool is_selected() const;
        bool is_new_image_available();
        cv::Mat get_latest_image();

        // control related
        void publish_move_command(double leftX, double leftY, double rightX, double rightY);
        void arm();
        void disarm();

    private:
        void create_publisher_and_subscriber();
        void destroy_publisher_and_subscriber();
        void fetch_discovery_agents_metadata();
        void on_image_received(const sensor_msgs::msg::Image::ConstSharedPtr &msg);

    private:
        rclcpp::Node *m_node;

        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_image_sub;
        rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr m_control_pub;
        rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedPtr m_discovery_service;
        rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedPtr m_trigger_action_service;

        std::map<std::string, sfg_agent_msgs::msg::Metadata> m_agent_metadata;
        std::string m_selected_robot_name;
        bool m_is_armed;

        std::mutex m_image_mutex;
        cv::Mat m_pending_image_rgba;
        bool m_new_image_available;
    };
}