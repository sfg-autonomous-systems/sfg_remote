#pragma once

#include <cv_bridge/cv_bridge.h>
#include <GL/gl.h>
#include <imgui/imgui.h>
#include <memory>
#include <rclcpp/rclcpp.hpp>

#include "sfg_imgui_vendor/gui_element.hpp"

namespace sfg_universal_remote_control::app
{
    class ImageFeed : public sfg_imgui_vendor::GuiElement
    {
    public:
        ImageFeed(rclcpp::Node *node);
        ImageFeed(const ImageFeed &) = delete;
        ImageFeed &operator=(const ImageFeed &) = delete;
        ImageFeed(ImageFeed &&) = delete;
        ImageFeed &operator=(ImageFeed &&) = delete;
        ~ImageFeed();

        void update_image_data(const cv::Mat &image);

    protected:
        void render_internal() override;

    private:
        rclcpp::Node *m_node;

        GLuint m_texture_id = 0;
        std::int32_t m_buffer_width = 0;
        std::int32_t m_buffer_height = 0;
    };
}