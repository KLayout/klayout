
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbDeviceModel.h"
#include "dbCircuit.h"

namespace db
{

// --------------------------------------------------------------------------------
//  DeviceModel class implementation

DeviceModel::DeviceModel ()
  : m_name (), mp_device_class (0), m_cell_index (std::numeric_limits<db::cell_index_type>::max ()), mp_netlist (0)
{
  //  .. nothing yet ..
}

DeviceModel::~DeviceModel ()
{
  //  .. nothing yet ..
}

DeviceModel::DeviceModel (db::DeviceClass *device_class, const std::string &name)
  : m_name (name), mp_device_class (device_class), m_cell_index (std::numeric_limits<db::cell_index_type>::max ()), mp_netlist (0)
{
  //  .. nothing yet ..
}

DeviceModel::DeviceModel (const DeviceModel &other)
  : tl::Object (other), mp_device_class (0), m_cell_index (std::numeric_limits<db::cell_index_type>::max ()), mp_netlist (0)
{
  operator= (other);
}

DeviceModel &DeviceModel::operator= (const DeviceModel &other)
{
  if (this != &other) {
    m_name = other.m_name;
    mp_device_class = other.mp_device_class;
    m_cell_index = other.m_cell_index;
    m_terminal_cluster_ids = other.m_terminal_cluster_ids;
  }
  return *this;
}

void DeviceModel::set_netlist (Netlist *netlist)
{
  mp_netlist = netlist;
}

void DeviceModel::set_name (const std::string &n)
{
  m_name = n;
}

void DeviceModel::set_cell_index (db::cell_index_type ci)
{
  m_cell_index = ci;
}

size_t DeviceModel::cluster_id_for_terminal (size_t terminal_id) const
{
  return terminal_id < m_terminal_cluster_ids.size () ? m_terminal_cluster_ids [terminal_id] : 0;
}

void DeviceModel::set_cluster_id_for_terminal (size_t terminal_id, size_t cluster_id)
{
  if (m_terminal_cluster_ids.size () <= terminal_id) {
    m_terminal_cluster_ids.resize (terminal_id + 1, 0);
  }
  m_terminal_cluster_ids [terminal_id] = cluster_id;
}

}
