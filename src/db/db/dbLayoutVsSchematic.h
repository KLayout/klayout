
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

#ifndef _HDR_dbLayoutVsSchematic
#define _HDR_dbLayoutVsSchematic

#include "dbCommon.h"
#include "dbLayoutToNetlist.h"
#include "dbNetlistCompare.h"
#include "dbNetlistCrossReference.h"

namespace db
{

/**
 *  @brief An extension of the LayoutToNetlist framework towards comparision vs. schematic
 *
 *  This aggregate holds the following entities in addition to the ones provided by
 *  the LayoutToNetlist entity:
 *
 *  * A reference netlist
 *  * A cross-reference object
 *
 *  The cross-reference object connects the extracted netlist with the reference netlist.
 *
 *  In addition to the steps required to create a LayoutToNetlist object, the following
 *  has to be provided:
 *
 *  * A reference netlist has to be loaded using "set_reference_netlist"
 *  * Netlist comparison has to be performed using the NetlistCompare object provided.
 *    This will establish the cross-reference between the two netlists.
 */
class DB_PUBLIC LayoutVsSchematic
  : public db::LayoutToNetlist
{
public:
  /**
   *  @brief The constructor
   *
   *  See the LayoutToNetlist for details.
   */
  LayoutVsSchematic (const db::RecursiveShapeIterator &iter);

  /**
   *  @brief Alternative constructor using an external deep shape storage
   *
   *  See the LayoutToNetlist for details.
   */
  LayoutVsSchematic (db::DeepShapeStore *dss, unsigned int layout_index = 0);

  /**
   *  @brief Alternative constructor for flat mode
   *
   *  See the LayoutToNetlist for details.
   */
  LayoutVsSchematic (const std::string &topcell_name, double dbu);

  /**
   *  @brief The default constructor
   */
  LayoutVsSchematic ();

  /**
   *  @brief The destructor
   */
  ~LayoutVsSchematic ();

  /**
   *  @brief Sets the reference netlist
   *
   *  This will establish the reference netlist for the comparison.
   *  The LayoutVsSchematic object will take ownership over the netlist
   *  object.
   *
   *  Setting the reference netlist will reset the cross-reference
   *  object.
   */
  void set_reference_netlist (db::Netlist *ref_netlist);

  /**
   *  @brief Gets the reference netlist
   */
  const db::Netlist *reference_netlist () const
  {
    return mp_reference_netlist.get ();
  }

  /**
   *  @brief Gets the reference netlist (non-const version)
   */
  db::Netlist *reference_netlist ()
  {
    return mp_reference_netlist.get ();
  }

  /**
   *  @brief Performs the comparison
   */
  bool compare_netlists(NetlistComparer *compare);

  /**
   *  @brief Gets the cross-reference object
   *
   *  This reference is 0 if the netlist compare has not been performed yet.
   */
  const db::NetlistCrossReference *cross_ref () const
  {
    return mp_cross_ref.get ();
  }

  /**
   *  @brief Gets the cross-reference object (non-const version)
   *
   *  This reference is 0 if the netlist compare has not been performed yet.
   */
  db::NetlistCrossReference *cross_ref ()
  {
    return mp_cross_ref.get ();
  }

  /**
   *  @brief Creates the cross-reference object if it isn't created yet
   *
   *  This method is provided for special purposes such as the reader.
   */
  db::NetlistCrossReference *make_cross_ref ();

  /**
   *  @brief Saves the database to the given path
   *
   *  Currently, the internal format will be used. If "short_format" is true, the short version
   *  of the format is used.
   *
   *  This is a convenience method. The low-level functionality is the LayoutVsSchematicWriter.
   */
  void save (const std::string &path, bool short_format);

  /**
   *  @brief Loads the database from the given path
   *
   *  This is a convenience method. The low-level functionality is the LayoutVsSchematicReader.
   */
  void load (const std::string &path);

private:
  //  no copying
  LayoutVsSchematic (const db::LayoutVsSchematic &other);
  LayoutVsSchematic &operator= (const db::LayoutVsSchematic &other);

  tl::shared_ptr<db::Netlist> mp_reference_netlist;
  tl::shared_ptr<db::NetlistCrossReference> mp_cross_ref;
};

}

#endif
