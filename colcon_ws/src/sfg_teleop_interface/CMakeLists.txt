cmake_minimum_required(VERSION 3.8)
project(sfg_teleop_interface)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

cmake_policy(SET CMP0072 NEW)
cmake_policy(SET CMP0079 NEW)

set(EXECUTABLE_NAME "remote_control_station")

# ########################################################################################################
# Find dependencies.                                                                                     #
# ########################################################################################################
find_package(PkgConfig REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sfg_agent_msgs REQUIRED)
find_package(sfg_utils REQUIRED)
find_package(sfg_imgui_vendor REQUIRED)
find_package(image_transport REQUIRED)

# ########################################################################################################
# Add nodes and libraries.                                                                               #
# ########################################################################################################
set(SOURCES

    # src/teleop_client.cpp
    # src/ros_pump.cpp
    # src/application.cpp
    # src/main.cpp
)

sfg_utils__add_component(${EXECUTABLE_NAME} "${PROJECT_NAME}::TeleopInterface"
    SOURCES
    "src/app/teleop_interface_gui.cpp"
    "src/teleop_interface.cpp"
    "src/teleop_interface_component.cpp"
    AMENT_DEPENDENCIES
    geometry_msgs
    image_transport
    rclcpp
    rclcpp_components
    sensor_msgs
    sfg_agent_msgs
    sfg_imgui_vendor
    sfg_utils
    std_msgs
)

# ########################################################################################################
# Add other files/directories.                                                                           #
# ########################################################################################################
install(DIRECTORY
    "config"
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
