
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

#include "layEditLineStylesForm.h"
#include "ui_EditLineStylesForm.h"

#include "layLayoutViewBase.h"
#include "tlExceptions.h"

#include "tlString.h"

#include <QBitmap>
#include <QInputDialog>

#include <algorithm>

namespace lay
{

struct CurrentStyleOp
  : public db::Op
{
  CurrentStyleOp (int pi, int ni)
    : db::Op (), prev_index (pi), new_index (ni)
  {
    //  .. nothing yet ..
  }

  int prev_index, new_index;
};

EditLineStylesForm::EditLineStylesForm (QWidget *parent, lay::LayoutViewBase *view, const lay::LineStyles &styles)
  : QDialog (parent), db::Object (0),
    m_selected (-1), m_styles (styles), m_manager (true), mp_view (view)
{
  m_selection_changed_enabled = false;

  mp_ui = new Ui::EditLineStylesForm ();

  mp_ui->setupUi (this);

  mp_ui->w_spin_box->setValue (32);

  manager (& m_manager);
  mp_ui->editor->manager (& m_manager);
  m_styles.manager (& m_manager);

  update ();

  connect (mp_ui->style_items, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
           this, SLOT (sel_changed (QListWidgetItem *, QListWidgetItem *)));
  connect (mp_ui->style_items, SIGNAL (itemDoubleClicked(QListWidgetItem*)),
           this, SLOT (double_clicked (QListWidgetItem *)));
  connect (mp_ui->new_button, SIGNAL (clicked ()), this, SLOT (new_button_clicked ()));
  connect (mp_ui->delete_button, SIGNAL (clicked ()), this, SLOT (delete_button_clicked ()));
  connect (mp_ui->clone_button, SIGNAL (clicked ()), this, SLOT (clone_button_clicked ()));
  connect (mp_ui->up_button, SIGNAL (clicked ()), this, SLOT (up_button_clicked ()));
  connect (mp_ui->down_button, SIGNAL (clicked ()), this, SLOT (down_button_clicked ()));
  connect (mp_ui->invert_button, SIGNAL (clicked ()), this, SLOT (invert_button_clicked ()));
  connect (mp_ui->clear_button, SIGNAL (clicked ()), this, SLOT (clear_button_clicked ()));
  connect (mp_ui->fliph_button, SIGNAL (clicked ()), this, SLOT (fliph_button_clicked ()));
  connect (mp_ui->sleft_button, SIGNAL (clicked ()), this, SLOT (sleft_button_clicked ()));
  connect (mp_ui->sright_button, SIGNAL (clicked ()), this, SLOT (sright_button_clicked ()));
  connect (mp_ui->undo_button, SIGNAL (clicked ()), this, SLOT (undo_button_clicked ()));
  connect (mp_ui->redo_button, SIGNAL (clicked ()), this, SLOT (redo_button_clicked ()));
  connect (mp_ui->w_spin_box, SIGNAL (valueChanged (int)), this, SLOT (size_changed ()));

  connect (mp_ui->editor, SIGNAL (changed ()), this, SLOT (edited ()));
  connect (mp_ui->editor, SIGNAL (size_changed ()), this, SLOT (editor_size_changed ()));

  mp_ui->style_items->setCurrentItem (mp_ui->style_items->item (mp_ui->style_items->count () - 1));
  mp_ui->style_items->scrollToItem (mp_ui->style_items->currentItem ());
  update_current_item ();

  m_selection_changed_enabled = true;
}

EditLineStylesForm::~EditLineStylesForm ()
{
  m_styles.manager (0);
  mp_ui->editor->manager (0);
  manager (0);

  delete mp_ui;
  mp_ui = 0;
}

static 
QIcon icon_from_data (const LineStyleInfo &info)
{
  QBitmap bitmap = info.get_bitmap (36, 26);
  QIcon icon (bitmap);
#ifdef _WIN32
  //  Hint: On Windows, this is necessary:
  icon.addPixmap (bitmap, QIcon::Selected);
#endif
  return icon;
}

namespace {
  struct display_order
  {
    bool operator () (lay::LineStyles::iterator a, lay::LineStyles::iterator b)
    {
      return a->order_index () < b->order_index ();
    }
  };
}

void 
EditLineStylesForm::update ()
{
  bool en = m_selection_changed_enabled;
  m_selection_changed_enabled = false;

  int row = mp_ui->style_items->currentRow ();

  mp_ui->style_items->clear ();

  std::vector <lay::LineStyles::iterator> iters;
  for (lay::LineStyles::iterator i = m_styles.begin_custom (); i != m_styles.end (); ++i) {
    iters.push_back (i);
  }
  std::sort (iters.begin (), iters.end (), display_order ());

  QColor c0 = palette ().color (QPalette::Base);
  QColor c1 = palette ().color (QPalette::Text);
  QColor cdis ((c0.red () + c1.red ()) / 2, 
               (c0.green () + c1.green ()) / 2, 
               (c0.blue () + c1.blue ()) / 2);

  //  fill the list of stipple items
  for (lay::LineStyles::iterator i = m_styles.begin (); i != m_styles.begin_custom (); ++i) {
    std::string name (i->name ());
    if (name.empty ()) {
      name = tl::sprintf ("#%d", std::distance (m_styles.begin (), i));
    }
    QListWidgetItem *item = new QListWidgetItem (icon_from_data (*i), tl::to_qstring (name), mp_ui->style_items);
    item->setForeground (cdis);
  }
  for (std::vector <lay::LineStyles::iterator>::const_iterator i = iters.begin (); i != iters.end (); ++i) {
    if ((*i)->order_index () > 0) {
      std::string name ((*i)->name ());
      if (name.empty ()) {
        name = tl::sprintf ("custom #%d", (*i)->order_index ());
      }
      new QListWidgetItem (icon_from_data (**i), tl::to_qstring (name), mp_ui->style_items);
    }
  }

  if (row >= mp_ui->style_items->count ()) {
    row = mp_ui->style_items->count () - 1;
  }
  mp_ui->style_items->setCurrentRow (row);

  m_selection_changed_enabled = en;
}

void
EditLineStylesForm::double_clicked (QListWidgetItem *citem)
{
  lay::LineStyles::iterator i = index_of (citem);
  if (i != m_styles.end () && i >= m_styles.begin_custom ()) {
    bool ok = false;
    QString new_name = QInputDialog::getText (this, 
                                              QObject::tr ("Edit Style Description"),
                                              QObject::tr ("Enter new description of style"),
                                              QLineEdit::Normal, tl::to_qstring (i->name ()), &ok);
    if (ok) {
      lay::LineStyleInfo p (*i);
      p.set_name (tl::to_string (new_name));
      m_styles.replace_style (std::distance (m_styles.begin (), i), p);
      update ();
    }
  }
}

void 
EditLineStylesForm::sel_changed (QListWidgetItem *, QListWidgetItem *)
{
  if (! m_selection_changed_enabled) {
    return;
  }

  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Current style")));
    manager ()->queue (this, new CurrentStyleOp (m_selected, mp_ui->style_items->currentRow ()));
    manager ()->commit ();
  }

