# Universal Remote Control

## Steam Deck Setup 
Install Ubuntu 24.04 LTS (noble numbat) using USB type C OTG cable/hub.
Username: sfg-user
Password: sfg-user
Hostname: sfg-steamdeck-01

Connect to SFG-6 wifi and assign DHCP reservation for 192.168.50.71 in OPNsense 

## Usage
For stationary usage a USB C (thunderbolt) dock can be connected to use external mouse, keyboard, monitor(s), ethernet, etc. 

For mobile usage, touch and trackpad input should work. The onscreen keyboard can be enabled in Ubuntu's accessibility settings.

Sound through built-in speaker does not work on Ubuntu.

Long press upper right menu button until short haptic feedback to toggle between system wide trackpad mode and gamepad mode.

Trackpad Mode:
- Right touchpad controls the mouse pointer
- Pressing the right touchpad works as left click
- Pressing the 'L2' trigger works as right click
- Pressing the 'B' button works as 'return' option

Gamepad Mode:
- No mouse pointer and click emulation
- Sticks, buttons, bumpers and triggers are available to user applications

Software can be installed like on any other ubuntu desktop machine.

## Work around for GUI app start problem
Run
```
sudo xhost +local:
``` 

## External resources
[Official Product Page](https://www.steamdeck.com/en/oled)

## Fix for development environments with nvidia GPU
Force generic platform profile (required on systems with nvidia GPU)
in .devcontainer/scripts/bash_utils/get_platftom insert
```
echo "generic"
return
```
at the top