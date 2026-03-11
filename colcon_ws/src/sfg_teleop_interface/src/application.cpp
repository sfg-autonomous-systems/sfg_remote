#include "sfg_teleop_interface/application.hpp"

namespace sfg_teleop_interface
{
    Application::Application(int argc, char **argv)
        : m_ros(argc, argv)
    {
        this->m_window = nullptr;
        this->m_renderer = nullptr;
        this->m_gamepad = nullptr;

        // create and register teleop node
        this->m_teleop_client_node = std::make_shared<TeleopClient>();
        this->m_ros.Add_Node(m_teleop_client_node);
        ((TeleopClient *)m_teleop_client_node.get())->SwitchToRobot("");
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

        // start spinning ROS nodes
        this->m_ros.Start();

        return true;
    }

    void Application::Run()
    {
        static int64_t last_cmd_send = 0;

        this->m_appState.running = true;

        while (this->m_appState.running)
        {
            this->Update();

            auto current_time = now_ms();
            if (current_time - last_cmd_send > min_cmd_period)
            {
                ((TeleopClient *)m_teleop_client_node.get())->PublishMoveCommand(m_left_stick.x_axis_val, m_left_stick.y_axis_val, m_right_stick.x_axis_val, m_right_stick.y_axis_val);
                last_cmd_send = current_time;
            }

            // peform ROS2 related I/O
            this->m_ros.Poll(microseconds(500));

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
        if (!SDL_SetAppMetadata("SFG Teleoperation Center", "0.0.0", ""))
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
        if (!SDL_CreateWindowAndRenderer("Remote Control Station",
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
        if (!SDL_CreateWindowAndRenderer("Remote Control Station",
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
        ((TeleopClient *)m_teleop_client_node.get())->PassRendererHandle(this->m_renderer);

        SDL_Log("Video Driver: %s \n", SDL_GetCurrentVideoDriver());
        SDL_HideWindow(this->m_window);

        // don't create a imgui.ini files every launch
        io.IniFilename = nullptr;

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
                this->m_appState.running = false;
            }
        }

        // Do state based input processing
        if (SDL_GetGamepadButton(this->m_gamepad, SDL_GAMEPAD_BUTTON_EAST))
        {
            ((TeleopClient *)m_teleop_client_node.get())->Disarm();
            this->m_appState.running = false;
        }

        if (SDL_GetGamepadButton(this->m_gamepad, SDL_GAMEPAD_BUTTON_WEST))
        {
            this->m_appState.show_robot_selector_dialog = true;
        }

        // manual dis/arming
        if (SDL_GetGamepadButton(this->m_gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
        {
            ((TeleopClient *)m_teleop_client_node.get())->Arm();
        }

        if (SDL_GetGamepadButton(this->m_gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
        {
            ((TeleopClient *)m_teleop_client_node.get())->Disarm();
        }

        // sample stick positions
        this->m_left_stick.x_axis_val = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        this->m_left_stick.y_axis_val = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        this->m_right_stick.x_axis_val = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        this->m_right_stick.y_axis_val = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTY);

        // update data for 2D GUI overlay elements here
        ImGuiIO &io = ImGui::GetIO();
        this->m_framerate = io.Framerate;
        // this->m_perf_stats.average_frametime = 1000.0f / io.Framerate;
    }

    void Application::Draw()
    {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowBgAlpha(0.3f);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        ImGui::Begin("Simple perf monitor", NULL, flags);

        // render overlay text in the corner
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", (1000.0f / this->m_framerate), this->m_framerate);
        ImGui::Text("Left  Stick:  X = %d | Y = %d", this->m_left_stick.x_axis_val, this->m_left_stick.y_axis_val);
        ImGui::Text("Right Stick:  X = %d | Y = %d", this->m_right_stick.x_axis_val, this->m_right_stick.y_axis_val);
        // ImGui::Text("Video fps: %.1f", this->m_video_decoder.GetVideoFPS());

        this->DrawSelectorDialog();

        // render Gui elements code goes here
        // ...

        ImGui::End();
        ImGui::Render();
    }

    void Application::DrawSelectorDialog()
    {
        if (this->m_appState.show_robot_selector_dialog)
        {
            ImGui::OpenPopup("AgentSelectDialog");
            m_appState.show_robot_selector_dialog = false;
        }

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("AgentSelectDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Select a robot");
            ImGui::Separator();

            for (const auto &[agent_name, metadata] : ((TeleopClient *)m_teleop_client_node.get())->GetAllRobotAgentMetadata())
            {
                if (ImGui::Button(agent_name.c_str(), ImVec2(250.0f, 0.0f)))
                {
                    ((TeleopClient *)m_teleop_client_node.get())->SwitchToRobot(agent_name);
                    ImGui::CloseCurrentPopup();
                }
            }

            if (ImGui::Button("Dummy", ImVec2(250, 0)))
            {
                ((TeleopClient *)m_teleop_client_node.get())->SwitchToRobot("Dummy");
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
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

        // render video frame texture
        // SDL_FRect dest{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)};
        ((TeleopClient *)m_teleop_client_node.get())->RenderVideoFrame(width, height);

        // render GUI elements on top of the scene
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), this->m_renderer);

        // present rendered frame
        SDL_RenderPresent(this->m_renderer);
    }

    const SDL_DisplayMode *Application::GetFullscreenMode() const
    {
        // Get all fullscreen modes for the display
        int count = 0;
        SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(this->m_displayId, &count);
        if (!modes || count == 0)
        {
            SDL_Log("No fullscreen modes found, using desktop mode.");
            return SDL_GetDesktopDisplayMode(this->m_displayId);
        }

        // Find the maximum resolution (by pixel area), then within those pick max refresh rate.
        // Many panels list multiple timings for the same resolution; we pick the highest Hz.
        const SDL_DisplayMode *best = modes[0];
        auto area = [](const SDL_DisplayMode *m)
        { return 1LL * m->w * m->h; };

        // First determine the maximum pixel area available
        long long max_area = 0;
        for (int i = 0; i < count; ++i)
        {
            max_area = std::max(max_area, area(modes[i]));
        }

        // Among modes having that area, pick the highest refresh rate; break ties by pixel format preference if you want.
        int best_hz = 0;
        for (int i = 0; i < count; ++i)
        {
            if (area(modes[i]) == max_area)
            {
                // int hz = (int)SDL_GetDisplayModeRefreshRate(modes[i]); // returns Hz as int
                int hz = static_cast<int>(modes[i]->refresh_rate);
                if (hz > best_hz)
                {
                    best_hz = hz;
                    best = modes[i];
                }
            }
        }

        return best; // NOTE: pointer is owned by SDL; don't free it.
    }

}
