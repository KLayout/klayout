
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


#include "layMargin.h"

#include <algorithm>

namespace lay
{

Margin::Margin (double value, bool relative)
  : m_relative_value (0.0), m_absolute_value (0.0), m_relative_mode (relative)
{
  if (relative) {
    m_relative_value = value;
  } else {
    m_absolute_value = value;
  }
}

std::string
Margin::to_string () const
{
  const double min_value = 1e-10;

  std::string res;
  if (m_relative_mode) {
    res = std::string ("*") + tl::to_string (m_relative_value);
    if (fabs (m_absolute_value) > min_value) {
      res += " ";
      res += tl::to_string (m_absolute_value);
    }
  } else {
    res = tl::to_string (m_absolute_value);
    if (fabs (m_relative_value) > min_value) {
      res += " *";
      res += tl::to_string (m_relative_value);
    }
  }

  return res;
}

Margin
Margin::from_string (const std::string &s)
{
  Margin res;

  tl::Extractor ex (s.c_str ());
  if (ex.test ("*")) {
    double v = 0.0;
    ex.read (v);
    res.set_relative_mode (true);
    res.set_relative_value (v);
    if (! ex.at_end ()) {
      ex.read (v);
      res.set_absolute_value (v);
    }
  } else {
    double v = 0.0;
    ex.read (v);
    res.set_relative_mode (false);
    res.set_absolute_value (v);
    if (ex.test ("*")) {
      ex.read (v);
      res.set_relative_value (v);
    }
  }

  return res;
}

double
Margin::get (double dim) const
{
  return m_relative_mode ? dim * m_relative_value : m_absolute_value;
}

double
Margin::get (const db::DBox &box) const
{
  return get (std::max (box.width (), box.height ()));
}

}

