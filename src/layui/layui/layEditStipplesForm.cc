
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include "layEditStipplesForm.h"
#include "ui_EditStipplesForm.h"

#include "layLayoutViewBase.h"
#include "tlExceptions.h"

#include "tlString.h"

#include <QBitmap>
#include <QInputDialog>

#include <algorithm>

namespace lay
{

struct CurrentPatternOp
  : public db::Op
{
  CurrentPatternOp (int pi, int ni)
    : db::Op (), prev_index (pi), new_index (ni)
  {
    //  .. nothing yet ..
  }

  int prev_index, new_index;
};

EditStipplesForm::EditStipplesForm (QWidget *parent, lay::LayoutViewBase *view, const lay::DitherPattern &pattern)
  : QDialog (parent), db::Object (0),
    m_selected (-1), m_pattern (pattern), m_manager (true), mp_view (view)
{
  m_selection_changed_enabled = false;

  mp_ui = new Ui::EditStipplesForm ();

  mp_ui->setupUi (this);

  mp_ui->h_spin_box->setValue (32);
  mp_ui->w_spin_box->setValue (32);

  manager (& m_manager);
  mp_ui->editor->manager (& m_manager);
  m_pattern.manager (& m_manager);

  update ();

  connect (mp_ui->stipple_items, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)), 
           this, SLOT (sel_changed (QListWidgetItem *, QListWidgetItem *)));
  connect (mp_ui->stipple_items, SIGNAL (itemDoubleClicked(QListWidgetItem*)), 
           this, SLOT (double_clicked (QListWidgetItem *)));
  connect (mp_ui->new_button, SIGNAL (clicked ()), this, SLOT (new_button_clicked ()));
  connect (mp_ui->delete_button, SIGNAL (clicked ()), this, SLOT (delete_button_clicked ()));
  connect (mp_ui->clone_button, SIGNAL (clicked ()), this, SLOT (clone_button_clicked ()));
  connect (mp_ui->up_button, SIGNAL (clicked ()), this, SLOT (up_button_clicked ()));
  connect (mp_ui->down_button, SIGNAL (clicked ()), this, SLOT (down_button_clicked ()));
  connect (mp_ui->invert_button, SIGNAL (clicked ()), this, SLOT (invert_button_clicked ()));
  connect (mp_ui->clear_button, SIGNAL (clicked ()), this, SLOT (clear_button_clicked ()));
  connect (mp_ui->rotate_button, SIGNAL (clicked ()), this, SLOT (rotate_button_clicked ()));
  connect (mp_ui->fliph_button, SIGNAL (clicked ()), this, SLOT (fliph_button_clicked ()));
  connect (mp_ui->flipv_button, SIGNAL (clicked ()), this, SLOT (flipv_button_clicked ()));
  connect (mp_ui->sleft_button, SIGNAL (clicked ()), this, SLOT (sleft_button_clicked ()));
  connect (mp_ui->sright_button, SIGNAL (clicked ()), this, SLOT (sright_button_clicked ()));
  connect (mp_ui->sup_button, SIGNAL (clicked ()), this, SLOT (sup_button_clicked ()));
  connect (mp_ui->sdown_button, SIGNAL (clicked ()), this, SLOT (sdown_button_clicked ()));
  connect (mp_ui->undo_button, SIGNAL (clicked ()), this, SLOT (undo_button_clicked ()));
  connect (mp_ui->redo_button, SIGNAL (clicked ()), this, SLOT (redo_button_clicked ()));
  connect (mp_ui->h_spin_box, SIGNAL (valueChanged (int)), this, SLOT (size_changed ()));
  connect (mp_ui->w_spin_box, SIGNAL (valueChanged (int)), this, SLOT (size_changed ()));

  connect (mp_ui->editor, SIGNAL (changed ()), this, SLOT (edited ()));
  connect (mp_ui->editor, SIGNAL (size_changed ()), this, SLOT (editor_size_changed ()));

  mp_ui->stipple_items->setCurrentItem (mp_ui->stipple_items->item (mp_ui->stipple_items->count () - 1));
  mp_ui->stipple_items->scrollToItem (mp_ui->stipple_items->currentItem ());
  update_current_item ();

  m_selection_changed_enabled = true;
}

EditStipplesForm::~EditStipplesForm ()
{
  m_pattern.manager (0);
  mp_ui->editor->manager (0);
  manager (0);

  delete mp_ui;
  mp_ui = 0;
}

