
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

#include "layItemDelegates.h"

#include <QApplication>
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QAbstractItemView>

namespace lay
{

// --------------------------------------------------------------------------------------

HTMLItemDelegate::HTMLItemDelegate (QObject *parent)
  : QStyledItemDelegate (parent)
{
  m_icon_margin = 6;
  m_icon_spacing = 6;
  m_text_margin = 4;
  m_text_height = -1;
  m_text_width = -1;
  m_plain_text = false;
  m_anchors_clickable = false;
}

void
HTMLItemDelegate::set_anchors_clickable (bool a)
{
  m_anchors_clickable = a;
}

void
HTMLItemDelegate::set_plain_text (bool pt)
{
  m_plain_text = pt;
}

void
HTMLItemDelegate::set_icon_margin (int m)
{
  m_icon_margin = m;
}

void
HTMLItemDelegate::set_icon_spacing (int s)
{
  m_icon_spacing = s;
}

void
HTMLItemDelegate::set_text_margin (int m)
{
  m_text_margin = m;
}

void
HTMLItemDelegate::set_text_height (int h)
{
  m_text_height = h;
}

void
HTMLItemDelegate::set_text_width (int w)
{
  m_text_width = w;
}

void
HTMLItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
#if QT_VERSION >= 0x60000
  QStyleOptionViewItem option_v4 = option;
#else
  QStyleOptionViewItemV4 option_v4 = option;
#endif
  initStyleOption (&option_v4, index);

  //  let the text take all the available space (fixes #144)
  option_v4.showDecorationSelected = true;

  bool is_enabled = (option_v4.state & QStyle::State_Enabled);
  if ((index.flags () & 0x10000) != 0) {
    //  the item wants to be drawn "disabled"
    is_enabled = false;
  }

  option_v4.state |= QStyle::State_Enabled;

  QStyle *style = option_v4.widget ? option_v4.widget->style () : QApplication::style ();

  QTextDocument doc;
  if (m_plain_text) {
    doc.setPlainText (option_v4.text);
  } else {
    doc.setHtml (option_v4.text);
  }
  doc.setTextWidth (m_text_width);
  doc.setDocumentMargin (m_text_margin);
  doc.setDefaultFont (option_v4.font);

  option_v4.text = QString ();
  style->drawControl (QStyle::CE_ItemViewItem, &option_v4, painter);

  QAbstractTextDocumentLayout::PaintContext ctx;

  if (option_v4.state & QStyle::State_Selected) {
    ctx.palette.setColor (QPalette::Text, option_v4.palette.color (QPalette::Active, QPalette::HighlightedText));
  } else if (! is_enabled) {
    ctx.palette.setColor (QPalette::Text, option_v4.palette.color (QPalette::Disabled, QPalette::Text));
  } else {
    ctx.palette.setColor (QPalette::Text, option_v4.palette.color (QPalette::Text));
  }

  QRect text_rect = style->subElementRect (QStyle::SE_ItemViewItemText, &option_v4);
  painter->save ();
  QPoint tr = text_rect.topLeft ();
  painter->translate (tr);
  painter->setClipRect (text_rect.translated (-tr));
  doc.documentLayout ()->draw (painter, ctx);
  painter->restore ();
}

QSize
HTMLItemDelegate::sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
{
#if QT_VERSION >= 0x60000
  QStyleOptionViewItem option_v4 = option;
#else
  QStyleOptionViewItemV4 option_v4 = option;
#endif
  initStyleOption (&option_v4, index);

  const QAbstractItemView *view = dynamic_cast<const QAbstractItemView *> (option_v4.widget);
  QSize icon_size (0, 0);
  if (view) {
    icon_size = view->iconSize ();
  }

  QTextDocument doc;
  if (m_plain_text) {
    doc.setPlainText (option_v4.text);
  } else {
    doc.setHtml (option_v4.text);
  }
  doc.setTextWidth (m_text_width);
  doc.setDocumentMargin (m_text_margin);
  bool has_icon = ! option_v4.icon.isNull ();
  int th = m_text_height < 0 ? int (doc.size ().height ()) : m_text_height;
  return QSize (m_text_width + (has_icon ? icon_size.width () + m_icon_spacing : 0), std::max (has_icon ? icon_size.height () + 2 * m_icon_margin : 0, th));
}

bool
HTMLItemDelegate::editorEvent (QEvent *event, QAbstractItemModel * /*model*/, const QStyleOptionViewItem &option, const QModelIndex &index)
{
  if ((event->type () == QEvent::MouseButtonRelease || event->type () == QEvent::MouseButtonPress) && ! m_plain_text && m_anchors_clickable) {

    QMouseEvent *mouse_event = static_cast<QMouseEvent *> (event);

#if QT_VERSION >= 0x60000
    QStyleOptionViewItem option_v4 = option;
#else
    QStyleOptionViewItemV4 option_v4 = option;
#endif
    initStyleOption (&option_v4, index);

    QTextDocument doc;
    doc.setHtml (option_v4.text);
    doc.setTextWidth (m_text_width);
    doc.setDocumentMargin (m_text_margin);

    QStyle *style = option_v4.widget ? option_v4.widget->style () : QApplication::style ();
    QRect text_rect = style->subElementRect (QStyle::SE_ItemViewItemText, &option_v4);

    QString a = doc.documentLayout ()->anchorAt (mouse_event->pos () - text_rect.topLeft ());
    if (! a.isNull ()) {
      if (event->type () == QEvent::MouseButtonRelease) {
        emit anchor_clicked (a);
      }
    }

  }

  return false;
}

}

#endif