  update_current_item ();
}

void
EditLineStylesForm::update_current_item ()
{
  mp_ui->w_spin_box->blockSignals (true);

  lay::LineStyles::iterator i = index_of (mp_ui->style_items->currentItem ());
  if (i == m_styles.end ()) {

    m_selected = -1;
    mp_ui->editor->set_style (*lay::LineStyleInfo ().pattern (), 32);
    mp_ui->editor->set_readonly (true);
    mp_ui->toolbar->setEnabled (false);
    mp_ui->w_spin_box->setValue (32);

  } else {

    mp_ui->editor->set_style (*i->pattern (), i->width ());
    bool readonly = (i < m_styles.begin_custom ());
    mp_ui->editor->set_readonly (readonly);
    mp_ui->toolbar->setEnabled (!readonly);
    mp_ui->w_spin_box->setValue (i->width ());

    m_selected = std::distance (m_styles.begin (), i);

  }

  mp_ui->w_spin_box->blockSignals (false);
}

void
EditLineStylesForm::select_item (int index)
{
  bool en = m_selection_changed_enabled;
  m_selection_changed_enabled = false;

  mp_ui->style_items->setCurrentItem (mp_ui->style_items->item (index));

  manager ()->queue (this, new CurrentStyleOp (m_selected, index));

  update_current_item ();

  m_selection_changed_enabled = en;
}

