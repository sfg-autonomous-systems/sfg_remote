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

        void refresh_discovered_agents();
        void switch_to_agent(const std::string &name);
        void detach_from_agent();

        // Getters.
        const std::map<std::string, sfg_agent_msgs::msg::Metadata> &get_all_agents() const;
        const sfg_agent_msgs::msg::Metadata *get_metadata_for_selected() const;
        bool is_armed() const;
        bool has_selected() const;
        bool is_new_image_available();
        cv::Mat get_image();

        // Control related.
        void publish_cmd_vel(double left_x, double left_y, double right_x, double right_y);
        void arm();
        void disarm();

    private:
        void create_publisher_and_subscriber();
        void destroy_publisher_and_subscriber();
        void get_discovered_agents();
        void image_callback(const sensor_msgs::msg::Image::ConstSharedPtr &msg);

        rclcpp::Node *m_node;

        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr m_image_subscriber;
        rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr m_cmd_vel_publisher;
        rclcpp::Client<sfg_agent_msgs::srv::GetDiscoveredAgents>::SharedPtr m_get_discovered_agents_client;
        rclcpp::Client<sfg_agent_msgs::srv::TriggerAction>::SharedPtr m_trigger_action_client;

        std::map<std::string, sfg_agent_msgs::msg::Metadata> m_agents;
        std::string m_selected_agent_name = "";
        bool m_is_armed = false;

        std::mutex m_image_mutex;
        cv::Mat m_image;
        bool m_new_image_available = false;
    };
}