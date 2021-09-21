
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

#ifndef HDR_layD25View
#define HDR_layD25View

#include <QDialog>

#include "tlObject.h"
#include "layBrowser.h"

namespace Ui
{
  class D25View;
}

namespace lay
{
  class LayoutView;
}

namespace lay
{

class D25View
  : public lay::Browser
{
Q_OBJECT 

public:
  D25View (lay::Dispatcher *root, lay::LayoutView *view);
  ~D25View ();

  virtual void menu_activated (const std::string &symbol);
  virtual void deactivated ();
  virtual void activated ();

protected:
  void accept ();
  void reject ();

private slots:
  void fit_button_clicked ();
  void scale_factor_changed (double f);
  void scale_slider_changed (int value);
  void scale_value_edited ();
  void vscale_factor_changed (double f);
  void vscale_slider_changed (int value);
  void vscale_value_edited ();
  void init_failed ();

private:
  Ui::D25View *mp_ui;
};

}

#endif

