
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "laySearchReplaceConfigPage.h"
#include "layDispatcher.h"

namespace lay
{

const std::string cfg_sr_window_state ("sr-window-state");
const std::string cfg_sr_window_mode ("sr-window-mode");
const std::string cfg_sr_window_dim ("sr-window-dim");
const std::string cfg_sr_max_item_count ("sr-max-item-count");

static struct {
  SearchReplaceDialog::window_type mode;
  const char *string;
} window_modes [] = {
  { SearchReplaceDialog::DontChange,    "dont-change" },
  { SearchReplaceDialog::FitCell,       "fit-cell"    },
  { SearchReplaceDialog::FitMarker,     "fit-marker"  },
  { SearchReplaceDialog::Center,        "center"      },
  { SearchReplaceDialog::CenterSize,    "center-size" }
};

void
SearchReplaceWindowModeConverter::from_string (const std::string &value, SearchReplaceDialog::window_type &mode)
{
  for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
    if (value == window_modes [i].string) {
      mode = window_modes [i].mode;
      return;
    }
  }
  throw tl::Exception (tl::to_string (QObject::tr ("Invalid search result browser window mode: ")) + value);
}

std::string 
SearchReplaceWindowModeConverter::to_string (SearchReplaceDialog::window_type mode)
{
  for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
    if (mode == window_modes [i].mode) {
      return window_modes [i].string;
    }
  }
  return "";
}

// ------------------------------------------------------------

SearchReplaceConfigPage::SearchReplaceConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  Ui::SearchReplaceConfigPage::setupUi (this);

  connect (cbx_window, SIGNAL (currentIndexChanged (int)), this, SLOT (window_changed (int)));
}

void 
SearchReplaceConfigPage::setup (lay::Dispatcher *root)
{
  std::string value;

  //  window mode
  SearchReplaceDialog::window_type wmode = SearchReplaceDialog::FitMarker;
  root->config_get (cfg_sr_window_mode, wmode, SearchReplaceWindowModeConverter ());
  cbx_window->setCurrentIndex (int (wmode));

  //  window dimension
  double wdim = 1.0;
  root->config_get (cfg_sr_window_dim, wdim);
  le_window->setText (tl::to_qstring (tl::to_string (wdim)));
    
  //  max. instance count
  unsigned int max_item_count = 1000;
  root->config_get (cfg_sr_max_item_count, max_item_count);
  le_max_items->setText (tl::to_qstring (tl::to_string (max_item_count)));

  //  enable controls
  window_changed (int (wmode));
}

void
SearchReplaceConfigPage::window_changed (int m)
{
  le_window->setEnabled (m == int (SearchReplaceDialog::FitMarker) || m == int (SearchReplaceDialog::CenterSize));
}

void 
SearchReplaceConfigPage::commit (lay::Dispatcher *root)
{
  double dim = 1.0;
  tl::from_string (tl::to_string (le_window->text ()), dim);

  unsigned int max_item_count = 1000;
  tl::from_string (tl::to_string (le_max_items->text ()), max_item_count);

  root->config_set (cfg_sr_window_mode, SearchReplaceDialog::window_type (cbx_window->currentIndex ()), SearchReplaceWindowModeConverter ());
  root->config_set (cfg_sr_window_dim, dim);
  root->config_set (cfg_sr_max_item_count, max_item_count);
}

}

