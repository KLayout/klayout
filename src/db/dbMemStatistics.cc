
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#include "dbMemStatistics.h"
#include "tlLog.h"

namespace db 
{

size_t mem_used (const std::string &s)
{
  return sizeof (std::string) + s.capacity ();
}

size_t mem_reqd (const std::string &s)
{
  return sizeof (std::string) + s.size ();
}

size_t mem_used (const std::vector<bool> &v)
{
  return sizeof (std::vector<bool>) + v.capacity () / 8;
}

size_t mem_reqd (const std::vector<bool> &v)
{
  return sizeof (std::vector<bool>) + v.size () / 8;
}


MemStatistics::MemStatistics ()
{
  m_layout_info_used = m_layout_info_reqd = 0;
  m_cell_info_used = m_cell_info_reqd = 0;
  m_inst_trees_used = m_inst_trees_reqd = 0;
  m_shapes_info_used = m_shapes_info_reqd = 0;
  m_shapes_cache_used = m_shapes_cache_reqd = 0;
  m_shape_trees_used = m_shape_trees_reqd = 0;
  m_instances_used = m_instances_reqd = 0;
}

void 
MemStatistics::print () const
{
  tl::info << "Memory usage:";
  tl::info << "  Layout info    " << m_layout_info_used << " (used) " << m_layout_info_reqd << " (reqd)";
  tl::info << "  Cell info      " << m_cell_info_used << " (used) " << m_cell_info_reqd << " (reqd) ";
  tl::info << "  Instances      " << m_instances_used << " (used) " << m_instances_reqd << " (reqd) ";
  tl::info << "  Instance trees " << m_inst_trees_used << " (used) " << m_inst_trees_reqd << " (reqd) ";
  tl::info << "  Shapes info    " << m_shapes_info_used << " (used) " << m_shapes_info_reqd << " (reqd) ";
  tl::info << "  Shapes cache   " << m_shapes_cache_used << " (used) " << m_shapes_cache_reqd << " (reqd) ";
  tl::info << "  Shape trees    " << m_shape_trees_used << " (used) " << m_shape_trees_reqd << " (reqd) ";
  tl::info << "  Total          " << (m_layout_info_used + m_cell_info_used + m_instances_used + m_inst_trees_used + m_shapes_info_used + m_shapes_cache_used + m_shape_trees_used) << " (used) " 
                                  << (m_layout_info_reqd + m_cell_info_reqd + m_instances_reqd + m_inst_trees_reqd + m_shapes_info_reqd + m_shapes_cache_reqd + m_shape_trees_reqd) << " (reqd) ";
}

}

