
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


#include "edtPCellParametersDialog.h"

#include <QPushButton>

namespace edt
{

PCellParametersDialog::PCellParametersDialog (QWidget *parent)
  : QDialog (parent)
{
  setupUi (this);

  connect (buttons->button (QDialogButtonBox::Apply), SIGNAL (clicked ()), this, SLOT (apply_pressed ()));
}

void
PCellParametersDialog::apply_pressed ()
{
  emit parameters_changed ();
  parameters_changed_event ();
}

std::vector<tl::Variant> 
PCellParametersDialog::get_parameters () 
{
  return parameters->get_parameters ();
}

void 
PCellParametersDialog::set_parameters (const std::vector<tl::Variant> &p)
{
  parameters->set_parameters (p);
}

int
PCellParametersDialog::exec (const db::Layout *layout, lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &p)
{
  parameters->setup (layout, view, cv_index, pcell_decl, p);
  return QDialog::exec ();
}

}
