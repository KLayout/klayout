
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

#if defined(HAVE_QT)

#ifndef HDR_layEditStipplesForm
#define HDR_layEditStipplesForm

#include <QDialog>

#include "layDitherPattern.h"
#include "dbObject.h"

class QListWidgetItem;

namespace Ui
{
  class EditStipplesForm;
}

namespace lay
{

class LayoutViewBase;

class EditStipplesForm
  : public QDialog, public db::Object
{
  Q_OBJECT 

public:
  EditStipplesForm (QWidget *parent, lay::LayoutViewBase *view, const lay::DitherPattern &pattern);
  ~EditStipplesForm ();

  //  ...

  const lay::DitherPattern &pattern () const
  {
    return m_pattern;
  }

  int selected () const
  {
    return m_selected;
  }
  
  void undo (db::Op *op);
  void redo (db::Op *op);

public slots:
  void sel_changed (QListWidgetItem *current, QListWidgetItem *); 
  void double_clicked (QListWidgetItem *item); 
  void new_button_clicked ();
  void delete_button_clicked ();
  void clone_button_clicked ();
  void up_button_clicked ();
  void down_button_clicked ();
  void clear_button_clicked ();
  void invert_button_clicked ();
  void fliph_button_clicked ();
  void flipv_button_clicked ();
  void rotate_button_clicked ();
  void sleft_button_clicked ();
  void sup_button_clicked ();
  void sright_button_clicked ();
  void sdown_button_clicked ();
  void undo_button_clicked ();
  void redo_button_clicked ();
  void edited ();
  void size_changed ();

private slots:
  void editor_size_changed ();

protected:
  lay::DitherPattern::iterator current ();
  lay::DitherPattern::iterator index_of (QListWidgetItem *item);
  void update ();
  void update_current_item ();
  void select_item (int index);
  void handle_op (db::Op *op, bool undo);

private:
  Ui::EditStipplesForm *mp_ui;
  int m_selected;
  lay::DitherPattern m_pattern;
  db::Manager m_manager;
  lay::LayoutViewBase *mp_view;
  bool m_selection_changed_enabled;
};

}

#endif

#endif  //  defined(HAVE_QT)
