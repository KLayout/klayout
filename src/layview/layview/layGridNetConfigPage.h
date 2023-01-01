
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

#if defined(HAVE_QT)

#ifndef HDR_layGridNetConfigPage
#define HDR_layGridNetConfigPage

#include "layPluginConfigPage.h"

namespace Ui {
  class GridNetConfigPage;
}

namespace lay {

class ColorButton;

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

}

#endif

#endif  //  defined(HAVE_QT)
