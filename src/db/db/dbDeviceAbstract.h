
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

#ifndef _HDR_dbDeviceAbstract
#define _HDR_dbDeviceAbstract

#include "dbCommon.h"
#include "dbNet.h"
#include "dbPoint.h"
#include "dbMemStatistics.h"

#include "tlObject.h"

#include <vector>

namespace db
{

class Netlist;

/**
 *  @brief A device abstract
 *
 *  A device abstract represents the geometrical properties of a device. It basically links
 *  to a cell and clusters for indicating the terminal geometry of the device.
 */
class DB_PUBLIC DeviceAbstract
  : public tl::Object
{
public:
  /**
   *  @brief Default constructor
   */
  DeviceAbstract ();

  /**
   *  @brief The constructor
   */
  DeviceAbstract (db::DeviceClass *device_class, const std::string &name = std::string ());

  /**
   *  @brief Copy constructor
   */
  DeviceAbstract (const DeviceAbstract &other);

  /**
   *  @brief Assignment
   */
  DeviceAbstract &operator= (const DeviceAbstract &other);

  /**
   *  @brief Destructor
   */
  ~DeviceAbstract ();

  /**
   *  @brief Gets the device class
   */
  const DeviceClass *device_class () const
  {
    return mp_device_class;
  }

  /**
   *  @brief Sets the device class
   */
  void set_device_class (DeviceClass *dc)
  {
    mp_device_class = dc;
  }

  /**
   *  @brief Gets the netlist the device lives in (const version)
   *  This pointer is 0 if the device abstract isn't added to a netlist
   */
  const Netlist *netlist () const
  {
    return mp_netlist;
  }

  /**
   *  @brief Gets the netlist the device lives in (non-const version)
   *  This pointer is 0 if the device abstract isn't added to a netlist
   */
  Netlist *netlist ()
  {
    return mp_netlist;
  }

  /**
   *  @brief Sets the name
   */
  void set_name (const std::string &n);

  /**
   *  @brief Gets the name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the device cell index
   *  In the layout, a device is represented by a cell. This attribute gives the index of this
   *  cell.
   */
  void set_cell_index (db::cell_index_type ci);

  /**
   *  @brief Gets the device cell index
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Gets the cluster ID for a given terminal
   *  This attribute connects the device terminal with a terminal cluster
   */
  size_t cluster_id_for_terminal (size_t terminal_id) const;

  /**
   *  @brief Sets the cluster ID for a given terminal
   */
  void set_cluster_id_for_terminal (size_t terminal_id, size_t cluster_id);

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_terminal_cluster_ids, true, (void *) this);
  }

private:
  friend class Netlist;

  std::string m_name;
  db::DeviceClass *mp_device_class;
  db::cell_index_type m_cell_index;
  std::vector<size_t> m_terminal_cluster_ids;
  Netlist *mp_netlist;

  /**
   *  @brief Sets the netlist
   */
  void set_netlist (Netlist *netlist);
};

/**
 *  @brief Memory statistics for LayoutToNetlist
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const DeviceAbstract &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif
