
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_layItemDelegates
#define HDR_layItemDelegates

#include "layuiCommon.h"

#include <QStyledItemDelegate>

namespace lay
{

// --------------------------------------------------------------------------------------

/**
 *  @brief A delegate displaying the display text as HTML formatted text
 */
class LAYUI_PUBLIC HTMLItemDelegate
  : public QStyledItemDelegate
{
Q_OBJECT

public:
  HTMLItemDelegate (QObject *parent);

  virtual void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  virtual QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const;
  virtual bool editorEvent (QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);

  void set_anchors_clickable (bool a);
  bool anchors_clickable () const
  {
    return m_anchors_clickable;
  }

  void set_icon_margin (int m);
  int icon_margin () const
  {
    return m_icon_margin;
  }

  void set_icon_spacing (int s);
  int icon_spacing () const
  {
    return m_icon_spacing;
  }

  void set_text_margin (int m);
  int text_margin () const
  {
    return m_text_margin;
  }

  void set_text_height (int h);
  int text_height () const
  {
    return m_text_height;
  }

  void set_text_width (int w);
  int text_width () const
  {
    return m_text_width;
  }

  void set_plain_text (bool pt);
  bool plain_text () const
  {
    return m_plain_text;
  }

signals:
  void anchor_clicked (const QString &url);

private:
  int m_icon_margin, m_icon_spacing, m_text_margin;
  int m_text_width, m_text_height;
  bool m_plain_text;
  bool m_anchors_clickable;
};


}

#endif

#endif  //  defined(HAVE_QT)
