
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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



#ifndef HDR_dbNetExtractor
#define HDR_dbNetExtractor

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbCellMapping.h"
#include "tlTypeTraits.h"

namespace db
{

class NetLayer
{
public:
  NetLayer (unsigned int index)
    : m_layer_index (index)
  {
    //  .. nothing yet ..
  }

  unsigned int layer_index () const
  {
    return m_layer_index;
  }

private:
  unsigned int m_layer_index;
};

/**
 *  @brief The net extractor
 *
 *  ...
 */
class DB_PLUGIN_PUBLIC NetExtractor
{
public:
  /**
   *  @brief Constructs a net extractor
   */
  NetExtractor ();

  ~NetExtractor ();

  // @@@
  void open (const db::Layout &orig_layout, db::cell_index_type orig_top_cell);
  NetLayer load (unsigned int layer_index);
  NetLayer bool_and (NetLayer a, NetLayer b);
  NetLayer bool_not (NetLayer a, NetLayer b);
  void output (NetLayer a, const db::LayerProperties &lp);
  db::Layout *layout_copy () const;

private:
  //  no copying
  NetExtractor (const db::NetExtractor &);
  NetExtractor &operator= (const db::NetExtractor &);

  NetLayer and_or_not (NetLayer a, NetLayer b, bool is_and);

  // @@@
  const db::Layout *mp_orig_layout; // @@@ should be a smart pointer
  db::Layout *mp_layout;
  db::Cell *mp_top_cell;
  db::CellMapping m_cm;
};

}

namespace tl
{

template <>
struct type_traits<db::NetLayer> : public tl::type_traits<void>
{
  //  mark "NetLayer" as not having a default ctor
  typedef tl::false_tag has_default_constructor;
};

template <>
struct type_traits<db::NetExtractor> : public tl::type_traits<void>
{
  //  mark "NetExtractor" as not copyable
  typedef tl::false_tag has_copy_constructor;
};

}

#endif