static 
QIcon icon_from_data (const uint32_t * const *p)
{
  unsigned char data [5 * 36];
  memset (data, 0x00, sizeof (data));

  for (unsigned int i = 1; i < 35; ++i) {
    for (unsigned int j = 0; j < 5; ++j) {
      data [i * 5 + j] = 0xff;
    }
  }

  for (unsigned int i = 0; i < 32; ++i) {
    uint32_t w = *(p [32 - 1 - i]);
    for (unsigned int j = 0; j < 32; ++j) {
      if (! (w & (1 << j))) {
        data [5 * (i + 2) + (j + 1) / 8] &= ~(1 << ((j + 1) % 8));
      }
    }
  }

  QBitmap bitmap (QBitmap::fromData (QSize (34, 36), data, QImage::Format_MonoLSB));
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
    bool operator () (lay::DitherPattern::iterator a, lay::DitherPattern::iterator b)
    {
      return a->order_index () < b->order_index ();
    }
  };
}

void 
EditStipplesForm::update ()
{
  bool en = m_selection_changed_enabled;
  m_selection_changed_enabled = false;

  int row = mp_ui->stipple_items->currentRow ();

  mp_ui->stipple_items->clear ();

  std::vector <lay::DitherPattern::iterator> iters; 
  for (lay::DitherPattern::iterator i = m_pattern.begin_custom (); i != m_pattern.end (); ++i) {
    iters.push_back (i);
  }
  std::sort (iters.begin (), iters.end (), display_order ());

  QColor c0 = palette ().color (QPalette::Base);
  QColor c1 = palette ().color (QPalette::Text);
  QColor cdis ((c0.red () + c1.red ()) / 2, 
               (c0.green () + c1.green ()) / 2, 
               (c0.blue () + c1.blue ()) / 2);

  //  fill the list of stipple items
  for (lay::DitherPattern::iterator i = m_pattern.begin (); i != m_pattern.begin_custom (); ++i) {
    std::string name (i->name ());
    if (name.empty ()) {
      name = tl::sprintf ("#%d", std::distance (m_pattern.begin (), i));
    }
    QListWidgetItem *item = new QListWidgetItem (icon_from_data (i->pattern ()), tl::to_qstring (name), mp_ui->stipple_items);
#if QT_VERSION >= 0x60000
    item->setForeground (cdis);
#else
    item->setTextColor (cdis);
#endif
  }
  for (std::vector <lay::DitherPattern::iterator>::const_iterator i = iters.begin (); i != iters.end (); ++i) {
    if ((*i)->order_index () > 0) {
      std::string name ((*i)->name ());
      if (name.empty ()) {
        name = tl::sprintf ("custom #%d", (*i)->order_index ());
      }
      new QListWidgetItem (icon_from_data ((*i)->pattern ()), tl::to_qstring (name), mp_ui->stipple_items);
    }
  }

  if (row >= mp_ui->stipple_items->count ()) {
    row = mp_ui->stipple_items->count () - 1;
  }
  mp_ui->stipple_items->setCurrentRow (row);

  m_selection_changed_enabled = en;
}

void
EditStipplesForm::double_clicked (QListWidgetItem *citem)
{
  lay::DitherPattern::iterator i = index_of (citem);
  if (i != m_pattern.end () && i >= m_pattern.begin_custom ()) {
    bool ok = false;
    QString new_name = QInputDialog::getText (this, 
                                              QObject::tr ("Edit Stipple Description"),
                                              QObject::tr ("Enter new description of pattern"),
                                              QLineEdit::Normal, tl::to_qstring (i->name ()), &ok);
    if (ok) {
      lay::DitherPatternInfo p (*i);
      p.set_name (tl::to_string (new_name));
      m_pattern.replace_pattern (std::distance (m_pattern.begin (), i), p); 
      update ();
    }
  }
}

void 
EditStipplesForm::sel_changed (QListWidgetItem *, QListWidgetItem *)
{
  if (! m_selection_changed_enabled) {
    return;
  }

  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Current pattern")));
    manager ()->queue (this, new CurrentPatternOp (m_selected, mp_ui->stipple_items->currentRow ()));
    manager ()->commit ();
  }

  update_current_item ();
}

