#!/bin/bash

# delete existing .env file
rm .devcontainer_user/.env

# run init script for devcontainer with hardcoded args (to avoid vscode)
.devcontainer/scripts/initialize_command LOCAL_WORKSPACE_DIRECTORY=${PWD} CONTAINER_WORKSPACE_DIRECTORY=/workspace

