
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

#ifndef HDR_layDragDropData
#define HDR_layDragDropData

#include "laybasicCommon.h"

#include "dbLayout.h"
#include "dbLibrary.h"

#include <QByteArray>
#include <QMimeData>

#include <vector>

namespace lay
{

LAYBASIC_PUBLIC const char *drag_drop_mime_type ();

/**
 *  @brief A helper class required to store the drag/drop data
 *
 *  Drag/drop data is basically a collection of key/value pairs.
 *  A category string is provided to identify the kind of data.
 */

class LAYBASIC_PUBLIC DragDropDataBase
{
public:
  /**
   *  @brief Default constructor
   */
  DragDropDataBase () { }

  /**
   *  @brief Dtor
   */
  virtual ~DragDropDataBase () { }

  /**
   *  @brief Serializes itself to an QByteArray
   */
  virtual QByteArray serialized () const = 0;

  /**
   *  @brief Try deserialization from an QByteArray
   *
   *  Returns false, if deserialization failed.
   */
  virtual bool deserialize (const QByteArray &ba) = 0;

  /**
   *  @brief Create a QMimeData object from the object
   */
  QMimeData *to_mime_data () const;
};

/**
 *  @brief Drag/drop data for a cell
 */

class LAYBASIC_PUBLIC CellDragDropData
  : public DragDropDataBase
{
public:
  /**
   *  @brief Default ctor
   */
  CellDragDropData ()
    : mp_layout (0), mp_library (0), m_cell_index (0), m_is_pcell (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specifies drag & drop of a cell
   *
   *  @param layout the layout where the cell lives in
   *  @param cell_index The index of the cell
   */
  CellDragDropData (const db::Layout *layout, const db::Library *library, db::cell_index_type cell_or_pcell_index, bool is_pcell, const std::vector<tl::Variant> &pcell_params = std::vector<tl::Variant> ())
    : mp_layout (layout), mp_library (library), m_cell_index (cell_or_pcell_index), m_is_pcell (is_pcell), m_pcell_params (pcell_params)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the layout object where the cell lives in
   */
  const db::Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Gets the layout object where the cell lives in
   */
  const db::Library *library () const
  {
    return mp_library;
  }

  /**
   *  @brief PCell parameters
   */
  const std::vector<tl::Variant> &pcell_params () const
  {
    return m_pcell_params;
  }

  /**
   *  @brief Gets the index of the cell
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Gets a value indicating whether the cell is a pcell
   */
  bool is_pcell () const
  {
    return m_is_pcell;
  }

  /**
   *  @brief Serializes itself to an QByteArray
   */
  virtual QByteArray serialized () const;

  /**
   *  @brief Try deserialization from an QByteArray
   *
   *  Returns false, if deserialization failed.
   */
  bool deserialize (const QByteArray &ba);

private:
  const db::Layout *mp_layout;
  const db::Library *mp_library;
  db::cell_index_type m_cell_index;
  bool m_is_pcell;
  std::vector<tl::Variant> m_pcell_params;
};

}

#endif

#endif  //  defined(HAVE_QT)