void
EditStipplesForm::update_current_item ()
{
  mp_ui->w_spin_box->blockSignals (true);
  mp_ui->h_spin_box->blockSignals (true);

  lay::DitherPattern::iterator i = index_of (mp_ui->stipple_items->currentItem ());
  if (i == m_pattern.end ()) {

    m_selected = -1;
    mp_ui->editor->set_pattern (lay::DitherPatternInfo ().pattern (), 32, 32);
    mp_ui->editor->set_readonly (true);
    mp_ui->toolbar->setEnabled (false);
    mp_ui->w_spin_box->setValue (32);
    mp_ui->h_spin_box->setValue (32);

  } else {

    mp_ui->editor->set_pattern (i->pattern (), i->width (), i->height ());
    bool readonly = (i < m_pattern.begin_custom ());
    mp_ui->editor->set_readonly (readonly);
    mp_ui->toolbar->setEnabled (!readonly);
    mp_ui->w_spin_box->setValue (i->width ());
    mp_ui->h_spin_box->setValue (i->height ());

    m_selected = std::distance (m_pattern.begin (), i);

  }

  mp_ui->w_spin_box->blockSignals (false);
  mp_ui->h_spin_box->blockSignals (false);
}

void
EditStipplesForm::select_item (int index)
{
  bool en = m_selection_changed_enabled;
  m_selection_changed_enabled = false;

  mp_ui->stipple_items->setCurrentItem (mp_ui->stipple_items->item (index));
  mp_ui->stipple_items->scrollToItem (mp_ui->stipple_items->currentItem ());

  if (manager ()) {
    manager ()->queue (this, new CurrentPatternOp (m_selected, index));
  }

  update_current_item ();

  m_selection_changed_enabled = en;
}

void
EditStipplesForm::new_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("New pattern")));
  }

  lay::DitherPatternInfo p;
  unsigned int oi = m_pattern.begin ()[m_pattern.add_pattern (p)].order_index () - 1;

  update ();
  select_item (oi + std::distance (m_pattern.begin (), m_pattern.begin_custom ()));

  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::clone_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Clone pattern")));
  }

  lay::DitherPattern::iterator c = current ();

  unsigned int oi = 0;
  lay::DitherPattern::iterator iempty = m_pattern.end ();
  for (lay::DitherPattern::iterator i = m_pattern.begin_custom (); i != m_pattern.end (); ++i) {
    if (i->order_index () == 0) {
      iempty = i;
    } else if (i->order_index () > oi) {
      oi = i->order_index ();
    } 
  }

  lay::DitherPatternInfo p;
  if (c != m_pattern.end ()) {
    p = *c;
  }
  p.set_order_index (oi + 1);
  p.set_name ("");
  m_pattern.replace_pattern (std::distance (m_pattern.begin (), iempty), p); 

  update ();
  select_item (oi + std::distance (m_pattern.begin (), m_pattern.begin_custom ()));

  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::delete_button_clicked ()
{
  BEGIN_PROTECTED

  lay::DitherPattern::iterator i = current ();

  if (i != m_pattern.end () && i >= m_pattern.begin_custom ()) {

    for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
      if (int (l->eff_dither_pattern (true /*real*/)) == std::distance (m_pattern.begin (), i)) {
        throw tl::Exception (tl::to_string (QObject::tr ("Cannot delete stipple: stipple is being used by layer '")) + l->display_string (mp_view, true /*real*/) + "'");
      }
    }

    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("Delete pattern")));
    }

    if (mp_ui->stipple_items->currentRow () + 1 == mp_ui->stipple_items->count ()) {
      select_item (mp_ui->stipple_items->currentRow () - 1);
    }

    lay::DitherPatternInfo info;
    m_pattern.replace_pattern (std::distance (m_pattern.begin (), i), info);

    m_pattern.renumber ();
    update ();

    if (manager ()) {
      manager ()->commit ();
    }

  }

  END_PROTECTED
}

