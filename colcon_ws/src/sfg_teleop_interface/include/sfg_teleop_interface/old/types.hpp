#pragma once

namespace sfg_teleop_interface
{
    struct AnalogStickState
    {
        int x_axis_val = 0;
        int y_axis_val = 0;
        bool pressed = false;
    };

    struct AppState
    {
        bool start_stream_marker = false;
        bool running = true;
        bool show_robot_selector_dialog = false;
    };
}