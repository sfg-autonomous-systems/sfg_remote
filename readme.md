# Universal Remote Control

## [Steam Deck](https://www.steamdeck.com/en/oled) Setup (Bazzite DX)
1. [Download](https://download.bazzite.gg/bazzite-deck-stable-live-amd64.iso) and [install](https://docs.bazzite.gg/General/Installation_Guide/) Bazzite for Steam Deck (KDE Desktop). During installation, configure:
    
    - Display name: SFG Agent
    - Username: `sfg-agent`
    - Hostname: `sfg-universal-remote-control-XX` (replace `XX` with the appropriate index for the given device, e.g. if it is the third universal remote control, choose `03`)

2. If prompted, create or log in with a Steam account.

3. After installation, rebase to Bazzite DX:

    ```bash
    brh rebase bazzite-dx:stable
    ```

4. Enable SSH access:

    ```bash
    sudo systemctl enable --now sshd
    ```

5. Connect the device to the `SFG-5G`/`SFG-6G` WiFi network.

## Agent Setup

1. Clone this repository with submodules:

    ```bash
    git clone --recurse-submodules <repo-url>
    ```

2. Open the repository in VS Code.

3. Reopen the workspace in the included devcontainer through VS Code. The initialize command provides an option to create a desktop shortcut for the universal remote control.

4. Build the workspace:

    ```bash
    cd "${COLCON_WS}" && rm -rf build install log && colcon build
    ```

5. Launch the application:

    ```bash
    source "${COLCON_WS}/install/setup.bash" && ros2 launch sfg_universal_remote_control_bringup bringup.launch.py
    ```

**Note:** If started via the desktop shortcut alone, the container and software stack are not built automatically. Run the VS Code dev container workflow at least once beforehand.

**Note:** You might need to execute `xhost +local:` on the host once after a reboot in order for the agent's GUI to display.

## Agent Usage

Use gamepad mode when running the universal remote control application.

### Universal Remote Control Inputs

| Control (Steam Deck) | SDL input | Action in universal remote control |
|---|---|---|
| B button | `SDL_GAMEPAD_BUTTON_EAST` | Requests application shutdown (pushes `SDL_EVENT_QUIT` and triggers ROS shutdown). |
| X button | `SDL_GAMEPAD_BUTTON_WEST` | Opens the agent selector dialog. |
| L1 bumper | `SDL_GAMEPAD_BUTTON_LEFT_SHOULDER` | Arms the selected agent. |
| R1 bumper | `SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER` | Disarms the selected agent. |
| Left stick (X/Y) | `SDL_GAMEPAD_AXIS_LEFTX`, `SDL_GAMEPAD_AXIS_LEFTY` | Drives translational velocity: `linear.y` from X (lateral), `linear.x` from Y (longitudinal). |
| Right stick (X/Y) | `SDL_GAMEPAD_AXIS_RIGHTX`, `SDL_GAMEPAD_AXIS_RIGHTY` | Drives rotational velocity: `angular.z` from X (yaw), `angular.y` from Y (pitch). |

**Notes:**

- Velocity commands are published only while the agent is armed.
- Input is sampled at 40Hz.
- Stick values are normalized, clamped, and filtered with deadzones before publishing `/cmd_vel`.