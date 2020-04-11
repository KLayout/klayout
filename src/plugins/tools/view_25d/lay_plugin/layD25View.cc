
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


#include "layD25View.h"
#include "layLayoutView.h"

#include "ui_D25View.h"

#include <stdio.h>

namespace lay
{

D25View::D25View (QWidget *parent)
  : QDialog (parent)
{
  mp_ui = new Ui::D25View ();
  mp_ui->setupUi (this);

  // @@@
}

D25View::~D25View ()
{
  delete mp_ui;
  mp_ui = 0;
}

int 
D25View::exec_dialog (lay::LayoutView *view)
{
  mp_view.reset (view);
  mp_ui->d25_view->attach_view (view);

  int ret = QDialog::exec ();

  mp_ui->d25_view->attach_view (0);

  return ret;
}

void 
D25View::accept ()
{
  QDialog::accept ();
  // @@@
}

}

