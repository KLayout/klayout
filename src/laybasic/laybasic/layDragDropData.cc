
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

#include "layDragDropData.h"

#include <QDataStream>
#include <QIODevice>

namespace lay
{

// ---------------------------------------------------------------
//  Implementation of DragDropDataBase

const char *drag_drop_mime_type ()
{
  return "application/klayout-ddd";
}

QMimeData *
DragDropDataBase::to_mime_data () const
{
  QMimeData *mimeData = new QMimeData();
  mimeData->setData (QString::fromUtf8 (drag_drop_mime_type ()), serialized ());
  return mimeData;
}

// ---------------------------------------------------------------
//  Implementation of CellDragDropData

QByteArray
CellDragDropData::serialized () const
{
  QByteArray data;
  QDataStream stream (&data, QIODevice::WriteOnly);

  stream << QString::fromUtf8 ("CellDragDropData");
  stream << (quintptr) mp_layout;
  stream << (quintptr) mp_library;
  stream << m_cell_index;
  stream << m_is_pcell;
  stream << int (m_pcell_params.size ());
  for (std::vector<tl::Variant>::const_iterator i = m_pcell_params.begin (); i != m_pcell_params.end (); ++i) {
    stream << tl::to_qstring (i->to_parsable_string ());
  }

  return data;
}

bool
CellDragDropData::deserialize (const QByteArray &ba)
{
  QDataStream stream (const_cast<QByteArray *> (&ba), QIODevice::ReadOnly);

  QString tag;
  stream >> tag;

  if (tag == QString::fromUtf8 ("CellDragDropData")) {

    quintptr p = 0;
    stream >> p;
    mp_layout = reinterpret_cast <const db::Layout *> (p);
    stream >> p;
    mp_library = reinterpret_cast <const db::Library *> (p);
    stream >> m_cell_index;
    stream >> m_is_pcell;

    m_pcell_params.clear ();
    int n = 0;
    stream >> n;
    while (n-- > 0) {
      QString s;
      stream >> s;
      std::string stl_s = tl::to_string (s);
      tl::Extractor ex (stl_s.c_str ());
      m_pcell_params.push_back (tl::Variant ());
      ex.read (m_pcell_params.back ());
    }

    return true;

  } else {

    return false;

  }
}

}

#endif
