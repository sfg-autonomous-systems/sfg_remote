# Universal Remote Control

## Steam Deck Setup 
Install Ubuntu 24.04 LTS (noble numbat) or Fedora Bazzite DX (developer experience) using a boot stick
and a USB type C OTG cable/hub.

Username: sfg-agent
Hostname: sfg-universal-remote-control-01

Connect to the device to SFG-6G and/or SFG-5G WiFi network.

## Usage
For stationary usage a USB C (thunderbolt) dock can be connected to use external mouse, keyboard, monitor(s), ethernet, etc. 

For mobile usage, touch and trackpad input should work. The onscreen keyboard can be enabled in the accessibility settings.

Long press upper right menu button until short haptic feedback to toggle between system wide trackpad mode and gamepad mode.

Trackpad Mode:
- Right touchpad controls the mouse pointer
- Pressing the right touchpad works as left click
- Pressing the 'L2' trigger works as right click
- Pressing the 'B' button works as 'return' option

Gamepad Mode:
- No mouse pointer and click emulation
- Sticks, buttons, bumpers and triggers are available to user applications

## Hack for containerized GUI apps
Run the following command in a terminal window directly on the device after boot.
```
xhost +local:
``` 
It's very important to to this before doing anything else, otherwise the GUI applications 
from containerized environments fail to launch properly.

## Additional resources
[Official Product Page](https://www.steamdeck.com/en/oled)