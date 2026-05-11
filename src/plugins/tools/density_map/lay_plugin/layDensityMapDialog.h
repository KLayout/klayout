
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


#ifndef HDR_layDensityMapDialog
#define HDR_layDensityMapDialog

#include "ui_DensityMapDialog.h"

#include "layLayoutView.h"
#include "layBrowser.h"
#include "layMarker.h"

namespace lay
{

class DensityMapDialog
  : public lay::Browser,
    private Ui::DensityMapDialog
{
Q_OBJECT 

public:
  DensityMapDialog (lay::Dispatcher *root, lay::LayoutViewBase *view);
  ~DensityMapDialog ();

public slots:
  void box_selection_clicked ();
  void source_selection_clicked ();
  void accept ();

private:
  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  //  implementation of the lay::Plugin interface
  void menu_activated (const std::string &symbol);

};

}

#endif

