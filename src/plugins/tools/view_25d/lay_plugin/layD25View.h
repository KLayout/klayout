
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
  : public QDialog
{
Q_OBJECT 

public:
  D25View (QWidget *parent);
  ~D25View ();

  int exec_dialog (lay::LayoutView *view);

protected:
  void accept ();

private slots:
  void fit_button_clicked ();
  void scale_factor_changed (double f);
  void scale_slider_changed (int value);
  void init_failed ();

private:
  Ui::D25View *mp_ui;
  tl::weak_ptr<lay::LayoutView> mp_view;
};

}

#endif

