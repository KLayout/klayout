
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


#ifndef HDR_edtPCellParametersDialog
#define HDR_edtPCellParametersDialog

#include "dbPCellDeclaration.h"
#include "ui_PCellParametersDialog.h"

#include <QDialog>

namespace lay
{
  class LayoutView;
}

namespace edt
{

/**
 *  @brief A QScrollArea that displays and allows editing PCell parameters
 */
class PCellParametersDialog
  : public QDialog, private Ui::PCellParametersDialog
{
Q_OBJECT

public:
  /**
   *  @brief Constructor: create a dialog showing the given parameters
   *  @param parent The parent widget
   */
  PCellParametersDialog (QWidget *parent);

  /**
   *  @brief Executes the parameter dialog
   *  @param layout The layout in which the PCell instance resides
   *  @param view The layout view from which to take layers for example
   *  @param cv_index The index of the cellview in "view"
   *  @param pcell_decl The PCell declaration
   *  @param parameters The parameter values to show (if empty, the default values are used)
   */
  int exec (const db::Layout *layout, lay::LayoutView *view, int cv_index, const db::PCellDeclaration *pcell_decl, const db::pcell_parameters_type &p);

  /**
   *  @brief Get the current parameters
   */
  std::vector<tl::Variant> get_parameters ();

  /**
   *  @brief Sets the given parameters as values
   */
  void set_parameters (const  std::vector<tl::Variant> &values);

  tl::Event parameters_changed_event;

signals:
  void parameters_changed ();

private slots:
  void apply_pressed ();
};

}

#endif
