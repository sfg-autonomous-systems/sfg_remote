cmake_minimum_required(VERSION 3.8)
project(sfg_teleop_interface)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ########################################################################################################
# Find dependencies.                                                                                     #
# ########################################################################################################
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(sfg_agent_msgs REQUIRED)
find_package(sfg_utils REQUIRED)

# ...
add_executable(remote_control_station src/remote_control_station.cpp)
target_include_directories(remote_control_station PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(remote_control_station PUBLIC c_std_99 cxx_std_17) # Require C99 and C++17

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
