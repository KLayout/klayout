
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


#ifndef HDR_layNetlistBrowser
#define HDR_layNetlistBrowser

#include "layPlugin.h"
#include "layColorPalette.h"
#include "ui_NetlistBrowserConfigPage.h"
#include "ui_NetlistBrowserConfigPage2.h"

#include "dbTrans.h"

#include <algorithm>

namespace lay
{

struct NetlistBrowserConfig
{
  enum net_window_type { DontChange = 0, FitNet, Center, CenterSize };
};

class NetlistBrowserConfigPage
  : public lay::ConfigPage,
    private Ui::NetlistBrowserConfigPage
{
  Q_OBJECT

public:
  NetlistBrowserConfigPage (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void window_changed (int);
};

class NetlistBrowserConfigPage2
  : public lay::ConfigPage,
    private Ui::NetlistBrowserConfigPage2
{
  Q_OBJECT

public:
  NetlistBrowserConfigPage2 (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void color_button_clicked ();

private:
  void update_colors ();

  lay::ColorPalette m_palette;
};

class NetlistBrowserWindowModeConverter
{
public:
  void from_string (const std::string &value, lay::NetlistBrowserConfig::net_window_type &mode);
  std::string to_string (lay::NetlistBrowserConfig::net_window_type mode);
};

}

#endif

