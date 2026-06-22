#!/bin/bash

# Must still be executed on the dektop GUI directly before running this script !!!
# xhost +local:

# --- Configuration ---
# Update these variables to match your setup
PROJECT_DIR=".devcontainer_user"
COMPOSE_FILE="compose.user.yaml"
SERVICE_NAME="sfg_agent_devcontainer" # <-- Replace with the actual service name from your yaml

cd "$PROJECT_DIR" || { echo "Directory not found"; exit 1; }

echo "Starting Docker Compose stack..."
# Start the stack in detached mode so the script can continue
docker compose -f "$COMPOSE_FILE" up -d

echo "Launching ROS application..."
# Execute the ROS command inside the running container.
# This command blocks the script from continuing until the ImGui application is closed.
# Note: If your container doesn't automatically source ROS, you might need to change the 
# command to: bash -c "source /opt/ros/humble/setup.bash && ros2 launch..."
docker compose -f "$COMPOSE_FILE" exec "$SERVICE_NAME" bash -c "source /opt/ros/humble/setup.bash && source /workspace/universal_remote_control/colcon_ws/install/setup.bash && ros2 launch sfg_remote_control_bringup bringup.launch.py"

echo "Application closed. Tearing down the stack..."
# Shut down the containers once the exec command finishes
docker compose -f "$COMPOSE_FILE" down

echo "Done!"
exit
