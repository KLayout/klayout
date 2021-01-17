
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


#ifndef HDR_rdbMarkerBrowser
#define HDR_rdbMarkerBrowser

#include "layPlugin.h"
#include "ui_MarkerBrowserConfigPage.h"
#include "ui_MarkerBrowserConfigPage2.h"

#include "dbTrans.h"

#include <algorithm>

namespace rdb
{

enum context_mode_type { AnyCell = 0 , DatabaseTop, Current, CurrentOrAny, Local };
enum window_type { DontChange = 0, FitCell, FitMarker, Center, CenterSize };

class MarkerBrowserConfigPage
  : public lay::ConfigPage,
    private Ui::MarkerBrowserConfigPage
{
  Q_OBJECT 

public:
  MarkerBrowserConfigPage (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void window_changed (int);
};

class MarkerBrowserConfigPage2
  : public lay::ConfigPage,
    private Ui::MarkerBrowserConfigPage2
{
  Q_OBJECT 

public:
  MarkerBrowserConfigPage2 (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);
};

class MarkerBrowserContextModeConverter
{
public:
  void from_string (const std::string &value, rdb::context_mode_type &mode);
  std::string to_string (rdb::context_mode_type mode);
};

class MarkerBrowserWindowModeConverter
{
public:
  void from_string (const std::string &value, rdb::window_type &mode);
  std::string to_string (rdb::window_type mode);
};

}

#endif