void 
EditStipplesForm::up_button_clicked ()
{
  lay::DitherPattern::iterator c = current ();

  if (c != m_pattern.end () && c >= m_pattern.begin_custom ()) {

    unsigned int oi = c->order_index ();
    if (oi > 1) {

      for (lay::DitherPattern::iterator i = m_pattern.begin_custom (); i != m_pattern.end (); ++i) {
        if (i->order_index () == oi - 1) {

          if (manager ()) {
            manager ()->transaction (tl::to_string (QObject::tr ("Move pattern up")));
          }

          lay::DitherPatternInfo info;
          info = *i;
          info.set_order_index (oi);
          m_pattern.replace_pattern (std::distance (m_pattern.begin (), i), info);

          info = *c;
          info.set_order_index (oi - 1);
          m_pattern.replace_pattern (std::distance (m_pattern.begin (), c), info);

          update ();
          select_item (oi - 2 + std::distance (m_pattern.begin (), m_pattern.begin_custom ()));

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
EditStipplesForm::down_button_clicked ()
{
  lay::DitherPattern::iterator c = current ();

  if (c != m_pattern.end () && c >= m_pattern.begin_custom ()) {

    unsigned int oi = c->order_index ();

    for (lay::DitherPattern::iterator i = m_pattern.begin_custom (); i != m_pattern.end (); ++i) {
      if (i->order_index () == oi + 1) {

        if (manager ()) {
          manager ()->transaction (tl::to_string (QObject::tr ("Move pattern down")));
        }

        lay::DitherPatternInfo info;
        info = *i;
        info.set_order_index (oi);
        m_pattern.replace_pattern (std::distance (m_pattern.begin (), i), info);

        info = *c;
        info.set_order_index (oi + 1);
        m_pattern.replace_pattern (std::distance (m_pattern.begin (), c), info);

        update ();
        select_item (oi + std::distance (m_pattern.begin (), m_pattern.begin_custom ()));

        if (manager ()) {
          manager ()->commit ();
        }

        return;

      }
    }

  }
}

void
EditStipplesForm::editor_size_changed ()
{
  mp_ui->w_spin_box->blockSignals (true);
  mp_ui->h_spin_box->blockSignals (true);
  mp_ui->w_spin_box->setValue (mp_ui->editor->sx ());
  mp_ui->h_spin_box->setValue (mp_ui->editor->sy ());
  mp_ui->w_spin_box->blockSignals (false);
  mp_ui->h_spin_box->blockSignals (false);
}

void
EditStipplesForm::size_changed ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Change pattern size")));
  }
  mp_ui->editor->set_size (mp_ui->w_spin_box->value (), mp_ui->h_spin_box->value ());
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::invert_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Invert pattern")));
  }
  mp_ui->editor->invert ();
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::clear_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Clear pattern")));
  }
  mp_ui->editor->clear ();
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::rotate_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Rotate pattern")));
  }
  mp_ui->editor->rotate (90);
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::fliph_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Flip horizontal")));
  }
  mp_ui->editor->fliph ();
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::flipv_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Flip vertical")));
  }
  mp_ui->editor->flipv ();
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::sleft_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Shift left")));
  }
  mp_ui->editor->shift (-1, 0);
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::sup_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Shift up")));
  }
  mp_ui->editor->shift (0, 1);
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::sright_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Shift right")));
  }
  mp_ui->editor->shift (1, 0);
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::sdown_button_clicked ()
{
  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Shift down")));
  }
  mp_ui->editor->shift (0, -1);
  if (manager ()) {
    manager ()->commit ();
  }
}

void 
EditStipplesForm::undo_button_clicked ()
{
  m_manager.undo ();
  update ();
}

void 
EditStipplesForm::redo_button_clicked ()
{
  m_manager.redo ();
  update ();
}


lay::DitherPattern::iterator 
EditStipplesForm::current ()
{
  return index_of (mp_ui->stipple_items->currentItem ());
}

lay::DitherPattern::iterator 
EditStipplesForm::index_of (QListWidgetItem *item)
{
  int row = mp_ui->stipple_items->row (item);

  if (row >= std::distance (m_pattern.begin (), m_pattern.begin_custom ())) {
    for (lay::DitherPattern::iterator i = m_pattern.begin_custom (); i != m_pattern.end (); ++i) {
      if (int (i->order_index ()) - 1 + std::distance (m_pattern.begin (), m_pattern.begin_custom ()) == row) {
        return i;
      }
    }
  } else if (row >= 0) {
    return m_pattern.begin () + row;
  }
  
  return m_pattern.end ();
}

void
EditStipplesForm::edited ()
{
  if (mp_ui->stipple_items->currentItem ()) {

    lay::DitherPattern::iterator i = current ();

    if (i != m_pattern.end () && i >= m_pattern.begin_custom ()) {

      lay::DitherPatternInfo info (*i);
      info.set_pattern (mp_ui->editor->pattern (), mp_ui->editor->sx (), mp_ui->editor->sy ());
      m_pattern.replace_pattern (std::distance (m_pattern.begin (), i), info);

      mp_ui->stipple_items->currentItem ()->setIcon (icon_from_data (info.pattern ()));

    }

  }
}

void 
EditStipplesForm::handle_op (db::Op *op, bool undo)
{
  CurrentPatternOp *cp_op = dynamic_cast<CurrentPatternOp *> (op);
  if (cp_op) {

    m_selection_changed_enabled = false;

    update ();

    mp_ui->stipple_items->setCurrentItem (mp_ui->stipple_items->item (undo ? cp_op->prev_index : cp_op->new_index));
    update_current_item ();

    m_selection_changed_enabled = true;

  }
}

void
EditStipplesForm::undo (db::Op *op)
{
  handle_op (op, true);
}

void
EditStipplesForm::redo (db::Op *op)
{
  handle_op (op, false);
}

}

#endif
