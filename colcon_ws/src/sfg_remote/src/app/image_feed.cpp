#include "sfg_remote/app/image_feed.hpp"

namespace sfg_remote::app
{
    ImageFeed::ImageFeed(rclcpp::Node *node) : m_node(node)
    {
    }

    ImageFeed::~ImageFeed()
    {
        if (m_texture_id != 0)
        {
            glDeleteTextures(1, &m_texture_id);
        }
    }

    void ImageFeed::update_image_data(const cv::Mat &image)
    {
        // Retrieve actual size of updated image.
        auto image_width = image.cols;
        auto image_height = image.rows;

        if (m_texture_id == 0)
        {
            glGenTextures(1, &m_texture_id);
        }

        glBindTexture(GL_TEXTURE_2D, m_texture_id);

        // Only recreate texture buffer when image dimension changes.
        if (image_width != m_buffer_width || image_height != m_buffer_height)
        {
            // Set basic texture parameters.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Upload image data into texture buffer on the GPU.
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                image_width,
                image_height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE, image.data);

            // Remember dimension of allocated buffer.
            m_buffer_width = image_width;
            m_buffer_height = image_height;

            RCLCPP_INFO(m_node->get_logger(), "Reallocated texture buffer (%d x %d).", image_width, image_height);
        }
        else
        {
            // Just overwrite the existing GPU buffer (much faster).
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_width, image_height, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void ImageFeed::render_internal()
    {
        if (m_texture_id == 0)
        {
            return;
        }

        auto &io = ImGui::GetIO();
        auto texture_id = static_cast<ImTextureID>(m_texture_id);

        // Display the image in the background.
        ImGui::GetBackgroundDrawList()->AddImage(
            texture_id,
            ImVec2(0.0f, 0.0f),
            io.DisplaySize);
    }
}