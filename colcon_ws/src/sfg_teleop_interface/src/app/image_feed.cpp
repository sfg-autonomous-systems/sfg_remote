#include "sfg_teleop_interface/app/image_feed.hpp"

namespace sfg_teleop_interface::app
{
    ImageFeed::ImageFeed(rclcpp::Node *node) : m_node(node)
    {
        this->init_texture();
    }

    ImageFeed::~ImageFeed()
    {
        if (m_node)
        {
            m_node = nullptr;
        }
    }

    void ImageFeed::init_texture()
    {
        // preallocate 2D texture
        glGenTextures(1, &m_texture_id);
    }

    void ImageFeed::update_image_data(const cv::Mat &image)
    {
        // retrieve actual size of updated image
        m_image_width = image.cols;
        m_image_height = image.rows;

        glBindTexture(GL_TEXTURE_2D, m_texture_id);

        // Set basic texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // upload image data into texture buffer on the GPU
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     m_image_width,
                     m_image_height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE, image.data);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void ImageFeed::render_internal()
    {
        ImGui::Begin("Camera View");

        if (m_texture_id != 0)
        {
            // Cast OpenGL texture ID (GLuint) to ImTextureID
            // ImTextureID img_id = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(m_texture_id));

            // Display the image
            // ImGui::Image(img_id, ImVec2(static_cast<float>(m_image_width), static_cast<float>(m_image_height)));
        }
        else
        {
            // draw placeholder instead
            ImGui::Text("No stream received yet.");
        }

        ImGui::End();
    }
}