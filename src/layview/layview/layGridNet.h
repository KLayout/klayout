
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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
#if defined(HAVE_QT)
#  include "layPluginConfigPage.h"
#endif
#include "tlColor.h"
#include "dbTypes.h"
#include "dbBox.h"

namespace Ui {
  class GridNetConfigPage;
}

namespace lay {

class LayoutViewBase;
class ColorButton;

class GridNetPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const;
#if defined(HAVE_QT)
  virtual lay::ConfigPage *config_page (QWidget *parent, std::string &title) const;
#endif
  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *, lay::LayoutViewBase *view) const;
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

  GridNet (lay::LayoutViewBase *view);

private:
  virtual void render_bg (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas);

  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  lay::LayoutViewBase *mp_view;
  bool m_visible;
  bool m_show_ruler;
  double m_grid;
  tl::Color m_color;
  tl::Color m_grid_color;
  tl::Color m_axis_color;
  tl::Color m_ruler_color;
  GridStyle m_style0;
  GridStyle m_style1;
  GridStyle m_style2;
};

class GridNetStyleConverter
{
public:
  void from_string (const std::string &value, lay::GridNet::GridStyle &style);
  std::string to_string (lay::GridNet::GridStyle style);
};

}

#endif

