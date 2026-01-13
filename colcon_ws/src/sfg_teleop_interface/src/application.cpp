#include "sfg_teleop_interface/application.hpp"

namespace sfg_teleop_interface
{
    Application::Application(int argc, char **argv)
        : m_ros(argc, argv)
    {
        this->m_window = nullptr;
        this->m_renderer = nullptr;
        this->m_gamepad = nullptr;
    }

    Application::~Application()
    {
        if (this->m_gamepad != nullptr)
        {
            SDL_CloseGamepad(this->m_gamepad);
            this->m_gamepad = nullptr;
        }

        if (this->m_renderer != nullptr)
        {
            /*
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            */

            SDL_DestroyRenderer(this->m_renderer);
            this->m_renderer = nullptr;
        }

        if (this->m_window != nullptr)
        {
            SDL_DestroyWindow(this->m_window);
            this->m_window = nullptr;
        }

        SDL_Quit();
    }

    bool Application::Init()
    {
        bool result = this->InitGui();
        if (!result)
        {
            return false;
        }

        result = this->InitGamepad();
        if (!result)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", "No Gamepad device available", this->m_window);
            return false;
        }

        // init additional stuff here
        // ...

        // show application window
        SDL_ShowWindow(this->m_window);

        return true;
    }

    void Application::Run()
    {
        this->m_running = true;

        while (this->m_running)
        {
            this->Update();

            this->m_ros.Poll(std::chrono::microseconds(500));

            this->Render();

// actively cap application speed if no sync is used
#ifndef _USE_ADAPTIVE_SYNC
            SDL_Delay(min_frame_time_ms);
#endif
        }
    }

    bool Application::InitGui()
    {
        return true;
    }

    bool Application::InitGamepad()
    {
        return true;
    }

    void Application::Update()
    {
        // poll for input event
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            // pass through events to ImGUI
            // ImGui_ImplSDL3_ProcessEvent(&event);

            // process based on the event type
            if (event.type == SDL_EVENT_QUIT ||
                event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                event.type == SDL_EVENT_GAMEPAD_REMOVED)
            {
                this->m_running = false;
            }
        }

        // Do state based input processing
        if (SDL_GetGamepadButton(this->m_gamepad, SDL_GAMEPAD_BUTTON_EAST))
        {
            m_running = false;
        }

        // update data for 2D GUI overlay elements here
        // ...
    }

    void Application::Render()
    {
    }
}
