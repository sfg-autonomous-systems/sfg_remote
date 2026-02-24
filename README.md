# Universal Remote Control

Fix to force generic platform profile (required on systems with nvidia GPU)
in .devcontainer/scripts/bash_utils/get_platftom insert
```
echo "generic"
return
```
at the top