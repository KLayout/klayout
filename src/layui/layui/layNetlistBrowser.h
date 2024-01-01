
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef HDR_layNetlistBrowser
#define HDR_layNetlistBrowser

#include "layPlugin.h"
#include "layPluginConfigPage.h"
#include "layColorPalette.h"

#include "dbTrans.h"

#include <algorithm>

namespace Ui
{
  class NetlistBrowserConfigPage;
  class NetlistBrowserConfigPage2;
}

namespace lay
{

struct NetlistBrowserConfig
{
  enum net_window_type { DontChange = 0, FitNet, Center, CenterSize };
};

class NetlistBrowserConfigPage
  : public lay::ConfigPage
{
  Q_OBJECT

public:
  NetlistBrowserConfigPage (QWidget *parent);
  ~NetlistBrowserConfigPage ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void window_changed (int);

private:
  Ui::NetlistBrowserConfigPage *mp_ui;
};

class NetlistBrowserConfigPage2
  : public lay::ConfigPage
{
  Q_OBJECT

public:
  NetlistBrowserConfigPage2 (QWidget *parent);
  ~NetlistBrowserConfigPage2 ();

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void color_button_clicked ();

private:
  void update_colors ();

  Ui::NetlistBrowserConfigPage2 *mp_ui;
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

#endif  //  defined(HAVE_QT)
