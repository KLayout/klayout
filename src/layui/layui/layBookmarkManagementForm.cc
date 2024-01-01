
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

#include "layBookmarkManagementForm.h"
#include "dbCellInst.h"

#include "ui_BookmarkManagementForm.h"

#include <QListWidgetItem>

namespace lay
{

// ------------------------------------------------------------

class BookmarkListLVI 
  : public QListWidgetItem
{
public:
  BookmarkListLVI (QListWidget *parent, const std::string &name, const lay::DisplayState &state)
    : QListWidgetItem (tl::to_qstring (name), parent), m_state (state)
  {
    setFlags (flags () | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  const lay::DisplayState &state () const
  {
    return m_state;
  }

private:
  lay::DisplayState m_state;
};

// ------------------------------------------------------------

BookmarkManagementForm::BookmarkManagementForm (QWidget *parent, const char *name, const lay::BookmarkList &bookmarks, const std::set<size_t> &selected)
  : QDialog (parent),
    m_bookmarks (bookmarks)
{
  mp_ui = new Ui::BookmarkManagementForm ();
  setObjectName (QString::fromUtf8 (name));

  mp_ui->setupUi (this);

  QListWidgetItem *first_item = 0;

  for (size_t i = 0; i < m_bookmarks.size (); ++i) {
    QListWidgetItem *item = new BookmarkListLVI (mp_ui->bookmark_list, m_bookmarks.name (i), m_bookmarks.state (i));
    item->setSelected (selected.find (i) != selected.end ());
    if (! first_item && item->isSelected ()) {
      first_item = item;
    }
  }

  if (first_item) {
    mp_ui->bookmark_list->scrollToItem (first_item);
  }

  connect (mp_ui->delete_button, SIGNAL (clicked ()), this, SLOT (delete_pressed ()));
}

void
BookmarkManagementForm::delete_pressed ()
{
  QList<QListWidgetItem *> sel = mp_ui->bookmark_list->selectedItems ();
  for (QList<QListWidgetItem *>::const_iterator i = sel.begin (); i != sel.end (); ++i) {
    delete *i;
  }
}

void
BookmarkManagementForm::accept ()
{
  m_bookmarks.clear ();
  m_bookmarks.reserve (mp_ui->bookmark_list->count ());

  //  TODO: is there an iterator? Here we use the trick to select all and then get the
  //  list of items
  mp_ui->bookmark_list->selectAll ();
  QList<QListWidgetItem *> sel = mp_ui->bookmark_list->selectedItems ();
  for (QList<QListWidgetItem *>::const_iterator i = sel.begin (); i != sel.end (); ++i) {
    BookmarkListLVI *bm = dynamic_cast<BookmarkListLVI *> (*i);
    if (bm) {
      m_bookmarks.add (tl::to_string (bm->text ()), bm->state ());
    }
  }

  QDialog::accept ();
}

}

#endif

