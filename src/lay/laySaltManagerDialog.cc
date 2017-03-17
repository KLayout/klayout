
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

#include "laySaltManagerDialog.h"
#include "laySalt.h"
#include "tlString.h"

#include <QAbstractItemModel>
#include <QAbstractTextDocumentLayout>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QPainter>
#include <QDir>

namespace lay
{

class SaltModel
  : public QAbstractItemModel
{
public:
  SaltModel (QObject *parent, lay::Salt *salt)
    : QAbstractItemModel (parent), mp_salt (salt)
  {
    //  .. nothing yet ..
  }

  QVariant data (const QModelIndex &index, int role) const
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

    } else {
      return QVariant ();
    }
  }

  QModelIndex index (int row, int column, const QModelIndex &parent) const
  {
    if (parent.isValid ()) {
      return QModelIndex ();
    } else {
      return createIndex (row, column);
    }
  }

  QModelIndex parent (const QModelIndex & /*index*/) const
  {
    return QModelIndex ();
  }

  int columnCount(const QModelIndex & /*parent*/) const
  {
    return 1;
  }

  int rowCount (const QModelIndex &parent) const
  {
    if (parent.isValid ()) {
      return 0;
    } else {
      return mp_salt->end_flat () - mp_salt->begin_flat ();
    }
  }

public:
  lay::Salt *mp_salt;
};

class SaltItemDelegate
  : public QStyledItemDelegate
{
public:
  SaltItemDelegate (QObject *parent)
    : QStyledItemDelegate (parent)
  {
    // .. nothing yet ..
  }

  void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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

  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    const int textWidth = 500;

    QStyleOptionViewItemV4 optionV4 = option;
    initStyleOption (&optionV4, index);

    QTextDocument doc;
    doc.setHtml (optionV4.text);
    doc.setTextWidth (textWidth);
    return QSize (textWidth, doc.size ().height ());
  }
};

// @@@
lay::Salt salt;
static bool salt_initialized = false;
void make_salt ()
{
  if (!salt_initialized) {
    salt_initialized = true;
    salt.add_location (tl::to_string (QDir::homePath () + QString::fromUtf8("/.klayout/salt")));
  }
}
// @@@

SaltManagerDialog::SaltManagerDialog (QWidget *parent)
  : QDialog (parent)
{
  Ui::SaltManagerDialog::setupUi (this);

  salt = lay::Salt (); salt_initialized = false; // @@@
  make_salt (); // @@@
  salt_view->setModel (new SaltModel (this, &salt));
  salt_view->setItemDelegate (new SaltItemDelegate (this));

  // ...
}

}
