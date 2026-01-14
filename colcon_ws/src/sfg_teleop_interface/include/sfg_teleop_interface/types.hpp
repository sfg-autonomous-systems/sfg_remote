#pragma once

namespace sfg_teleop_interface
{
    struct AnalogStickState
    {
        int x_axis_val = 0;
        int y_axis_val = 0;
        bool pressed = false;
    };
}