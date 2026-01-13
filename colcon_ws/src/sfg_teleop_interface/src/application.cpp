#include "sfg_teleop_interface/application.hpp"

namespace sfg_teleop_interface
{
    Application::Application(int argc, char **argv)
        : m_ros(argc, argv),
          m_running(false)
    {
    }

    Application::~Application()
    {
    }

    bool Application::Init()
    {
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
            // SDL_Delay(min_frame_time_ms);
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
    }

    void Application::Render()
    {
    }
}