void
EditLineStylesForm::new_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("New style")));
  }

  lay::LineStyleInfo s;
  s.set_pattern (0x55555555, 32);
  unsigned int oi = m_styles.begin ()[m_styles.add_style (s)].order_index () - 1;

  update ();
  select_item (oi + std::distance (m_styles.begin (), m_styles.begin_custom ()));

  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::clone_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Clone style")));
  }

  lay::LineStyles::iterator c = current ();

  unsigned int oi = 0;
  lay::LineStyles::iterator iempty = m_styles.end ();
  for (lay::LineStyles::iterator i = m_styles.begin_custom (); i != m_styles.end (); ++i) {
    if (i->order_index () == 0) {
      iempty = i;
    } else if (i->order_index () > oi) {
      oi = i->order_index ();
    } 
  }

  lay::LineStyleInfo s;
  if (c != m_styles.end ()) {
    s = *c;
  }
  s.set_order_index (oi + 1);
  s.set_name ("");
  m_styles.replace_style (std::distance (m_styles.begin (), iempty), s);

  update ();
  select_item (oi + std::distance (m_styles.begin (), m_styles.begin_custom ()));

  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::delete_button_clicked ()
{
  BEGIN_PROTECTED

  lay::LineStyles::iterator i = current ();

  if (i != m_styles.end () && i >= m_styles.begin_custom ()) {

    for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
      if (int (l->eff_line_style (true /*real*/)) == std::distance (m_styles.begin (), i)) {
        throw tl::Exception (tl::to_string (QObject::tr ("Cannot delete style: style is being used by layer '")) + l->display_string (mp_view, true /*real*/) + "'");
      }
    }

    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("Delete style")));
    }

    if (mp_ui->style_items->currentRow () + 1 == mp_ui->style_items->count ()) {
      select_item (mp_ui->style_items->currentRow () - 1);
    }

    lay::LineStyleInfo info;
    m_styles.replace_style (std::distance (m_styles.begin (), i), info);

    m_styles.renumber ();
    update ();

    if (manager ()) {
      manager ()->commit ();
    }

  }

  END_PROTECTED
}

void 
EditLineStylesForm::up_button_clicked ()
{
  lay::LineStyles::iterator c = current ();

  if (c != m_styles.end () && c >= m_styles.begin_custom ()) {

    unsigned int oi = c->order_index ();
    if (oi > 1) {

      for (lay::LineStyles::iterator i = m_styles.begin_custom (); i != m_styles.end (); ++i) {
        if (i->order_index () == oi - 1) {

          if (manager ()) {
            manager ()->transaction (tl::to_string (QObject::tr ("Move style up")));
          }

          lay::LineStyleInfo info;
          info = *i;
          info.set_order_index (oi);
          m_styles.replace_style (std::distance (m_styles.begin (), i), info);

          info = *c;
          info.set_order_index (oi - 1);
          m_styles.replace_style (std::distance (m_styles.begin (), c), info);

          update ();
          select_item (oi - 2 + std::distance (m_styles.begin (), m_styles.begin_custom ()));

          if (manager ()) {
            manager ()->commit ();
          }

          return;

        }
      }

    }

  }
}

