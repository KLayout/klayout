Boxxy Plugin (Third-Party Script)

Overview
- Boxxy adds a box drawing tool that behaves like the built-in Box tool but also works when no layer is selected: it will draw on the first drawing layer in the current layer list.

Installation
- Copy `boxxy_plugin.rb` to your KLayout macros folder (Ruby):
  - Linux/macOS: `~/.klayout/macros/`
  - Windows: `%APPDATA%\KLayout\macros\`
- Restart KLayout. A new tool entry “Boxxy” appears in the toolbar (with the box icon) and behaves like the Box tool with auto-layer selection.

Notes
- This plugin is implemented via the Application API (`RBA::PluginFactory`/`RBA::Plugin`).
- It creates interactive preview via a marker and inserts the final box into the selected (or auto-selected) layer.
- ESC cancels the current drawing.

Diagnostics
- If the submenu does not appear, ensure the macro is enabled and re-run it (or restart KLayout). You can check registered symbols in Macro Console:
  `puts RBA::LayoutView::menu_symbols.grep(/boxxy/)`
