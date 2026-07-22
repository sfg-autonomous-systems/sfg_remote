cmake_minimum_required(VERSION 3.8)
project(sfg_remote)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

cmake_policy(SET CMP0072 NEW)

# ########################################################################################################
# Find dependencies.                                                                                     #
# ########################################################################################################
find_package(ament_cmake REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(sfg_agent_msgs REQUIRED)
find_package(sfg_imgui_vendor REQUIRED)
find_package(sfg_utils REQUIRED)
find_package(std_msgs REQUIRED)

# ########################################################################################################
# Add nodes and libraries.                                                                               #
# ########################################################################################################
sfg_utils__add_component("remote" "${PROJECT_NAME}::Remote"
    SOURCES
    "src/app/agent_manager.cpp"
    "src/app/image_feed.cpp"
    "src/app/remote_gui.cpp"
    "src/remote.cpp"
    "src/remote_component.cpp"
    DEPENDENCIES
    cv_bridge::cv_bridge
    ${geometry_msgs_TARGETS}
    image_transport::image_transport
    rclcpp::rclcpp
    rclcpp_components::component
    ${sensor_msgs_TARGETS}
    ${sfg_agent_msgs_TARGETS}
    sfg_imgui_vendor::sfg_imgui_vendor
    sfg_utils::sfg_utils
    ${std_msgs_TARGETS}
    EXPORT_DEPENDENCIES
    cv_bridge
    geometry_msgs
    image_transport
    rclcpp
    sfg_imgui_vendor
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
