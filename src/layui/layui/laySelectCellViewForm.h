
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

#ifndef HDR_SelectCellViewForm
#define HDR_SelectCellViewForm

#include "layuiCommon.h"

#include <string>
#include <vector>

#include <QDialog>

namespace Ui
{
  class SelectCellViewForm;
}

namespace lay
{

class CellView;
class LayoutViewBase;

class LAYUI_PUBLIC SelectCellViewForm
  : public QDialog
{
  Q_OBJECT 

public:
  SelectCellViewForm (QWidget *parent, lay::LayoutViewBase *view, const std::string &title, bool single = false);

  /**
   *  @brief Set the selection to a single selection
   */
  void set_selection (int sel);

  /**
   *  @brief Set the title
   */
  void set_title (const std::string &title);
  
  /**
   *  @brief Set the caption
   */
  void set_caption (const std::string &caption);
  
  /**
   *  @brief This method must be called on all cv's before setup () to make the cv's known
   */
  void tell_cellview (const lay::CellView &cv);

  /**
   *  @brief Obtain the selected cellview's index (with changes)
   */
  std::vector <int> selected_cellviews () const;

  /**
   *  @brief Obtain the selected cellview's index (in single selection mode)
   */
  int selected_cellview () const;

  /**
   *  @brief Return true, if all cellviews are selected
   */
  bool all_selected () const;

public slots:
  virtual void select_all ();

private:
  Ui::SelectCellViewForm *mp_ui;
};

}

#endif

#endif  //  defined(HAVE_QT)