void 
EditLineStylesForm::down_button_clicked ()
{
  lay::LineStyles::iterator c = current ();

  if (c != m_styles.end () && c >= m_styles.begin_custom ()) {

    unsigned int oi = c->order_index ();

    for (lay::LineStyles::iterator i = m_styles.begin_custom (); i != m_styles.end (); ++i) {
      if (i->order_index () == oi + 1) {

        if (manager ()) {
          manager ()->transaction (tl::to_string (QObject::tr ("Move style down")));
        }

        lay::LineStyleInfo info;
        info = *i;
        info.set_order_index (oi);
        m_styles.replace_style (std::distance (m_styles.begin (), i), info);

        info = *c;
        info.set_order_index (oi + 1);
        m_styles.replace_style (std::distance (m_styles.begin (), c), info);

        update ();
        select_item (oi + std::distance (m_styles.begin (), m_styles.begin_custom ()));

        if (manager ()) {
          manager ()->commit ();
        }

        return;

      }
    }

  }
}

void
EditLineStylesForm::editor_size_changed ()
{
  mp_ui->w_spin_box->blockSignals (true);
  mp_ui->w_spin_box->setValue (mp_ui->editor->sx ());
  mp_ui->w_spin_box->blockSignals (false);
}

void
EditLineStylesForm::size_changed ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Change style size")));
  }
  mp_ui->editor->set_size (mp_ui->w_spin_box->value ());
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::invert_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Invert style")));
  }
  mp_ui->editor->invert ();
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::clear_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Clear style")));
  }
  mp_ui->editor->clear ();
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::fliph_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Flip style")));
  }
  mp_ui->editor->fliph ();
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::sleft_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Shift left")));
  }
  mp_ui->editor->shift (-1);
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::sright_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Shift right")));
  }
  mp_ui->editor->shift (1);
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditLineStylesForm::undo_button_clicked ()
{
  m_manager.undo ();
  update ();
}

void 
EditLineStylesForm::redo_button_clicked ()
{
  m_manager.redo ();
  update ();
}


lay::LineStyles::iterator
EditLineStylesForm::current ()
{
  return index_of (mp_ui->style_items->currentItem ());
}

lay::LineStyles::iterator
EditLineStylesForm::index_of (QListWidgetItem *item)
{
  int row = mp_ui->style_items->row (item);

  if (row >= std::distance (m_styles.begin (), m_styles.begin_custom ())) {
    for (lay::LineStyles::iterator i = m_styles.begin_custom (); i != m_styles.end (); ++i) {
      if (int (i->order_index ()) - 1 + std::distance (m_styles.begin (), m_styles.begin_custom ()) == row) {
        return i;
      }
    }
  } else if (row >= 0) {
    return m_styles.begin () + row;
  }
  
  return m_styles.end ();
}

void
EditLineStylesForm::edited ()
{
  if (mp_ui->style_items->currentItem ()) {

    lay::LineStyles::iterator i = current ();

    if (i != m_styles.end () && i >= m_styles.begin_custom ()) {

      lay::LineStyleInfo info (*i);
      info.set_pattern (mp_ui->editor->style (), mp_ui->editor->sx ());
      m_styles.replace_style (std::distance (m_styles.begin (), i), info);

      mp_ui->style_items->currentItem ()->setIcon (icon_from_data (info));

    }

  }
}

void 
EditLineStylesForm::handle_op (db::Op *op, bool undo)
{
  CurrentStyleOp *cp_op = dynamic_cast<CurrentStyleOp *> (op);
  if (cp_op) {

    m_selection_changed_enabled = false;

    update ();

    mp_ui->style_items->setCurrentItem (mp_ui->style_items->item (undo ? cp_op->prev_index : cp_op->new_index));
    update_current_item ();

    m_selection_changed_enabled = true;

  }
}

void
EditLineStylesForm::undo (db::Op *op)
{
  handle_op (op, true);
}

void
EditLineStylesForm::redo (db::Op *op)
{
  handle_op (op, false);
}

}

#endif
