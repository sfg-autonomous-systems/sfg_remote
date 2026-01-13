#pragma once

#include <chrono>
#include <memory>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_joystick.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "sfg_teleop_interface/ros_pump.hpp"

// compile options
// #define _USE_FULLSCREEN
// #define _USE_ADAPTIVE_SYNC

namespace sfg_teleop_interface
{
    const int window_width = 1280;
    const int window_height = 720;
    const uint32_t min_frame_time_ms = 2;

    struct JoystickIdDel
    {
        void operator()(SDL_JoystickID *p) const noexcept
        {
            if (p)
                SDL_free(p);
        }
    };

    using JoystickIdPtr = std::unique_ptr<SDL_JoystickID, JoystickIdDel>;

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
        void Draw();
        void Render();

    private:
        SDL_Window *m_window;
        SDL_DisplayID m_displayId = 0;
        SDL_Renderer *m_renderer;
        SDL_Gamepad *m_gamepad;

        bool m_running = false;
        RosPump m_ros;
    };
}
