
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


#include "dbMemStatistics.h"
#include "tlLog.h"

#ifdef __GNUG__
#include <memory>
#include <cstdlib>
#include <cxxabi.h>

/**
 *  @brief Demangles symbol names for better readability
 */
static std::string demangle (const std::string &name)
{
  int status = 1;
  char *dn = abi::__cxa_demangle(name.c_str (), 0, 0, &status);
  if (status == 0) {
    std::string res (dn);
    std::free (dn);
    return res;
  } else {
    return name;
  }
}

#else

static std::string demangle (const std::string &name)
{
  return name;
}

#endif

namespace db 
{

void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::string &x, bool no_self, void *parent)
{
  if (! no_self) {
    stat->add (typeid (std::string), (void *) &x, sizeof (std::string), sizeof (std::string), parent, purpose, cat);
  }
  stat->add (typeid (char []), (void *) x.c_str (), x.capacity (), x.size (), (void *) &x, purpose, cat);
}

void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const tl::Variant &x, bool no_self, void *parent)
{
  if (! no_self) {
    stat->add (typeid (tl::Variant), (void *) &x, sizeof (tl::Variant), sizeof (tl::Variant), parent, purpose, cat);
  }
  //  TODO: add content
}

void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::vector<bool> &x, bool no_self, void *parent)
{
  if (! no_self) {
    stat->add (typeid (std::vector<bool>), (void *) &x, sizeof (std::vector<bool>), sizeof (std::vector<bool>), parent, purpose, cat);
  }
  stat->add (typeid (bool []), (void *) 0 /*n/a*/, x.capacity () / 8, x.size () / 8, (void *) &x, purpose, cat);
}

// --------------------------------------------------------------------------------------

MemStatistics::MemStatistics ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------------------------

MemStatisticsCollector::MemStatisticsCollector (bool detailed)
  : m_detailed (detailed)
{
  //  .. nothing yet ..
}

void
MemStatisticsCollector::print ()
{
  std::map<purpose_t, std::string> p2s;
  p2s[None]            = "(none)         ";
  p2s[LayoutInfo]      = "Layout info    ";
  p2s[CellInfo]        = "Cell info      ";
  p2s[Instances]       = "Instances      ";
  p2s[InstTrees]       = "Instance trees ";
  p2s[ShapesInfo]      = "Shapes info    ";
  p2s[ShapesCache]     = "Shapes cache   ";
  p2s[ShapeTrees]      = "Shape trees    ";
  p2s[Netlist]         = "Netlist        ";
  p2s[LayoutToNetlist] = "Netlist layout ";

  if (m_detailed) {

    tl::info << "Memory usage per type:";
    for (std::map<const std::type_info *, std::pair<size_t, size_t> >::const_iterator t = m_per_type.begin (); t != m_per_type.end (); ++t) {
      tl::info << "  " << demangle (t->first->name ()) << ": " << t->second.first << " (used) " << t->second.second << " (reqd)";
    }

    tl::info << "Memory usage per category:";
    for (std::map<std::pair<purpose_t, int>, std::pair<size_t, size_t> >::const_iterator t = m_per_cat.begin (); t != m_per_cat.end (); ++t) {
      tl::info << "  " << p2s[t->first.first] << "[" << t->first.second << "]: " << t->second.first << " (used) " << t->second.second << " (reqd)";
    }

  }

  tl::info << "Memory usage per master category:";
  std::pair<size_t, size_t> tot;
  for (std::map<purpose_t, std::pair<size_t, size_t> >::const_iterator t = m_per_purpose.begin (); t != m_per_purpose.end (); ++t) {
    tl::info << "  " << p2s[t->first] << ": " << t->second.first << " (used) " << t->second.second << " (reqd)";
    tot.first += t->second.first;
    tot.second += t->second.second;
  }
  tl::info << "  Total          : " << tot.first << " (used) " << tot.second << " (reqd)";
}

void
MemStatisticsCollector::add (const std::type_info &ti, void * /*ptr*/, size_t size, size_t used, void * /*parent*/, purpose_t purpose, int cat)
{
  if (m_detailed) {

    m_per_type[&ti].first += used;
    m_per_type[&ti].second += size;

    std::pair<size_t, size_t> &i = m_per_cat [std::make_pair (purpose, cat)];
    i.first += used;
    i.second += size;

  }

  std::pair<size_t, size_t> &j = m_per_purpose [purpose];
  j.first += used;
  j.second += size;
}

}

