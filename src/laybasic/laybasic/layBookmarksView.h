
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#ifndef HDR_layBookmarksView
#define HDR_layBookmarksView

#include "laybasicCommon.h"

#include "layBookmarkList.h"

#include <QFrame>
#include <QListView>

#include <set>

namespace lay
{

class LayoutView;
class AbstractMenu;

/**
 *  @brief A widget to display a bookmark list
 */
class LAYBASIC_PUBLIC BookmarksView
  : public QFrame
{
Q_OBJECT

public:
  BookmarksView (LayoutView *view, QWidget *parent, const char *name);
  ~BookmarksView ();

  void set_background_color (QColor c);
  void set_text_color (QColor c);
  void follow_selection (bool f);

  std::set<size_t> selected_bookmarks ();

  void refresh ();

public slots:
  void bookmark_triggered (const QModelIndex &index);
  void current_bookmark_changed (const QModelIndex &index);
  void context_menu (const QPoint &p);

private:
  LayoutView *mp_view;
  QListView *mp_bookmarks;
  bool m_follow_selection;
};

} // namespace lay

#endif

