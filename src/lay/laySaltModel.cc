
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "laySaltModel.h"
#include "laySalt.h"

#include <QIcon>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QListView>

namespace lay
{

// --------------------------------------------------------------------------------------

SaltItemDelegate::SaltItemDelegate (QObject *parent)
  : QStyledItemDelegate (parent)
{
  // .. nothing yet ..
}

void
SaltItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QStyleOptionViewItemV4 optionV4 = option;
  initStyleOption (&optionV4, index);

  QStyle *style = optionV4.widget ? optionV4.widget->style () : QApplication::style ();

  QTextDocument doc;
  doc.setHtml (optionV4.text);

  optionV4.text = QString ();
  style->drawControl (QStyle::CE_ItemViewItem, &optionV4, painter);

  QAbstractTextDocumentLayout::PaintContext ctx;

  if (optionV4.state & QStyle::State_Selected) {
    ctx.palette.setColor (QPalette::Text, optionV4.palette.color (QPalette::Active, QPalette::HighlightedText));
  }

  QRect textRect = style->subElementRect (QStyle::SE_ItemViewItemText, &optionV4);
  painter->save ();
  painter->translate (textRect.topLeft ());
  painter->setClipRect (textRect.translated (-textRect.topLeft ()));
  doc.documentLayout()->draw (painter, ctx);
  painter->restore ();
}

QSize
SaltItemDelegate::sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  const int textWidth = 500;

  QStyleOptionViewItemV4 optionV4 = option;
  initStyleOption (&optionV4, index);

  const QListView *view = dynamic_cast<const QListView *> (optionV4.widget);
  QSize icon_size (0, 0);
  if (view) {
    icon_size = view->iconSize ();
  }

  QTextDocument doc;
  doc.setHtml (optionV4.text);
  doc.setTextWidth (textWidth);
  return QSize (textWidth + icon_size.width () + 6, std::max (icon_size.height () + 12, int (doc.size ().height ())));
}

// --------------------------------------------------------------------------------------

SaltModel::SaltModel (QObject *parent, lay::Salt *salt)
  : QAbstractItemModel (parent), mp_salt (salt)
{
  //  .. nothing yet ..
}

QVariant 
SaltModel::data (const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {

    const lay::SaltGrain *g = mp_salt->begin_flat ()[index.row ()];

    std::string text = "<html><body>";
    text += "<h4>";
    text += tl::escaped_to_html (g->name ());
    if (!g->version ().empty ()) {
      text += " ";
      text += tl::escaped_to_html (g->version ());
    }
    if (!g->title ().empty ()) {
      text += " - ";
      text += tl::escaped_to_html (g->title ());
    }
    text += "</h4>";
    if (!g->doc ().empty ()) {
      text += "<p>";
      text += tl::escaped_to_html (g->doc ());
      text += "</p>";
    }
    text += "</body></html>";

    return tl::to_qstring (text);

  } else if (role == Qt::DecorationRole) {

    int icon_dim = 64;

    const lay::SaltGrain *g = mp_salt->begin_flat ()[index.row ()];
    if (g->icon ().isNull ()) {
      return QIcon (":/salt_icon.png");
    } else {

      QImage img = g->icon ();
      if (img.width () == icon_dim && img.height () == icon_dim) {
        return QPixmap::fromImage (img);
      } else {

        img = img.scaled (QSize (icon_dim, icon_dim), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QImage final_img (icon_dim, icon_dim, QImage::Format_ARGB32);
        final_img.fill (QColor (0, 0, 0, 0));
        QPainter painter (&final_img);
        painter.drawImage ((icon_dim - img.width ()) / 2, (icon_dim - img.height ()) / 2, img);

        return QPixmap::fromImage (final_img);

      }

    }

  } else {
    return QVariant ();
  }
}

QModelIndex 
SaltModel::index (int row, int column, const QModelIndex &parent) const
{
  if (parent.isValid ()) {
    return QModelIndex ();
  } else {
    return createIndex (row, column, mp_salt->begin_flat () [row]);
  }
}

QModelIndex 
SaltModel::parent (const QModelIndex & /*index*/) const
{
  return QModelIndex ();
}

int 
SaltModel::columnCount(const QModelIndex & /*parent*/) const
{
  return 1;
}

int 
SaltModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ()) {
    return 0;
  } else {
    return mp_salt->end_flat () - mp_salt->begin_flat ();
  }
}

SaltGrain *
SaltModel::grain_from_index (const QModelIndex &index) const
{
  if (index.isValid ()) {
    return static_cast<SaltGrain *> (index.internalPointer ());
  } else {
    return 0;
  }
}

void 
SaltModel::update ()
{
  reset ();
}

}
