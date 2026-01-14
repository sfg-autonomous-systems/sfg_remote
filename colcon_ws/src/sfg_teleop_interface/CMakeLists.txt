cmake_minimum_required(VERSION 3.8)
project(sfg_teleop_interface)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ########################################################################################################
# Find dependencies.                                                                                     #
# ########################################################################################################
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sfg_agent_msgs REQUIRED)
find_package(sfg_utils REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

# ########################################################################################################
# Add nodes and libraries.                                                                               #
# ########################################################################################################
# ImGui directories
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_BACKENDS ${IMGUI_DIR}/backends)

add_executable(remote_control_station

    # Imgui sources
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_BACKENDS}/imgui_impl_sdl3.cpp
    ${IMGUI_BACKENDS}/imgui_impl_sdlrenderer3.cpp

    src/control_command_publisher.cpp
    src/telemetry_subscriber.cpp
    src/ros_pump.cpp
    src/application.cpp
    src/main.cpp
)
ament_target_dependencies(remote_control_station ${SDL3_LIBRARIES} rclcpp std_msgs geometry_msgs)
target_include_directories(remote_control_station PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_BACKENDS}
    ${SDL3_INCLUDE_DIRS}
)

install(TARGETS remote_control_station
    DESTINATION lib/${PROJECT_NAME})

# ########################################################################################################
# Add other files/directories.                                                                           #
# ########################################################################################################
install(DIRECTORY
    DESTINATION "share/${PROJECT_NAME}/"
)

if(BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)

    # the following line skips the linter which checks for copyrights
    # comment the line when a copyright and license is added to all source files
    set(ament_cmake_copyright_FOUND TRUE)

    # the following line skips cpplint (only works in a git repo)
    # comment the line when this package is in a git repo and when
    # a copyright and license is added to all source files
    set(ament_cmake_cpplint_FOUND TRUE)
    ament_lint_auto_find_test_dependencies()
endif()

ament_package()
