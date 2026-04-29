/*
    file: main.cpp
    written by Elias Geiger
*/

#include "sfg_teleop_interface/application.hpp"

int main(int argc, char **argv)
{
    sfg_teleop_interface::Application app(argc, argv);

    bool result = app.Init();
    if (!result)
    {
        std::cout << "Initialization failed! Exit now. \n";
        return EXIT_FAILURE;
    }

    std::cout << "Initialization complete. Start main loop ... \n";
    app.Run();

    std::cout << "Exit now \n";
    return EXIT_SUCCESS;
}