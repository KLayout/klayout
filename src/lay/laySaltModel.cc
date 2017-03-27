
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

  bool is_enabled = (optionV4.state & QStyle::State_Enabled);
  optionV4.state |= QStyle::State_Enabled;

  QStyle *style = optionV4.widget ? optionV4.widget->style () : QApplication::style ();

  QTextDocument doc;
  doc.setHtml (optionV4.text);

  optionV4.text = QString ();
  style->drawControl (QStyle::CE_ItemViewItem, &optionV4, painter);

  QAbstractTextDocumentLayout::PaintContext ctx;

  if (optionV4.state & QStyle::State_Selected) {
    ctx.palette.setColor (QPalette::Text, optionV4.palette.color (QPalette::Active, QPalette::HighlightedText));
  } else if (! is_enabled) {
    ctx.palette.setColor (QPalette::Text, optionV4.palette.color (QPalette::Disabled, QPalette::Text));
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
  create_ordered_list ();
}

Qt::ItemFlags
SaltModel::flags (const QModelIndex &index) const
{
  Qt::ItemFlags f = QAbstractItemModel::flags (index);

  const lay::SaltGrain *g = grain_from_index (index);
  if (g && ! is_enabled (g->name ())) {
    f &= ~Qt::ItemIsSelectable;
    f &= ~Qt::ItemIsEnabled;
  }

  return f;
}

QVariant
SaltModel::data (const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {

    const lay::SaltGrain *g = grain_from_index (index);
    if (!g) {
      return QVariant ();
    }

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

    std::map<std::string, std::pair<Severity, std::string> >::const_iterator m = m_messages.find (g->name ());
    if (m != m_messages.end ()) {
      if (m->second.first == Warning || m->second.first == Error) {
        text += "<p><font color=\"#ff0000\"><b>" + tl::escaped_to_html (m->second.second) + "</b></font></p>";
      } else if (m->second.first == Info) {
        text += "<p><font color=\"#c0c0c0\">" + tl::escaped_to_html (m->second.second) + "</font></p>";
      } else {
        text += "<p>" + tl::escaped_to_html (m->second.second) + "</p>";
      }
    }

    text += "</body></html>";

    return tl::to_qstring (text);

  } else if (role == Qt::DecorationRole) {

    int icon_dim = 64;

    const lay::SaltGrain *g = grain_from_index (index);
    if (!g) {
      return QVariant ();
    }

    QImage img;
    if (g->icon ().isNull ()) {
      img = QImage (":/salt_icon.png");
    } else {
      img = g->icon ();
    }

    if (img.width () != icon_dim || img.height () != icon_dim) {

      QImage scaled = img.scaled (QSize (icon_dim, icon_dim), Qt::KeepAspectRatio, Qt::SmoothTransformation);

      img = QImage (icon_dim, icon_dim, QImage::Format_ARGB32);
      img.fill (QColor (0, 0, 0, 0));
      QPainter painter (&img);
      painter.drawImage ((icon_dim - scaled.width ()) / 2, (icon_dim - scaled.height ()) / 2, scaled);

    }

    if (m_marked.find (g->name ()) != m_marked.end ()) {
      QPainter painter (&img);
      QImage warn (":/marked_64.png");
      painter.drawImage (0, 0, warn);
    }

    std::map<std::string, std::pair<Severity, std::string> >::const_iterator m = m_messages.find (g->name ());
    if (m != m_messages.end ()) {
      if (m->second.first == Warning) {
        QPainter painter (&img);
        QImage warn (":/warn_16.png");
        painter.drawImage (0, 0, warn);
      } else if (m->second.first == Error) {
        QPainter painter (&img);
        QImage warn (":/error_16.png");
        painter.drawImage (0, 0, warn);
      } else if (m->second.first == Info) {
        QPainter painter (&img);
        QImage warn (":/info_16.png");
        painter.drawImage (0, 0, warn);
      }
    }

    return QPixmap::fromImage (img);

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
    return createIndex (row, column, m_ordered_grains [row]);
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
    return int (m_ordered_grains.size ());
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

bool
SaltModel::is_marked (const std::string &name) const
{
  return m_marked.find (name) != m_marked.end ();
}

bool
SaltModel::is_enabled (const std::string &name) const
{
  return m_disabled.find (name) == m_disabled.end ();
}

void
SaltModel::set_marked (const std::string &name, bool marked)
{
  if (marked != is_marked (name)) {
    if (! marked) {
      m_marked.erase (name);
    } else {
      m_marked.insert (name);
    }
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, 0, QModelIndex ()));
  }
}

void
SaltModel::clear_marked ()
{
  if (! m_marked.empty ()) {
    m_marked.clear ();
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, 0, QModelIndex ()));
  }
}

void
SaltModel::set_enabled (const std::string &name, bool enabled)
{
  if (enabled != is_enabled (name)) {
    if (enabled) {
      m_disabled.erase (name);
    } else {
      m_disabled.insert (name);
    }
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, 0, QModelIndex ()));
  }
}

void
SaltModel::enable_all ()
{
  if (! m_disabled.empty ()) {
    m_disabled.clear ();
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, 0, QModelIndex ()));
  }
}

void
SaltModel::clear_order ()
{
  m_display_order.clear ();
}

void
SaltModel::reset_order (const std::string &name)
{
  m_display_order.erase (name);
}

void
SaltModel::set_order (const std::string &name, int order)
{
  m_display_order[name] = order;
}

void
SaltModel::set_message (const std::string &name, Severity severity, const std::string &message)
{
  bool needs_update = false;
  if (message.empty ()) {
    if (m_messages.find (name) != m_messages.end ()) {
      m_messages.erase (name);
      needs_update = true;
    }
  } else {
    std::map<std::string, std::pair<Severity, std::string> >::iterator m = m_messages.find (name);
    if (m == m_messages.end () || m->second.second != message || m->second.first != severity) {
      m_messages.insert (std::make_pair (name, std::make_pair (severity, message)));
      needs_update = true;
    }
  }

  if (needs_update) {
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, 0, QModelIndex ()));
  }
}

void
SaltModel::clear_messages ()
{
  if (! m_messages.empty ()) {
    m_messages.clear ();
    emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex ()) - 1, 0, QModelIndex ()));
  }
}

void
SaltModel::update ()
{
  create_ordered_list ();
  reset ();
}

void
SaltModel::create_ordered_list ()
{
  m_ordered_grains.clear ();

  if (m_display_order.empty ()) {

    for (Salt::flat_iterator i = mp_salt->begin_flat (); i != mp_salt->end_flat (); ++i) {
      m_ordered_grains.push_back (*i);
    }

  } else {

    int min_order = m_display_order.begin ()->second;
    int max_order = min_order;
    min_order = std::min (min_order, 0);
    max_order = std::max (max_order, 0);

    for (std::map<std::string, int>::const_iterator i = m_display_order.begin (); i != m_display_order.end (); ++i) {
      min_order = std::min (min_order, i->second);
      max_order = std::max (max_order, i->second);
    }

    for (int o = min_order; o <= max_order; ++o) {
      for (Salt::flat_iterator i = mp_salt->begin_flat (); i != mp_salt->end_flat (); ++i) {
        std::map<std::string, int>::const_iterator d = m_display_order.find ((*i)->name ());
        int oi = 0;
        if (d != m_display_order.end ()) {
          oi = d->second;
        }
        if (oi == o) {
          m_ordered_grains.push_back (*i);
        }
      }
    }

  }
}

}
