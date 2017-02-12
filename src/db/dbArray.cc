
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


#include "dbArray.h"

namespace db
{

ArrayRepository::ArrayRepository ()
{
  //  .. nothing yet ..
}

ArrayRepository::ArrayRepository (const ArrayRepository &d)
{
  operator= (d);
}

ArrayRepository::~ArrayRepository ()
{
  clear ();
}

void
ArrayRepository::clear ()
{
  for (repositories::iterator r = m_reps.begin (); r != m_reps.end (); ++r) {
    for (basic_repository::iterator rr = r->begin (); rr != r->end (); ++rr) {
      delete *rr;
    }
  }
  m_reps.clear ();
}

ArrayRepository &
ArrayRepository::operator= (const ArrayRepository &d)
{
  clear ();

  for (repositories::const_iterator r = d.m_reps.begin (); r != d.m_reps.end (); ++r) {
    m_reps.push_back (basic_repository ());
    for (basic_repository::const_iterator rr = r->begin (); rr != r->end (); ++rr) {
      m_reps.back ().insert ((*rr)->basic_clone ());
    }
  }
  return *this;
}

size_t 
ArrayRepository::mem_used () const
{
  size_t mem = 0;
  for (repositories::const_iterator r = m_reps.begin (); r != m_reps.end (); ++r) {
    mem += db::mem_used (*r);
    for (basic_repository::const_iterator rr = r->begin (); rr != r->end (); ++rr) {
      mem += (*rr)->mem_used ();
    }
  }
  return mem + sizeof (*this);
}

size_t 
ArrayRepository::mem_reqd () const
{
  size_t mem = 0;
  for (repositories::const_iterator r = m_reps.begin (); r != m_reps.end (); ++r) {
    mem += db::mem_reqd (*r);
    for (basic_repository::const_iterator rr = r->begin (); rr != r->end (); ++rr) {
      mem += (*rr)->mem_reqd ();
    }
  }
  return mem + sizeof (*this);
}

}

