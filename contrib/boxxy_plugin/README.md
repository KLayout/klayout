Boxxy Plugin (Third-Party Script)

Overview
- Boxxy adds a box drawing tool that behaves like the built-in Box tool but also works when no layer is selected: it will draw on the first drawing layer in the current layer list.

Installation
- Copy `boxxy_plugin.rb` to your KLayout macros folder (Ruby):
  - Linux/macOS: `~/.klayout/macros/`
  - Windows: `%APPDATA%\KLayout\macros\`
- Restart KLayout. A new tool entry “Boxxy” appears in the toolbar (with the box icon) and behaves like the Box tool with auto-layer selection.

Notes
- Boxxy is implemented as a pure scripting plugin via the Application API (`RBA::PluginFactory`/`RBA::Plugin`).
- It creates interactive preview via a marker and inserts the final box into the selected (or auto-selected) layer.
- ESC cancels the current drawing.

