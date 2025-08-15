# Boxxy Plugin

## Overview
- Boxxy adds a box drawing tool similar to the built-in Box tool and auto-selects a sensible target layer when none is explicitly chosen.
- Works in Editor mode only. In Viewer mode, it shows a dismissible warning.

## Current Functionality
- Editor-only activation: registers toolbar entry and Edit menu items only in Editor mode.
- Auto layer selection: uses the current selection if present; otherwise a saved default layer (if set); otherwise the first drawing layer.
- Default layer helpers: Edit > Boxxy > “Set Default Layer From Current” and “Clear Default Layer”.
- Clean abort: preview marker is destroyed on ESC; cursor resets to default.

## Installation
1. Copy `boxxy_plugin.py` to your KLayout Python folder:
    - Linux/macOS: `~/.klayout/python/`
    - Windows: `%APPDATA%\KLayout\python\`
2. Restart KLayout.
3. Open Macros > Macro Development and navigate to Python tab.
4. Choose folder "Local - python branch" and select "boxxy_plugin".
5. Run the script (F5) to enable the plugin functionality.


## How To Use
1. Start KLayout in Editor mode (Menu: File > Setup > Application > Editing Mode and restart, or start with `-e`).
2. Open a layout; ensure a drawing layer is present in the layer list.
3. Click the “Boxxy” tool button in the toolbar (box icon) or select it from the tool menu.
4. First click sets one corner; move the mouse to preview; second click inserts the box.
5. Press ESC at any time to cancel without creating geometry.
