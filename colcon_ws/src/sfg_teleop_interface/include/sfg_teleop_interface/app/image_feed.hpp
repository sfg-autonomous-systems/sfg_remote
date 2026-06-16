#pragma once

#include "sfg_imgui_vendor/gui_element.hpp"
#include <cv_bridge/cv_bridge.h>
#include <GL/gl.h>
#include <imgui/imgui.h>
#include <memory>
#include <rclcpp/rclcpp.hpp>

namespace sfg_teleop_interface::app
{
    class ImageFeed : public sfg_imgui_vendor::GuiElement
    {
    public:
        ImageFeed(rclcpp::Node *node);
        ~ImageFeed();

        void update_image_data(const cv::Mat &image);

    protected:
        void render_internal() override;

    private:
        void init_texture();

    private:
        rclcpp::Node *m_node;

        GLuint m_texture_id = 0;
        int m_image_width = 0;
        int m_image_height = 0;
    };
}