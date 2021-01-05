
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#ifndef HDR_layGridNet
#define HDR_layGridNet

#include "layViewObject.h"
#include "layPlugin.h"
#include "dbTypes.h"
#include "dbBox.h"

namespace Ui {
  class GridNetConfigPage;
}

namespace lay {

class LayoutView;
class ColorButton;

class GridNetPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const;
  virtual lay::ConfigPage *config_page (QWidget *parent, std::string &title) const;
  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *, lay::LayoutView *view) const;
};

class GridNetConfigPage 
  : public lay::ConfigPage
{
Q_OBJECT

public:
  GridNetConfigPage (QWidget *parent);
  ~GridNetConfigPage ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

private:
  Ui::GridNetConfigPage *mp_ui;
  lay::ColorButton *mp_grid_color_cbtn;
  lay::ColorButton *mp_grid_grid_color_cbtn;
  lay::ColorButton *mp_grid_axis_color_cbtn;
  lay::ColorButton *mp_grid_ruler_color_cbtn;
};

class GridNet
  : public lay::BackgroundViewObject,
    public lay::Plugin
{
public: 
  enum GridStyle {
    Invisible = 0, 
    //  dot styles:
    Dots, DottedLines, LightDottedLines, TenthDottedLines,  
    //  line styles:
    Crosses, Lines, TenthMarkedLines, 
    //  others
    CheckerBoard
  };

  GridNet (lay::LayoutView *view);

private:
  virtual void render_bg (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas);

  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  lay::LayoutView *mp_view;
  bool m_visible;
  bool m_show_ruler;
  double m_grid;
  QColor m_color;
  QColor m_grid_color;
  QColor m_axis_color;
  QColor m_ruler_color;
  GridStyle m_style0;
  GridStyle m_style1;
  GridStyle m_style2;
};

}

#endif

