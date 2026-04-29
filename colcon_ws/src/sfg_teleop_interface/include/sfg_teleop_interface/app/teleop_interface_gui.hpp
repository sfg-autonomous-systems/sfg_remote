#pragma once

#include <imgui/imgui.h>
#include <rclcpp/rclcpp.hpp>
#include <SDL3/SDL.h>

#include "sfg_imgui_vendor/gui_element.hpp"

namespace sfg_teleop_interface::app
{
    class TeleopInterfaceGui : public sfg_imgui_vendor::GuiElement
    {
    public:
        TeleopInterfaceGui(rclcpp::Node *node);

    protected:
        void render_internal() override;

    private:
    };
}