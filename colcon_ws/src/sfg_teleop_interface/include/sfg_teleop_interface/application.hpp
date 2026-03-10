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

#include "sfg_teleop_interface/types.hpp"
#include "sfg_teleop_interface/ros_pump.hpp"
#include "sfg_teleop_interface/teleop_client.hpp"

// compile options
#define _USE_FULLSCREEN
// #define _USE_ADAPTIVE_SYNC

namespace sfg_teleop_interface
{
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    const int window_width = 1920;
    const int window_height = 1080;

    const uint32_t min_frame_time_ms = 15;
    const int64_t min_cmd_period = 25;

    const std::string robot_name = "sfg_go2_01";
    // const std::string robot_name = "sfg_tb4_02";

    static std::int64_t now_ms()
    {
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

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
        const SDL_DisplayMode *GetFullscreenMode() const;

        void Update();
        void Draw();
        void DrawSelectorDialog();
        void Render();

    private:
        SDL_Window *m_window;
        SDL_DisplayID m_displayId = 0;
        SDL_Renderer *m_renderer;
        SDL_Gamepad *m_gamepad;

        AnalogStickState m_left_stick = {};
        AnalogStickState m_right_stick = {};

        AppState m_appState;

        float m_framerate = 0.0f;

        RosPump m_ros;
        TeleopClient::SharedPtr m_teleop_client_node;
    };
}
