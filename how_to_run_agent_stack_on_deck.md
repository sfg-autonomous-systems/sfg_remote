# Host: Run initialize_command. Only required once. (mount /tmp, pick ROS domain ID 100 twice)
#   .devcontainer/scripts/initialize_command LOCAL_WORKSPACE_DIRECTORY=${PWD} CONTAINER_WORKSPACE_DIRECTORY=/workspace

# Host: cd to devcontainer_user directory and start the compose stack with compose up -f compose.user.yaml.
#   cd .devcontainer_user/
#   podman compose -f compose.user.yaml build --no-cache

# Host: Inside dockerfile.user set up the CMD command to run post_start_command and post_attach_command, source workspace, and start your software stack.
#   podman compose -f compose.user.yaml up -d

# Enter interactive bash shell on running container
#   podman exec -it <container-name> bash

# This should end up opening up your GUI and any related ROS nodes required for communication. (not yet)


