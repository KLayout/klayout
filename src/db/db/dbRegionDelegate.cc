
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


#include "dbRegionDelegate.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------

RegionDelegate::RegionDelegate ()
{
  m_base_verbosity = 30;
  m_report_progress = false;
  m_merged_semantics = true;
  m_strict_handling = false;
  m_merge_min_coherence = false;
}

RegionDelegate::RegionDelegate (const RegionDelegate &other)
  : ShapeCollectionDelegateBase ()
{
  operator= (other);
}

RegionDelegate &
RegionDelegate::operator= (const RegionDelegate &other)
{
  if (this != &other) {
    m_base_verbosity = other.m_base_verbosity;
    m_report_progress = other.m_report_progress;
    m_merged_semantics = other.m_merged_semantics;
    m_strict_handling = other.m_strict_handling;
    m_merge_min_coherence = other.m_merge_min_coherence;
  }
  return *this;
}

RegionDelegate::~RegionDelegate ()
{
  //  .. nothing yet ..
}

void RegionDelegate::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void RegionDelegate::disable_progress ()
{
  m_report_progress = false;
}

void RegionDelegate::set_base_verbosity (int vb)
{
  m_base_verbosity = vb;
}

void RegionDelegate::set_min_coherence (bool f)
{
  if (f != m_merge_min_coherence) {
    m_merge_min_coherence = f;
    min_coherence_changed ();
  }
}

void RegionDelegate::set_merged_semantics (bool f)
{
  if (f != m_merged_semantics) {
    m_merged_semantics = f;
    merged_semantics_changed ();
  }
}

void RegionDelegate::set_strict_handling (bool f)
{
  m_strict_handling = f;
}

}

