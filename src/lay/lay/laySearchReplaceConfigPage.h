
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


#ifndef HDR_laySearchReplaceConfigPage
#define HDR_laySearchReplaceConfigPage

#include "layPlugin.h"
#include "layPluginConfigPage.h"
#include "laySearchReplaceDialog.h"

#include "ui_SearchReplaceConfigPage.h"

#include <string>

namespace lay
{

extern const std::string cfg_sr_window_state;
extern const std::string cfg_sr_window_mode;
extern const std::string cfg_sr_window_dim;
extern const std::string cfg_sr_max_item_count;

class SearchReplaceWindowModeConverter
{
public:
  void from_string (const std::string &value, SearchReplaceDialog::window_type &mode);
  std::string to_string (SearchReplaceDialog::window_type mode);
};

class SearchReplaceConfigPage
  : public lay::ConfigPage,
    private Ui::SearchReplaceConfigPage
{
  Q_OBJECT 

public:
  SearchReplaceConfigPage (QWidget *parent);

  virtual void setup (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);

public slots:
  void window_changed (int);
};

}

#endif

