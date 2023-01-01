
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

#include "dbDeviceAbstract.h"
#include "dbCircuit.h"
#include "dbNetlist.h"

namespace db
{

// --------------------------------------------------------------------------------
//  DeviceAbstract class implementation

DeviceAbstract::DeviceAbstract ()
  : m_name (), mp_device_class (0), m_cell_index (std::numeric_limits<db::cell_index_type>::max ()), mp_netlist (0)
{
  //  .. nothing yet ..
}

DeviceAbstract::~DeviceAbstract ()
{
  //  .. nothing yet ..
}

DeviceAbstract::DeviceAbstract (db::DeviceClass *device_class, const std::string &name)
  : m_name (name), mp_device_class (device_class), m_cell_index (std::numeric_limits<db::cell_index_type>::max ()), mp_netlist (0)
{
  //  .. nothing yet ..
}

DeviceAbstract::DeviceAbstract (const DeviceAbstract &other)
  : tl::Object (other), mp_device_class (0), m_cell_index (std::numeric_limits<db::cell_index_type>::max ()), mp_netlist (0)
{
  operator= (other);
}

DeviceAbstract &DeviceAbstract::operator= (const DeviceAbstract &other)
{
  if (this != &other) {
    m_name = other.m_name;
    mp_device_class = other.mp_device_class;
    m_cell_index = other.m_cell_index;
    m_terminal_cluster_ids = other.m_terminal_cluster_ids;
  }
  return *this;
}

void DeviceAbstract::set_netlist (Netlist *netlist)
{
  mp_netlist = netlist;
}

void DeviceAbstract::set_name (const std::string &n)
{
  m_name = n;
  if (mp_netlist) {
    mp_netlist->m_device_abstract_by_name.invalidate ();
  }
}

void DeviceAbstract::set_cell_index (db::cell_index_type ci)
{
  m_cell_index = ci;
  if (mp_netlist) {
    mp_netlist->m_device_abstract_by_cell_index.invalidate ();
  }
}

size_t DeviceAbstract::cluster_id_for_terminal (size_t terminal_id) const
{
  return terminal_id < m_terminal_cluster_ids.size () ? m_terminal_cluster_ids [terminal_id] : 0;
}

void DeviceAbstract::set_cluster_id_for_terminal (size_t terminal_id, size_t cluster_id)
{
  if (m_terminal_cluster_ids.size () <= terminal_id) {
    m_terminal_cluster_ids.resize (terminal_id + 1, 0);
  }
  m_terminal_cluster_ids [terminal_id] = cluster_id;
}

}
