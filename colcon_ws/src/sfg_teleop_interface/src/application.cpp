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
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();

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
        // set descriptive metadata about application
        if (!SDL_SetAppMetadata("", "", ""))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
        }

        // ouput compiled and linked version numbers of SDL3
        const int compiled = SDL_VERSION;
        const int linked = SDL_GetVersion();
        SDL_Log("Detected platform: %s \n", SDL_GetPlatform());
        SDL_Log("Compiled with SDL version %d.%d.%d \n", SDL_VERSIONNUM_MAJOR(compiled), SDL_VERSIONNUM_MINOR(compiled), SDL_VERSIONNUM_MICRO(compiled));
        SDL_Log("Linked against SDL version %d.%d.%d \n", SDL_VERSIONNUM_MAJOR(linked), SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked));

        // init SDL3 API
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            return false;
        }

        // get id of primary display and auto detect native monitor resolution and refresh rate
        this->m_displayId = SDL_GetPrimaryDisplay();
        if (this->m_displayId == 0)
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            return false;
        }

#ifdef _USE_FULLSCREEN

        const SDL_DisplayMode *fullscreenMode = this->GetFullscreenMode();
        if (!fullscreenMode)
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            return false;
        }
        else
        {
            SDL_Log("Fullscreen mode detected: %dx%d @ %d Hz", fullscreenMode->w, fullscreenMode->h, static_cast<int>(fullscreenMode->refresh_rate));
        }

        // then create window and renderer
        SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_KEYBOARD_GRABBED;
        if (!SDL_CreateWindowAndRenderer("Robot Control Center Prototype",
                                         fullscreenMode->w, fullscreenMode->h,
                                         window_flags,
                                         &this->m_window,
                                         &this->m_renderer))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            return false;
        }

        // configure mode and set to fullscreen
        if (!SDL_SetWindowFullscreenMode(this->m_window, fullscreenMode))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            return false;
        }

        if (!SDL_SetWindowFullscreen(this->m_window, true))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            return false;
        }

#else
        // create window and renderer
        SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_KEYBOARD_GRABBED;
        if (!SDL_CreateWindowAndRenderer("Teleoperation Center Prototype",
                                         window_width, window_height,
                                         window_flags,
                                         &this->m_window,
                                         &this->m_renderer))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            return false;
        }
#endif

        int sync_mode = SDL_RENDERER_VSYNC_DISABLED;
#ifdef _USE_ADAPTIVE_SYNC
        sync_mode = SDL_RENDERER_VSYNC_ADAPTIVE;
#endif

        // configure sync between monitor and renderer (either no sync or adaptive sync technology)
        if (!SDL_SetRenderVSync(this->m_renderer, sync_mode))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            std::cerr << "Failed to configure monitor sync mode!\n";
        }

        if (this->m_renderer == nullptr)
        {
            std::cerr << "Renderer is nullptr! Abort.\n";
            return false;
        }

        // configure blend mode to be used when rendering
        if (!SDL_SetRenderDrawBlendMode(this->m_renderer, SDL_BLENDMODE_BLEND))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            std::cerr << "Failed to configure renderer blend mode!\n";
        }

        // move the window to center position on the screen
        if (!SDL_SetWindowPosition(this->m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED))
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsDark();

        // obtain scaling factor from primary monitor
        float main_scale = SDL_GetDisplayContentScale(this->m_displayId);
        if (main_scale == 0.0f)
        {
            SDL_Log("SDL3 Error: %s", SDL_GetError());
            std::cerr << "invalid monitor scaling factor!\n";
            return false;
        }

        // setup scaling
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;

        // setup renderer backend
        ImGui_ImplSDL3_InitForSDLRenderer(this->m_window, this->m_renderer);
        ImGui_ImplSDLRenderer3_Init(this->m_renderer);

        SDL_Log("Video Driver: %s \n", SDL_GetCurrentVideoDriver());
        SDL_HideWindow(this->m_window);

        return true;
    }

    bool Application::InitGamepad()
    {
        // List all detected joysticks
        int numJoysticks = 0;
        JoystickIdPtr joystickIdList(SDL_GetJoysticks(&numJoysticks));

        std::cout << "Found " << numJoysticks << " joystick(s) \n";

        // Open each joystick that can be interpreted as a Gamepad
        for (int i = 0; i < numJoysticks; i++)
        {
            SDL_JoystickID jid = joystickIdList.get()[i];
            if (SDL_IsGamepad(jid))
            {
                this->m_gamepad = SDL_OpenGamepad(jid);
                if (!this->m_gamepad)
                {
                    std::cerr << "Failed to open gamepad " << jid
                              << ": " << SDL_GetError() << "\n";
                    continue;
                }

                const char *name = SDL_GetGamepadName(this->m_gamepad);
                SDL_GamepadType type = SDL_GetGamepadType(this->m_gamepad);

                std::cout << "Opened gamepad ID " << jid
                          << " | Name: " << (name ? name : "Unknown")
                          << " | Type: " << type << "\n";

                return true;
            }
            else
            {
                std::cout << "Joystick ID " << jid << " is not recognized as a gamepad \n";
            }
        }

        return false;
    }

    void Application::Update()
    {
        // poll for input event
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            // pass through events to ImGUI
            ImGui_ImplSDL3_ProcessEvent(&event);

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
        // ImGuiIO &io = ImGui::GetIO();
        // this->m_perf_stats.average_fps = io.Framerate;
        // this->m_perf_stats.average_frametime = 1000.0f / io.Framerate;
    }

    void Application::Draw()
    {
        // ImGuiIO &io = ImGui::GetIO();

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowBgAlpha(0.3f);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        ImGui::Begin("Simple perf monitor", NULL, flags);

        // render overlay text in the corner
        ImGui::Text("Hello World!");
        // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", this->m_perf_stats.average_frametime, this->m_perf_stats.average_fps);
        // ImGui::Text("Video fps: %.1f", this->m_video_decoder.GetVideoFPS());

        // render Gui elements code goes here
        // ...

        ImGui::End();
        ImGui::Render();
    }

    void Application::Render()
    {
        this->Draw();

        // clear the render target (backbuffer)
        ImGuiIO &io = ImGui::GetIO();
        SDL_SetRenderScale(this->m_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(this->m_renderer, 255.0f, 255.0f, 255.0f, 1.0f);
        SDL_RenderClear(this->m_renderer);

        // render directly with SDL
        int width, height = 0;
        SDL_GetWindowSize(this->m_window, &width, &height);

        // render GUI elements on top of the scene
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), this->m_renderer);

        // present rendered frame
        SDL_RenderPresent(this->m_renderer);
    }
}
