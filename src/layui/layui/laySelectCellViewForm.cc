
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

#include "laySelectCellViewForm.h"
#include "layCellView.h"
#include "layLayoutViewBase.h"

#include "ui_SelectCellViewForm.h"


namespace lay
{

// ------------------------------------------------------------

SelectCellViewForm::SelectCellViewForm (QWidget *parent, lay::LayoutViewBase *view, const std::string &title, bool single)
  : QDialog (parent)
{
  mp_ui = new Ui::SelectCellViewForm ();

  setObjectName (QString::fromUtf8 ("select_cv"));

  mp_ui->setupUi (this);

  if (single) {
    mp_ui->cvs_lb->setSelectionMode (QAbstractItemView::SingleSelection);
  }

  // signals and slots connections
  connect (mp_ui->ok_button, SIGNAL( clicked() ), this, SLOT( accept() ));
  connect (mp_ui->cancel_button, SIGNAL( clicked() ), this, SLOT( reject() ));
  connect (mp_ui->select_all_pb, SIGNAL( clicked() ), this, SLOT( select_all() ));

  if (single) {
    mp_ui->select_all_pb->hide ();
  }

  for (unsigned int i = 0; i < view->cellviews (); ++i) {
    tell_cellview (view->cellview (i));
  }

  set_title (title);
}

void 
SelectCellViewForm::set_selection (int sel)
{
  for (int i = 0; i < int (mp_ui->cvs_lb->count ()); ++i) {
    mp_ui->cvs_lb->item (i)->setSelected (false);
  }
  if (sel >= 0 && sel < int (mp_ui->cvs_lb->count ())) {
    mp_ui->cvs_lb->setCurrentItem (mp_ui->cvs_lb->item (sel));
    mp_ui->cvs_lb->item (sel)->setSelected (true);
  }
}

void 
SelectCellViewForm::set_title (const std::string &title)
{
  mp_ui->title_lbl->setText (tl::to_qstring (title));
}
  
void 
SelectCellViewForm::set_caption (const std::string &caption)
{
  setWindowTitle (tl::to_qstring (caption));
}
  
void 
SelectCellViewForm::tell_cellview (const lay::CellView &cv)
{
  mp_ui->cvs_lb->addItem (tl::to_qstring (cv->name ()));
  mp_ui->cvs_lb->setCurrentItem (0);
  mp_ui->cvs_lb->item (0)->setSelected (true);
}

bool 
SelectCellViewForm::all_selected () const
{
  for (int i = 0; i < int (mp_ui->cvs_lb->count ()); ++i) {
    if (! mp_ui->cvs_lb->item (i)->isSelected ()) {
      return false;
    }
  }
  return true;
}

std::vector <int> 
SelectCellViewForm::selected_cellviews () const
{
  std::vector <int> res;

  for (int i = 0; i < int (mp_ui->cvs_lb->count ()); ++i) {
    if (mp_ui->cvs_lb->item (i)->isSelected ()) {
      res.push_back (i);
    }
  }

  return res;
}

int 
SelectCellViewForm::selected_cellview () const
{
  for (int i = 0; i < int (mp_ui->cvs_lb->count ()); ++i) {
    if (mp_ui->cvs_lb->item (i)->isSelected ()) {
      return i;
    }
  }
  return -1;
}

void 
SelectCellViewForm::select_all ()
{
  mp_ui->cvs_lb->clearSelection ();  //  without this, not all items may be selected in "selectAll"
  mp_ui->cvs_lb->selectAll ();
}

}
  
#endif
