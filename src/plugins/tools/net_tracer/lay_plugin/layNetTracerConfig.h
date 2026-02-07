
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#ifndef HDR_layNetTracerConfig
#define HDR_layNetTracerConfig

#include "ui_NetTracerConfigPage.h"

#include "layPlugin.h"
#include "layPluginConfigPage.h"
#include "layColorPalette.h"

namespace lay
{

extern const std::string cfg_nt_marker_color;
extern const std::string cfg_nt_marker_cycle_colors;
extern const std::string cfg_nt_marker_cycle_colors_enabled;
extern const std::string cfg_nt_marker_dither_pattern;
extern const std::string cfg_nt_marker_line_width;
extern const std::string cfg_nt_marker_vertex_size;
extern const std::string cfg_nt_marker_halo;
extern const std::string cfg_nt_marker_intensity;
extern const std::string cfg_nt_window_mode;
extern const std::string cfg_nt_window_dim;
extern const std::string cfg_nt_max_shapes_highlighted;
extern const std::string cfg_nt_trace_depth;

enum nt_window_type { NTDontChange = 0, NTFitNet, NTCenter, NTCenterSize };

class NetTracerWindowModeConverter
{
public:
  void from_string (const std::string &value, lay::nt_window_type &mode);
  std::string to_string (lay::nt_window_type mode);
};

class NetTracerConfigPage
  : public lay::ConfigPage,
    private Ui::NetTracerConfigPage
{
  Q_OBJECT 

public:
  NetTracerConfigPage (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void window_changed (int);
  void color_button_clicked ();
  void update_colors ();

private:
  lay::ColorPalette m_palette;
};

}

#endif


