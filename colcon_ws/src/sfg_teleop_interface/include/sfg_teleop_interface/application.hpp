#pragma once

#include <chrono>
#include <memory>
#include <iostream>

#include "sfg_teleop_interface/ros_pump.hpp"

// compile options
// #define _USE_FULLSCREEN
// #define _USE_ADAPTIVE_SYNC

namespace sfg_teleop_interface
{
    const int window_width = 1280;
    const int window_height = 720;
    const uint32_t min_frame_time_ms = 2;

    class Application
    {
    public:
        Application(int argc, char **argv);
        virtual ~Application();
        Application(const Application &other) = delete;
        Application &operator=(const Application &) = delete;

        bool Init();
        void Run();

    private:
        bool InitGui();
        bool InitGamepad();

        void Update();
        void Render();

    private:
        RosPump m_ros;
        bool m_running;
    };
}
