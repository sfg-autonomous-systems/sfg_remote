#pragma once

#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <image_transport/image_transport.hpp>
#include <memory>
#include <mutex>
#include <rclcpp/rclcpp.hpp>

#include "sfg_agent_msgs/srv/get_discovered_agents.hpp"
#include "sfg_agent_msgs/srv/trigger_action.hpp"

namespace sfg_universal_remote_control::app
{
    class AgentManager
    {
    public:
        AgentManager(rclcpp::Node *node);
        AgentManager(const AgentManager &) = delete;
        AgentManager &operator=(const AgentManager &) = delete;
        AgentManager(AgentManager &&) = delete;
        AgentManager &operator=(AgentManager &&) = delete;
        ~AgentManager();

        void refresh_discovery_now();
        void switch_to_agent(const std::string &name);
        void detach_from_agent();

        // getters
        const std::map<std::string, sfg_agent_msgs::msg::Metadata> &get_all_agent_metadata() const;
        const sfg_agent_msgs::msg::Metadata *get_metadata_for_selected() const;
        bool is_armed() const;
        bool is_selected() const;
        bool is_new_image_available();
        cv::Mat get_latest_image();

        // control related
        void publish_move_command(double left_x, double left_y, double right_x, double right_y);
        void arm();
        void disarm();

    private:
        void create_publisher_and_subscriber();
        void destroy_publisher_and_subscriber();
        void fetch_discovery_agents_metadata();
        void on_image_received(const sensor_msgs::msg::Image::ConstSharedPtr &msg);

        rclcpp::Node *m_node;

        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_control_subscriber;
        rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr m_control_publisher;
        rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedPtr m_discovery_service;
        rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedPtr m_trigger_action_service;

        std::map<std::string, sfg_agent_msgs::msg::Metadata> m_agent_metadata;
        std::string m_selected_agent_name;
        bool m_is_armed;

        std::mutex m_image_mutex;
        cv::Mat m_pending_image_rgba;
        bool m_new_image_available;
    };
}