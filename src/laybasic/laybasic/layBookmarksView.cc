
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


#include "layBookmarksView.h"
#include "layLayoutView.h"
#include "layAbstractMenu.h"
#include "layAbstractMenuProvider.h"

#include "laybasicConfig.h"

#include <QVBoxLayout>

namespace lay
{

// --------------------------------------------------------------------------------------------

class BookmarkListModel
  : public QAbstractItemModel
{
public:
  BookmarkListModel (const lay::BookmarkList *bookmarks)
    : mp_bookmarks (bookmarks)
  {
    //  .. nothing yet ..
  }

  int rowCount (const QModelIndex &index) const
  {
    return index.isValid () ? 0 : int (mp_bookmarks->size ());
  }

  int columnCount (const QModelIndex &) const
  {
    return 1;
  }

  QVariant data (const QModelIndex &index, int role) const
  {
    if (role == Qt::DisplayRole && index.row () >= 0 && index.row () < int (mp_bookmarks->size ())) {
      return tl::to_qstring (mp_bookmarks->name (size_t (index.row ())));
    }

    return QVariant ();
  }

  QModelIndex index (int row, int column, const QModelIndex &parent) const
  {
    if (parent.isValid ()) {
      return QModelIndex ();
    } else {
      return createIndex (row, column);
    }
  }

  QModelIndex parent(const QModelIndex &) const
  {
    return QModelIndex ();
  }

  void refresh ()
  {
    dataChanged (createIndex (0, 0), createIndex (rowCount (QModelIndex ()), 1));
  }

private:
  const lay::BookmarkList *mp_bookmarks;
};

// --------------------------------------------------------------------------------------------

BookmarksView::BookmarksView (LayoutView *view, QWidget *parent, const char *name)
  : QFrame (parent), m_follow_selection (false)
{
  setObjectName (QString::fromUtf8 (name));

  mp_view = view;

  QVBoxLayout *layout = new QVBoxLayout ();
  layout->setMargin (0);
  setLayout (layout);

  mp_bookmarks = new QListView (this);
  layout->addWidget (mp_bookmarks);

  mp_bookmarks->setModel (new BookmarkListModel (&view->bookmarks ()));
  mp_bookmarks->setSelectionMode (QAbstractItemView::ExtendedSelection);
  mp_bookmarks->setContextMenuPolicy (Qt::CustomContextMenu);

  connect (mp_bookmarks, SIGNAL (customContextMenuRequested (const QPoint &)), this, SLOT (context_menu (const QPoint &)));
  connect (mp_bookmarks, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (bookmark_triggered (const QModelIndex &)));
  connect (mp_bookmarks->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_bookmark_changed (const QModelIndex &)));
}

BookmarksView::~BookmarksView ()
{
  //  .. nothing yet ..
}

std::set<size_t>
BookmarksView::selected_bookmarks ()
{
  QModelIndexList sel = mp_bookmarks->selectionModel ()->selectedIndexes ();
  std::set<size_t> res;
  for (QModelIndexList::const_iterator i = sel.begin (); i != sel.end (); ++i) {
    res.insert (int (i->row ()));
  }
  return res;
}

void
BookmarksView::follow_selection (bool f)
{
  m_follow_selection = f;
}

void
BookmarksView::init_menu (lay::AbstractMenu &menu)
{
  MenuLayoutEntry context_menu [] = {
    MenuLayoutEntry ("follow_selection",     tl::to_string (QObject::tr ("Follow Selection")),    std::make_pair (cfg_bookmarks_follow_selection, "?")),
    MenuLayoutEntry::separator ("ops_group"),
    MenuLayoutEntry ("manage_bookmarks",     tl::to_string (QObject::tr ("Manage Bookmarks")),    SLOT (cm_manage_bookmarks ())),
    MenuLayoutEntry ("load_bookmarks",       tl::to_string (QObject::tr ("Load Bookmarks")),      SLOT (cm_load_bookmarks ())),
    MenuLayoutEntry ("save_bookmarks",       tl::to_string (QObject::tr ("Save Bookmarks")),      SLOT (cm_save_bookmarks ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry main_menu [] = {
    MenuLayoutEntry ("@bookmarks_context_menu", "", context_menu),
    MenuLayoutEntry::last ()
  };

  menu.init (main_menu);
}

void
BookmarksView::set_background_color (QColor c)
{
  QPalette pl (mp_bookmarks->palette ());
  pl.setColor (QPalette::Base, c);
  mp_bookmarks->setPalette (pl);
}

void
BookmarksView::set_text_color (QColor c)
{
  QPalette pl (mp_bookmarks->palette ());
  pl.setColor (QPalette::Text, c);
  mp_bookmarks->setPalette (pl);
}

void
BookmarksView::refresh ()
{
  BookmarkListModel *model = dynamic_cast<BookmarkListModel *> (mp_bookmarks->model ());
  if (model) {
    model->refresh ();
  }
}

void
BookmarksView::context_menu (const QPoint &p)
{
  tl_assert (lay::AbstractMenuProvider::instance () != 0);

  QListView *bm_list = dynamic_cast<QListView *> (sender ());
  if (bm_list) {
    QMenu *ctx_menu = lay::AbstractMenuProvider::instance ()->menu ()->detached_menu ("bookmarks_context_menu");
    ctx_menu->exec (bm_list->mapToGlobal (p));
  }
}

void
BookmarksView::current_bookmark_changed (const QModelIndex &index)
{
  if (m_follow_selection) {
    bookmark_triggered (index);
  }
}

void
BookmarksView::bookmark_triggered (const QModelIndex &index)
{
  if (index.row () >= 0 && index.row () < int (mp_view->bookmarks ().size ())) {
    mp_view->goto_view (mp_view->bookmarks ().state (index.row ()));
  }
}

}

